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

#include "glfft.hpp"
#include <algorithm>
#include <stdexcept>
#include <numeric>
#include <fstream>
#include <sstream>
#include <assert.h>

#define GLFFT_SHADER_FROM_FILE

#ifndef GLFFT_SHADER_FROM_FILE
#include "glsl/fft_common.inc"
#include "glsl/fft_radix4.inc"
#include "glsl/fft_radix8.inc"
#include "glsl/fft_radix16.inc"
#include "glsl/fft_radix64.inc"
#include "glsl/fft_shared.inc"
#include "glsl/fft_main.inc"
#endif

using namespace std;
using namespace GLFFT;

struct WorkGroupSize
{
    unsigned x, y, z;
};

struct Radix
{
    WorkGroupSize size;
    unsigned num_workgroups_x;
    unsigned num_workgroups_y;
    unsigned radix;
    unsigned vector_size;
    bool shared_banked;
};

static void reduce(unsigned &wg_size, unsigned &divisor)
{
    if (divisor > 1 && wg_size >= divisor)
    {
        wg_size /= divisor;
        divisor = 1;
    }
    else if (divisor > 1 && wg_size < divisor)
    {
        divisor /= wg_size;
        wg_size = 1;
    }
}

static unsigned radix_to_wg_z(unsigned radix)
{
    switch (radix)
    {
        case 16:
            return 4;

        case 64:
            return 8;

        default:
            return 1;
    }
}

static Radix build_radix(unsigned Nx, unsigned Ny,
        Mode mode, unsigned vector_size, bool shared_banked, unsigned radix,
        WorkGroupSize size,
        bool pow2_stride)
{
    unsigned wg_x = 0, wg_y = 0;

    if (Ny == 1 && size.y > 1)
    {
        throw logic_error("WorkGroupSize.y must be 1, when Ny == 1.\n");
    }

    // To avoid too many threads per workgroup due to workgroup_size_z,
    // try to divide workgroup_size_y, then workgroup_size_x.
    // TODO: Make a better constraint solver which takes into account cache line sizes,
    // and image swizzling patterns, etc ... Not that critical though, since wisdom interface
    // will find the optimal options despite this.
    unsigned divisor = size.z;
    reduce(size.y, divisor);
    reduce(size.x, divisor);

    switch (mode)
    {
        case Vertical:
            // If we have pow2_stride, we need to transform 2^n + 1 elements horizontally,
            // so just add a single workgroup in X.
            // We pad by going up to pow2 stride anyways.
            // We will transform some garbage,
            // but it's better than transforming close to double the amount.
            wg_x = (2 * Nx) / (vector_size * size.x) + pow2_stride;
            wg_y = Ny / (size.y * radix);
            break;

        case VerticalDual:
            vector_size = max(vector_size, 4u);
            wg_x = (4 * Nx) / (vector_size * size.x);
            wg_y = Ny / (size.y * radix);
            break;

        case Horizontal:
            wg_x = (2 * Nx) / (vector_size * radix * size.x);
            wg_y = Ny / size.y;
            break;

        case HorizontalDual:
            vector_size = max(vector_size, 4u);
            wg_x = (4 * Nx) / (vector_size * radix * size.x);
            wg_y = Ny / size.y;
            break;

        default:
            assert(0);
    }

    return { size, wg_x, wg_y, radix, vector_size, shared_banked };
}

// Resolve radices are simpler, and don't yet support different vector sizes, etc.
static Radix build_resolve_radix(unsigned Nx, unsigned Ny, WorkGroupSize size)
{
    return { size, Nx / size.x, Ny / size.y, 2, 2, false };
}

// Smaller FFT with larger workgroups are not always possible to create.
static bool is_radix_valid(unsigned Nx, unsigned Ny,
        Mode mode, unsigned vector_size, unsigned radix,
        WorkGroupSize size,
        bool pow2_stride)
{
    auto res = build_radix(Nx, Ny,
            mode, vector_size, false, radix,
            size,
            pow2_stride);

    return res.num_workgroups_x > 0 && res.num_workgroups_y > 0;
}

