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

#ifndef TORUS_H
#define TORUS_H

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <string>

#include "Matrix.h"

/** 
 * \brief Abstract class that draws torus on the screen. It stores generic data describing the drawn torus:
          - its radii,
          - number of points used to model it,
          - ID of a program associated with it,
          - ID of a vertex array object in which pointers to the corrresponding data are stored. 
          Derived classes must determine a way of initializing vertex attribute arrays and drawing the torus model.
 */
class Torus
{
protected:
    /**
     * Path to the directory containing shaders and textures.
     */
    static std::string resourceDirectory;
    /**
     * \brief Number of coordinates for one vertex.
     */
    static const unsigned int vertexComponentsCount = 4;
    /**
     * \brief Number of circles in torus model.
     */
    static const unsigned int circlesCount = 12;
    /**
     * \brief Number of points in one circle.
     */
    static const unsigned int pointsPerCircleCount = 12;
    /**
     * \brief Total number of vertices in torus model.
     */
    static const unsigned int torusVerticesCount = pointsPerCircleCount * circlesCount;
    /**
     * \brief Total number of components in torus model, needed to determine the size of vertex arrays.
     */
    static const unsigned int componentsCount = torusVerticesCount * vertexComponentsCount;

    /**
     * \brief Distance between the center of torus and the center of its tube. 
     */
    float torusRadius;
    /**
     * \brief Radius of circles that model the tube.
     */
    float circleRadius;

    /** 
     * \brief ID of a program linked to the torus model.
     */
    GLuint programID;
    /** 
     * \brief ID of a vertex array object that stores pointers to vertex data sources used to rasterize given mesh.
     */
    GLuint vaoID;

    /**
     * \brief Protected constructor used to do intialization general to all torus objects.
     */
    Torus(void);
    
    /**
     * \brief Initialize vertex attribute arrays and buffer objects coresponding to them.
     *        Make sure that programID has been set before this function is called.
     * \return false if error reported, true otherwise.
     */
    virtual bool initializeVertexAttribs() = 0;

    /**
     * \brief Sets the uniform color of the drawn torus.
     *
     * \param red   [in] Value for red channel.
     * \param green [in] Value for green channel.
     * \param blue  [in] Value for blue channel.
     * \param alpha [in] Vlaue for alpha channel.
     */
    void setColor(float red, float green, float blue, float alpha);

    /**
     * \brief Initialize constant OpenGL components such as program, shaders and constant matrices.
     *
     * \param vertexShaderPath   [in] Path of the file containing vertex shader source.
     * \param fragmentShaderPath [in] Path of the file containing fragment shader source.
     */
    void setupGraphics(const std::string vertexShaderPath, const std::string fragmentShaderPath);

public:

    /**
     * \brief Frees allocated memory.
     */
    virtual ~Torus(void);

    /**
     * \brief Draw the torus model.
     *
     * \param rotationVector Vector with rotation parameters to be passed to the vertex shader.
     */
    virtual void draw(float* rotationVector) = 0;

    /**
     * \brief Pass the correctly defined projection matrix to the program related to the torus model.
     *
     * \param projectionMatrix [in] Projection matrix which will be passed to the vertex shader.
     */
    void setProjectionMatrix(MaliSDK::Matrix* projectionMatrix);

    /**
     * \brief Set the resource directory for all tori.
     *
     * \param requiredResourceDirectory [in] The required resource directory.
     */
    void static setResourceDirectory(std::string requiredResourceDirectory);
};

#endif /* TORUS_H */