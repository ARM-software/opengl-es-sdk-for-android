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
#include "InstancedSolidTorus.h"
#include "Mathematics.h"
#include "Matrix.h"
#include "Shader.h"
#include "TorusModel.h"

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <cmath>
#include <string>

using namespace MaliSDK;
using std::string;

InstancedSolidTorus::InstancedSolidTorus(float torusRadius, float circleRadius)
{
    /* Initialize class fields. */
    this->torusRadius  = torusRadius;
    this->circleRadius = circleRadius;

    /* Name of the file in which fragment shader's body is located. */
    const string fragmentShaderPath = resourceDirectory + "Instanced_Tessellation_Instanced_shader.frag";

    /* Name of the file in which vertex shader's body is located. */
    const string vertexShaderPath = resourceDirectory + "Instanced_Tessellation_Instanced_shader.vert";

    /* Initialize OpenGL components. */
    setupGraphics(vertexShaderPath, fragmentShaderPath);

    /* Create control mesh and initialize uniform buffers corresponding to it. */
    initializeControlUniformBuffers();
    /* Create patch data and initialize vertex attribs corresponding to it. */
    initializeVertexAttribs();

    /* Set torus color to green. */
    setColor(0.0f, 0.7f, 0.0f, 1.0f);

    /* Configure light parameters. */
    setLightParameters();
}

InstancedSolidTorus::~InstancedSolidTorus()
{
    GLuint buffersArray[]   = {controlIndicesBufferID, controlVerticesBufferID, patchIndicesBufferID, patchVertexBufferID};
    GLint  buffersArraySize = sizeof(buffersArray) / sizeof(buffersArray[0]);

    /* Delete all buffers corresponding to this class. */
    GL_CHECK(glDeleteBuffers(buffersArraySize, &controlIndicesBufferID));
}

/* [Draw solid torus] */
void InstancedSolidTorus::draw(float* rotationVector)
{
    /* Location of rotation vector. */
    GLint rotationVectorLocation = GL_CHECK(glGetUniformLocation(programID, "rotationVector"));

    /* Set required OpenGL ES state. */
    GL_CHECK(glUseProgram     (programID                                    ));
    GL_CHECK(glBindVertexArray(vaoID                                        ));
    GL_CHECK(glBindBuffer     (GL_ELEMENT_ARRAY_BUFFER, patchIndicesBufferID));

    if (rotationVectorLocation != -1)
    {
        /* Pass rotation parameters to the shader. */
        GL_CHECK(glUniform3fv(rotationVectorLocation, 1, rotationVector));
    }
    else
    {
        LOGE("Could not locate \"rotationVector\" uniform in program [%d]", programID);
    }

    /* Draw patchInstancesCount instances of patchTriangleIndicesCount triangles. */ 
    GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, patchTriangleIndicesCount, GL_UNSIGNED_INT, 0, patchInstancesCount));
}
/* [Draw solid torus] */

bool InstancedSolidTorus::initializeControlUniformBuffers()
{
    float        torusVertices       [componentsCount];
    unsigned int controlPointsIndices[controlPointsIndicesCount];

    /* Generate torus vertices which can be used to construct Bezier surfaces. */
    TorusModel::generateBezierVertices(torusRadius, circleRadius, torusVertices);
    /* Calculate the indices that will divide generated torus vertices into patches. */
    TorusModel::calculateControlPointsIndices(patchDimension, patchInstancesCount, controlPointsIndicesCount, controlPointsIndices);

    GLuint controlIndicesBlockIndex = GL_CHECK(glGetUniformBlockIndex(programID, "ControlPointsIndices"));

    if (controlIndicesBlockIndex != GL_INVALID_INDEX)
    {
        /* Pass control points indices to corresponding uniform block. */
        GL_CHECK(glGenBuffers         (1,                 &controlIndicesBufferID                                           ));
        GL_CHECK(glBindBuffer         (GL_UNIFORM_BUFFER, controlIndicesBufferID                                            ));
        GL_CHECK(glBufferData         (GL_UNIFORM_BUFFER, sizeof(controlPointsIndices), controlPointsIndices, GL_STATIC_DRAW));
        GL_CHECK(glUniformBlockBinding(programID,         controlIndicesBlockIndex,     0                                   ));
        GL_CHECK(glBindBufferBase     (GL_UNIFORM_BUFFER, 0,                            controlIndicesBufferID              ));
    }
    else
    {
        LOGE("Could not locate \"ControlPointsIndices\" uniform block in program [%d].", programID);
    }

    GLuint controlVerticesBlockIndex = GL_CHECK(glGetUniformBlockIndex(programID, "ControlPointsVertices"));

    if (controlVerticesBlockIndex != GL_INVALID_INDEX)
    {
        /* Pass control points vertices to corresponding uniform block. */
        GL_CHECK(glGenBuffers         (1,                 &controlVerticesBufferID                                      ));
        GL_CHECK(glBindBuffer         (GL_UNIFORM_BUFFER, controlVerticesBufferID                                       ));
        GL_CHECK(glBufferData         (GL_UNIFORM_BUFFER, componentsCount * sizeof(float), torusVertices, GL_STATIC_DRAW));
        GL_CHECK(glUniformBlockBinding(programID,         controlVerticesBlockIndex,       1                            ));
        GL_CHECK(glBindBufferBase     (GL_UNIFORM_BUFFER, 1,                               controlVerticesBufferID      ));
    }
    else
    {
        LOGE("Could not locate \"ControlPointsVertices\" uniform block in program [%d].", programID);
    }

    return true;
}

