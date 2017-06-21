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

#include "TorusModel.h"

#include "Platform.h"
#include "Mathematics.h"

#include <cassert>

namespace MaliSDK
{
    void TorusModel::generateNormals(unsigned int circlesCount, unsigned int pointsPerCircleCount, float* normals)
    {
        unsigned int normalSize = 3;
        unsigned int index = 0;

        for (unsigned int horizontalIndex = 0; horizontalIndex < circlesCount; ++horizontalIndex)
        {
            /* Angle in radians on XZ plane. */
            float phi = (float) horizontalIndex * 2.0f * M_PI / circlesCount;

            Vec3f horizontalTangent = {-sinf(phi), 0.0f, cosf(phi)};

            for (unsigned int verticalIndex = 0; verticalIndex < pointsPerCircleCount; ++verticalIndex)
            {
                /* Angle in radians on XY plane. */
                float theta  = (float) verticalIndex * 2.0f * M_PI / pointsPerCircleCount;

                Vec3f verticalTangent = {-cosf(phi) * sinf(theta), cosf(theta), -sinf(phi) * sinf(theta)};

                assert(index < circlesCount * pointsPerCircleCount * normalSize);

                normals[index++] = horizontalTangent.z * verticalTangent.y - horizontalTangent.y * verticalTangent.z;
                normals[index++] = horizontalTangent.x * verticalTangent.z - horizontalTangent.z * verticalTangent.x;
                normals[index++] = horizontalTangent.y * verticalTangent.x - horizontalTangent.x * verticalTangent.y;
            }
        }
    }

    void TorusModel::calculateControlPointsIndices(unsigned int patchDimension, unsigned int patchInstancesCount, unsigned int controlPointsIndicesCount, unsigned int* controlPointsIndices)
    {
        if (controlPointsIndices == NULL)
        {
            LOGE("Cannot use null pointer while calculating control points indices.");
    
            return;
        }
    
        /* Definition of needed constants. Torus continuity cannot be guaranteed with other parameters. */
        const unsigned int pointsPerCircleCount = 12;
        const unsigned int circlesCount         = 12;
        const unsigned int torusVerticesCount   = pointsPerCircleCount * circlesCount;
    
        /* Index of a vertex from which a patch starts. */
        unsigned int startIndex = 0;
        /* Index of a circle from which vertex indices are currently taken. */
        unsigned int currentCircle = 0;
    
        /* Index variable. */
        unsigned int index = 0;
    
        /* Loop that creates patches for each instance of patch primitives. Successive patches wrap around torus horizontally. */
        for (unsigned int instanceIndex = 0; instanceIndex < patchInstancesCount; ++instanceIndex)
        {
            /* Iterate in horizontal axis. */
            for (unsigned int x = 0; x < patchDimension; ++x)
            {
                /* Determine index of current circle from which the vertex indices are taken. */
                currentCircle = startIndex / pointsPerCircleCount;
    
                /* Iterate in vertical axis. */
                for (unsigned int y = 0; y < patchDimension; ++y)
                {
                    unsigned int currentIndex = startIndex + y;
    
                    /* Make closing patches end up at the very first vertex of each circle. */
                    if (currentIndex >= pointsPerCircleCount * (currentCircle + 1))
                    {
                        currentIndex -= pointsPerCircleCount;
                    }
    
                    controlPointsIndices[index++] = currentIndex;
    
                    assert(index <= controlPointsIndicesCount);
                }
    
                /* Get indices from the next circle. */
                startIndex += pointsPerCircleCount;
    
                /* Make closing patches end up at the very first circle. */
                if (startIndex >= torusVerticesCount)
                {
                    startIndex -= torusVerticesCount;
                }
            }
    
            /* Neighbouring patches always share one edge, so start index of the next patch should start from the last column of the previous patch. */
            startIndex -= pointsPerCircleCount;
    
            /* When the whole row is finished, move to the next one. */
            if (currentCircle == 0)
            {
                startIndex += patchDimension - 1;
            }
        }
    }
    void TorusModel::calculatePatchData(unsigned int patchDensity, float* patchVertices, unsigned int* patchTriangleIndices)
    {
        if (patchVertices == NULL || patchTriangleIndices == NULL)
        {
            LOGE("Cannot use null pointers while calculating patch data.");
    
            return;
        }

        /* Total number of components describing a patch (only U/V components are definied). */
        const unsigned int patchComponentsCount = patchDensity * patchDensity * 2;
        /* Number of indices that needs to be defined to draw quads consisted of triangles (6 points per quad needed) over the entire patch. */
        const unsigned int patchTriangleIndicesCount = (patchDensity - 1) * (patchDensity - 1) * 6;

        /* Number of components in a single vertex. */
        const unsigned int uvComponentsCount = 2;
        /* Number of vertices needed to draw a quad as two separate triangles. */
        const unsigned int verticesPerQuadCount = 6;
    
        /* Current index of a patch vertex. */
        unsigned int uvIndex = 0;
        /* Current index for indices array. */
        unsigned int triangleVertexIndex = 0;
    
        for (unsigned int x = 0; x < patchDensity; ++x)
        {
            /* Horizontal component. */
            float u = (float) x / (patchDensity - 1);
    
            for (unsigned int y = 0; y < patchDensity; ++y)
            {
                /* Vertical component. */
                float v = (float) y / (patchDensity - 1);
    
                patchVertices[uvIndex++] = u;
                patchVertices[uvIndex++] = v;
    
                assert(uvIndex <= patchComponentsCount);
            }
        }
    
        /*
         * Indices are determined in the following manner:
         * 
         * 0 -> 1 -> 16 -> 16 -> 1 -> 17 -> 1 -> 2 -> 17 -> 17 -> 2 -> 18 -> ...
         *
         * 2----18----34---...
         * |  /  |  /  |
         * | /   | /   |
         * 1----17----33---...
         * |  /  |  /  |
         * | /   | /   |
         * 0----16----32----...
         */
        for (unsigned int x = 0; x < patchDensity - 1; ++x)
        {
            for (unsigned int y = 0; y < patchDensity - 1; ++y)
            {
                patchTriangleIndices[triangleVertexIndex++] = patchDensity *  x      + y;
                patchTriangleIndices[triangleVertexIndex++] = patchDensity *  x      + y + 1;
                patchTriangleIndices[triangleVertexIndex++] = patchDensity * (x + 1) + y;
    
                patchTriangleIndices[triangleVertexIndex++] = patchDensity * (x + 1) + y;
                patchTriangleIndices[triangleVertexIndex++] = patchDensity *  x      + y + 1;
                patchTriangleIndices[triangleVertexIndex++] = patchDensity * (x + 1) + y + 1;
    
                assert(triangleVertexIndex <= patchTriangleIndicesCount);
            }
        }
    }

