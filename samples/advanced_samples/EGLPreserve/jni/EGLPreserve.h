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

#ifndef EGLPRESERVE_H
#define EGLPRESERVE_H

#define GLES_VERSION 2

#include <GLES2/gl2.h>

/* These indices describe the cube triangle strips, separated by degenerate triangles where necessary. */
static const GLubyte cubeIndices[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 0, 1,   1, 1,   1, 7, 3, 5,   5, 6,   6, 0, 4, 2,
};

/* Tri strips, so quads are in this order:
 *
 * 2 ----- 3
 * | \     |
 * |   \   |6 - 7
 * |     \ || \ |
 * 0 ----- 14 - 5
 */
static const float cubeVertices[] =
{
    -0.5f, -0.5f,  0.5f, /* 0 */
     0.5f, -0.5f,  0.5f, /* 1 */
    -0.5f,  0.5f,  0.5f, /* 2 */
     0.5f,  0.5f,  0.5f, /* 3 */
    -0.5f,  0.5f, -0.5f, /* 4 */
     0.5f,  0.5f, -0.5f, /* 5 */
    -0.5f, -0.5f, -0.5f, /* 6 */
     0.5f, -0.5f, -0.5f, /* 7 */
};

static const float cubeColors[] =
{
    0.0f, 0.0f, 0.0f, 1.0f, /* 0 */
    1.0f, 0.0f, 0.0f, 1.0f, /* 1 */
    0.0f, 1.0f, 0.0f, 1.0f, /* 2 */
    1.0f, 1.0f, 0.0f, 1.0f, /* 3 */
    0.0f, 0.0f, 1.0f, 1.0f, /* 4 */
    1.0f, 0.0f, 1.0f, 1.0f, /* 5 */
    0.0f, 1.0f, 1.0f, 1.0f, /* 6 */
    1.0f, 1.0f, 1.0f, 1.0f, /* 7 */
};

#endif /* EGLPRESERVE_H */