static double find_cost(unsigned Nx, unsigned Ny, Mode mode, unsigned radix,
        const FFTOptions &options, const FFTWisdom &wisdom)
{
    auto opt = wisdom.find_optimal_options(Nx, Ny, radix, mode, SSBO, SSBO, options.type);

    // Return a very rough estimate if we cannot find cost.
    // The cost functions generated here are expected to be huge,
    // always much larger than true cost functions.
    // The purpose of this is to give a strong bias towards radices we have wisdom for.
    // We also give a bias towards larger radices, since they are generally more BW efficient.
    return opt ? opt->first.cost : Nx * Ny * (log2(float(radix)) + 2.0f);
}

struct CostPropagate
{
    CostPropagate() = default;
    CostPropagate(double cost, vector<unsigned> radices)
        : cost(cost), radices(move(radices)) {}

    void merge_if_better(const CostPropagate &a, const CostPropagate &b)
    {
        double new_cost = a.cost + b.cost;

        if ((cost == 0.0 || new_cost < cost) && a.cost != 0.0 && b.cost != 0.0)
        {
            cost = new_cost;
            radices = a.radices;
            radices.insert(end(radices), begin(b.radices), end(b.radices));
        }
    }

    double cost = 0.0;
    vector<unsigned> radices;
};

static vector<Radix> split_radices(unsigned Nx, unsigned Ny, Mode mode, Target input_target, Target output_target,
        const FFTOptions &options,
        bool pow2_stride, const FFTWisdom &wisdom, double &accumulate_cost)
{
    unsigned N;
    switch (mode)
    {
        case Vertical:
        case VerticalDual:
            N = Ny;
            break;

        case Horizontal:
        case HorizontalDual:
            N = Nx;
            break;

        default:
            return {};
    }

    // N == 1 is for things like Nx1 transforms where we don't do any vertical transforms.
    if (N == 1)
    {
        return {};
    }

    // Treat cost 0.0 as invalid.
    double cost_table[8] = {0.0};
    CostPropagate cost_propagate[32];

    // Fill table with fastest known ways to do radix 4, radix 8, radix 16, and 64.
    // We'll then find the optimal subdivision which has the lowest additive cost.
    cost_table[2] = find_cost(Nx, Ny, mode,  4, options, wisdom);
    cost_table[3] = find_cost(Nx, Ny, mode,  8, options, wisdom);
    cost_table[4] = find_cost(Nx, Ny, mode, 16, options, wisdom);
    cost_table[6] = find_cost(Nx, Ny, mode, 64, options, wisdom);

    auto is_valid = [&](unsigned radix) -> bool {
        unsigned workgroup_size_z = radix_to_wg_z(radix);
        auto &opt = wisdom.find_optimal_options_or_default(Nx, Ny, radix, mode, SSBO, SSBO, options);

        // We don't want pow2_stride to round up a very inefficient work group and make the is_valid test pass.
        return is_radix_valid(Nx, Ny,
                mode, opt.vector_size, radix,
                { opt.workgroup_size_x, opt.workgroup_size_y, workgroup_size_z },
                false);
    };

    // If our work-space is too small to allow certain radices, we disable them from consideration here.
    for (unsigned i = 2; i <= 6; i++)
    {
        // Don't check the composite radix.
        if (i == 5)
        {
            continue;
        }

        if (is_valid(1 << i))
        {
            cost_propagate[i] = CostPropagate(cost_table[i], { 1u << i });
        }
    }

    // Now start bubble this up all the way to N, starting from radix 16.
    for (unsigned i = 4; (1u << i) <= N; i++)
    {
        auto &target = cost_propagate[i];

        for (unsigned r = 2; i - r >= r; r++)
        {
            target.merge_if_better(cost_propagate[r], cost_propagate[i - r]);
        }

        if ((1u << i) == N && target.cost == 0.0)
        {
            throw logic_error("There is no possible subdivision ...\n");
        }
    }

    // Ensure that the radix splits are sensible.
    // A radix-N non p-1 transform mandates that p factor is at least N.
    // Sort the splits so that larger radices come first.
    // For composite radices like 16 and 64, they are built with 4x4 and 8x8, so we only
    // need p factors for 4 and 8 for those cases.
    // The cost function doesn't depend in which order we split the radices.
    auto &cost = cost_propagate[unsigned(log2(float(N)))];
    auto radices = move(cost.radices);

    sort(begin(radices), end(radices), greater<unsigned>());

    if (accumulate(begin(radices), end(radices), 1u, multiplies<unsigned>()) != N)
    {
        throw logic_error("Radix splits are invalid.");
    }

    vector<Radix> radices_out;
    radices_out.reserve(radices.size());

    // Fill in the structs with all information.
    for (auto radix : radices)
    {
        bool first = radices_out.empty();
        bool last = radices_out.size() + 1 == radices.size();

        // Use known performance options as a fallback.
        // We used SSBO -> SSBO cost functions to find the optimal radix splits,
        // but replace first and last options with Image -> SSBO / SSBO -> Image cost functions if appropriate.
        auto &orig_opt = wisdom.find_optimal_options_or_default(Nx, Ny, radix, mode, SSBO, SSBO, options);
        auto &opts = wisdom.find_optimal_options_or_default(Nx, Ny, radix, mode,
                first ? input_target : SSBO,
                last ? output_target : SSBO,
                { orig_opt, options.type });

        radices_out.push_back(build_radix(Nx, Ny,
                    mode, opts.vector_size, opts.shared_banked, radix,
                    { opts.workgroup_size_x, opts.workgroup_size_y, radix_to_wg_z(radix) },
                    pow2_stride));
    }

    accumulate_cost += cost.cost;
    return radices_out;
}

