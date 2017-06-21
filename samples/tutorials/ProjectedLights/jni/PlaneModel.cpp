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

#include "Common.h"
#include "PlaneModel.h"
#include "Mathematics.h"

namespace MaliSDK
{
    /** Please see header for the specification. */
    void PlaneModel::getNormals(float** normalsPtrPtr,
                                int*    numberOfCoordinatesPtr)
    {
        ASSERT(normalsPtrPtr != NULL,
               "Cannot use null pointer while calculating coordinates.");

        /* 2 triangles, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfNormalsCoordinates = NUMBER_OF_TRIANGLES_IN_QUAD *
                                               NUMBER_OF_TRIANGLE_VERTICES *
                                               NUMBER_OF_POINT_COORDINATES;

        /* Allocate memory for result array. */
        *normalsPtrPtr = (float*) malloc(numberOfNormalsCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*normalsPtrPtr != NULL,
               "Could not allocate memory for result array.");

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        for (int i = 0; i < numberOfNormalsCoordinates; i += NUMBER_OF_TRIANGLE_VERTICES)
        {
            (*normalsPtrPtr)[currentIndex++] = 0.0f;
            (*normalsPtrPtr)[currentIndex++] = 1.0f;
            (*normalsPtrPtr)[currentIndex++] = 0.0f;
        }

        if (numberOfCoordinatesPtr != NULL)
        {
            *numberOfCoordinatesPtr = numberOfNormalsCoordinates;
        }
    }

    /** Please see header for the specification. */
    void PlaneModel::getTriangleRepresentation(float** coordinatesPtrPtr,
                                               int*    numberOfCoordinatesPtr,
                                               float   scalingFactor)
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
        /* Coordinates of a points: A, B, C and D as shown in a schema above. */
        const Vec3f coordinatesOfPointA = {-1.0f, 0.0f, -1.0f };
        const Vec3f coordinatesOfPointB = { 1.0f, 0.0f, -1.0f };
        const Vec3f coordinatesOfPointC = { 1.0f, 0.0f,  1.0f };
        const Vec3f coordinatesOfPointD = {-1.0f, 0.0f,  1.0f };

        ASSERT(coordinatesPtrPtr != NULL,
               "Cannot use null pointer while calculating plane coordinates.")

        /* 2 triangles, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfSquareCoordinates = NUMBER_OF_TRIANGLES_IN_QUAD *
                                              NUMBER_OF_TRIANGLE_VERTICES *
                                              NUMBER_OF_POINT_COORDINATES;

        /* Allocate memory for result array. */
        *coordinatesPtrPtr = (float*) malloc(numberOfSquareCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*coordinatesPtrPtr != NULL,
               "Could not allocate memory for plane coordinates result array.")

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* First triangle. */
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.z;

        /* B */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointB.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointB.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointB.z;

        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.z;

        /* Second triangle. */
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointA.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointC.z;
        /* D */
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointD.x;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointD.y;
        (*coordinatesPtrPtr)[currentIndex++] = coordinatesOfPointD.z;

        if (scalingFactor != 1.0f)
        {
            for (int i = 0; i < numberOfSquareCoordinates; i++)
            {
                (*coordinatesPtrPtr)[i] *= scalingFactor;
            }
        }

        if (numberOfCoordinatesPtr != NULL)
        {
            *numberOfCoordinatesPtr = numberOfSquareCoordinates;
        }
    }
}
