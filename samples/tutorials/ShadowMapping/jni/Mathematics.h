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

#ifndef MATHEMATICS_H
    #define MATHEMATICS_H

    #include "VectorTypes.h"
    
    #include <cmath>
    #include <cstdlib>

    #ifndef M_PI
        /** \brief The value of pi approximation. */
        #define M_PI 3.14159265358979323846f
    #endif /* M_PI */

    #ifndef NUMBER_OF_CUBE_FACES
        /** \brief Number of faces which make up a cubic shape. */
        #define NUMBER_OF_CUBE_FACES (6)
    #endif /* NUMBER_OF_CUBE_FACES */

    #ifndef NUMBER_OF_POINT_COORDINATES
        /** \brief Number of coordinates for a point in 3D space. */
        #define NUMBER_OF_POINT_COORDINATES (3)
    #endif /* NUMBER_OF_POINT_COORDINATES */

    #ifndef NUMBER_OF_TRIANGLE_VERTICES
        /** \brief Number of vertices which make up a traingle shape. */
        #define NUMBER_OF_TRIANGLE_VERTICES (3)
    #endif /* NUMBER_OF_TRIANGLE_VERTICES */

    #ifndef NUMBER_OF_TRIANGLES_IN_QUAD
       /** \brief Number of triangles which make up a quad. */
        #define NUMBER_OF_TRIANGLES_IN_QUAD (2)
    #endif /* NUMBER_OF_TRIANGLES_IN_QUAD */


    namespace MaliSDK
    {
        /**
         * \brief Convert an angle in degrees to radians.
         *
         * \param degrees The angle (in degrees) to convert to radians.
         *
         * \return As per description.
         */
        inline float degreesToRadians(float degrees)
        {
            return M_PI * degrees / 180.0f;
        }

        /**
        * \brief Convert an angle in radians to degrees.
        *
        * \param radians The angle (in radians) to convert to degrees.
        *
        * \return As per description.
        */
        inline float radiansToDegrees(float radians)
        {
            return radians * 180.0f / M_PI;
        }
    }
#endif /* MATHEMATICS_H */