    void TorusModel::calculateWireframeIndices(unsigned int circlesCount, unsigned int pointsPerCircleCount, unsigned int* indices)
    {
        const unsigned int torusVerticesCount = circlesCount * pointsPerCircleCount;

        for (unsigned int i = 0; i < circlesCount; ++i)
        {
            for (unsigned int j = 0; j < pointsPerCircleCount; ++j)
            {
                /* Starting point for vertical and horizontal lines. */
                unsigned int lineStart     = i * pointsPerCircleCount + j;
                /* Horiznotal end of the currently determined line. */
                unsigned int horizontalEnd = (i + 1) * pointsPerCircleCount + j;
                /* Vertical end of the currently determined line. */
                unsigned int verticalEnd   = i * pointsPerCircleCount + j + 1;

                /* From the last circle, horizontal lines go to the first one. */
                if (horizontalEnd >= torusVerticesCount)
                {
                    horizontalEnd -= torusVerticesCount;
                }

                /* From the last point in the circle, vertical lines go to the first one. */
                if (verticalEnd >= (i + 1) * pointsPerCircleCount)
                {
                    verticalEnd -= pointsPerCircleCount;
                }

                /* Determine horizontal line indices. */
                indices[(i * pointsPerCircleCount + j) * 4    ] = lineStart;
                indices[(i * pointsPerCircleCount + j) * 4 + 1] = horizontalEnd;

                /* Determine vertical line indices. */
                indices[(i * pointsPerCircleCount + j) * 4 + 2] = lineStart;
                indices[(i * pointsPerCircleCount + j) * 4 + 3] = verticalEnd;
            }
        }
    }

    void TorusModel::generateVertices(float torusRadius, float circleRadius, unsigned int circlesCount, unsigned int pointsPerCircleCount, float* vertices)
    {
        if (vertices == NULL)
        {
            LOGE("Cannot use null pointer while calculating torus vertices.");

            return;
        }

        /* Index variable. */
        unsigned int componentIndex = 0;

        for (unsigned int horizontalIndex = 0; horizontalIndex < circlesCount; ++horizontalIndex)
        {
            /* Angle in radians on XZ plane. */
            float xyAngle = (float) horizontalIndex * 2.0f * M_PI / circlesCount;

            for (unsigned int verticalIndex = 0; verticalIndex < pointsPerCircleCount; ++verticalIndex)
            {
                /* Angle in radians on XY plane. */
                float theta  = (float) verticalIndex * 2.0f * M_PI / pointsPerCircleCount;

                /* X coordinate. */
                vertices[componentIndex++] = (torusRadius + circleRadius * cosf(theta)) * cosf(xyAngle);
                /* Y coordinate. */
                vertices[componentIndex++] = circleRadius * sinf(theta);
                /* Z coordinate. */
                vertices[componentIndex++] = (torusRadius + circleRadius * cosf(theta)) * sinf(xyAngle);
                /* W coordinate. */
                vertices[componentIndex++] = 1.0f;
            }
        }
    }