GLuint ProgramCache::find_program(const Parameters &parameters) const
{
    auto itr = programs.find(parameters);
    if (itr != end(programs))
    {
        return itr->second.get();
    }
    else
    {
        return 0;
    }
}

void ProgramCache::insert_program(const Parameters &parameters, GLuint program)
{
    programs[parameters] = Program(program);
}

GLuint FFT::get_program(const Parameters &params)
{
    GLuint prog = cache->find_program(params);
    if (!prog)
    {
        prog = build_program(params);
        if (!prog)
        {
            throw runtime_error("Failed to compile shader.\n");
        }
        cache->insert_program(params, prog);
    }
    return prog;
}

static inline unsigned mode_to_input_components(Mode mode)
{
    switch (mode)
    {
        case HorizontalDual:
        case VerticalDual:
            return 4;

        case Horizontal:
        case Vertical:
        case ResolveComplexToReal:
            return 2;

        case ResolveRealToComplex:
            return 1;

        default:
            return 0;
    }
}

FFT::FFT(unsigned Nx, unsigned Ny,
        unsigned radix, unsigned p,
        Mode mode, Target input_target, Target output_target,
        std::shared_ptr<ProgramCache> program_cache, const FFTOptions &options)
    : cache(move(program_cache)), size_x(Nx), size_y(Ny)
{
    set_texture_offset_scale(0.5f / Nx, 0.5f / Ny, 1.0f / Nx, 1.0f / Ny);

    if (!Nx || !Ny || (Nx & (Nx - 1)) || (Ny & (Ny - 1)))
    {
        throw logic_error("FFT size is not POT.");
    }

    if (p != 1 && input_target != SSBO)
    {
        throw logic_error("P != 1 only supported with SSBO as input.");
    }

    if (p < radix && output_target != SSBO)
    {
        throw logic_error("P < radix only supported with SSBO as output.");
    }

    // We don't really care about transform direction since it's just a matter of sign-flipping twiddles,
    // but we have to obey some fundamental assumptions of resolve passes.
    Direction direction = mode == ResolveComplexToReal ? Inverse : Forward;

    Radix res;
    if (mode == ResolveRealToComplex || mode == ResolveComplexToReal)
    {
        res = build_resolve_radix(Nx, Ny, { options.performance.workgroup_size_x, options.performance.workgroup_size_y, 1 });
    }
    else
    {
        res = build_radix(Nx, Ny,
                mode, options.performance.vector_size, options.performance.shared_banked, radix,
                { options.performance.workgroup_size_x, options.performance.workgroup_size_y, radix_to_wg_z(radix) },
                false);
    }

    const Parameters params = {
        res.size.x,
        res.size.y,
        res.size.z,
        res.radix,
        res.vector_size,
        direction,
        mode,
        input_target,
        output_target,
        p == 1,
        false,
        res.shared_banked,
        options.type.fp16, options.type.input_fp16, options.type.output_fp16,
        options.type.normalize,
    };

    if (res.num_workgroups_x == 0 || res.num_workgroups_y == 0)
    {
        throw logic_error("Invalid workgroup sizes for this radix.");
    }

    unsigned uv_scale_x = res.vector_size / mode_to_input_components(mode);
    const Pass pass = {
        params,
        res.num_workgroups_x, res.num_workgroups_y,
        uv_scale_x,
        get_program(params),
        0u,
    };

    passes.push_back(pass);
}

