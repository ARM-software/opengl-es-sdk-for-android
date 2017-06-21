/* Copyright (c) 2015-2017, ARM Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define FFT_FP16 1

#include "vector_math.h"

#include "fftwater.hpp"
#include <cmath>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace GLFFT;

FFTWater::FFTWater(
        float amplitude,
        vec2 wind_velocity,
        uvec2 resolution,
        vec2 size,
        vec2 normalmap_freq_mod)
    :
        wind_velocity(wind_velocity),
        wind_dir(vec_normalize(wind_velocity)),
        Nx(resolution.x), Nz(resolution.y), size(size), size_normal(size / normalmap_freq_mod)
{
    // Factor in Phillips spectrum.
    L = vec_dot(wind_velocity, wind_velocity) / G;

    // Use half-res for displacementmap since it's so low-resolution.
    displacement_downsample = 1;

    distribution.resize(Nx * Nz);
    distribution_normal.resize(Nx * Nz);

    // Normalize amplitude a bit based on the heightmap size.
    amplitude *= 0.3f / sqrt(size.x * size.y);

    generate_distribution(distribution.data(), size, amplitude, 0.02f);
    generate_distribution(distribution_normal.data(), size_normal,
            amplitude * sqrt(normalmap_freq_mod.x * normalmap_freq_mod.y), 0.02f);

    distribution_displacement.resize((Nx * Nz) >> (displacement_downsample * 2));
    downsample_distribution(distribution_displacement.data(), distribution.data(), displacement_downsample);

    // Check if we can render to FP16, if so, we can do mipmaping of FP16 in fragment instead where appropriate.
    mipmap_fp16 = common_has_extension("GL_EXT_color_buffer_half_float");

    init_gl_fft();
}

static inline int alias(int x, int N)
{
    if (x > N / 2)
        x -= N;
    return x;
}

void FFTWater::downsample_distribution(cfloat *out, const cfloat *in, unsigned rate_log2)
{
    // Pick out the lower frequency samples only which is the same as downsampling "perfectly".
    unsigned out_width = Nx >> rate_log2;
    unsigned out_height = Nz >> rate_log2;

    for (unsigned z = 0; z < out_height; z++)
    {
        for (unsigned x = 0; x < out_width; x++)
        {
            int alias_x = alias(x, out_width);
            int alias_z = alias(z, out_height);

            if (alias_x < 0)
            {
                alias_x += Nx;
            }

            if (alias_z < 0)
            {
                alias_z += Nz;
            }

            out[z * out_width + x] = in[alias_z * Nx + alias_x];
        }
    }
}

cfloat FFTWater::phillips(vec2 k, float max_l)
{
    // See Tessendorf paper for details.
    float k_len = vec_length(k);
    if (k_len == 0.0f)
    {
        return 0.0f;
    }

    float kL = k_len * L;
    vec2 k_dir = vec_normalize(k);
    float kw = vec_dot(k_dir, wind_dir);

    return
        pow(kw * kw, 1.0f) *                        // Directional
        exp(-1.0 * k_len * k_len * max_l * max_l) * // Suppress small waves at ~max_l.
        exp(-1.0f / (kL * kL)) *
        pow(k_len, -4.0f);
}

void FFTWater::generate_distribution(cfloat *distribution, vec2 size, float amplitude, float max_l)
{
    // Modifier to find spatial frequency.
    vec2 mod = vec2(2.0f * M_PI) / size;

    for (unsigned z = 0; z < Nz; z++)
    {
        for (unsigned x = 0; x < Nx; x++)
        {
            auto &v = distribution[z * Nx + x];
            vec2 k = mod * vec2(alias(x, Nx), alias(z, Nz));

            // Gaussian distributed noise with unit variance.
            cfloat dist = cfloat(normal_dist(engine), normal_dist(engine));
            v = dist * amplitude * sqrt(0.5f * phillips(k, max_l));
        }
    }
}

void FFTWater::update_phase(float time)
{
    vec2 mod = vec2(2.0f * M_PI) / size;
    vec2 mod_normal = vec2(2.0f * M_PI) / size_normal;

    // Generate new FFTs
    GL_CHECK(glUseProgram(prog_generate_height.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, distribution_buffer.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freq_height.get()));
    GL_CHECK(glUniform2f(0, mod.x, mod.y));
    GL_CHECK(glUniform1f(1, time));
    GL_CHECK(glUniform2ui(2, Nx, Nz));
    // We only need to generate half the frequencies due to C2R transform.
    GL_CHECK(glDispatchCompute(Nx / 64, Nz, 1));

    GL_CHECK(glUseProgram(prog_generate_displacement.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, distribution_buffer_displacement.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freq_displacement.get()));
    GL_CHECK(glUniform2f(0, mod.x, mod.y));
    GL_CHECK(glUniform1f(1, time));
    GL_CHECK(glDispatchCompute((Nx >> displacement_downsample) / 64, (Nz >> displacement_downsample), 1));

    GL_CHECK(glUseProgram(prog_generate_normal.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, distribution_buffer_normal.get()));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freq_normal.get()));
    GL_CHECK(glUniform2f(0, mod_normal.x, mod_normal.y));
    GL_CHECK(glUniform1f(1, time));
    GL_CHECK(glDispatchCompute(Nx / 64, Nz, 1));

    // The three compute jobs above are independent so we only need to barrier here.
    GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
}

void FFTWater::compute_ifft()
{
    // Compute the iFFT
    // Ping-pong the textures we use so we can run fragment and compute in parallel without triggering lots of extra driver work.
    texture_index ^= 1;
    fft_height->process(heightmap[texture_index].get(), freq_height.get());
    fft_displacement->process(displacementmap[texture_index].get(), freq_displacement.get());
    fft_normal->process(normalmap[texture_index].get(), freq_normal.get());
    GL_CHECK(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
}

void FFTWater::generate_mipmaps()
{
    // Mipmap heightmap in compute.
    // If we mipmap with fragment, we will have to wait for previous frame to complete rendering first.
    // This creates a stall where vertex shading will run without any fragment processing active, which is very bad for pipelining.
    // We also cannot use default mipmapping anyways,
    // since we want to treat (0, 0) as top-left pixel for heightmap/displacementmap
    // and apply half-texel offsets as needed.
    //
    // While we don't need to mipmap normalmap and gradientjacobian in compute, implement this as a fallback
    // if FP16 rendering extension is not supported.
    if (mipmap_fp16)
    {
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, gradientjacobianmap[texture_index].get()));
        GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, normalmap[texture_index].get()));
        GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
    }

    // Do not output to the two highest mipmap levels.
    for (unsigned l = 0; (Nx >> l) >= 8 && (Nz >> l) >= 8; l++)
    {
        // Fallback path.
        if (!mipmap_fp16)
        {
            // There is no rg16f image format, just use R32UI reinterpretation which is the same thing.
            compute_mipmap(prog_mipmap_normal, normalmap[texture_index], GL_R32UI,
                    Nx >> l, Nz >> l, l + 1);
            compute_mipmap(prog_mipmap_gradient_jacobian, gradientjacobianmap[texture_index], GL_RGBA16F,
                    Nx >> l, Nz >> l, l + 1);
        }

        compute_mipmap(prog_mipmap_height, heightdisplacementmap[texture_index], GL_RGBA16F, Nx >> l, Nz >> l, l + 1);

        // Avoid memory barriers for every dispatch since we can compute 3 separate miplevels before flushing load-store caches.
        GL_CHECK(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
    }
}

void FFTWater::update(float time)
{
    update_phase(time);
    compute_ifft();
    // Generate final textures ready for vertex and fragment shading.
    bake_height_gradient();
    generate_mipmaps();
}

void FFTWater::bake_height_gradient()
{
    GL_CHECK(glUseProgram(prog_bake_height_gradient.get()));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, heightmap[texture_index].get()));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, displacementmap[texture_index].get()));

    // Height and displacement are sampled in vertex shaders only, so stick them together.
    GL_CHECK(glBindImageTexture(0, heightdisplacementmap[texture_index].get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));

    // Gradients from heightmap and the jacobian are only sampled in fragment, so group them together.
    GL_CHECK(glBindImageTexture(1, gradientjacobianmap[texture_index].get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F));

    GL_CHECK(glUniform4f(0,
            1.0f / Nx, 1.0f / Nz,
            1.0f / (Nx >> displacement_downsample),
            1.0f / (Nz >> displacement_downsample)));
    GL_CHECK(glUniform4f(1,
            Nx / size.x, Nz / size.y,
            (Nx >> displacement_downsample) / size.x,
            (Nz >> displacement_downsample) / size.y));

    GL_CHECK(glDispatchCompute(Nx / 8, Nz / 8, 1));
    GL_CHECK(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));
}

void FFTWater::compute_mipmap(const Program &program, const Texture &texture, GLenum format, unsigned Nx, unsigned Nz, unsigned l)
{
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture.get()));

    GLint min_filter;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
    // Make sure we're not sampling from the level we're trying to write to.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST));

    GL_CHECK(glUseProgram(program.get()));

    GL_CHECK(glBindImageTexture(0, texture.get(), l, GL_FALSE, 0, GL_WRITE_ONLY, format));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, l - 1));
    GL_CHECK(glUniform1i(0, l - 1));
    GL_CHECK(glUniform2f(1, 1.0f / Nx, 1.0f / Nz));

    unsigned threads_x = Nx / 2;
    unsigned threads_z = Nz / 2;
    GL_CHECK(glDispatchCompute(threads_x / 4, threads_z / 4, 1));

    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter));
}

void FFTWater::init_texture(Texture &tex, GLenum format, unsigned levels, unsigned width, unsigned height,
        GLenum mag_filter, GLenum min_filter)
{
    tex.init(width, height, levels, format, GL_REPEAT, GL_REPEAT, min_filter, mag_filter);
}

void FFTWater::init_gl_fft()
{
    // Compile compute shaders.
    prog_generate_height = Program(common_compile_compute_shader_from_file("water_generate_height.comp"));
    prog_generate_displacement = Program(common_compile_compute_shader_from_file("water_generate_displacement.comp"));
    prog_generate_normal = Program(common_compile_compute_shader_from_file("water_generate_normal.comp"));

    prog_bake_height_gradient = Program(common_compile_compute_shader_from_file("bake_height_gradient.comp"));
    prog_mipmap_height = Program(common_compile_compute_shader_from_file("mipmap_height.comp"));
    prog_mipmap_normal = Program(common_compile_compute_shader_from_file("mipmap_normal.comp"));
    prog_mipmap_gradient_jacobian = Program(common_compile_compute_shader_from_file("mipmap_gradjacobian.comp"));

    auto cache = make_shared<ProgramCache>();

    // Use FP16 FFT.
    FFTOptions options;
    options.type.fp16 = FFT_FP16;
    options.type.input_fp16 = FFT_FP16;
    options.type.output_fp16 = FFT_FP16;

    // Sensible default values for Mali.
    options.performance.workgroup_size_x = 8;
    options.performance.workgroup_size_y = 4;
    options.performance.vector_size = 4;
    options.performance.shared_banked = false;

    // Create three FFTs for heightmap, displacementmap and high-frequency normals.
    fft_height = unique_ptr<FFT>(new FFT(Nx, Nz,
                ComplexToReal, Inverse, SSBO, ImageReal, cache, options));
    fft_displacement = unique_ptr<FFT>(new FFT(Nx >> displacement_downsample, Nz >> displacement_downsample,
                ComplexToComplex, Inverse, SSBO, Image, cache, options));
    fft_normal = unique_ptr<FFT>(new FFT(Nx, Nz,
                ComplexToComplex, Inverse, SSBO, Image, move(cache), options));

    normal_levels = unsigned(log2(max(float(Nx), float(Nz)))) + 1;

    for (unsigned i = 0; i < 2; i++)
    {
        // R32F since GLES 3.1 does not support r16f format for image load/store.
        init_texture(heightmap[i],
                GL_R32F, 1,
                Nx, Nz,
                GL_NEAREST,
                GL_NEAREST);

        init_texture(displacementmap[i],
                GL_RG16F, 1,
                Nx >> displacement_downsample, Nz >> displacement_downsample,
                GL_LINEAR,
                GL_LINEAR);

        // Ignore the two highest mipmap levels, since we would like to avoid micro dispatches that just write 1 texel.
        init_texture(normalmap[i], GL_RG16F, normal_levels - 2, Nx, Nz, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);

        init_texture(heightdisplacementmap[i],
                GL_RGBA16F, normal_levels - 2,
                Nx, Nz,
                GL_LINEAR,
                GL_LINEAR_MIPMAP_NEAREST);

        init_texture(gradientjacobianmap[i],
                GL_RGBA16F, normal_levels - 2,
                Nx, Nz,
                GL_LINEAR,
                GL_LINEAR_MIPMAP_LINEAR);
    }

    distribution_buffer.init(distribution.data(), Nx * Nz * sizeof(cfloat), GL_STATIC_COPY);
    distribution_buffer_displacement.init(distribution_displacement.data(),
            (Nx * Nz * sizeof(cfloat)) >> (displacement_downsample * 2),
            GL_STATIC_COPY);
    distribution_buffer_normal.init(distribution_normal.data(), Nx * Nz * sizeof(cfloat), GL_STATIC_COPY);

    distribution.clear();
    distribution_displacement.clear();
    distribution_normal.clear();

    // Copy distributions to the GPU.
    freq_height.init(nullptr, (Nx * Nz * sizeof(cfloat)) >> FFT_FP16, GL_STREAM_COPY);
    freq_normal.init(nullptr, (Nx * Nz * sizeof(cfloat)) >> FFT_FP16, GL_STREAM_COPY);
    freq_displacement.init(nullptr, ((Nx * Nz * sizeof(cfloat)) >> FFT_FP16) >> (displacement_downsample * 2), GL_STREAM_COPY);
}

