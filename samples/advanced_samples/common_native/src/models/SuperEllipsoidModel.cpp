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

#include "SuperEllipsoidModel.h"

#include "Platform.h"
#include "Mathematics.h"

#include <cstdlib>

namespace MaliSDK
{      
    void SuperEllipsoidModel::create(int samples, float n1, float n2, float scale, float** roundedCubeCoordinates, float** roundedCubeNormalVectors, int* numberOfVertices, int* numberOfCoordinates, int* numberOfNormals)
    {
        /* Check if samples is different than 0. */
        if (samples == 0)
        {
            LOGE("Number of samples can't be 0.");
            return;
        }

        /* 
         * Computation of the numberOfCoordinates results from the algorithm used in this function.
         * In the first "for" loop we can see that this loop iterates samples / 2 times. Then in the second loop
         * we can see that this loop will go samples times. 
         * In total, there are samples / 2 * samples iterations.
         * Each iteration produces 6 vertices and each vertex has 4 coordinates.
         */
        *numberOfCoordinates = samples / 2 * samples * 6 * 4;

        /* The number of normals is the same except that each normal only has 3 coordinates. */
        *numberOfNormals = samples / 2 * samples * 6 * 3;

        /* Allocate memory to store rounded cube's coordinates. */
        *roundedCubeCoordinates = new float[*numberOfCoordinates];

        if (roundedCubeCoordinates == NULL)
        {
            LOGE("Pointer roundedCubeCoordinates is NULL.");
            return;
        }

        /* Allocate memory to store rounded cube's normal vectors. */
        *roundedCubeNormalVectors = new float[*numberOfNormals];

        if (roundedCubeNormalVectors == NULL)
        {
            LOGE("Pointer roundedCubeNormalVectors is NULL.");
            return;
        }

        /* Calculate the number of vertices for the rounded cube. */
        *numberOfVertices = *numberOfCoordinates / 4;

	    /* Values to store temporarily vertices and normal vectors. */
        Vec3f vertex, normalVector;

	    /* Value vertexIndex determines the beginning of array where vertices should be saved. */
	    int vertexIndex = 0;

	    /* Value normalVectorIndex determines the beginning of array where normal vectors should be saved. */
	    int normalVectorIndex = 0;
 
	    /* These values will change xzAngle and xyAngle. */
        float xzAngleDelta = 2.0f * M_PI / samples;
        float xyAngleDelta = 2.0f * M_PI / samples;
 
	    /* Angle used to compute vertices and normal vectors. */
        float xyAngle = -M_PI / 2.0f;
 
	    /* This loop goes samples / 2 times because in each second loop we create 2 triangles. */
        for (int j = 0; j < samples / 2; j++)
        {
		    /* Angle used to compute vertices and normal vectors. */
            float xzAngle = -M_PI;
 
            for (int i = 0; i < samples; i++)
            {
                /* Triangle #1 */
			    /* Calculate first vertex and normal vector. */
                vertex       = sample         (xyAngle, xzAngle, n1, n2, scale);
                normalVector = calculateNormal(xyAngle, xzAngle, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);

			    /* Calculate second vertex and normal vector. */
                vertex       = sample         (xyAngle + xyAngleDelta, xzAngle, n1, n2, scale);
                normalVector = calculateNormal(xyAngle + xyAngleDelta, xzAngle, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);

			    /* Calculate third vertex and normal vector. */
                vertex		 = sample         (xyAngle + xyAngleDelta, xzAngle + xzAngleDelta, n1, n2, scale);
                normalVector = calculateNormal(xyAngle + xyAngleDelta, xzAngle + xzAngleDelta, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);
 
                /* Triangle #2 */
			    /* Calculate first vertex and normal vector. */
                vertex		 = sample         (xyAngle, xzAngle, n1, n2, scale);
                normalVector = calculateNormal(xyAngle, xzAngle, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);

			    /* Calculate second vertex and normal vector. */
                vertex		 = sample         (xyAngle + xyAngleDelta, xzAngle + xzAngleDelta, n1, n2, scale);
                normalVector = calculateNormal(xyAngle + xyAngleDelta, xzAngle + xzAngleDelta, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);

			    /* Calculate third vertex and normal vector. */
                vertex		 = sample         (xyAngle, xzAngle + xzAngleDelta, n1, n2, scale);
                normalVector = calculateNormal(xyAngle, xzAngle + xzAngleDelta, n1, n2, scale);

			    /* Put vertex and normal vector coordinates into array. */
			    storeVertexAndNormalVectorInArray(vertex, normalVector, vertexIndex, normalVectorIndex, *roundedCubeCoordinates, *roundedCubeNormalVectors);
 
			    /* Change xzAngle value. */
                xzAngle += xzAngleDelta;
            }
		
		    /* Change xyAngle value. */
            xyAngle += xyAngleDelta;
        }
    }

