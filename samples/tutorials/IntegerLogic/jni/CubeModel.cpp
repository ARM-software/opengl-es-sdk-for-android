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

#include "Common.h"
#include "CubeModel.h"

#include <cstdlib>

namespace MaliSDK
{
    /** Please see header for the specification. */
    void CubeModel::getNormals(float** normalsPtrPtr,
                               int*    numberOfCoordinatesPtr)
    {
       /* Set the same normals for both triangles from each face.
        * For details: see example for getCubeTriangleRepresentation() function. */
        ASSERT(normalsPtrPtr != NULL,
               "Cannot use null pointer while calculating coordinates");

        /* Index of an array we will put new point coordinates at. */
        int       currentIndex                   = 0;
        /* 6 faces of open cube, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfCubeNormalsCoordinates = NUMBER_OF_CUBE_FACES        *
                                                   NUMBER_OF_TRIANGLES_IN_QUAD *
                                                   NUMBER_OF_TRIANGLE_VERTICES *
                                                   NUMBER_OF_POINT_COORDINATES;
        /* There are 2 triangles for each face. Each triangle consists of 3 vertices. */
        const int numberOfCoordinatesForOneFace  = NUMBER_OF_TRIANGLES_IN_QUAD *
                                                   NUMBER_OF_TRIANGLE_VERTICES;

        /* Allocate memory for result array. */
        *normalsPtrPtr = (float*) malloc(numberOfCubeNormalsCoordinates * sizeof(float));

        /* Is allocation successful?. */
        ASSERT(*normalsPtrPtr != NULL,
               "Could not allocate memory for result array");

        /* Top face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] = 0;
            (*normalsPtrPtr)[currentIndex++] = 1;
            (*normalsPtrPtr)[currentIndex++] = 0;
        }

        /* Bottom face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] =  0;
            (*normalsPtrPtr)[currentIndex++] = -1;
            (*normalsPtrPtr)[currentIndex++] =  0;
        }

        /* Back face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] =  0;
            (*normalsPtrPtr)[currentIndex++] =  0;
            (*normalsPtrPtr)[currentIndex++] = -1;
        }

        /* Front face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] = 0;
            (*normalsPtrPtr)[currentIndex++] = 0;
            (*normalsPtrPtr)[currentIndex++] = 1;
        }

        /* Right face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] = 1;
            (*normalsPtrPtr)[currentIndex++] = 0;
            (*normalsPtrPtr)[currentIndex++] = 0;
        }

        /* Left face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normalsPtrPtr)[currentIndex++] = -1;
            (*normalsPtrPtr)[currentIndex++] =  0;
            (*normalsPtrPtr)[currentIndex++] =  0;
        }

        if (numberOfCoordinatesPtr != NULL)
        {
            *numberOfCoordinatesPtr = numberOfCubeNormalsCoordinates;
        }
    }

    /** Please see header for the specification. */
    void CubeModel::getTriangleRepresentation(float** coordinatesPtrPtr,
                                              int*    numberOfCoordinatesPtr,
                                              int*    numberOfPointsPtr,
                                              float   scalingFactor)
    {
        ASSERT(coordinatesPtrPtr != NULL,
               "Cannot use null pointer while calculating coordinates");

        /* Index of an array we will put new point coordinates at. */
        int       currentIndex                    = 0;
        /* 6 faces of open cube, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfCubeTrianglePoints      = NUMBER_OF_CUBE_FACES        *
                                                    NUMBER_OF_TRIANGLES_IN_QUAD *
                                                    NUMBER_OF_TRIANGLE_VERTICES;
        const int numberOfCubeTriangleCoordinates = numberOfCubeTrianglePoints *
                                                    NUMBER_OF_POINT_COORDINATES;

        /* Allocate memory for result array. */
        *coordinatesPtrPtr = (float*) malloc(numberOfCubeTriangleCoordinates * sizeof(float));

        /* Is allocation successful?. */
        ASSERT(*coordinatesPtrPtr != NULL,
               "Could not allocate memory for result array.")

        /* Example:
         * Coordinates for cube points:
         * A -1.0f,  1.0f,  1.0f
         * B -1.0f,  1.0f, -1.0f
         * C  1.0f,  1.0f, -1.0f
         * D  1.0f,  1.0f,  1.0f
         * E -1.0f, -1.0f,  1.0f
         * F -1.0f, -1.0f, -1.0f
         * G  1.0f, -1.0f, -1.0f
         * H  1.0f, -1.0f,  1.0f
         * Create 2 triangles for each face of the cube. Vertices are written in clockwise order.
         *       B ________ C
         *      / |     /  |
         *  A ......... D  |
         *    .   |   .    |
         *    .  F|_ _.___ |G
         *    . /     .  /
         *  E ......... H
         */

        const Vec3f pointA = {-1.0f,  1.0f,  1.0f};
        const Vec3f pointB = {-1.0f,  1.0f, -1.0f};
        const Vec3f pointC = { 1.0f,  1.0f, -1.0f};
        const Vec3f pointD = { 1.0f,  1.0f,  1.0f};
        const Vec3f pointE = {-1.0f, -1.0f,  1.0f};
        const Vec3f pointF = {-1.0f, -1.0f, -1.0f};
        const Vec3f pointG = { 1.0f, -1.0f, -1.0f};
        const Vec3f pointH = { 1.0f, -1.0f,  1.0f};

        /* Fill the array with coordinates. */
        /* Top face. */
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = pointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.z;
        /* B */
        (*coordinatesPtrPtr)[currentIndex++] = pointB.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = pointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.z;

        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = pointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = pointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.z;
        /* D */
        (*coordinatesPtrPtr)[currentIndex++] = pointD.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.z;

        /* Bottom face. */
        /* F */
        (*coordinatesPtrPtr)[currentIndex++] = pointF.x;;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.y;;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.z;;
        /* E */
        (*coordinatesPtrPtr)[currentIndex++] = pointE.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.z;
        /* H */
        (*coordinatesPtrPtr)[currentIndex++] = pointH.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.z;

        /* F */
        (*coordinatesPtrPtr)[currentIndex++] = pointF.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.z;
        /* H */
        (*coordinatesPtrPtr)[currentIndex++] = pointH.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.z;
        /* G */
        (*coordinatesPtrPtr)[currentIndex++] = pointG.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.z;

        /* Back face. */
        /* G */
        (*coordinatesPtrPtr)[currentIndex++] = pointG.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = pointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.z;
        /* B */
        (*coordinatesPtrPtr)[currentIndex++] = pointB.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.z;

        /* G */
        (*coordinatesPtrPtr)[currentIndex++] = pointG.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.z;
        /* B */
        (*coordinatesPtrPtr)[currentIndex++] = pointB.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.z;
        /* F */
        (*coordinatesPtrPtr)[currentIndex++] = pointF.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.z;

        /* Front face. */
        /* E */
        (*coordinatesPtrPtr)[currentIndex++] = pointE.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.z;
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = pointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.z;
        /* D */
        (*coordinatesPtrPtr)[currentIndex++] = pointD.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.z;

        /* E */
        (*coordinatesPtrPtr)[currentIndex++] = pointE.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.z;
        /* D */
        (*coordinatesPtrPtr)[currentIndex++] = pointD.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.z;
        /* H */
        (*coordinatesPtrPtr)[currentIndex++] = pointH.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.z;

        /* Right face. */
        /* H */
        (*coordinatesPtrPtr)[currentIndex++] = pointH.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.z;
        /* D */
        (*coordinatesPtrPtr)[currentIndex++] = pointD.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointD.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = pointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.z;

        /* H */
        (*coordinatesPtrPtr)[currentIndex++] = pointH.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointH.z;
        /* C */
        (*coordinatesPtrPtr)[currentIndex++] = pointC.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointC.z;
        /* G */
        (*coordinatesPtrPtr)[currentIndex++] = pointG.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointG.z;

        /* Left face. */
        /* F */
        (*coordinatesPtrPtr)[currentIndex++] = pointF.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.z;
        /* B */
        (*coordinatesPtrPtr)[currentIndex++] = pointB.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointB.z;
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = pointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.z;

        /* F */
        (*coordinatesPtrPtr)[currentIndex++] = pointF.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointF.z;
        /* A */
        (*coordinatesPtrPtr)[currentIndex++] = pointA.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointA.z;
        /* E */
        (*coordinatesPtrPtr)[currentIndex++] = pointE.x;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.y;
        (*coordinatesPtrPtr)[currentIndex++] = pointE.z;

        /* Calculate size of a cube. */
        if (scalingFactor != 1.0f)
        {
            for (int i = 0; i < numberOfCubeTriangleCoordinates; i++)
            {
                (*coordinatesPtrPtr)[i] *= scalingFactor;
            }
        }

        if (numberOfCoordinatesPtr != NULL)
        {
            *numberOfCoordinatesPtr = numberOfCubeTriangleCoordinates;
        }

        if (numberOfPointsPtr != NULL)
        {
            *numberOfPointsPtr = numberOfCubeTrianglePoints;
        }
    }
}
