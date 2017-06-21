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

/**
 * \file Cube.cpp
 * \brief A simple rotating cube.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

#include <jni.h>
#include <android/log.h>

#include "Cube.h"
#include "AndroidPlatform.h"
#include "Text.h"
#include "Shader.h"
#include "Texture.h"
#include "Matrix.h"
#include "Timer.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.cube/";
string vertexShaderFilename = "Cube_cube.vert";
string fragmentShaderFilename = "Cube_cube.frag";

/* Shader variables. */
GLuint programID;
GLint iLocPosition;
GLint iLocColor; 
GLint iLocMVP;

int windowWidth = -1;
int windowHeight = -1;

/* A text object to draw text on the screen. */
Text *text;

bool setupGraphics(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    
    /* Full paths to the shader and texture files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do: src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), windowWidth, windowHeight);
    text->addString(0, 0, "Simple Cube Example", 255, 255, 0, 255);

    /* Process shaders. */
    GLuint fragmentShaderID, vertexShaderID;
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

    /* Create programID (ready to attach shaders) */
    programID = GL_CHECK(glCreateProgram());

    /* Attach shaders and link programID */
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));

    /* Get attribute locations of non-fixed attributes like colour and texture coordinates. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "av4position"));
    iLocColor = GL_CHECK(glGetAttribLocation(programID, "av3colour"));

    LOGD("iLocPosition = %i\n", iLocPosition);
    LOGD("iLocColor   = %i\n", iLocColor);

    /* Get uniform locations */
    iLocMVP = GL_CHECK(glGetUniformLocation(programID, "mvp"));

    LOGD("iLocMVP      = %i\n", iLocMVP);

    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glEnable(GL_DEPTH_TEST));

    /* Set clear screen color. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

    return true;
}

void renderFrame(void)
{
    GL_CHECK(glUseProgram(programID));

    /* Enable attributes for position, color and texture coordinates etc. */
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
    GL_CHECK(glEnableVertexAttribArray(iLocColor));

    /* Populate attributes for position, color and texture coordinates etc. */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices));
    GL_CHECK(glVertexAttribPointer(iLocColor, 3, GL_FLOAT, GL_FALSE, 0, colors));

    static float angleX = 0, angleY = 0, angleZ = 0;
    /*
     * Do some rotation with Euler angles. It is not a fixed axis as
     * quaternions would be, but the effect is nice.
     */
    Matrix modelView = Matrix::createRotationX(angleX);
    Matrix rotation = Matrix::createRotationY(angleY);
    
    modelView = rotation * modelView;
    
    rotation = Matrix::createRotationZ(angleZ);
    
    modelView = rotation * modelView;
    
    /* Pull the camera back from the cube */
    modelView[14] -= 2.5;
    
    Matrix perspective = Matrix::matrixPerspective(45.0f, windowWidth/(float)windowHeight, 0.01f, 100.0f);
    Matrix modelViewPerspective = perspective * modelView;

    GL_CHECK(glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, modelViewPerspective.getAsArray()));

    /* Update cube's rotation angles for animating. */
    angleX += 3;
    angleY += 2;
    angleZ += 1;

    if(angleX >= 360) angleX -= 360;
    if(angleX < 0) angleX += 360;
    if(angleY >= 360) angleY -= 360;
    if(angleY < 0) angleY += 360;
    if(angleZ >= 360) angleZ -= 360;
    if(angleZ < 0) angleZ += 360;

    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 36));

    /* Draw any text. */
    text->draw();
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_cube_Cube_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_cube_Cube_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_cube_Cube_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