static inline void print_radix_splits(const vector<Radix> radices[2])
{
    glfft_log("Transform #1\n");
    for (auto &radix : radices[0])
    {
        glfft_log("  Size: (%u, %u, %u)\n",
                radix.size.x, radix.size.y, radix.size.z);
        glfft_log("  Dispatch: (%u, %u)\n",
                radix.num_workgroups_x, radix.num_workgroups_y);
        glfft_log("  Radix: %u\n",
                radix.radix);
        glfft_log("  VectorSize: %u\n\n",
                radix.vector_size);
    }

    glfft_log("Transform #2\n");
    for (auto &radix : radices[1])
    {
        glfft_log("  Size: (%u, %u, %u)\n",
                radix.size.x, radix.size.y, radix.size.z);
        glfft_log("  Dispatch: (%u, %u)\n",
                radix.num_workgroups_x, radix.num_workgroups_y);
        glfft_log("  Radix: %u\n",
                radix.radix);
        glfft_log("  VectorSize: %u\n\n",
                radix.vector_size);
    }
}

static inline unsigned type_to_input_components(Type type)
{
    switch (type)
    {
        case ComplexToComplex:
        case ComplexToReal:
            return 2;

        case RealToComplex:
            return 1;

        case ComplexToComplexDual:
            return 4;

        default:
            return 0;
    }
}

