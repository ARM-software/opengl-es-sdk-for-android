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
#include "Matrix.h"
#include "Shader.h"
#include "TorusModel.h"
#include "WireframeTorus.h"

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <string>

using namespace MaliSDK;
using std::string;

WireframeTorus::WireframeTorus(float torusRadius, float circleRadius)
{
    /* Initialize class fields. */
    this->torusRadius     = torusRadius;
    this->circleRadius    = circleRadius;
    this->indicesBufferID = 0;

    /* Name of the file in which fragment shader's body is located. */
    const string fragmentShaderPath = resourceDirectory + "Instanced_Tessellation_Wireframe_shader.frag";
    /* Name of the file in which vertex shader's body is located. */
    const string vertexShaderPath   = resourceDirectory + "Instanced_Tessellation_Wireframe_shader.vert";

    /* Initialize shaders and program corresponding to the constructed torus object. */
    setupGraphics(vertexShaderPath, fragmentShaderPath);

    /* Determine indices of the mesh. */
    initializeBufferForIndices();

    /* Generate buffers and vertex arrays to store torus vertices and colors associated with them. */
    initializeVertexAttribs();

    /* Set wireframe color to orange. */
    setColor(1.0f, 0.3f, 0.0f, 1.0f);
}

WireframeTorus::~WireframeTorus()
{
    GL_CHECK(glDeleteBuffers(1, &indicesBufferID));
}

void WireframeTorus::initializeBufferForIndices()
{
    /* Temporary array to store determined indices. */
    unsigned int indices[indicesCount];

    TorusModel::calculateWireframeIndices(circlesCount, pointsPerCircleCount, indices);

    /* Create GL_ELEMENT_ARRAY_BUFFER and store indices in it. */
    GL_CHECK(glGenBuffers(1, &indicesBufferID));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferID));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof(int), indices, GL_STATIC_DRAW));
}

/* [Draw wireframe torus] */
void WireframeTorus::draw(float* rotationVector)
{
    GLint rotationVectorLocation = GL_CHECK(glGetUniformLocation(programID, "rotationVector"));

    /* Set required elements to draw mesh torus. */
    GL_CHECK(glUseProgram(programID));
    GL_CHECK(glBindVertexArray(vaoID));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBufferID));

    /* Pass Model-View matrix elements to the shader. */
    GL_CHECK(glUniform3fv(rotationVectorLocation, 1, rotationVector));

    /* Draw lines described by previously determined indices. */
    GL_CHECK(glDrawElements(GL_LINES, indicesCount, GL_UNSIGNED_INT, 0));
}
/* [Draw wireframe torus] */

bool WireframeTorus::initializeVertexAttribs()
{
    /* Get input attribute locations. */
    GLint  positionLocation = GL_CHECK(glGetAttribLocation(programID, "position"));

    /* ID of a buffer that stores vertices. */
    GLuint vertexBufferID = 0;

    /* Temporary arrays to keep vertex and color data. */
    float torusVertices[componentsCount];

    TorusModel::generateVertices(torusRadius, circleRadius, circlesCount, pointsPerCircleCount, torusVertices);

    /* Generate and bind vertex array object. */
    GL_CHECK(glGenVertexArrays(1, &vaoID));
    GL_CHECK(glBindVertexArray(vaoID));

    if (positionLocation != -1)
    {
        /* Generate and bind buffer object to store vertex data. */
        GL_CHECK(glGenBuffers(1, &vertexBufferID));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID));

        /* Store torus vertices inside the generated buffer. */
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, componentsCount * sizeof(float), torusVertices, GL_STATIC_DRAW));

        /* Set vertex attrib pointer to the beginning of the bound array buffer. */
        GL_CHECK(glVertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, 0, NULL));
        GL_CHECK(glEnableVertexAttribArray(positionLocation));
    }
    else
    {
        LOGE("Could not locate \"position\" input attribute in program [%d].", programID);
        return false;
    }

    return true;
}
