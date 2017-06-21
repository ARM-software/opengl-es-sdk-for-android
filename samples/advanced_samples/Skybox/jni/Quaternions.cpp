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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "Quaternions.h"

/* Please see the header for specification */
Quaternion construct_quaternion(float x, float y, float z, float degs)
{
    Quaternion init = { 0.0f, 0.0f, 0.0f, 0.0f };

    float ang = (float) M_PI * degs / 180.0f;
    float res = sin(ang / 2.0f);

    init.w = cos(ang / 2.0f);
    init.x = x * res;
    init.y = y * res;
    init.z = z * res;

    return init;
}

/* Please see the header for specification */
void construct_modelview_matrix(Quaternion quaternion, float* mat)
{
    if (mat == 0)
    {
        fprintf(stderr, "Pointer to a modelview matrix points to NULL.\n");

        exit(EXIT_FAILURE);
    }

    mat[ 0] = 1.0f - 2.0f * (quaternion.y * quaternion.y + quaternion.z * quaternion.z);
    mat[ 1] = 2.0f        * (quaternion.x * quaternion.y + quaternion.z * quaternion.w);
    mat[ 2] = 2.0f        * (quaternion.x * quaternion.z - quaternion.y * quaternion.w);
    mat[ 3] = 0.0f;
    mat[ 4] = 2.0f        * (quaternion.x * quaternion.y - quaternion.z * quaternion.w);
    mat[ 5] = 1.0f - 2.0f * (quaternion.x * quaternion.x + quaternion.z * quaternion.z);
    mat[ 6] = 2.0f        * (quaternion.z * quaternion.y + quaternion.x * quaternion.w);
    mat[ 7] = 0.0f;
    mat[ 8] = 2.0f        * ( quaternion.x * quaternion.z + quaternion.y * quaternion.w);
    mat[ 9] = 2.0f        * ( quaternion.y * quaternion.z - quaternion.x * quaternion.w);
    mat[10] = 1.0f - 2.0f * ( quaternion.x * quaternion.x + quaternion.y * quaternion.y);
    mat[11] = 0.0f;
    mat[12] = 0.0f;
    mat[13] = 0.0f;
    mat[14] = 0.0f;
    mat[15] = 1.0f;
}

/* Please see the header for specification */
Quaternion multiply_quaternions(Quaternion a, Quaternion b)
{
    Quaternion res;

    res.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    res.x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
    res.y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
    res.z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;

    return res;
}