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

#include "SphereModel.h"
#include "Platform.h"
#include "Mathematics.h"

#include <cstdlib>
#include <cmath>
#include <cassert>

namespace MaliSDK
{
    void SphereModel::getPointRepresentation(const float radius, const int numberOfSamples, int* numberOfCoordinates, float** coordinates)
    {
       /* 
        * Sphere vertices are created according to rule: 
        * Create a circle at north pole of the sphere, consisting of numberOfSample points.
        * Create following circles, ending at south pole of sphere. The sphere now consists of numberOfSample circles.
        * Theta value (indicating longitude) runs from 0 to 2*M_PI.
        * Radius value (indicating latitude) runs from -radius to radius. 
        */

        /* Check if parameters have compatibile values. */
        if (radius <= 0.0f)
        {
            LOGE("radius value has to be greater than zero.");

            return;
        }

        if (numberOfSamples <= 0)
        {
            LOGE("numberOfSamples value has to be greater than zero.");

            return;
        }
        if (coordinates == NULL)
        {
            LOGE("Cannot use null pointer while calculating coordinates.");

            return;
        }

        /* Maximum value of an error (used to compare float values). */
        const float epsilon = 0.001f;
        /* Index of an array we will put new point coordinates at. */
        int indexOfSphereArray = 0;
        /* Maximum longitude. */
        float maxTheta = (2.0f * M_PI);
        /* Value of longitude step. */
        float thetaStep = maxTheta / float(numberOfSamples);
        /* Value of latitude step. */ 
        float radiusStep = (2 * radius) / float(numberOfSamples-1);
        /* Index of longitude loop. */
        int thetaIndex = 0;
        /* Index of latitude loop. */
        int radiusIndex = 0;
        /* Number of coordinates which a sphere consists of. Each point (which a sphere consists of) consists of 3 coordinates: x, y, z. */
        const int numberOfSphereCoordinates = numberOfSamples * numberOfSamples * 3;

        /* Allocate memory for result array. */
        *coordinates = (float*) malloc (numberOfSphereCoordinates * sizeof(float));

        if (*coordinates == NULL)
        {
           LOGE("Could not allocate memory for result array.");

            return;
        }

        /* Loop through circles from north to south. */
        for (float r = -radius; 
                   r < radius + epsilon; 
                   r = -radius + radiusIndex * radiusStep)
        {
            thetaIndex = 0;

            /* Protect against rounding errors.. */
            if (r > radius)
            {
                r = radius;
            }

            /* Loop through all points of the circle. */
            for (float theta = 0.0f; 
                       theta < maxTheta; 
                       theta = thetaIndex * thetaStep)
            {
                /* Compute x, y and z coordinates for the considered point. */
                float x = sqrt((radius * radius) - (r * r)) * cosf(theta);
                float y = sqrt((radius * radius) - (r * r)) * sinf(theta);
                float z = r;

                (*coordinates)[indexOfSphereArray] = x;
                indexOfSphereArray++;

                (*coordinates)[indexOfSphereArray] = y;
                indexOfSphereArray++;

                (*coordinates)[indexOfSphereArray] = z;
                indexOfSphereArray++;

                thetaIndex ++;
            }

            radiusIndex ++;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfSphereCoordinates;
        }
    }