FFT::FFT(unsigned Nx, unsigned Ny,
        Type type, Direction direction, Target input_target, Target output_target,
        std::shared_ptr<ProgramCache> program_cache, const FFTOptions &options, const FFTWisdom &wisdom)
    : cache(move(program_cache)), size_x(Nx), size_y(Ny)
{
    set_texture_offset_scale(0.5f / Nx, 0.5f / Ny, 1.0f / Nx, 1.0f / Ny);

    size_t temp_buffer_size = Nx * Ny * sizeof(float) * (type == ComplexToComplexDual ? 4 : 2);
    temp_buffer_size >>= options.type.output_fp16;

    temp_buffer.init(nullptr, temp_buffer_size, GL_STREAM_COPY);
    if (output_target != SSBO)
    {
        temp_buffer_image.init(nullptr, temp_buffer_size, GL_STREAM_COPY);
    }

    bool expand = false;
    if (type == ComplexToReal || type == RealToComplex)
    {
        // If we're doing C2R or R2C, we'll need double the scratch memory,
        // so make sure we're dividing Nx *after* allocating.
        Nx /= 2;
        expand = true;
    }

    // Sanity checks.
    if (!Nx || !Ny || (Nx & (Nx - 1)) || (Ny & (Ny - 1)))
    {
        throw logic_error("FFT size is not POT.");
    }

    if (type == ComplexToReal && direction == Forward)
    {
        throw logic_error("ComplexToReal transforms requires inverse transform.");
    }

    if (type == RealToComplex && direction != Forward)
    {
        throw logic_error("RealToComplex transforms requires forward transform.");
    }

    if (type == RealToComplex && input_target == Image)
    {
        throw logic_error("Input real-to-complex must use ImageReal target.");
    }

    if (type == ComplexToReal && output_target == Image)
    {
        throw logic_error("Output complex-to-real must use ImageReal target.");
    }

    vector<Radix> radices[2];
    Mode modes[2];
    Target targets[4];

    switch (direction)
    {
        case Forward:
            modes[0] = type == ComplexToComplexDual ? HorizontalDual : Horizontal;
            modes[1] = type == ComplexToComplexDual ? VerticalDual : Vertical;

            targets[0] = input_target;
            targets[1] = Ny > 1 ? SSBO : output_target;
            targets[2] = targets[1];
            targets[3] = output_target;

            radices[0] = split_radices(Nx, Ny, modes[0], targets[0], targets[1], options, false, wisdom, cost);
            radices[1] = split_radices(Nx, Ny, modes[1], targets[2], targets[3], options, expand, wisdom, cost);
            break;

        case Inverse:
        case InverseConvolve:
            modes[0] = type == ComplexToComplexDual ? VerticalDual : Vertical;
            modes[1] = type == ComplexToComplexDual ? HorizontalDual : Horizontal;

            targets[0] = input_target;
            targets[1] = Ny > 1 ? SSBO : input_target;
            targets[2] = targets[1];
            targets[3] = output_target;

            radices[0] = split_radices(Nx, Ny, modes[0], targets[0], targets[1], options, expand, wisdom, cost);
            radices[1] = split_radices(Nx, Ny, modes[1], targets[2], targets[3], options, false, wisdom, cost);
            break;
    }

#if 1
    print_radix_splits(radices);
#endif

    passes.reserve(radices[0].size() + radices[1].size() + expand);

    unsigned index = 0;
    unsigned last_index = (radices[1].empty() && !expand) ? 0 : 1;

    for (auto &radix_direction : radices)
    {
        unsigned p = 1;
        unsigned i = 0;
        
        // If we have R2C or C2R, we have a padded buffer to accomodate 2^n + 1 elements horizontally.
        // For simplicity, this is implemented as a shader variant.
        bool pow2_stride = expand && modes[index] == Vertical;

        for (auto &radix : radix_direction)
        {
            // If this is the last pass and we're writing to an image, use a special shader variant.
            bool last_pass = index == last_index && i == radix_direction.size() - 1;

            bool input_fp16 = passes.empty() ? options.type.input_fp16 : options.type.output_fp16;
            Target out_target = last_pass ? output_target : SSBO;
            Target in_target = passes.empty() ? input_target : SSBO;
            Direction dir = direction == InverseConvolve && !passes.empty() ? Inverse : direction;
            unsigned uv_scale_x = radix.vector_size / type_to_input_components(type);

            const Parameters params = {
                radix.size.x,
                radix.size.y,
                radix.size.z,
                radix.radix,
                radix.vector_size,
                dir,
                modes[index],
                in_target,
                out_target,
                p == 1,
                pow2_stride,
                radix.shared_banked,
                options.type.fp16, input_fp16, options.type.output_fp16,
                options.type.normalize,
            };

            // For last pass, we don't know how our resource will be used afterwards,
            // so let barrier decisions be up to the API user.
            const Pass pass = {
                params,
                radix.num_workgroups_x, radix.num_workgroups_y,
                uv_scale_x,
                get_program(params),
                last_pass ? 0u : GL_SHADER_STORAGE_BARRIER_BIT,
            };

            passes.push_back(pass);

            p *= radix.radix;
            i++;
        }

        // After the first transform direction, inject either a real-to-complex resolve or complex-to-real resolve.
        // This way, we avoid having special purpose transforms for all FFT variants.
        if (index == 0 && (type == ComplexToReal || type == RealToComplex))
        {
            bool input_fp16 = passes.empty() ? options.type.input_fp16 : options.type.output_fp16;
            bool last_pass = radices[1].empty();
            Direction dir = direction == InverseConvolve && !passes.empty() ? Inverse : direction;
            Target in_target = passes.empty() ? input_target : SSBO;
            Target out_target = last_pass ? output_target : SSBO;
            Mode mode = type == ComplexToReal ? ResolveComplexToReal : ResolveRealToComplex;
            unsigned uv_scale_x = 1;

            auto base_opts = options;
            base_opts.type.input_fp16 = input_fp16;

            auto &opts = wisdom.find_optimal_options_or_default(Nx, Ny, 2, mode, in_target, out_target, base_opts);
            auto res = build_resolve_radix(Nx, Ny, { opts.workgroup_size_x, opts.workgroup_size_y, 1 });

            const Parameters params = {
                res.size.x,
                res.size.y,
                res.size.z,
                res.radix,
                res.vector_size,
                dir,
                mode,
                in_target,
                out_target,
                true,
                false,
                false,
                base_opts.type.fp16, base_opts.type.input_fp16, base_opts.type.output_fp16,
                base_opts.type.normalize,
            };

            const Pass pass = {
                params,
                Nx / res.size.x,
                Ny / res.size.y,
                uv_scale_x,
                get_program(params),
                GL_SHADER_STORAGE_BARRIER_BIT,
            };

            passes.push_back(pass);
        }

        index++;
    }
}

