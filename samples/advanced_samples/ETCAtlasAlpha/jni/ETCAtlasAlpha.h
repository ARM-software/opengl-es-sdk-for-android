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

#ifndef ETCATLASALPHA_H
#define ETCATLASALPHA_H

#define GLES_VERSION 2

#include <GLES2/gl2.h>

/* These indices describe triangle strips, separated by degenerate triangles where necessary. */
static const GLubyte indices[] =
{
     0,  1,  2,  3,    3,  4,    4,  5,  6,  7,    7,  8,
     8,  9, 10, 11,   11, 12,   12, 13, 14, 15,   15, 16,
    16, 17, 18, 19,   19, 20,   20, 21, 22, 23,   23, 24,
    24, 25, 26, 27,   27, 28,   28, 29, 30, 31,   31, 32,
    32, 33, 34, 35,
};

/* Tri strips, so quads are in this order:
 * 
 * 0 ----- 24 - 68-10 etc.
 * |     / || / |9-11
 * |   /   |5 - 7
 * | /     |
 * 1 ----- 3
 */
static const GLfloat vertices[] =
{
    /* 256 x 128 */
    -1.000000f,  1.0000f, 0.0f, /* 0 */
    -1.000000f,  0.4666f, 0.0f, /* 1 */
    -0.200000f,  1.0000f, 0.0f, /* 2 */
    -0.200000f,  0.4666f, 0.0f, /* 3 */
    /* 128 x 64 */
    -0.200000f,  1.0000f, 0.0f, /* 4 */
    -0.200000f,  0.7333f, 0.0f, /* 5 */
     0.200000f,  1.0000f, 0.0f, /* 6 */
     0.200000f,  0.7333f, 0.0f, /* 7 */
    /* 64 x 32 */
     0.200000f,  1.0000f, 0.0f, /* 8 */
     0.200000f,  0.8666f, 0.0f, /* 9 */
     0.400000f,  1.0000f, 0.0f, /* 10 */
     0.400000f,  0.8666f, 0.0f, /* 11 */
    /* 32 x 16 */
     0.400000f,  1.0000f, 0.0f, /* 12 */
     0.400000f,  0.9333f, 0.0f, /* 13 */
     0.500000f,  1.0000f, 0.0f, /* 14 */
     0.500000f,  0.9333f, 0.0f, /* 15 */
    /* 16 x 8 */
     0.500000f,  1.0000f, 0.0f, /* 16 */
     0.500000f,  0.9666f, 0.0f, /* 17 */
     0.550000f,  1.0000f, 0.0f, /* 18 */
     0.550000f,  0.9666f, 0.0f, /* 19 */
    /* 8 x 4 */
     0.550000f,  1.0000f, 0.0f, /* 20 */
     0.550000f,  0.9833f, 0.0f, /* 21 */
     0.575000f,  1.0000f, 0.0f, /* 22 */
     0.575000f,  0.9833f, 0.0f, /* 23 */
    /* 4 x 2 */
     0.575000f,  1.0000f, 0.0f, /* 24 */
     0.575000f,  0.9916f, 0.0f, /* 25 */
     0.587500f,  1.0000f, 0.0f, /* 26 */
     0.587500f,  0.9916f, 0.0f, /* 27 */
    /* 2 x 1 */
     0.587500f,  1.0000f, 0.0f, /* 28 */
     0.587500f,  0.9958f, 0.0f, /* 29 */
     0.593750f,  1.0000f, 0.0f, /* 30 */
     0.593750f,  0.9958f, 0.0f, /* 31 */
    /* 1 x 1 */
     0.593750f,  1.0000f, 0.0f, /* 32 */
     0.593750f,  0.9958f, 0.0f, /* 33 */
     0.596875f,  1.0000f, 0.0f, /* 34 */
     0.596875f,  0.9958f, 0.0f, /* 35 */
};

/* Because textures are loaded flipped, (0, 0) refers to top left corner. */
/* The texture orientation is the same for each quad. */
static const GLfloat textureCoordinates[] =
{
    0.0f, 0.0f, /* 0 */
    0.0f, 1.0f, /* 1 */
    1.0f, 0.0f, /* 2 */
    1.0f, 1.0f, /* 3 */

    0.0f, 0.0f, /* 4 */
    0.0f, 1.0f, /* 5 */
    1.0f, 0.0f, /* 6 */
    1.0f, 1.0f, /* 7 */

    0.0f, 0.0f, /* 8 */
    0.0f, 1.0f, /* 9 */
    1.0f, 0.0f, /* 10 */
    1.0f, 1.0f, /* 11 */

    0.0f, 0.0f, /* 12 */
    0.0f, 1.0f, /* 13 */
    1.0f, 0.0f, /* 14 */
    1.0f, 1.0f, /* 15 */

    0.0f, 0.0f, /* 16 */
    0.0f, 1.0f, /* 17 */
    1.0f, 0.0f, /* 18 */
    1.0f, 1.0f, /* 19 */

    0.0f, 0.0f, /* 20 */
    0.0f, 1.0f, /* 21 */
    1.0f, 0.0f, /* 22 */
    1.0f, 1.0f, /* 23 */

    0.0f, 0.0f, /* 24 */
    0.0f, 1.0f, /* 25 */
    1.0f, 0.0f, /* 26 */
    1.0f, 1.0f, /* 27 */

    0.0f, 0.0f, /* 28 */
    0.0f, 1.0f, /* 29 */
    1.0f, 0.0f, /* 30 */
    1.0f, 1.0f, /* 31 */

    0.0f, 0.0f, /* 32 */
    0.0f, 1.0f, /* 33 */
    1.0f, 0.0f, /* 34 */
    1.0f, 1.0f, /* 35 */
};

#endif /* ETCATLASALPHA_H */