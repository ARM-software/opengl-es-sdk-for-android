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

// For the most part used by the implementation.

#ifndef GLFFT_COMMON_HPP__
#define GLFFT_COMMON_HPP__

#include "glfft_interface.hpp"
#include <functional>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace GLFFT
{

enum Direction
{
    /// Forward FFT transform.
    Forward = -1,
    /// Inverse FFT transform, but with two inputs (in frequency domain) which are multiplied together
    /// for convolution.
    InverseConvolve = 0,
    /// Inverse FFT transform.
    Inverse = 1
};

enum Mode
{
    Horizontal,
    HorizontalDual,
    Vertical,
    VerticalDual,

    ResolveRealToComplex,
    ResolveComplexToReal,
};

enum Type
{
    /// Regular complex-to-complex transform.
    ComplexToComplex,
    /// Complex-to-complex dual transform where the complex value is four-dimensional,
    /// i.e. a vector of two complex values. Typically used to transform RGBA data.
    ComplexToComplexDual,
    /// Complex-to-real transform. N / 2 + 1 complex values are used per row with a stride of N complex samples.
    ComplexToReal,
    /// Real-to-complex transform. N / 2 + 1 complex output samples are created per row with a stride of N complex samples.
    RealToComplex
};

enum Target
{
    /// GL_SHADER_STORAGE_BUFFER
    SSBO,
    /// Textures, when used as output, type is determined by transform type.
    /// ComplexToComplex / RealToComplex -> GL_RG16F
    /// ComplexToComplexDual -> GL_RGBA16F
    Image,
    /// Real-valued (single component) textures, when used as output, type is determined by transform type.
    /// ComplexToReal -> GL_R32F (because GLES 3.1 doesn't have GL_R16F image type).
    ImageReal
};

struct Parameters
{
    unsigned workgroup_size_x;
    unsigned workgroup_size_y;
    unsigned workgroup_size_z;
    unsigned radix;
    unsigned vector_size;
    Direction direction;
    Mode mode;
    Target input_target;
    Target output_target;
    bool p1;
    bool pow2_stride;
    bool shared_banked;
    bool fft_fp16, input_fp16, output_fp16;
    bool fft_normalize;

    bool operator==(const Parameters &other) const
    {
        return std::memcmp(this, &other, sizeof(Parameters)) == 0;
    }
};

/// @brief Options for FFT implementation.
/// Defaults for performance as conservative.
struct FFTOptions
{
    struct Performance
    {
        /// Workgroup size used in layout(local_size_x).
        /// Only affects performance, however, large values may make implementations of smaller sized FFTs impossible.
        /// FFT constructor will throw in this case.
        unsigned workgroup_size_x = 4;
        /// Workgroup size used in layout(local_size_x).
        /// Only affects performance, however, large values may make implementations of smaller sized FFTs impossible.
        /// FFT constructor will throw in this case.
        unsigned workgroup_size_y = 1;
        /// Vector size. Very GPU dependent. "Scalar" GPUs prefer 2 here, vector GPUs prefer 4 (and maybe 8).
        unsigned vector_size = 2;
        /// Whether to use banked shared memory or not.
        /// Desktop GPUs prefer true here, false for mobile in general.
        bool shared_banked = false;
    } performance;

    struct Type
    {
        /// Whether internal shader should be mediump float.
        bool fp16 = false;
        /// Whether input SSBO is a packed 2xfp16 format. Otherwise, regular FP32.
        bool input_fp16 = false;
        /// Whether output SSBO is a packed 2xfp16 format. Otherwise, regular FP32.
        bool output_fp16 = false;
        /// Whether to apply 1 / N normalization factor.
        bool normalize = false;
    } type;
};

}

namespace std
{
    template<>
    struct hash<GLFFT::Parameters>
    {
        std::size_t operator()(const GLFFT::Parameters &params) const
        {
            std::size_t h = 0;
            hash<uint8_t> hasher;
            for (std::size_t i = 0; i < sizeof(GLFFT::Parameters); i++)
            {
                h ^= hasher(reinterpret_cast<const uint8_t*>(&params)[i]);
            }

            return h;
        }
    };
}

namespace GLFFT
{

class Buffer
{
    public:
        Buffer() = default;
        ~Buffer();

        Buffer(GLuint buffer);
        Buffer& operator=(Buffer &&buffer);
        Buffer(Buffer &&buffer);

        void init(const void *data, size_t size, GLenum access);

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class Texture
{
    public:
        Texture() = default;
        ~Texture();

        Texture(GLuint tex);
        Texture& operator=(Texture &&texture);
        Texture(Texture &&texture);

        void init(unsigned width, unsigned height, unsigned levels, GLenum internal_format,
                GLenum wrap_s = GL_REPEAT, GLenum wrap_t = GL_REPEAT,
                GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST);
        void upload(const void *data, GLenum format, GLenum type,
                unsigned x_off, unsigned y_off, unsigned width, unsigned height);

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class Program
{
    public:
        Program() = default;
        ~Program();

        Program(GLuint prog);
        Program& operator=(Program &&prog);
        Program(Program &&prog);

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class ProgramCache
{
    public:
        GLuint find_program(const Parameters &parameters) const;
        void insert_program(const Parameters &parameters, GLuint program);

        size_t cache_size() const { return programs.size(); }

    private:
        std::unordered_map<Parameters, Program> programs;
};

}

#endif