string FFT::load_shader_string(const char *path)
{
    char *buf = nullptr;
    if (!glfft_read_file_string(path, &buf))
    {
        throw runtime_error("Failed to load shader file from disk.\n");
    }

    string ret(buf);
    free(buf);
    return ret;
}

void FFT::store_shader_string(const char *path, const string &source)
{
    ofstream file(path);
    file.write(source.data(), source.size());
}

GLuint FFT::build_program(const Parameters &params)
{
    string str;
    str.reserve(16 * 1024);

#if 0
    glfft_log("Building program:\n");
    glfft_log(
            "   WG_X:      %u\n"
            "   WG_Y:      %u\n"
            "   WG_Z:      %u\n"
            "   P1:        %u\n"
            "   Radix:     %u\n"
            "   Dir:       %d\n"
            "   Mode:      %u\n"
            "   InTarget:  %u\n"
            "   OutTarget: %u\n"
            "   POW2:      %u\n"
            "   FP16:      %u\n"
            "   InFP16:    %u\n"
            "   OutFP16:   %u\n"
            "   Norm:      %u\n",
            params.workgroup_size_x,
            params.workgroup_size_y,
            params.workgroup_size_z,
            params.p1,
            params.radix,
            params.direction,
            params.mode,
            params.input_target,
            params.output_target,
            params.pow2_stride,
            params.fft_fp16,
            params.input_fp16,
            params.output_fp16,
            params.fft_normalize);
#endif

    if (params.p1)
    {
        str += "#define FFT_P1\n";
    }

    if (params.pow2_stride)
    {
        str += "#define FFT_POW2_STRIDE\n";
    }

    if (params.fft_fp16)
    {
        str += "#define FFT_FP16\n";
    }

    if (params.input_fp16)
    {
        str += "#define FFT_INPUT_FP16\n";
    }

    if (params.output_fp16)
    {
        str += "#define FFT_OUTPUT_FP16\n";
    }

    if (params.fft_normalize)
    {
        str += "#define FFT_NORMALIZE\n";
    }

    if (params.direction == InverseConvolve)
    {
        str += "#define FFT_CONVOLVE\n";
    }

    str += params.shared_banked ? "#define FFT_SHARED_BANKED 1\n" : "#define FFT_SHARED_BANKED 0\n";

    str += params.direction == Forward ? "#define FFT_FORWARD\n" : "#define FFT_INVERSE\n";
    str += string("#define FFT_RADIX ") + to_string(params.radix) + "\n";
    
    unsigned vector_size = params.vector_size;
    switch (params.mode)
    {
        case VerticalDual:
            str += "#define FFT_DUAL\n";
            str += "#define FFT_VERT\n";
            break;

        case Vertical:
            str += "#define FFT_VERT\n";
            break;

        case HorizontalDual:
            str += "#define FFT_DUAL\n";
            str += "#define FFT_HORIZ\n";
            break;

        case Horizontal:
            str += "#define FFT_HORIZ\n";
            break;

        case ResolveRealToComplex:
            str += "#define FFT_RESOLVE_REAL_TO_COMPLEX\n";
            str += "#define FFT_HORIZ\n";
            vector_size = 2;
            break;

        case ResolveComplexToReal:
            str += "#define FFT_RESOLVE_COMPLEX_TO_REAL\n";
            str += "#define FFT_HORIZ\n";
            vector_size = 2;
            break;
    }

    switch (params.input_target)
    {
        case ImageReal:
            str += "#define FFT_INPUT_REAL\n";
            // Fallthrough
        case Image:
            str += "#define FFT_INPUT_TEXTURE\n";
            break;

        default:
            break;
    }

    switch (params.output_target)
    {
        case ImageReal:
            str += "#define FFT_OUTPUT_REAL\n";
            // Fallthrough
        case Image:
            str += "#define FFT_OUTPUT_IMAGE\n";
            break;

        default:
            break;
    }

    switch (vector_size)
    {
        case 2:
            str += "#define FFT_VEC2\n";
            break;

        case 4:
            str += "#define FFT_VEC4\n";
            break;

        case 8:
            str += "#define FFT_VEC8\n";
            break;
    }

    str += string("layout(local_size_x = ") +
        to_string(params.workgroup_size_x) +
        ", local_size_y = " +
        to_string(params.workgroup_size_y) +
        ", local_size_z = " +
        to_string(params.workgroup_size_z) +
        ") in;\n";

#ifdef GLFFT_SHADER_FROM_FILE
    str += load_shader_string("fft_common.comp");
    switch (params.radix)
    {
        case 4:
            str += load_shader_string("fft_radix4.comp");
            break;

        case 8:
            str += load_shader_string("fft_radix8.comp");
            break;

        case 16:
            str += load_shader_string("fft_radix4.comp");
            str += load_shader_string("fft_shared.comp");
            str += load_shader_string("fft_radix16.comp");
            break;

        case 64:
            str += load_shader_string("fft_radix8.comp");
            str += load_shader_string("fft_shared.comp");
            str += load_shader_string("fft_radix64.comp");
            break;
    }
    str += load_shader_string("fft_main.comp");
#else
    str += Blob::fft_common_source;
    switch (params.radix)
    {
        case 4:
            str += Blob::fft_radix4_source;
            break;

        case 8:
            str += Blob::fft_radix8_source;
            break;

        case 16:
            str += Blob::fft_radix4_source;
            str += Blob::fft_shared_source;
            str += Blob::fft_radix16_source;
            break;

        case 64:
            str += Blob::fft_radix8_source;
            str += Blob::fft_shared_source;
            str += Blob::fft_radix64_source;
            break;
    }
    str += Blob::fft_main_source;
#endif

    GLuint prog = compile_compute_shader(str.c_str());
    if (!prog)
    {
        puts(str.c_str());
    }

#if 0
    char shader_path[1024];
    snprintf(shader_path, sizeof(shader_path), "glfft_shader_radix%u_first%u_mode%u_in_target%u_out_target%u.comp.src",
            params.radix, params.p1, params.mode, unsigned(params.input_target), unsigned(params.output_target));
    store_shader_string(shader_path, str);
#endif

    return prog;
}

