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

#include "PlaneModel.h"
#include "Common.h"

#include <cassert>

namespace MaliSDK
{
    void PlaneModel::getTriangleRepresentationUVCoordinates(int* numberOfCoordinates, float** uvCoordinates)
    {
        /* Example:
         *  v   D __________ C
         *  .    |        / |
         * / \   |     /    |
         *  |    |  /       |
         *  |    |/_________|
         *  |   A            B
         *  |----------> u
         */
        ASSERT(uvCoordinates != NULL, "Cannot use null pointer while calculating coordinates.");

        /* 2 triangles, 3 points of triangle, 2 coordinates for each point. */
        const int numberOfUVCoordinates = numberOfSquareTriangles *
                                          numberOfTrianglePoints  *
                                          numberOfPointUvCoordinates;

        /* Allocate memory for result array. */
        *uvCoordinates = (float*) malloc (numberOfUVCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*uvCoordinates != NULL, "Could not allocate memory for result array.");

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /*** First triangle. ***/
        /* A */
        /* u */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* B */
        /* u */
        (*uvCoordinates)[currentIndex++] = 1.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* C */
        /* u */
        (*uvCoordinates)[currentIndex++] = 1.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 1.0f;

        /*** Second triangle. ***/
        /* A */
        /* u */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* C */
        /* u */
        (*uvCoordinates)[currentIndex++] = 1.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 1.0f;
        /* D */
        /* u */
        (*uvCoordinates)[currentIndex++] = 0.0f;
        /* v */
        (*uvCoordinates)[currentIndex++] = 1.0f;

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfUVCoordinates;
        }
    }

    /* [Generate triangular representation of a plane] */
    void PlaneModel::getTriangleRepresentation(int* numberOfPoints, int* numberOfCoordinates, float** coordinates)
    {
        /* Example:
         *  z   D __________ C
         *  .    |        / |
         * / \   |     /    |
         *  |    |  /       |
         *  |    |/_________|
         *  |   A            B
         *  |----------> x
         */
        ASSERT(coordinates != NULL, "Cannot use null pointer while calculating coordinates.");

        /* Define point coordinates. */
        const Vec4f pointA = {-1.0f, 0.0f, -1.0f, 1.0f};
        const Vec4f pointB = { 1.0f, 0.0f, -1.0f, 1.0f};
        const Vec4f pointC = { 1.0f, 0.0f,  1.0f, 1.0f};
        const Vec4f pointD = {-1.0f, 0.0f,  1.0f, 1.0f};

        /* 2 triangles, 3 points of triangle, 4 coordinates for each point. */
        const int numberOfSquarePoints      = numberOfSquareTriangles *
                                              numberOfTrianglePoints;
        const int numberOfSquareCoordinates = numberOfSquarePoints    *
                                              numberOfPointCoordinates;

        /* Allocate memory for result array. */
        *coordinates = (float*) malloc (numberOfSquareCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*coordinates != NULL, "Could not allocate memory for result array.");

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* First triangle. */
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        (*coordinates)[currentIndex++] = pointA.w;
        /* B */
        (*coordinates)[currentIndex++] = pointB.x;
        (*coordinates)[currentIndex++] = pointB.y;
        (*coordinates)[currentIndex++] = pointB.z;
        (*coordinates)[currentIndex++] = pointB.w;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;
        (*coordinates)[currentIndex++] = pointC.w;

        /* Second triangle. */
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        (*coordinates)[currentIndex++] = pointA.w;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;
        (*coordinates)[currentIndex++] = pointC.w;
        /* D */
        (*coordinates)[currentIndex++] = pointD.x;
        (*coordinates)[currentIndex++] = pointD.y;
        (*coordinates)[currentIndex++] = pointD.z;
        (*coordinates)[currentIndex++] = pointD.w;

        if (numberOfPoints != NULL)
        {
            *numberOfPoints = numberOfSquarePoints;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfSquareCoordinates;
        }
    }
    /* [Generate triangular representation of a plane] */

    /* [Get plane normals] */
    void PlaneModel::getNormals(int* numberOfCoordinates, float** normals)
    {
        ASSERT(normals != NULL, "Cannot use null pointer while calculating coordinates.");

        /* 2 triangles, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfNormalsCoordinates = numberOfSquareTriangles *
                                               numberOfTrianglePoints  *
                                               numberOfPointCoordinates;

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* Allocate memory for result array. */
        *normals = (float*) malloc (numberOfNormalsCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*normals != NULL, "Could not allocate memory for result array.");

        for (int i = 0; i < numberOfNormalsCoordinates; i += numberOfPointCoordinates)
        {
            (*normals)[currentIndex++] = 0.0f;
            (*normals)[currentIndex++] = 1.0f;
            (*normals)[currentIndex++] = 0.0f;
            (*normals)[currentIndex++] = 1.0f;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfNormalsCoordinates;
        }
    }
    /* [Get plane normals] */

    void PlaneModel::transform(Matrix transform, int numberOfCoordinates, float** squareCoordinates)
    {
        /* Loop through all the coordinates and transform them using the rotation matrix. */
        for (int allCoordinates = 0; allCoordinates < numberOfCoordinates; allCoordinates += numberOfPointCoordinates)
        {
            Vec4f currentVertex = {(*squareCoordinates)[allCoordinates],
                                   (*squareCoordinates)[allCoordinates + 1],
                                   (*squareCoordinates)[allCoordinates + 2],
                                   (*squareCoordinates)[allCoordinates + 3]};

            Vec4f rotatedVertex = Matrix::vertexTransform(&currentVertex, &transform);

            (*squareCoordinates)[allCoordinates]     = rotatedVertex.x;
            (*squareCoordinates)[allCoordinates + 1] = rotatedVertex.y;
            (*squareCoordinates)[allCoordinates + 2] = rotatedVertex.z;
            (*squareCoordinates)[allCoordinates + 3] = rotatedVertex.w;
        }
    }
}