    void TorusModel::calculateTriangleStripIndices(unsigned int circlesCount, unsigned int pointsPerCircleCount, unsigned int* indices)
    {
        const unsigned int torusVerticesCount = circlesCount * pointsPerCircleCount;
        const unsigned int torusIndicesCount  = (2 * circlesCount + 1) * pointsPerCircleCount + 1;

        unsigned int counter = 0;
        unsigned int currentIndex = 0;

        indices[counter++] = currentIndex;

        bool isLastStrip = false;

        for (unsigned int stripIndex = 0; stripIndex < pointsPerCircleCount; ++stripIndex)
        {
            assert(currentIndex == stripIndex);
            
            /* Set initial index for the current strip. */
            currentIndex += 1;

            isLastStrip = currentIndex >= pointsPerCircleCount;

            assert(counter < torusIndicesCount);

            indices[counter++] = isLastStrip ? (currentIndex - pointsPerCircleCount) : currentIndex;

            for (unsigned int circleIndex = 0; circleIndex < circlesCount; ++circleIndex)
            {
                currentIndex = currentIndex + pointsPerCircleCount - 1;

                if (currentIndex >= torusVerticesCount)
                {
                    currentIndex -= torusVerticesCount;
                }

                assert(counter < torusIndicesCount);

                indices[counter++] = currentIndex;

                currentIndex += 1;

                assert(counter < torusIndicesCount);

                indices[counter++] = isLastStrip ? currentIndex - pointsPerCircleCount : currentIndex;
            }
        }
    }

    void TorusModel::generateBezierVertices(float torusRadius, float circleRadius, float* vertices)
    {
        if (vertices == NULL)
        {
            LOGE("Cannot use null pointer while calculating torus vertices.");

            return;
        }

        /* Coefficient relating radius of a circle to the distance between middle patch control point and the closest edge point. */
        const float kappa = 4.0f * (sqrtf(2.0f) - 1.0f) / 3.0f;
        /* Angle between circle radius connecting a patch edge point and a line segment connecting the circle center and a middle patch control point. */
        const float alpha = atanf(kappa);
        /* Length of a line segment connecting circle center and a middle control point. */
        const float distortedCircleRadius = circleRadius * sqrt(1.0f + kappa * kappa);
        /* Length of a line segment connecting torus center and a middle control poin. */
        const float distortedTorusRadius = torusRadius  * sqrt(1.0f + kappa * kappa);
        /* Each circle is divided into 4 quadrants to simplify calculations. */
        const int quadrantsCount = 4;
        /* Number of circles in torus model. */
        const int circlesCount = 12;
        /* Number of points in one circle. */
        const int pointsPerCircleCount = 12;

        /* Angle in horizontal plane XZ, used only to point on edge points. */
        float phi = 0.0f;
        /* Angle in vertical plane XY, used only to point on edge points. */
        float theta = 0.0f;

        /* Index of currently calculated component. */
        unsigned int componentIndex = 0;

        /* Iterate through all circles. */
        for (int horizontalIndex = 0; horizontalIndex < circlesCount; ++horizontalIndex)
        {
            /* Index of a circle in a torus quadrant. */
            const int currentCircleModulo = horizontalIndex % (quadrantsCount - 1);

            /* Temporary variables holding current values of radii and angles. */
            float currentTorusRadius;
            float currentCircleRadius;
            float currentPhi;
            float currentTheta;

            switch (currentCircleModulo)
            {
                case 0:
                    /* Edge points take non-distorted parameters. */
                    currentTorusRadius = torusRadius;
                    currentPhi         = phi;
                    break;
                case 1:
                    /* 1st middle point. Angle value is related to the angle of preceding edge point. */
                    currentTorusRadius = distortedTorusRadius;
                    currentPhi         = phi + alpha;
                    break;
                case 2:
                    /* Second middle point. Angle value is related to the angle of the following edge point. */
                    phi                = (float) (horizontalIndex + 1) * M_PI / (2 * (quadrantsCount - 1));
                    currentTorusRadius = distortedTorusRadius;
                    currentPhi         = phi - alpha;
                    break;
            }

            for (int verticalIndex = 0; verticalIndex < pointsPerCircleCount; ++verticalIndex)
            {
                /* Index of a point in a circle quadrant. */
                const int currentPointModulo = verticalIndex   % (quadrantsCount - 1);

                switch (currentPointModulo)
                {
                    case 0:
                        /* Edge points take non-distorted parameters. */
                        currentCircleRadius = circleRadius;
                        currentTheta        = theta;
                        break;
                    case 1:
                        /* 1st middle point. Angle value is related to the angle of preceding edge point. */
                        currentCircleRadius = distortedCircleRadius;
                        currentTheta        = theta + alpha;
                        break;
                    case 2:
                        /* Second middle point. Angle value is related to the angle of the following edge point. */
                        theta               = (float) (verticalIndex + 1) * M_PI / (2 * (quadrantsCount - 1));
                        currentCircleRadius = distortedCircleRadius;
                        currentTheta        = theta - alpha;
                }

                /* Store values in the array. */
                vertices[componentIndex++] = (currentTorusRadius + currentCircleRadius * cosf(currentTheta)) * cosf(currentPhi);
                vertices[componentIndex++] =  currentCircleRadius * sinf(currentTheta);
                vertices[componentIndex++] = (currentTorusRadius + currentCircleRadius * cosf(currentTheta)) * sinf(currentPhi);
                vertices[componentIndex++] = 1.0f;
            }
        }
    }
}