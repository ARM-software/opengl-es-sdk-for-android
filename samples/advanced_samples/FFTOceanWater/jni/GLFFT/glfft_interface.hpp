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

#ifndef GLFFT_INTERFACE_HPP__
#define GLFFT_INTERFACE_HPP__

// Implement this header somewhere in your include path and include relevant GL/GLES API headers.
#include "glfft_api_headers.hpp"

#ifndef GLFFT_GLSL_LANG_STRING
#error GLFFT_GLSL_LANG_STRING must be defined to e.g. "#version 310 es\n" or "#version 430 core\n".
#endif

#ifndef GLFFT_LOG_OVERRIDE
// Implement this.
void glfft_log(const char *fmt, ...);
#else
#define glfft_log GLFFT_LOG_OVERRIDE
#endif

#ifndef GLFFT_TIME_OVERRIDE
// Implement this.
void glfft_time();
#else
#define glfft_time GLFFT_TIME_OVERRIDE
#endif

#ifndef GLFFT_READ_FILE_STRING_OVERRIDE
// Implement this.
bool glfft_read_file_string(const char *path, char **out_buf);
#else
#define glfft_read_file_string GLFFT_READ_FILE_STRING_OVERRIDE
#endif

#endif
