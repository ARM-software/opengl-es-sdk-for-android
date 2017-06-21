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
    void CubeModel::getTriangleRepresentation(float scalingFactor, int* numberOfPoints, int* numberOfCoordinates, float** coordinates)
    {
        ASSERT(coordinates != NULL, "Cannot use null pointer while calculating coordinates.");

        /* 6 faces, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfVerticesToGenerate      = numberOfCubeFaces       *
                                                    numberOfSquareTriangles *
                                                    numberOfTrianglePoints ;
        const int numberOfCubeTriangleCoordinates = numberOfVerticesToGenerate *
                                                    numberOfPointCoordinates;

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* Allocate memory for result array. */
        *coordinates = (float*) malloc (numberOfCubeTriangleCoordinates * sizeof(float));

        /* Is allocation successful?. */
        ASSERT(*coordinates != NULL, "Could not allocate memory for result array.");

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

        /* Define point coordinates. */
        Vec3f pointA = {-1.0f,  1.0f,  1.0f};
        Vec3f pointB = {-1.0f,  1.0f, -1.0f};
        Vec3f pointC = { 1.0f,  1.0f, -1.0f};
        Vec3f pointD = { 1.0f,  1.0f,  1.0f};
        Vec3f pointE = {-1.0f, -1.0f,  1.0f};
        Vec3f pointF = {-1.0f, -1.0f, -1.0f};
        Vec3f pointG = { 1.0f, -1.0f, -1.0f};
        Vec3f pointH = { 1.0f, -1.0f,  1.0f};

        /* Fill the array with coordinates. */
        /* Top face. */
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        /* B */
        (*coordinates)[currentIndex++] = pointB.x;
        (*coordinates)[currentIndex++] = pointB.y;
        (*coordinates)[currentIndex++] = pointB.z;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;

        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;
        /* D */
        (*coordinates)[currentIndex++] = pointD.x;
        (*coordinates)[currentIndex++] = pointD.y;
        (*coordinates)[currentIndex++] = pointD.z;

        /* Bottom face. */
        /* E */
        (*coordinates)[currentIndex++] = pointE.x;
        (*coordinates)[currentIndex++] = pointE.y;
        (*coordinates)[currentIndex++] = pointE.z;
        /* F */
        (*coordinates)[currentIndex++] = pointF.x;
        (*coordinates)[currentIndex++] = pointF.y;
        (*coordinates)[currentIndex++] = pointF.z;
        /* G */
        (*coordinates)[currentIndex++] = pointG.x;
        (*coordinates)[currentIndex++] = pointG.y;
        (*coordinates)[currentIndex++] = pointG.z;

        /* E */
        (*coordinates)[currentIndex++] = pointE.x;
        (*coordinates)[currentIndex++] = pointE.y;
        (*coordinates)[currentIndex++] = pointE.z;
        /* G */
        (*coordinates)[currentIndex++] = pointG.x;
        (*coordinates)[currentIndex++] = pointG.y;
        (*coordinates)[currentIndex++] = pointG.z;
        /* H */
        (*coordinates)[currentIndex++] = pointH.x;
        (*coordinates)[currentIndex++] = pointH.y;
        (*coordinates)[currentIndex++] = pointH.z;

        /* Back face. */
        /* G */
        (*coordinates)[currentIndex++] = pointG.x;
        (*coordinates)[currentIndex++] = pointG.y;
        (*coordinates)[currentIndex++] = pointG.z;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;
        /* B */
        (*coordinates)[currentIndex++] = pointB.x;
        (*coordinates)[currentIndex++] = pointB.y;
        (*coordinates)[currentIndex++] = pointB.z;

        /* G */
        (*coordinates)[currentIndex++] = pointG.x;
        (*coordinates)[currentIndex++] = pointG.y;
        (*coordinates)[currentIndex++] = pointG.z;
        /* B */
        (*coordinates)[currentIndex++] = pointB.x;
        (*coordinates)[currentIndex++] = pointB.y;
        (*coordinates)[currentIndex++] = pointB.z;
        /* F */
        (*coordinates)[currentIndex++] = pointF.x;
        (*coordinates)[currentIndex++] = pointF.y;
        (*coordinates)[currentIndex++] = pointF.z;

        /* Front face. */
        /* E */
        (*coordinates)[currentIndex++] = pointE.x;
        (*coordinates)[currentIndex++] = pointE.y;
        (*coordinates)[currentIndex++] = pointE.z;
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        /* D */
        (*coordinates)[currentIndex++] = pointD.x;
        (*coordinates)[currentIndex++] = pointD.y;
        (*coordinates)[currentIndex++] = pointD.z;

        /* E */
        (*coordinates)[currentIndex++] = pointE.x;
        (*coordinates)[currentIndex++] = pointE.y;
        (*coordinates)[currentIndex++] = pointE.z;
        /* D */
        (*coordinates)[currentIndex++] = pointD.x;
        (*coordinates)[currentIndex++] = pointD.y;
        (*coordinates)[currentIndex++] = pointD.z;
        /* H */
        (*coordinates)[currentIndex++] = pointH.x;
        (*coordinates)[currentIndex++] = pointH.y;
        (*coordinates)[currentIndex++] = pointH.z;

        /* Right face. */
        /* H */
        (*coordinates)[currentIndex++] = pointH.x;
        (*coordinates)[currentIndex++] = pointH.y;
        (*coordinates)[currentIndex++] = pointH.z;
        /* D */
        (*coordinates)[currentIndex++] = pointD.x;
        (*coordinates)[currentIndex++] = pointD.y;
        (*coordinates)[currentIndex++] = pointD.z;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;

        /* H */
        (*coordinates)[currentIndex++] = pointH.x;
        (*coordinates)[currentIndex++] = pointH.y;
        (*coordinates)[currentIndex++] = pointH.z;
        /* C */
        (*coordinates)[currentIndex++] = pointC.x;
        (*coordinates)[currentIndex++] = pointC.y;
        (*coordinates)[currentIndex++] = pointC.z;
        /* G */
        (*coordinates)[currentIndex++] = pointG.x;
        (*coordinates)[currentIndex++] = pointG.y;
        (*coordinates)[currentIndex++] = pointG.z;

        /* Left face. */
        /* F */
        (*coordinates)[currentIndex++] = pointF.x;
        (*coordinates)[currentIndex++] = pointF.y;
        (*coordinates)[currentIndex++] = pointF.z;
        /* B */
        (*coordinates)[currentIndex++] = pointB.x;
        (*coordinates)[currentIndex++] = pointB.y;
        (*coordinates)[currentIndex++] = pointB.z;
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;

        /* F */
        (*coordinates)[currentIndex++] = pointF.x;
        (*coordinates)[currentIndex++] = pointF.y;
        (*coordinates)[currentIndex++] = pointF.z;
        /* A */
        (*coordinates)[currentIndex++] = pointA.x;
        (*coordinates)[currentIndex++] = pointA.y;
        (*coordinates)[currentIndex++] = pointA.z;
        /* E */
        (*coordinates)[currentIndex++] = pointE.x;
        (*coordinates)[currentIndex++] = pointE.y;
        (*coordinates)[currentIndex++] = pointE.z;

        /* Calculate size of a cube. */
        for (int i = 0; i < numberOfCubeTriangleCoordinates; i++)
        {
            (*coordinates)[i] = scalingFactor * (*coordinates)[i];
        }
        
        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfCubeTriangleCoordinates;
        }

        if (numberOfPoints != NULL)
        {
            *numberOfPoints = numberOfVerticesToGenerate;
        }
    }

    void CubeModel::getNormals(int* numberOfCoordinates, float** normals)
    {
        /* Set the same normals for both triangles from each face.
         * For details: see example for getCubeTriangleRepresentation() function. */
        ASSERT(normals != NULL, "Cannot use null pointer while calculating coordinates.");

        /* There are 2 triangles for each face. Each triangle consists of 3 vertices. */
        const int numberOfCoordinatesForOneFace  = numberOfSquareTriangles * numberOfTrianglePoints;
        /* 6 faces, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfCubeNormalsCoordinates = numberOfCubeFaces             *
                                                   numberOfCoordinatesForOneFace *
                                                   numberOfPointCoordinates;

        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* Allocate memory for result array. */
        *normals = (float*) malloc (numberOfCubeNormalsCoordinates * sizeof(float));

        /* Is allocation successful? */
        ASSERT(*normals != NULL, "Could not allocate memory for result array.");

        /* Top face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] = 0;
            (*normals)[currentIndex++] = 1;
            (*normals)[currentIndex++] = 0;
        }

        /* Bottom face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] =  0;
            (*normals)[currentIndex++] = -1;
            (*normals)[currentIndex++] =  0;
        }

        /* Back face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] =  0;
            (*normals)[currentIndex++] =  0;
            (*normals)[currentIndex++] = -1;
        }

        /* Front face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] = 0;
            (*normals)[currentIndex++] = 0;
            (*normals)[currentIndex++] = 1;
        }

        /* Right face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] = 1;
            (*normals)[currentIndex++] = 0;
            (*normals)[currentIndex++] = 0;
        }

        /* Left face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex++] = -1;
            (*normals)[currentIndex++] =  0;
            (*normals)[currentIndex++] =  0;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfCubeNormalsCoordinates;
        }
    }
}
