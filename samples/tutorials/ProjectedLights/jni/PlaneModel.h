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

#ifndef PLANE_MODEL_H
#define PLANE_MODEL_H

#include "VectorTypes.h"
#include "Matrix.h"

namespace MaliSDK
{
    /**
     * \brief Functions for generating Plane shapes.
     */
    class PlaneModel
    {
    public:
        /**
        * \brief Get normals for plane placed in XZ space.
        *
        * \param normalsPtrPtr          Deref will be used to store generated normals. Cannot be null.
        * \param numberOfCoordinatesPtr Number of generated coordinates.
        */
        static void getNormals(float** normalsPtrPtr, int* numberOfCoordinatesPtr);

        /** 
         * \brief Get coordinates of points which make up a plane. The plane is located in XZ space.
         *
         * \param coordinatesPtrPtr      Deref will be used to store generated coordinates. Cannot be null.
         * \param numberOfCoordinatesPtr Number of generated coordinates.
         * \param scalingFactor          Scaling factor indicating size of a plane.
         */
        static void getTriangleRepresentation(float** coordinatesPtrPtr, int* numberOfCoordinatesPtr, float scalingFactor);
    };
}
#endif /* PLANE_MODEL_H */