GLuint FFT::compile_compute_shader(const char *src)
{
    GL_CHECK(GLuint program = glCreateProgram());
    if (!program)
    {
        return 0;
    }

    GL_CHECK(GLuint shader = glCreateShader(GL_COMPUTE_SHADER));

    const char *sources[] = { GLFFT_GLSL_LANG_STRING, src };
    GL_CHECK(glShaderSource(shader, 2, sources, NULL));
    GL_CHECK(glCompileShader(shader));

    GLint status;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
        vector<char> buf(len);
        GL_CHECK(glGetShaderInfoLog(shader, len, &out_len, buf.data()));
        glfft_log("GLFFT: Shader log:\n%s\n\n", buf.data());

        GL_CHECK(glDeleteShader(shader));
        GL_CHECK(glDeleteProgram(program));
        return 0;
    }

    GL_CHECK(glAttachShader(program, shader));
    GL_CHECK(glLinkProgram(program));
    GL_CHECK(glDeleteShader(shader));

    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &status));
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;
        GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len));
        vector<char> buf(len);
        GL_CHECK(glGetProgramInfoLog(program, len, &out_len, buf.data()));
        glfft_log("Program log:\n%s\n\n", buf.data());

        GL_CHECK(glDeleteProgram(program));
        GL_CHECK(glDeleteShader(shader));
        return 0;
    }

    return program;
}

double FFT::bench(GLuint output, GLuint input,
        unsigned warmup_iterations, unsigned iterations, unsigned dispatches_per_iteration, double max_time)
{
    GL_CHECK(glFinish());
    for (unsigned i = 0; i < warmup_iterations; i++)
    {
        process(output, input);
    }
    GL_CHECK(glFinish());

    unsigned runs = 0;
    double start_time = glfft_time();
    double total_time = 0.0;

    for (unsigned i = 0; i < iterations && (((glfft_time() - start_time) < max_time) || i == 0); i++)
    {
        double iteration_start = glfft_time();
        for (unsigned d = 0; d < dispatches_per_iteration; d++)
        {
            process(output, input);
            GL_CHECK(glMemoryBarrier(GL_ALL_BARRIER_BITS));
            runs++;
        }
        GL_CHECK(glFinish());
        double iteration_end = glfft_time();
        total_time += iteration_end - iteration_start;
    }

    return total_time / runs;
}