    Vec3f SuperEllipsoidModel::calculateNormal(float xyAngle, float xzAngle, float n1, float n2, float scale)
    { 
	     /* Pre-calculate sine and cosine values for both angles. */
         float cosPhi  = cosf(xyAngle);
	     float cosBeta = cosf(xzAngle);
         float sinPhi  = sinf(xyAngle);
	     float sinBeta = sinf(xzAngle);

        /*
         * The equations for x, y and z coordinates are:
         * x = 1 / rx * cos^(2 - n1)(xyAngle) * cos^(2 - n2)(xzAngle)
         * y = 1 / ry * sin^(2 - n1)(xyAngle)
         * z = 1 / rz * cos^(2 - n1)(xyAngle) * sin^(2 - n2)(xzAngle)
         *
         * where:
         * -M_PI/2 <= xyAngle <= M_PI/2
         * -M_PI   <= xzAngle <= M_PI
         * 0 < n1, n2 < infinity
         *
         * As for two-dimensional case rx, ry, and rz are scale factors for each axis (axis intercept). 
         */

         /* Normal vector to be returned. */
         Vec3f normal;
 
         normal.x = signum(cosPhi) * powf(fabs(cosPhi), 2 - n1) * signum(cosBeta) * powf(fabs(cosBeta), 2 - n2) / scale;
	     normal.y = signum(sinPhi) * powf(fabs(sinPhi), 2 - n1) / scale;
         normal.z = signum(cosPhi) * powf(fabs(cosPhi), 2 - n1) * signum(sinBeta) * powf(fabs(sinBeta), 2 - n2) / scale;     
 
	     /* Normalize vector. */
         normal.normalize();
 
         return normal;
    }

    Vec3f SuperEllipsoidModel::sample(float xyAngle, float xzAngle, float n1, float n2, float scale)
    {
	    /* Pre-calculate sine and cosine values for both angles. */
        const float xyAngleCos = cosf(xyAngle); 
	    const float xzAngleCos = cosf(xzAngle);
        const float xyAngleSin = sinf(xyAngle); 
	    const float xzAngleSin = sinf(xzAngle);

        /*
         * The equations for x, y and z coordinates are given below:
         *
         * x = rx * cos^n1(xyAngle) * cos^n2(xzAngle)
         * y = ry * sin^n1(xyAngle)
         * z = rz * cos^n1(xyAngle) * sin^n2(xzAngle)
         *
         * where:
         *
         * -M_PI/2 <= xyAngle <= M_PI/2
         * -M_PI   <= xzAngle <= M_PI
         * 0 < n1, n2 < infinity
         *
         * As for two-dimensional case rx, ry, and rz are scale factors for each axis (axis intercept). 
	     */

        /* Vertex to be returned. */ 
        Vec3f vertex;

        vertex.x = scale * MaliSDK::signum(xyAngleCos) * powf(fabs(xyAngleCos), n1) * MaliSDK::signum(xzAngleCos) * powf(fabs(xzAngleCos), n2);
        vertex.y = scale * MaliSDK::signum(xyAngleSin) * powf(fabs(xyAngleSin), n1);
        vertex.z = scale * MaliSDK::signum(xyAngleCos) * powf(fabs(xyAngleCos), n1) * MaliSDK::signum(xzAngleSin) * powf(fabs(xzAngleSin), n2);
	
	    return vertex;
    }

    void SuperEllipsoidModel::storeVertexAndNormalVectorInArray(const Vec3f& vertex, const Vec3f& normalVector, int& vertexIndex, int& normalVectorIndex, float* roundedCubeCoordinates, float* roundedCubeNormalVectors)
    {
	    /* Save vertex and increment counter/index. */
	    roundedCubeCoordinates[vertexIndex++] = vertex.x;
	    roundedCubeCoordinates[vertexIndex++] = vertex.y;
	    roundedCubeCoordinates[vertexIndex++] = vertex.z;
	    roundedCubeCoordinates[vertexIndex++] = 1.0f;

	    /* Save normal vector and increment counter/index. */
	    roundedCubeNormalVectors[normalVectorIndex++] = normalVector.x;
	    roundedCubeNormalVectors[normalVectorIndex++] = normalVector.y;
	    roundedCubeNormalVectors[normalVectorIndex++] = normalVector.z;
    }
}