    void SphereModel::getTriangleRepresentation(const float radius, const int numberOfSamples, int* numberOfCoordinates, float** coordinates)
    {
        /* Check if parameters have compatibile values. */
        if (radius <= 0.0f)
        {
            LOGE("radius value has to be greater than zero.");

            return;
        }

        if (numberOfSamples <= 0)
        {
            LOGE("numberOfSamples value has to be greater than zero.");

            return;
        }
        if (coordinates == NULL)
        {
            LOGE("Cannot use null pointer while calculating coordinates.");

            return;
        }
        /* Array holding coordinates of points which a sphere consists of. */
        float* pointCoordinates = NULL;
        /* Current index used for accessing coordinates array. */
        int sphereTriangleCoordinateIndex = 0;
        /* Index of current loop iteration. */
        int iterationIndex = 1;
        /* There are 18 coordinates created in one loop: 3 coordinates * 3 triangle vertices * 2 triangles. */
        int numberOfCoordinatesCreatedInOneLoop = 18;
        /* Number of coordinates of all triangles which a sphere consists of.
        * (numberOfSamples - 1) - for each circle (excluding last one)
        * numberOfSamples - for each point on a single circle
        * 2 - number of triangles that start at given point
        * 3 - number of points that make up a triangle
        * 3 - coordinates per each point.
        */
        const int numberOfSphereTriangleCoordinates = (numberOfSamples - 1) * numberOfSamples * 2 * 3 * 3;

        /* Compute coordinates of points which make up a sphere. */
        getPointRepresentation(radius, numberOfSamples, NULL, &pointCoordinates);

        if (pointCoordinates == NULL)
        {
            LOGE("Could not get coordinates of points which make up a sphere.");
            return;
        }

        *coordinates = (float*) malloc (numberOfSphereTriangleCoordinates * sizeof(float));

        if (*coordinates == NULL)
        {
            LOGE("Could not allocate memory for result array.");
            return;
        }

        /* Each point has 3 coordinates: x, y, z. */
        for (int pointIndex = 0; 
                 pointIndex < (numberOfSphereTriangleCoordinates / numberOfCoordinatesCreatedInOneLoop) * 3; 
                 pointIndex += 3)
        {
            /* First triangle. ==> */

            /* Building of a triangle is started from point at index described by pointIndex. 
             * Values of x, y and z coordinates are written one after another in pointCoordinates array starting at index of point value. 
             */
            /* Coordinate x. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex];
            sphereTriangleCoordinateIndex++;
            /* Coordinate y. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 1];
            sphereTriangleCoordinateIndex++;
            /* Coordinate z. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 2];
            sphereTriangleCoordinateIndex++;

            /* Check if this is the last point of circle. */
            if ((iterationIndex % numberOfSamples) == 0 )
            {
                /* Second point of triangle.
                 * If this is the last point lying on a circle,
                 * second point that makes up a triangle is the point with coordinates taken from the first point lying on current circle.
                 * According to example above: the second point for first triangle starting at point D1 (last point on a current circle) is point A1.
                 * Index (to receive x coordinate): [point - 3 * (numberOfSamples)] stands for x coordinate of point D0 (corresponding point from previous circle), 
                 * [+ 3] stands for next point (first point of current circle - A1).  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex - 3 * (numberOfSamples) + 3];
                sphereTriangleCoordinateIndex++;                                                       
                /* Coordinate y. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex - 3 * (numberOfSamples) + 3 + 1];
                sphereTriangleCoordinateIndex++;                                                       
                /* Coordinate z. */                                     
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex - 3 * (numberOfSamples) + 3 + 2];
                sphereTriangleCoordinateIndex++;

                /* Third point of triangle.
                 * If this is the last point lying on a circle,
                 * third point that makes up a triangle is the point with coordinates taken from the first point lying on next circle.
                 * According to example above: the third point for first triangle starting at point D1 (last point on a current circle) is point A2.
                 * Index (to receive x coordinate): [point + 3] stands for x coordinate of point A2 (first point on next circle).  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3];
                sphereTriangleCoordinateIndex++;                                                          
                /* Coordinate y. */                                     
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 1];
                sphereTriangleCoordinateIndex++;                                                          
                /* Coordinate z. */                                     
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 2];
                sphereTriangleCoordinateIndex++;
            }
            else
            {
                /* Second point of triangle.
                 * If this is not the last point lying on a circle,
                 * second point that makes up a triangle is the point with coordinates taken from the next point lying on current circle. 
                 * According to example above: the second point for first triangle starting at (for example) point A1 (not last point on a circle) is point B1.
                 * Index (to receive x coordinate): [point + 3] stands for x coordinate of point B1 (next point in array).  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3];
                sphereTriangleCoordinateIndex++;
                /* Coordinate y. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 1];
                sphereTriangleCoordinateIndex++;
                /* Coordinate z. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 2];
                sphereTriangleCoordinateIndex++;

                /* Third point of triangle.
                 * If this is not the last point lying on a circle,
                 * third point that makes up a triangle is the point with coordinates taken from the next from corresponding point lying on next circle.
                 * According to example above: the third point for first triangle starting at (for example) point A1 (not last point on a circle) is point B2.
                 * Index (to receive x coordinate): [point + numberOfSamples * 3] stands for x coordinate of point A2 (corresponding point lying on next circle), 
                 * [+ 3] stands for nex point on that circle - B2.  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + numberOfSamples * 3 + 3];
                sphereTriangleCoordinateIndex++;                                                  
                /* Coordinate y. */                                     
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + numberOfSamples * 3 + 3 + 1];
                sphereTriangleCoordinateIndex++;                                                  
                /* Coordinate z. */                                     
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + numberOfSamples * 3 + 3 + 2];
                sphereTriangleCoordinateIndex++;
            }
            /* <== First triangle. */

            /* Second triangle. ==> */

            /* Building of a triangle is started from point at index described by pointIndex. 
             * Values of x, y and z coordinates are written one after another in pointCoordinates array starting at index of point value. 
             */
            /* Coordinate x. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex];
            sphereTriangleCoordinateIndex++;
            /* Coordinate y. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 1];
            sphereTriangleCoordinateIndex++;
            /* Coordinate z. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 2];
            sphereTriangleCoordinateIndex++;

            /* Check if this is a last point of circle. */
            if ((iterationIndex % numberOfSamples) == 0 )
            {   /* Second point of triangle.
                 * If this is the last point lying on a circle,
                 * second point that makes up a triangle is the point with coordinates taken from the first point lying on next circle.
                 * According to example above: the second point for first triangle starting at point D1 (last point on a current circle) is point A2.
                 * Index (to receive x coordinate): [point + 3] stands for x coordinate of point A2 (first point on next circle).  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3];
                sphereTriangleCoordinateIndex++;
                /* Coordinate y. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 1];
                sphereTriangleCoordinateIndex++;
                /* Coordinate z. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 + 2];
                sphereTriangleCoordinateIndex++;
            }
            else
            {
                /* Second point of triangle.
                 * If this is not the last point lying on a circle,
                 * second point that makes up a triangle is the point with coordinates taken from the next from corresponding point lying on next circle.
                 * According to example above: the second point for first triangle starting at (for example) point A1 (not last point on a circle) is point B2.
                 * Index (to receive x coordinate): [point + numberOfSamples * 3] stands for x coordinate of point A2 (corresponding point lying on next circle), 
                 * [+ 3] stands for nex point on that circle - B2.  
                 */
                /* Coordinate x. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 * numberOfSamples + 3];
                sphereTriangleCoordinateIndex++;
                /* Coordinate y. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 * numberOfSamples + 3 + 1];
                sphereTriangleCoordinateIndex++;
                /* Coordinate z. */
                (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 * numberOfSamples + 3 + 2];
                sphereTriangleCoordinateIndex++;
            }

            /* Third point of triangle that makes up a triangle is the point with coordinates taken from the corresponding point lying on next circle.
             * According to example above: the third point for second triangle starting at point A1 (for example) is point A2.
             * Index (to receive x coordinate): [point + 3 * (numberOfSamples)] stands for x coordinate of point A2 (corresponding point from next circle).  
             */
            /* Coordinate x. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 * numberOfSamples];
            sphereTriangleCoordinateIndex++;
            /* Coordinate y. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3  *numberOfSamples + 1];
            sphereTriangleCoordinateIndex++;
            /* Coordinate z. */
            (*coordinates)[sphereTriangleCoordinateIndex] = pointCoordinates[pointIndex + 3 * numberOfSamples + 2];
            sphereTriangleCoordinateIndex++;

            /* <== Second triangle. */

            iterationIndex++;
        }

        if (numberOfCoordinates != NULL)
        {
            *numberOfCoordinates = numberOfSphereTriangleCoordinates;
        }

        /* Deallocate memory. */
        free(pointCoordinates);
        pointCoordinates = NULL;
    }


    
}