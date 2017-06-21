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

#ifndef INSTANCING_H
#define INSTANCING_H

namespace MaliSDK
{
    /** Name of a fragment shader file. */
    #define FRAGMENT_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.instancing/files/fragment_shader_source.frag")
    /* Number of colour components: we will be using RGBA values. */
    #define NUMBER_OF_COLOR_COMPONENTS (4)
    /* [Define number of elements to render] */
    /* Number of cubes that are drawn on a screen. */
    #define NUMBER_OF_CUBES (10)
/* [Define number of elements to render] */
    /** Name of a vertex shader file. */
    #define VERTEX_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.instancing/files/vertex_shader_source.vert")
}
#endif /* INSTANCING_H */
