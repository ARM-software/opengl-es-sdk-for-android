/* Copyright (c) 2014-2017, ARM Limited and Contributors
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

#include "vector_math.h"

#include <stddef.h>
#include <stdio.h>
#include <string>

#include "EGLRuntime.h"
#include "Platform.h"
using namespace MaliSDK;

#include <GLES3/gl31.h>

GLuint common_compile_shader(const char *vs_source, const char *fs_source);
GLuint common_compile_compute_shader(const char *cs_source);

GLuint common_compile_shader_from_file(const char *vs_source, const char *fs_source);
GLuint common_compile_compute_shader_from_file(const char *cs_source);

void common_set_basedir(const char *basedir);
FILE *common_fopen(const char *path, const char *mode);

std::string common_get_path(const char *basepath);

#endif
