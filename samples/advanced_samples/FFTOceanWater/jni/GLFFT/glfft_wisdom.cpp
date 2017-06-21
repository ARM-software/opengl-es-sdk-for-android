/* Copyright (C) 2015 Hans-Kristian Arntzen <maister@archlinux.us>
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

#include "glfft_wisdom.hpp"
#include "glfft.hpp"
#include <utility>

using namespace std;
using namespace GLFFT;

FFTStaticWisdom FFTWisdom::get_static_wisdom_from_renderer(const char *renderer)
{
    FFTStaticWisdom res;

    GLint value = 0;
    GL_CHECK(glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value));

    if (strstr(renderer, "GeForce"))
    {
        glfft_log("Detected GeForce GPU.\n");
        res.min_workgroup_size = 32; // Warp threads.
        res.min_workgroup_size_shared = 32;
        res.max_workgroup_size = min(value, 256); // Very unlikely that more than 256 threads will do anything good.
        res.min_vector_size = 2;
        res.max_vector_size = 2;
        res.shared_banked = FFTStaticWisdom::True;
    }
    else if (strstr(renderer, "Radeon"))
    {
        glfft_log("Detected Radeon GPU.\n");
        res.min_workgroup_size = 64; // Wavefront threads (GCN).
        res.min_workgroup_size_shared = 128;
        res.max_workgroup_size = min(value, 256); // Very unlikely that more than 256 threads will do anything good.
        // TODO: Find if we can restrict this to 2 or 4 always.
        res.min_vector_size = 2;
        res.max_vector_size = 4;
        res.shared_banked = FFTStaticWisdom::True;
    }
    else if (strstr(renderer, "Mali"))
    {
        glfft_log("Detected Mali GPU.\n");

        res.min_workgroup_size = 4;
        res.min_workgroup_size_shared = 4;
        res.max_workgroup_size = 64; // Going beyond 64 threads per WG is not a good idea.
        res.min_vector_size = 4;
        res.max_vector_size = 4;
        res.shared_banked = FFTStaticWisdom::False;
    }
    // TODO: Add more GPUs.

    return res;
}

const pair<double, FFTOptions::Performance> FFTWisdom::learn_optimal_options(unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target,
        const FFTOptions::Type &type)
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            type,
        },
        0.0,
    };

    auto itr = library.find(pass);
    if (itr != end(library))
    {
        return make_pair(itr->first.cost, itr->second);
    }
    else
    {
        auto result = study(pass, type);
        pass.cost = result.first;
        library[pass] = result.second;

        return result;
    }
}

void FFTWisdom::learn_optimal_options_exhaustive(unsigned Nx, unsigned Ny,
        Type type, Target input_target, Target output_target, const FFTOptions::Type &fft_type)
{
    bool learn_resolve = type == ComplexToReal || type == RealToComplex;
    Mode vertical_mode = type == ComplexToComplexDual ? VerticalDual : Vertical;
    Mode horizontal_mode = type == ComplexToComplexDual ? HorizontalDual : Horizontal;

    // Create wisdom for horizontal transforms and vertical transform.
    static const unsigned radices[] = { 4, 8, 16, 64 };
    for (auto radix : radices)
    {
        try
        {
            // If we're doing SSBO -> Image or Image -> SSBO. Create wisdom for the two variants.

            // Learn plain transforms.
            if (Ny > 1)
            {
                learn_optimal_options(Nx >> learn_resolve, Ny, radix, vertical_mode, SSBO, SSBO, fft_type);
            }
            learn_optimal_options(Nx >> learn_resolve, Ny, radix, horizontal_mode, SSBO, SSBO, fft_type);

            // Learn the first/last pass transforms. Can be fairly significant since accessing textures makes more sense with
            // block interleave and larger WG_Y sizes.
            if (input_target != SSBO)
            {
                if (Ny > 1)
                {
                    learn_optimal_options(Nx >> learn_resolve, Ny, radix, vertical_mode, input_target, SSBO, fft_type);
                }
                learn_optimal_options(Nx >> learn_resolve, Ny, radix, horizontal_mode, input_target, SSBO, fft_type);
            }

            if (output_target != SSBO)
            {
                if (Ny > 1)
                {
                    learn_optimal_options(Nx >> learn_resolve, Ny, radix, vertical_mode, SSBO, output_target, fft_type);
                }
                learn_optimal_options(Nx >> learn_resolve, Ny, radix, horizontal_mode, SSBO, output_target, fft_type);
            }
        }
        catch (...)
        {
            // If our default options cannot successfully create the radix pass (i.e. throws),
            // just ignore it for purpose of creating wisdom.
        }
    }

    auto resolve_type = fft_type;
    resolve_type.input_fp16 = resolve_type.output_fp16;
    Mode resolve_mode = type == ComplexToReal ? ResolveComplexToReal : ResolveRealToComplex;
    Target resolve_input_target = SSBO;

    // If we have C2R Nx1 transform, the first pass is resolve, so use those types.
    if (type == ComplexToReal && Ny == 1)
    {
        resolve_type = fft_type;
        resolve_input_target = input_target;
    }

    // If we need to do a resolve pass, train this case as well.
    if (learn_resolve)
    {
        try
        {
            // If Ny == 1 and we're doing RealToComplex, this will be the last pass, so use output_target as target.
            if (Ny == 1 && resolve_mode == ResolveRealToComplex)
            {
                learn_optimal_options(Nx >> learn_resolve, Ny, 2, resolve_mode, resolve_input_target, output_target, resolve_type);
            }
            else
            {
                learn_optimal_options(Nx >> learn_resolve, Ny, 2, resolve_mode, resolve_input_target, SSBO, resolve_type);
            }
        }
        catch (...)
        {
            // If our default options cannot successfully create the radix pass (i.e. throws),
            // just ignore it for purpose of creating wisdom.
        }
    }
}

double FFTWisdom::bench(GLuint output, GLuint input,
        const WisdomPass &pass, const FFTOptions &options, const shared_ptr<ProgramCache> &cache) const
{
    FFT fft(pass.pass.Nx, pass.pass.Ny, pass.pass.radix, pass.pass.input_target != SSBO ? 1 : pass.pass.radix,
            pass.pass.mode, pass.pass.input_target, pass.pass.output_target,
            cache, options);

    return fft.bench(output, input, params.warmup, params.iterations, params.dispatches, params.timeout);
}

static inline unsigned mode_to_size(Mode mode)
{
    switch (mode)
    {
        case VerticalDual:
        case HorizontalDual:
        case ResolveRealToComplex:
        case ResolveComplexToReal:
            return 4;

        default:
            return 2;
    }
}

std::pair<double, FFTOptions::Performance> FFTWisdom::study(const WisdomPass &pass, FFTOptions::Type type) const
{
    auto cache = make_shared<ProgramCache>();

    Buffer output;
    Buffer input;
    Texture output_tex;
    Texture input_tex;
    GLuint output_name = 0;
    GLuint input_name = 0;

    unsigned mode_size = mode_to_size(pass.pass.mode);
    vector<float> tmp(mode_size * pass.pass.Nx * pass.pass.Ny);

    if (pass.pass.input_target == SSBO)
    {
        input.init(tmp.data(), tmp.size() * sizeof(float) >> type.input_fp16, GL_STATIC_COPY);
        input_name = input.get();
    }
    else
    {
        GLenum internal_format = 0;
        GLenum format = 0;
        unsigned Nx = pass.pass.Nx;
        unsigned Ny = pass.pass.Ny;

        switch (pass.pass.mode)
        {
            case VerticalDual:
            case HorizontalDual:
                internal_format = GL_RGBA32F;
                format = GL_RGBA;
                break;

            case Vertical:
            case Horizontal:
                internal_format = GL_RG32F;
                format = GL_RG;
                break;

            case ResolveComplexToReal:
                internal_format = GL_RG32F;
                format = GL_RG;
                Nx *= 2;
                break;

            default:
                throw logic_error("Invalid input mode.\n");
        }

        input_tex.init(Nx, Ny, 1, internal_format);
        input_tex.upload(tmp.data(), format, GL_FLOAT, 0, 0, Nx, Ny);
        input_name = input_tex.get();
    }

    if (pass.pass.output_target == SSBO)
    {
        output.init(nullptr, tmp.size() * sizeof(float) >> type.output_fp16, GL_STREAM_COPY);
        output_name = output.get();
    }
    else
    {
        GLenum internal_format = 0;
        unsigned Nx = pass.pass.Nx;
        unsigned Ny = pass.pass.Ny;

        switch (pass.pass.mode)
        {
            case VerticalDual:
            case HorizontalDual:
                internal_format = GL_RGBA32F;
                break;

            case Vertical:
            case Horizontal:
                internal_format = GL_RG32F;
                break;

            case ResolveRealToComplex:
                internal_format = GL_RG32F;
                Nx *= 2;
                break;

            default:
                throw logic_error("Invalid output mode.\n");
        }

        output_tex.init(Nx, Ny, 1, internal_format);
        output_name = output_tex.get();
    }

    // Exhaustive search, look for every sensible combination, and find fastest parameters.
    // Get initial best cost with defaults.
    FFTOptions::Performance best_perf;
    double minimum_cost = bench(output_name, input_name, pass, { best_perf, type }, cache);

    static const FFTStaticWisdom::Tristate shared_banked_values[] = { FFTStaticWisdom::False, FFTStaticWisdom::True };
    static const unsigned vector_size_values[] = { 2, 4, 8 };
    static const unsigned workgroup_size_x_values[] = { 4, 8, 16, 32, 64, 128, 256 };
    static const unsigned workgroup_size_y_values[] = { 1, 2, 4, 8, };

    bool test_resolve = pass.pass.mode == ResolveComplexToReal || pass.pass.mode == ResolveRealToComplex;
    bool test_dual = pass.pass.mode == VerticalDual || pass.pass.mode == HorizontalDual;
    unsigned bench_count = 0;

    for (auto shared_banked : shared_banked_values)
    {
        // Useless test, since shared banked is only relevant for radix 16/64.
        if (pass.pass.radix < 16 && shared_banked)
        {
            continue;
        }

        bool fair_shared_banked = (pass.pass.radix < 16) ||
                                  (static_wisdom.shared_banked == FFTStaticWisdom::DontCare) ||
                                  (shared_banked == static_wisdom.shared_banked);

        if (!fair_shared_banked)
        {
            continue;
        }

        for (auto vector_size : vector_size_values)
        {
            // Resolve passes currently only support vector size 2. Shared banked makes no sense either.
            if (test_resolve && (vector_size != 2 || shared_banked))
            {
                continue;
            }

            // We can only use vector_size 8 with FP16.
            if (vector_size == 8 && (!type.fp16 || !type.input_fp16 || !type.output_fp16))
            {
                continue;
            }

            // Makes little sense to test since since vector_size will be bumped to 4 anyways.
            if (test_dual && vector_size < 4)
            {
                continue;
            }

            for (auto workgroup_size_x : workgroup_size_x_values)
            {
                for (auto workgroup_size_y : workgroup_size_y_values)
                {
                    unsigned workgroup_size  = workgroup_size_x * workgroup_size_y;

                    unsigned min_workgroup_size = pass.pass.radix >= 16 ? static_wisdom.min_workgroup_size_shared :
                                                                          static_wisdom.min_workgroup_size;

                    unsigned min_vector_size = test_dual ? max(4u, static_wisdom.min_vector_size) : static_wisdom.min_vector_size;
                    unsigned max_vector_size = test_dual ? max(4u, static_wisdom.max_vector_size) : static_wisdom.max_vector_size;

                    bool fair_workgroup_size = workgroup_size <= static_wisdom.max_workgroup_size &&
                                               workgroup_size >= min_workgroup_size;
                    if (pass.pass.Ny == 1 && workgroup_size_y > 1)
                    {
                        fair_workgroup_size = false;
                    }

                    if (!fair_workgroup_size)
                    {
                        continue;
                    }

                    // If we have dual mode, accept vector sizes larger than max.
                    bool fair_vector_size = test_resolve || (vector_size <= max_vector_size &&
                                                             vector_size >= min_vector_size);

                    if (!fair_vector_size)
                    {
                        continue;
                    }

                    FFTOptions::Performance perf;
                    perf.shared_banked = shared_banked;
                    perf.vector_size = vector_size;
                    perf.workgroup_size_x = workgroup_size_x;
                    perf.workgroup_size_y = workgroup_size_y;

                    try
                    {
                        // If workgroup sizes are too big for our test, this will throw.
                        double cost = bench(output_name, input_name, pass, { perf, type }, cache);
                        bench_count++;

#if 1
                        glfft_log("\nWisdom run (mode = %u, radix = %u):\n", pass.pass.mode, pass.pass.radix);
                        glfft_log("  Width:            %4u\n", pass.pass.Nx);
                        glfft_log("  Height:           %4u\n", pass.pass.Ny);
                        glfft_log("  Shared banked:     %3s\n", shared_banked ? "yes" : "no");
                        glfft_log("  Vector size:         %u\n", vector_size);
                        glfft_log("  Workgroup size: (%u, %u)\n", workgroup_size_x, workgroup_size_y);
                        glfft_log("  Cost:         %8.3g\n", cost);
#endif

                        if (cost < minimum_cost)
                        {
#if 1
                            glfft_log("  New optimal solution! (%g -> %g)\n", minimum_cost, cost);
#endif
                            best_perf = perf;
                            minimum_cost = cost;
                        }
                    }
                    catch (...)
                    {
                        // If we pass in bogus parameters,
                        // FFT will throw and we just ignore this.
                    }
                }
            }
        }
    }

    glfft_log("Tested %u variants!\n", bench_count);
    return make_pair(minimum_cost, best_perf);
}

const pair<const WisdomPass, FFTOptions::Performance>* FFTWisdom::find_optimal_options(unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target, const FFTOptions::Type &type) const
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            type,
        },
        0.0,
    };

    auto itr = library.find(pass);
    return itr != end(library) ? (&(*itr)) : nullptr;
}

const FFTOptions::Performance& FFTWisdom::find_optimal_options_or_default(unsigned Nx, unsigned Ny, unsigned radix,
        Mode mode, Target input_target, Target output_target, const FFTOptions &base_options) const
{
    WisdomPass pass = {
        {
            Nx, Ny, radix, mode, input_target, output_target,
            base_options.type,
        },
        0.0,
    };

    auto itr = library.find(pass);

#if 1
    if (itr == end(library))
    {
        glfft_log("Didn't find options for (%u x %u, radix %u, mode %u, input_target %u, output_target %u)\n",
                Nx, Ny, radix, unsigned(mode), unsigned(input_target), unsigned(output_target));
    }
#endif

    return itr != end(library) ? itr->second : base_options.performance;
}

