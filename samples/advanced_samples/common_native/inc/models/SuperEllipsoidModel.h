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

#ifndef SUPER_ELLIPSOID_MODEL_H
#define SUPER_ELLIPSOID_MODEL_H

#include "VectorTypes.h"

namespace MaliSDK
{
    /**
     * \brief Functions for generating super ellipsoid shapes.
     */
    class SuperEllipsoidModel
    {
    private:
       /** 
         * \brief Compute normal vector of a super ellipsoid. 
         *
         * Whole shape lies at the centre of a box of dimension (-1, 1) on every axis. 
         *
         * \param[in] xyAngle XY angle for which we compute a vertex.
         * \param[in] xzAngle XZ angle for which we compute a vertex. 
         * \param[in] n1 The "squareness" of our figure - property that tells how rounded the geometry will be in XZ space.
         * \param[in] n2 The "squareness" of our figure - property that tells how rounded the geometry will be in XY space.
         * \param[in] scale Scale factor applied to the object.
         *
         * \return The normal vector of the super ellipsoid.
         */
        inline static Vec3f calculateNormal(float xyAngle, float xzAngle, float n1, float n2, float scale);

        /**
         * \brief Compute vertex of a super ellipsoid. 
         *
         * Whole shape lies at the centre of a box of dimension (-1, 1) on every axis. 
         *
         * \param[in] xyAngle XY angle for which we compute a vertex.
         * \param[in] xzAngle XZ angle for which we compute a vertex. 
         * \param[in] n1 The "squareness" of our figure - property that tells how rounded the geometry will be in XZ space.
         * \param[in] n2 The "squareness" of our figure - property that tells how rounded the geometry will be in XY space.
         * \param[in] scale Scale factor applied to the object.
         *
         * \return point of the super ellipsoid.
         */
        inline static Vec3f sample(float xyAngle, float xzAngle, float n1, float n2, float scale);

        /**
         * \brief Put vertex and normal vector values into vertex array and normal vector array.
         *
         * We modify vertexIndex and normalVectorIndex inside this function in order to simplify
         * algorithm in the createSuperEllipsoid function.
         *
         * \param[in] vertex Vertex that will be saved in vertex array.
         * \param[in] normalVector Normal vector that will be saved in normal vector array.
         * \param[in] vertexIndex Vertex array index where new coordinate should be saved.
         * \param[out] normalVectorIndex Normal vector array index where new coordinate should be saved.
         * \param[out] roundedCubeCoordinates Pointer to an array that holds rounded cube's coordinates.
         * \param[out] roundedCubeNormalVectors Pointer to an array that holds rounded cube's normal vectors.
         */
        inline static void storeVertexAndNormalVectorInArray(const Vec3f& vertex, const Vec3f& normalVector, int& vertexIndex, int& normalVectorIndex, float* roundedCubeCoordinates, float* roundedCubeNormalVectors);

    public:
        /**
         * \brief Function that generates vertices and normal vectors of rounded cube. 
         *
         * It creates an "unit"-size ellipsoid (-1 to 1) at the origin.
         * This function generates vertices of superellipsoid, a versatile primitive that is controlled
         * by two parameters - n1 and n2. It can represent a sphere,
         * square box, and closed cylindrical can. The parameter samples determine the accuracy of
         * this primitive (if it has sharp or smooth edges). It should be noted that there are
         * some numerical issues with both very small or very large values of n1 and n2
         * parameters. Typically, for safety, they should be in the range of 0.1 to 5.0.
         * After using this function user should deallocate memory reserved for roundedCubeCoordinates and roundedCubeNormalVectors.
         *
         * \param[in] samples The number of triangles that will create super ellipsoid.
         * \param[in] n1 The "squareness" of our figure - property that tells how rounded the geometry will be in XZ space.
         * \param[in] n2 The "squareness" of our figure - property that tells how rounded the geometry will be in XY space.
         * \param[in] scale Scale factor applied to the object.
         * \param[out] roundedCubeCoordinates Pointer to an array that holds rounded cube's coordinates.
         * \param[out] roundedCubeNormalVectors Pointer to an array that holds rounded cube's normal vectors.
         * \param[out] numberOfVertices Number of generated vertices.
         * \param[out] numberOfCoordinates Number of generated coordinates.
         * \param[out] numberOfNormals Number of generated normal vectors.
         */
        static void create(int samples, float n1, float n2, float scale, float** roundedCubeCoordinates, float** roundedCubeNormalVectors, int* numberOfVertices, int* numberOfCoordinates, int* numberOfNormals);
    };
}
#endif /* SUPER_ELLIPSOID_MODEL_H */