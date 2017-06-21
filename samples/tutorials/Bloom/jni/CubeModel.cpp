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

#include "CubeModel.h"

namespace MaliSDK
{
    void CubeModel::getTriangleRepresentation(float scalingFactor, int* numberOfCoordinates, float** coordinates)
    {
        if (coordinates == NULL)
        {
            LOGE("Cannot use null pointer while calculating coordinates.");
            return;
        }

        /* 6 faces, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfCubeTriangleCoordinates = 6 * 2 * 3 * 3;
        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* Allocate memory for result array. */
        *coordinates = (float*) malloc (numberOfCubeTriangleCoordinates * sizeof(float));

        /* Is allocation successful?. */
        if (*coordinates == NULL)
        {
            LOGE("Could not allocate memory for result array.");

            return;
        }

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

        /* Fill the array with coordinates. */
        /* Top face. */
        /*A*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*B*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*C*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /*A*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*C*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*D*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;

        /* Bottom face. */
        /*E*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*F*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*G*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /*E*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*G*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*H*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;

        /* Back face. */
        /*G*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*C*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*B*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /*G*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*B*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*F*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /* Front face. */
        /*E*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*A*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*D*/
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;

        /*E*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*D*/
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        /*H*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;

        /* Right face. */
        /*H*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*D*/
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = 1.0f;
        currentIndex++;
        /*C*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /*H*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*C*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*G*/
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;

        /* Left face. */
        /*F*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*B*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*A*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;

        /*F*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        /*A*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;
        /*E*/
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] = -1.0f;
        currentIndex++;
        (*coordinates)[currentIndex] =  1.0f;
        currentIndex++;

        /* Calculate size of a cube. */
        for (int i = 0; i < numberOfCubeTriangleCoordinates; i++)
        {
            (*coordinates)[i] = scalingFactor * (*coordinates)[i];
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfCubeTriangleCoordinates;
        }
    }
    
    void CubeModel::getNormals(int* numberOfCoordinates, float** normals)
    {
        /* Set the same normals for both triangles from each face.
         * For details: see example for getCubeTriangleRepresentation() function.
         */

        if (normals == NULL)
        {
            LOGE("Cannot use null pointer while calculating coordinates.");

            return;
        }

        /* 6 faces, 2 triangles for each face, 3 points of triangle, 3 coordinates for each point. */
        const int numberOfCubeNormalsCoordinates = 6 * 2 * 3 * 3;
        /* Index of an array we will put new point coordinates at. */
        int currentIndex = 0;

        /* Allocate memory for result array. */
        *normals = (float*) malloc (numberOfCubeNormalsCoordinates * sizeof(float));

        /* Is allocation successfu?. */
        if (*normals == NULL)
        {
            LOGE("Could not allocate memory for result array.");

            return;
        }

        /* There are 2 triangles for each face. Each triangle consists of 3 vertices. */
        int numberOfCoordinatesForOneFace = 2 * 3;

        /* Top face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] = 0;
            currentIndex++;
            (*normals)[currentIndex] = 1;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
        }

        /* Bottom face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] =  0;
            currentIndex++;
            (*normals)[currentIndex] = -1;
            currentIndex++;
            (*normals)[currentIndex] =  0;
            currentIndex++;
        }

        /* Back face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] =  0;
            currentIndex++;
            (*normals)[currentIndex] =  0;
            currentIndex++;
            (*normals)[currentIndex] =  -1;
            currentIndex++;
        }

        /* Front face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] = 0;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
            (*normals)[currentIndex] = 1;
            currentIndex++;
        }

        /* Right face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] = 1;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
        }

        /* Left face. */
        for (int i = 0; i < numberOfCoordinatesForOneFace; i++)
        {
            (*normals)[currentIndex] = -1;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
            (*normals)[currentIndex] = 0;
            currentIndex++;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfCubeNormalsCoordinates;

        }
    }
}
