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

#ifndef QUATERNION_H
    #define QUATERNION_H

    /** Structure describing a single quaternion instance. */
    typedef struct Quaternion
    {
        /** Real part. */
        float w;
        /** X imaginary part. */
        float x;
        /** Y imaginary part. */
        float y;
        /** Z imaginary part. */
        float z;
    } Quaternion;

    /** Function that returns a new quaternion. It inserts the angle in the real part and the vector
     *  in the vector part, yielding a quaternion that represents a rotation around the axis.
     *  
     *  @param x    X coordinate of a vector defining a rotation axis.
     *  @param y    Y coordinate of a vector defining a rotation axis.
     *  @param z    Z coordinate of a vector defining a rotation axis.
     *  @param degs Rotation in degrees.
     *  @return     Result quaternion.
     */
    Quaternion construct_quaternion(float x, float y, float z, float degs);

    /** Constructs a modelview matrix based on a given quaternion.
     *
     *  @param quaternion Quaternion describing rotation angle and vector to be used for matrix creation.
     *  @param mat        Array to store a modelview matrix. Cannot be NULL.
     */
    void construct_modelview_matrix(Quaternion quaternion, float* mat);

    /** Multiplies quaternion a by b and returns the product.
     *  NOTE: quaternion multiplication is not commutative.
     *
     *  @param a Quaternion a.
     *  @param b Quaternion b.
     *  @return  Product a*b.
     */
    Quaternion multiply_quaternions(Quaternion a, Quaternion b);

#endif /* QUATERNION_H */
