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

#ifndef INSTANCED_SOLID_TORUS_H
#define INSTANCED_SOLID_TORUS_H

#include "Torus.h"

/**
 * \brief Class derived form Torus abstract class. It manages drawing of a rotating
 *        solid torus, built from separate patches. Each patch is modelled as a Bezier surface
 *        approximating surface of a perfect torus. To satisfy the C1 continuity between neighbour
 *        patches, the number of circles creating the torus and also the number of points in each
 *        circle is restricted to 12. It allows us to divide both circles of torus ("big" and "small")
 *        into 4 quadrants and approximate each of it using bicubic Bezier curves. Control mesh vertices has to
 *        be distored, so the derivatives on the patch edges are equal and resulting image is round. That is
 *        why we cannot use the regular way to determine control points.
 *        The patches are in fact very dense square-shaped meshes, used as input attributes by vertex shader. The shader
 *        changes their shape on the basis of the distorted control mesh and places them next to each other, forming a
 *        round torus.
 *        The class, apart from inherited components, manages:
 *        - 2 uniform buffers where control point vertices and indices are stored,
 *        - an array buffer storing patch vertices,
 *        - an element array buffer storing indices,
 *        which are used in a glDrawElementsInstanced() call. It is also capable of determining the needed indices arrays.
 */
class InstancedSolidTorus : public Torus
{
private:
    /**
     * \brief Number of control points in one dimension for a patch.
     */
    static const unsigned int patchDimension = 4;
    /** 
     * \brief Total number of control points in a patch.
     */
    static const unsigned int controlPointsInPatchCount = patchDimension * patchDimension;
    /**
     * \brief Number of quads in a patch.
     */
    static const unsigned int quadsInPatchCount = (patchDimension - 1) * (patchDimension - 1);
    /** 
     * \brief Number of indices needed to create a control mesh.
     */
    static const unsigned int controlPointsIndicesCount = controlPointsInPatchCount * torusVerticesCount / quadsInPatchCount;
    /** 
     * \brief Number of instances needed to draw the whole torus.
     */
    static const unsigned int patchInstancesCount = controlPointsIndicesCount / controlPointsInPatchCount;
    /** 
     * \brief Number of vertices in one edge of a patch.
     */
    static const unsigned int patchDensity = 16;
    /** 
     * \brief Total number of vertices in a patch.
     */
    static const unsigned int patchVerticesCount = patchDensity * patchDensity;
    /** 
     * \brief Total number of components describing a patch (only U/V components are defined).
     */
    static const unsigned int patchComponentsCount = patchVerticesCount * 2;
    /**
     * \brief Number of indices that need to be defined to draw quads consisting of triangles (6 points per quad needed) over the entire patch.
     */
    static const unsigned int patchTriangleIndicesCount = (patchDensity - 1) * (patchDensity - 1) * 6;

    /** 
     * \brief Index of a buffer that we bind to GL_UNIFORM_BUFFER binding point. It stores uniform control indices of torus control mesh.
     */
    GLuint controlIndicesBufferID;
    /** 
     * \brief Index of a buffer that we bind to GL_UNIFORM_BUFFER binding point. It stores uniform control vertices of torus control mesh.
     */
    GLuint controlVerticesBufferID;
    /** 
     * \brief Index of a buffer that we bind to GL_ELEMENT_ARRAY_BUFFER binding point. 
     *        It containts indices of patch triangles, so that we can use an element-type draw call to show the object.
     */
    GLuint patchIndicesBufferID;
    /** 
     * \brief Index of a buffer that we bind to GL_ARRAY_BUFFER binding point. 
     *        It stores patch vertices passed as an input to the corresponding vertex shader.
     */
    GLuint patchVertexBufferID;

    /**
     * \brief Initializes control mesh data and stores it in appropriate uniform buffers.
     */
    bool initializeControlUniformBuffers(void);

    bool initializeVertexAttribs(void);

    /**
     * \brief Sets directionl light parameters, such as light direction, its color and ambient intensity
     *        and passes it to corresponding uniforms in shader.
     */
    void setLightParameters(void);

public:

    /**
     * \brief Instantiates a representation of a solid torus, using user-provided radius and tube radius.
     *
     * \param torusRadius  [in] Distance between center of torus and center of its tube.
     * \param circleRadius [in] Radius of circles that model the tube.
     */
    InstancedSolidTorus(float torusRadius, float circleRadius);

    /**
     * \brief Frees allocated memory.
     */
    ~InstancedSolidTorus(void);

    /**
     * \brief Draws instanced solid torus.
     *
     * \param rotationVector [in] Vector of 3 elements storing rotation parameters to be passed to the vertex shader.
     */
    void draw(float* rotationVector);
};

#endif /* INSTANCED_SOLID_TORUS_H. */