bool InstancedSolidTorus::initializeVertexAttribs()
{
    /* Find input attribute location. */
    GLint positionLocation = GL_CHECK(glGetAttribLocation(programID, "patchUVPosition"));

    float        patchVertices       [patchComponentsCount];
    unsigned int patchTriangleIndices[patchTriangleIndicesCount];

    /* Determine input data. */
    TorusModel::calculatePatchData(patchDensity, patchVertices, patchTriangleIndices);

    /* Generate corresponding vertex array object. */
    GL_CHECK(glGenVertexArrays(1, &vaoID));
    GL_CHECK(glBindVertexArray(vaoID));

    if (positionLocation != -1)
    {
        /* Generate a buffer for input attribute data. */
        GL_CHECK(glGenBuffers(1,               &patchVertexBufferID));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, patchVertexBufferID ));
        /* Put data to the buffer. */
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, patchComponentsCount * sizeof(float), patchVertices, GL_STATIC_DRAW));
        /* Set vertex attribute pointer to the beginning of the buffer. */
        GL_CHECK(glVertexAttribPointer    (positionLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL));
        GL_CHECK(glEnableVertexAttribArray(positionLocation                                ));
    }
    else
    {
        LOGE("Could not locate \"patchUVposition\" input attribute in program [%d].", programID);
        return false;
    }

    /* Generate a buffer for indices used in DrawElements*() call. */
    GL_CHECK(glGenBuffers(1, &patchIndicesBufferID                                                                                       ));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patchIndicesBufferID                                                                  ));
    /* Put data to the buffer. */
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, patchTriangleIndicesCount * sizeof(unsigned int), patchTriangleIndices, GL_STATIC_DRAW));

    return true;
}

void InstancedSolidTorus::setLightParameters()
{
    /* Light angle equal to 30 degrees. */
    const float lightAngle = M_PI / 6.0f;
    /* White ligth color. */
    const float lightColor[] = {1.0f, 1.0f, 1.0f};
    /* Direction of the light vector. */
    const float lightDirection[] = {cosf(lightAngle), sinf(lightAngle), 2.0f};
    /* Value of ambient intensity. */
    const float ambientIntensity = 0.2f;

    /* Find light parameters locations. */
    GLint lightColorLocation       = GL_CHECK(glGetUniformLocation(programID, "light.lightColor"));
    GLint lightDirectionLocation   = GL_CHECK(glGetUniformLocation(programID, "light.lightDirection"));
    GLint ambientIntensityLocation = GL_CHECK(glGetUniformLocation(programID, "light.ambientIntensity"));

    GL_CHECK(glUseProgram(programID));

    /* Pass light parameter data. */
    if (lightColorLocation != -1)
    {
        GL_CHECK(glUniform3fv(lightColorLocation, 1, lightColor));
    }
    else
    {
        LOGE("Could not find \"light.lightColor\" uniform in program [%d]", programID);
    }

    if (lightDirectionLocation != -1)
    {
        GL_CHECK(glUniform3fv(lightDirectionLocation, 1, lightDirection));
    }
    else
    {
        LOGE("Could not find \"light.lightDirection\" uniform in program [%d]", programID);
    }

    if (ambientIntensityLocation != -1)
    {
        GL_CHECK(glUniform1f(ambientIntensityLocation, ambientIntensity));
    }
    else
    {
        LOGE("Could not find \"light.ambientIntensity\" uniform in program [%d]", programID);
    }
}
