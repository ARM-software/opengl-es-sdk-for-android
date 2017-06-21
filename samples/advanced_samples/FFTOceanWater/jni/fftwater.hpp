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

#ifndef FFTWATER_HPP__
#define FFTWATER_HPP__

#include "vector_math.h"
#include <complex>
#include <random>
#include <vector>
#include <memory>
#include "glfft.hpp"
#include "common.hpp"

using cfloat = std::complex<float>;

class FFTWater
{
    private:
        cfloat phillips(vec2 k, float max_l);

        vec2 wind_velocity;
        vec2 wind_dir;
        unsigned Nx, Nz;
        vec2 size, size_normal;
        float L;

        void generate_distribution(cfloat *distribution, vec2 size, float amplitude, float max_l);

        void generate_mipmaps();
        void compute_ifft();
        void bake_height_gradient();
        void update_phase(float time);

        std::normal_distribution<float> normal_dist{0.0f, 1.0f};
        std::default_random_engine engine;
        constexpr static float G = 9.81f;

        std::vector<cfloat> distribution;
        std::vector<cfloat> distribution_displacement;
        std::vector<cfloat> distribution_normal;

        GLFFT::Program prog_generate_height;
        GLFFT::Program prog_generate_normal;
        GLFFT::Program prog_generate_displacement;

        GLFFT::Program prog_bake_height_gradient;
        GLFFT::Program prog_mipmap_height;
        GLFFT::Program prog_mipmap_normal;
        GLFFT::Program prog_mipmap_gradient_jacobian;

        GLFFT::Texture heightmap[2];
        GLFFT::Texture displacementmap[2];
        GLFFT::Texture normalmap[2];

        GLFFT::Texture heightdisplacementmap[2];
        GLFFT::Texture gradientjacobianmap[2];

        unsigned texture_index = 0;
        unsigned normal_levels = 0;
        unsigned displacement_downsample = 0;

        GLFFT::Buffer distribution_buffer;
        GLFFT::Buffer distribution_buffer_displacement;
        GLFFT::Buffer distribution_buffer_normal;
        GLFFT::Buffer freq_height;
        GLFFT::Buffer freq_displacement;
        GLFFT::Buffer freq_normal;
        std::unique_ptr<GLFFT::FFT> fft_height;
        std::unique_ptr<GLFFT::FFT> fft_displacement;
        std::unique_ptr<GLFFT::FFT> fft_normal;
        void init_gl_fft();
        void downsample_distribution(cfloat *out, const cfloat *in, unsigned rate_log2);
        void compute_mipmap(const GLFFT::Program &program, const GLFFT::Texture &texture, GLenum format, unsigned Nx, unsigned Nz, unsigned level);
        void init_texture(GLFFT::Texture &tex, GLenum format, unsigned levels, unsigned width, unsigned height, GLenum mag_filter, GLenum min_filter);

        bool mipmap_fp16;

    public:
        FFTWater(
                float amplitude,
                vec2 wind_velocity,
                uvec2 resolution,
                vec2 size,
                vec2 normalmap_freq_mod);

        void update(float time);

        GLuint get_height_displacement() const { return heightdisplacementmap[texture_index].get(); }
        GLuint get_gradient_jacobian() const  { return gradientjacobianmap[texture_index].get(); }
        GLuint get_normal() const { return normalmap[texture_index].get(); }
        unsigned get_displacement_downsample() const { return displacement_downsample; }
};

#endif