void FFT::process(GLuint output, GLuint input, GLuint input_aux)
{
    if (passes.empty())
    {
        return;
    }

    GLuint buffers[2] = {
        input,
        passes.size() & 1 ?
            (passes.back().parameters.output_target != SSBO ? temp_buffer_image.get() : output) :
            temp_buffer.get(),
    };

    if (input_aux != 0)
    {
        if (passes.front().parameters.input_target != SSBO)
        {
            GL_CHECK(glActiveTexture(GL_TEXTURE1));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, input_aux));
            GL_CHECK(glBindSampler(1, texture.samplers[1]));
        }
        else
        {
            if (ssbo.input_aux.size != 0)
            {
                GL_CHECK(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, input_aux,
                        ssbo.input_aux.offset, ssbo.input_aux.size));
            }
            else
            {
                GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, input_aux));
            }
        }
    }

    GLuint current_program = 0;
    unsigned p = 1;
    unsigned pass_index = 0;
    for (auto &pass : passes)
    {
        if (pass.program != current_program)
        {
            GL_CHECK(glUseProgram(pass.program));
            current_program = pass.program;
        }

        if (pass.parameters.p1)
        {
            p = 1;
        }
        else
        {
            glUniform1ui(0, p);
        }

        p *= pass.parameters.radix;

        if (pass.parameters.input_target != SSBO)
        {
            GL_CHECK(glActiveTexture(GL_TEXTURE0));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, buffers[0]));
            GL_CHECK(glBindSampler(0, texture.samplers[0]));

            // If one compute thread reads multiple texels in X dimension, scale this accordingly.
            float scale_x = texture.scale_x * pass.uv_scale_x;
            GL_CHECK(glUniform2f(1, texture.offset_x, texture.offset_y));
            GL_CHECK(glUniform2f(2, scale_x, texture.scale_y));
        }
        else
        {
            if (buffers[0] == input && ssbo.input.size != 0)
            {
                GL_CHECK(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffers[0],
                        ssbo.input.offset, ssbo.input.size));
            }
            else if (buffers[0] == output && ssbo.output.size != 0)
            {
                // This can behave weirdly if output is an image and our temp buffers GLuint aliases with
                // the output texture name, but we shouldn't set ssbo.output.size in this case anyways.
                GL_CHECK(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffers[0],
                        ssbo.output.offset, ssbo.output.size));
            }
            else
            {
                GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[0]));
            }
        }

        if (pass.parameters.output_target != SSBO)
        {
            GLenum format = 0;

            // TODO: Make this more flexible, would require shader variants per-format though.
            if (pass.parameters.output_target == ImageReal)
            {
                format = GL_R32F;
            }
            else
            {
                switch (pass.parameters.mode)
                {
                    case VerticalDual:
                    case HorizontalDual:
                        format = GL_RGBA16F;
                        break;

                    case Vertical:
                    case Horizontal:
                    case ResolveRealToComplex:
                        format = GL_R32UI;
                        break;

                    default:
                        break;
                }
            }
            GL_CHECK(glBindImageTexture(0, output, 0, GL_FALSE, 0, GL_WRITE_ONLY, format));
        }
        else
        {
            if (buffers[1] == output && ssbo.output.size != 0)
            {
                GL_CHECK(glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, buffers[1],
                        ssbo.output.offset, ssbo.output.size));
            }
            else
            {
                GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1]));
            }
        }

        GL_CHECK(glDispatchCompute(pass.workgroups_x, pass.workgroups_y, 1));

        if (pass.barriers != 0)
        {
            GL_CHECK(glMemoryBarrier(pass.barriers));
        }

        if (pass_index == 0)
        {
            buffers[0] = passes.size() & 1 ?
                temp_buffer.get() :
                (passes.back().parameters.output_target != SSBO ? temp_buffer_image.get() : output);
        }

        swap(buffers[0], buffers[1]);
        pass_index++;
    }
}

