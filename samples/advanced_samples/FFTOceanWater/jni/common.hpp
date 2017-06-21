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

#ifndef COMMON_HPP__
#define COMMON_HPP__

#include <stddef.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include "EGLRuntime.h"
#include "Platform.h"
using namespace MaliSDK;

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

GLuint common_compile_shader(const char *vs_source, const char *fs_source);
GLuint common_compile_shader(const char *vs_source,
        const char *tc_source, const char *te_source, const char *geom_source,
        const char *fs_source);

GLuint common_compile_compute_shader(const char *cs_source);

GLuint common_compile_shader_from_file(const char *vs_source, const char *fs_source);
GLuint common_compile_shader_from_file(const char *vs_source,
        const char *tc_source, const char *te_source, const char *geom_source,
        const char *fs_source);
GLuint common_compile_compute_shader_from_file(const char *cs_source);

void common_set_basedir(const char *basedir);
FILE *common_fopen(const char *path, const char *mode);

std::string common_get_path(const char *basepath);

bool common_read_file_string(const char *path, char **out_buf);

inline bool common_has_extension(const char *ext)
{
    GL_CHECK(bool ret = strstr((const char*)glGetString(GL_EXTENSIONS), ext) != nullptr);
    if (ret)
    {
        LOGI("Extension %s is supported.\n", ext);
    }
    else
    {
        LOGI("Extension %s is unsupported.\n", ext);
    }
    return ret;
}

#endif
