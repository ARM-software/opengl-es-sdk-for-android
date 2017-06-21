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

#ifndef TORUS_MODEL_H
#define TORUS_MODEL_H

#include "VectorTypes.h"

namespace MaliSDK
{
    /**
     * \brief Functions for generating torus shapes.
     */
    class TorusModel
    {
    public:
        /**
         * \brief Generates torus's normal vectors.
         *
         * \param[in] circlesCount Number of circles in torus model.
         * \param[in] pointsPerCircleCount Number of points in one circle.
         * \param[out] normals Deref will be used to store normal vectors.
         */
        static void generateNormals(unsigned int circlesCount, unsigned int pointsPerCircleCount, float* normals);

        /**
         * \brief Determines indices for DrawElements() call for shaded torus drawn in triangle strip mode.
         *
         * \param[in] circlesCount Number of circles in torus model.
         * \param[in] pointsPerCircleCount Number of points in one circle.
         * \param[out] indices Deref will be used to store calculated indices.
         */
        static void calculateTriangleStripIndices(unsigned int circlesCount, unsigned int pointsPerCircleCount, unsigned int* indices);

        /**
         * \brief Generate vertices of the torus model. 
         * 
         * The vertices are grouped in circlesCount circles, where each circle consists of pointsPerCircleCount vertices.
         *
         * \param[in] torusRadius Distance between the center of torus and the center of its tube.
         * \param[in] circleRadius Radius of circles that model the tube.
         * \param[in] circlesCount Number of circles in torus model.
         * \param[in] pointsPerCircleCount Number of points in one circle.
         * \param[out] vertices Deref will be used to store generated vertices. Cannot be null.
         */
        static void generateVertices(float torusRadius, float circleRadius, unsigned int circlesCount, unsigned int pointsPerCircleCount, float* vertices);

        /**
         * \brief Determines an array of indices defining a mesh of control points for instanced torus patches.
         *
         * To simplify mathemathics, it is assumed that torus model consists of 12 circles, each built of
         * 12 points, so it is easy to divide each circle into 4 quadrants and define Bezier surfaces
         * approximating perfectly round torus.
         *
         * \param[in] patchDimension Number of control points in one dimension for a patch.
         * \param[in] patchInstancesCount Number of instances needed to draw the whole torus.
         * \param[in] controlPointsIndicesCount Number of indices needed to create a control mesh.
         * \param[out] controlPointsIndices Deref will be used to store control points indices. Cannot be null.
         */
        static void calculateControlPointsIndices(unsigned int patchDimension, unsigned int patchInstancesCount, unsigned int controlPointsIndicesCount, unsigned int* controlPointsIndices);

        /**
         * \brief Determines patch data for an instanced torus model.
         *
         * \param[in] patchDensity Number of vertices in one edge of a patch.
         * \param[out] patchVertices Deref will be used to store patch vertices. Cannot be null.
         * \param[out] patchTriangleIndices Deref will be used to store indices of triangle vertices. Cannot be null.
         */
        static void calculatePatchData(unsigned int patchDensity, float* patchVertices, unsigned int* patchTriangleIndices);

        /**
         * \brief Determines indices for glDrawElements() call for wireframed torus.
         *
         * \param[in] circlesCount Number of circles in torus model.
         * \param[in] pointsPerCircleCount Number of points in one circle.
         * \param[out] indices Deref will be used to store calculated indices.
         */
        static void calculateWireframeIndices(unsigned int circlesCount, unsigned int pointsPerCircleCount, unsigned int* indices);


        /**
         * \brief Generate torus vertices applying distortions to some of them. 
         *
         * The distortions in control mesh are needed for proper construction of Bezier surface patches.
         * It is assumed that each patch consists of 4 control rows and columns.
         * Hence, in each column and each row, we can distinguish 2 middle control points
         * and 2 edge control points, which are shared between patches. The middle control points have to be moved 
         * in such a way that C1 continuity between patches is satisfied.
         * Implemented algorithm assumes that each construction circle contains 12 points and the torus model consists
         * of 12 circles.
         *
         * \param[in] torusRadius Distance between the center of torus and the center of its tube.
         * \param[in] circleRadius Radius of circles that model the tube.
         * \param[out] vertices Deref will be used to sotre generated vertices. Cannot be null.
         */
        static void generateBezierVertices(float torusRadius, float circleRadius, float* vertices);
    };
}
#endif /* TORUS_MODEL_H */