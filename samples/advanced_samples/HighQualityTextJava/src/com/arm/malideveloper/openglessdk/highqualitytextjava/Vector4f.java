/* Copyright (c) 2013-2017, ARM Limited and Contributors
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

package com.arm.malideveloper.openglessdk.highqualitytextjava;

import java.lang.Math;

public class Vector4f {
    public float x;
    public float y;
    public float z;
    public float w = 1.0f;

    // Custom toString() Method.
    public String toString() {
        return "[" + x + ", " + y + ", " + z + ", " + w + "]";
    }

    // Default constructor
    public Vector4f() {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        w = 1.0f;
    }

    // Constructor
    public Vector4f(float aX,
                    float aY,
                    float aZ,
                    float aW) {
        x = aX;
        y = aY;
        z = aZ;
        w = aW;
    }

    public void set(float aX, float aY, float aZ, float aW) {
        x = aX;
        y = aY;
        z = aZ;
        w = aW;
    }

    /* [Make Pixel Coords] */
    public void makePixelCoords(float[] aMatrix,
                                int aViewportWidth,
                                int aViewportHeight) {
        // Transform the vector into screen coordinates we assumes aMatrix is ModelViewProjection matrix
        // transform method multiplies this vector by the matrix
        transform(aMatrix);

        // Make coordinates as homogenous
        x /= w;
        y /= w;
        z /= w;
        w = 1.0f;
        // Now the vector is normalized to the range [-1.0, 1.0]

        // Normalize values into NDC.
        x = 0.5f + x * 0.5f;
        y = 0.5f + y * 0.5f;
        z = 0.5f + z * 0.5f;
        w = 1.0f;
        // Currently the valuse are clipped to the [0.0, 1.0] range

        // Move coordinates into window space (in pixels)
        x *= (float) aViewportWidth;
        y *= (float) aViewportHeight;
    }
    /* [Make Pixel Coords] */

    // Multiply the vector by matrix
    public void transform(float[] aMatrix) {
        if (aMatrix.length < 16)
            return;

        Vector4f r = new Vector4f();
        //
        r.x  = x * aMatrix[ 0];
        r.x += y * aMatrix[ 4];
        r.x += z * aMatrix[ 8];
        r.x += w * aMatrix[12];

        r.y  = x * aMatrix[ 1];
        r.y += y * aMatrix[ 5];
        r.y += z * aMatrix[ 9];
        r.y += w * aMatrix[13];

        r.z  = x * aMatrix[ 2];
        r.z += y * aMatrix[ 6];
        r.z += z * aMatrix[10];
        r.z += w * aMatrix[14];

        r.w  = x * aMatrix[ 3];
        r.w += y * aMatrix[ 7];
        r.w += z * aMatrix[11];
        r.w += w * aMatrix[15];

        x = r.x;
        y = r.y;
        z = r.z;
        w = r.w;
    }

    static public Vector4f sub(final Vector4f aLeft,
                               final Vector4f aRight) {
        Vector4f v = new Vector4f(aLeft.x, aLeft.y, aLeft.z, aLeft.w);
        //
        v.x -= aRight.x;
        v.y -= aRight.y;
        v.z -= aRight.z;
        v.w -= aRight.w;
        //
        return v;
    }

    // Calculate length of the vector with 3 first components only [x, y, z]
    public float length3() {
        return (float)Math.sqrt(x * x + y * y + z * z);
    }

}
