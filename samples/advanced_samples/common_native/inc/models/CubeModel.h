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

#ifndef CUBE_MODEL_H
#define CUBE_MODEL_H

#include "VectorTypes.h"

namespace MaliSDK
{
    /**
     * \brief Functions for generating cube shapes.
     */
    class CubeModel
    {
    public:
        /** 
         * \brief Compute coordinates of points which make up a cube.
         *
         * \param[in] scalingFactor Scaling factor indicating size of a cube.
         * \param[out] numberOfCoordinates  Number of generated coordinates.
         * \param[out] coordinates Deref will be used to store generated coordinates. Cannot be null.
         */
        static void getTriangleRepresentation(float scalingFactor, int* numberOfCoordinates, float** coordinates);

        /** 
         * \brief Create normals for a cube which was created with getTriangleRepresentation() function.
         *
         * \param[out] numberOfCoordinates Number of generated coordinates.
         * \param[out] normals Deref will be used to store generated coordinates. Cannot be null.
         */
        static void getNormals(int* numberOfCoordinates, float** normals);
    };
}
#endif /* CUBE_MODEL_H */