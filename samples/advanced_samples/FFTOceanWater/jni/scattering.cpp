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

#include "scattering.hpp"
#include <utility>

Scattering::Scattering()
{
    prog = common_compile_compute_shader_from_file("scattering.comp");
}

Scattering::~Scattering()
{
    if (tex != 0)
    {
        GL_CHECK(glDeleteTextures(1, &tex));
    }

    if (prog != 0)
    {
        GL_CHECK(glDeleteProgram(prog));
    }
}

void Scattering::generate(unsigned size, vec3 sun_dir)
{
    if (tex != 0)
    {
        GL_CHECK(glDeleteTextures(1, &tex));
    }

    bool mipmap_fp16 = common_has_extension("GL_EXT_color_buffer_half_float");

    GL_CHECK(glGenTextures(1, &tex));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, tex));
    GL_CHECK(glTexStorage2D(GL_TEXTURE_CUBE_MAP, mipmap_fp16 ? int(log2(float(size))) + 1 : 1, GL_RGBA16F, size, size));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mipmap_fp16 ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR));

    GL_CHECK(glUseProgram(prog));
    GL_CHECK(glUniform3fv(0, 1, value_ptr(sun_dir)));

    int steps = 100;
    GL_CHECK(glUniform1i(1, steps));

    float start = 6500000.0f;
    float end = 7000000.0f;
    GL_CHECK(glUniform1f(2, start));
    GL_CHECK(glUniform1f(3, (end - start) / steps));

    GL_CHECK(glBindImageTexture(0, tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F));
    GL_CHECK(glDispatchCompute(size / 8, size / 8, 6));
    GL_CHECK(glMemoryBarrier(GL_ALL_BARRIER_BITS));

    if (mipmap_fp16)
    {
        GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, tex));
        GL_CHECK(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    }
}

