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
#include "Torus.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>

using namespace MaliSDK;
using std::string;

string Torus::resourceDirectory;

Torus::Torus()
{
    if (resourceDirectory.empty())
    {
        LOGE("Resource Directory has not been set\n");
    }
}

Torus::~Torus()
{
    GL_CHECK(glDeleteProgram(programID));
    GL_CHECK(glDeleteVertexArrays(1, &vaoID));
}

void Torus::setColor(float red, float green, float blue, float alpha)
{
    GLint colorLocation = GL_CHECK(glGetUniformLocation(programID, "color"));

    float color[] = {red, green, blue, alpha};

    GL_CHECK(glUseProgram(programID));

    if (colorLocation != -1)
    {
        GL_CHECK(glUniform4fv(colorLocation, 1, color));
    }
    else
    {
        LOGE("Could not locate \"color\" uniform in program [%d]", programID);
    }
}

void Torus::setProjectionMatrix(Matrix* projectionMatrix)
{
    GLint projectionMatrixLocation = GL_CHECK(glGetUniformLocation(programID, "projectionMatrix"));

    GL_CHECK(glUseProgram(programID));
    GL_CHECK(glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, projectionMatrix->getAsArray()));
}

void Torus::setupGraphics(const string vertexShaderPath, const string fragmentShaderPath)
{
    GLuint fragmentShaderID = 0;
    GLuint vertexShaderID   = 0;

    Shader::processShader(&vertexShaderID,   vertexShaderPath.c_str(),   GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

    programID = GL_CHECK(glCreateProgram());

    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));

    GL_CHECK(glLinkProgram(programID));

    float scalingFactor     =  0.7f;
    float cameraTranslation = -2.5f;

    Matrix cameraMatrix = Matrix::createTranslation(0.0f, 0.0f, cameraTranslation);
    Matrix scaleMatrix  = Matrix::createScaling(scalingFactor, scalingFactor, scalingFactor);

    GLint scaleMatrixLocation  = GL_CHECK(glGetUniformLocation(programID, "scaleMatrix"));
    GLint cameraMatrixLocation = GL_CHECK(glGetUniformLocation(programID, "cameraMatrix"));

    GL_CHECK(glUseProgram(programID));

    GL_CHECK(glUniformMatrix4fv(scaleMatrixLocation,  1, GL_FALSE, scaleMatrix.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(cameraMatrixLocation, 1, GL_FALSE, cameraMatrix.getAsArray()));
}

void Torus::setResourceDirectory(string requiredResourceDirectory)
{
    resourceDirectory = requiredResourceDirectory;
}
