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

#ifndef VECTORTYPES_H
#define VECTORTYPES_H

#include <cmath>

namespace MaliSDK
{
    /** \brief A 3D floating point vector
     *
     * Class containing three floating point numbers, useful for representing 3D coordinates.
     */
    class Vec3f
    {
    public:
        float x, y, z;

        /**
         * \brief Calculate dot product between two 3D floating point vectors.
         *
         * \param[in] vector1 First floating point vector that will be used to compute product.
         * \param[in] vector2 Second floating point vector that will be used to compute product.
         *
         * \return Floating point value that is a result of dot product of vector1 and vector2.
         */
        static float dot(Vec3f& vector1, Vec3f& vector2)
        {
            return (vector1.x * vector2.x + vector1.y * vector2.y + vector1.z * vector2.z);
        }

        /**
         * \brief Normalize 3D floating point vector.
         */
        void normalize(void)
        {
            float length = sqrt(x * x + y * y + z * z);

            x /= length;
            y /= length;
            z /= length;
        }

        /** \brief Calculate cross product between two 3D floating point vectors.
         *
         * \param[in] vector1 First floating point vector that will be used to compute cross product.
         * \param[in] vector2 Second floating point vector that will be used to compute cross product.
         *
         * \return Floating point vector that is a result of cross product of vector1 and vector2.
         */
        static Vec3f cross(const Vec3f& vector1, const Vec3f& vector2)
        {
            /* Floating point vector to be returned. */
            Vec3f crossProduct;

            crossProduct.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
            crossProduct.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
            crossProduct.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);

            return crossProduct;
        }
    };

    /** \brief A 4D floating point vector
     *
     * Class containing four floating point numbers.
     */
    class Vec4f
    {
    public:
        float x, y, z, w;

        /**
         * \brief Normalize 4D floating point vector.
         */
        void normalize(void)
        {
            float length = sqrt(x * x + y * y + z * z + w * w);

            x /= length;
            y /= length;
            z /= length;
            w /= length;
        }
    };
}
#endif /* VECTORTYPES_H */

