/* Copyright (c) 2012-2017, ARM Limited and Contributors
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

#ifndef ROTOZOOM_H
#define ROTOZOOM_H

#define GLES_VERSION 2 

#include <GLES2/gl2.h>

/* These indices describe the quad triangle strip. */
static const GLubyte quadIndices[] =
{
    0, 1, 2, 3,
};

/* Tri strips, so quad is in this order:
 *
 * 2 ----- 3
 * | \     |
 * |   \   |
 * |     \ |
 * 0 ----- 1
 */
static const float quadVertices[] =
{
    /* Front. */
    -1.0f, -1.0f,  0.0f, /* 0 */
     1.0f, -1.0f,  0.0f, /* 1 */
    -1.0f,  1.0f,  0.0f, /* 2 */
     1.0f,  1.0f,  0.0f, /* 3 */
};

static const float quadTextureCoordinates[] =
{
    /* Front. */
    0.0f, 1.0f, /* 0 */
    1.0f, 1.0f, /* 1 */
    0.0f, 0.0f, /* 2 */
    1.0f, 0.0f, /* 3 */ 
    /* Flipped Y coords. */
};

#endif /* ROTOZOOM_H */