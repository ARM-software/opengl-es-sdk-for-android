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
         * \brief Get coordinates of points which make up a plane. The plane is located in XZ space.
         * 
         * Triangles are made up of 4 components per vertex.
         *
         * \param[out] numberOfCoordinates Number of generated coordinates.
         * \param[out] coordinates Deref will be used to store generated coordinates. Cannot be null.
         */
        static void getTriangleRepresentation(int* numberOfCoordinates, float** coordinates);

        /**
         * \brief Get U/V 2D texture coordinates that can be mapped onto a plane generated from this class.
         *
         * \param[out] numberOfCoordinates Number of generated coordinates.
         * \param[out] uvCoordinates Deref will be used to store generated coordinates. Cannot be null.
         */
        static void getTriangleRepresentationUVCoordinates(int* numberOfCoordinates, float** uvCoordinates);

        /** 
         * \brief Get normals for plane placed in XZ space.
         *
         * \param[out] numberOfCoordinates Number of generated coordinates.
         * \param[out] normals Deref will be used to store generated normals. Cannot be null.
         */
        static void getNormals(int* numberOfCoordinates, float** normals);

        /**
         * \brief Transform a plane by a matrix.
         *
         * \param[in] transform The transformation Matrix to apply to the plane.
         * \param[in] numberOfCoordinates Number of coordinates which make up the plane.
         * \param[in, out] coordinates Pointer to the verticies to be transformed. The transformed verticies will be returned in the same memory. Cannot be null.
         */
        static void transform(Matrix transform, int numberOfCoordinates, float** coordinates);
    };
}
#endif /* PLANE_MODEL_H */