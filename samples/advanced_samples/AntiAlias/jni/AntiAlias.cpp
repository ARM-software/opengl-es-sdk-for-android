/* Copyright (c) 2013-2017, ARM Limited and Contributors
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
 * \file AntiAlias.cpp
 * \brief A sample to show how to enable AntiAliasing
 *
 * \warning AntiAliasing is disabled on Windows and Desktop Linux platforms.
 * If your graphics card supports it, enable it in WindowsPlatform.cpp or DesktopLinux.cpp
 * depending on your platform.
 *
 * EGL_SAMPLES is used to specifiy the level of AntiAliasing to be used.
 * On Mali platforms, 4x AntiAliasing incurs almost no performance penalty.
 */
 
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>

#include <jni.h>
#include <android/log.h>
#include <unistd.h> 

#include "AntiAlias.h"
#include "Text.h"
#include "Shader.h"
#include "Matrix.h"
#include "AndroidPlatform.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.antialias/";
string vertexShaderFilename = "AntiAlias_triangle.vert";
string fragmentShaderFilename = "AntiAlias_triangle.frag";

/* Shader variables. */
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocFillColor = -1;
GLint iLocProjection = -1;

/* EGL variables. */
int numberOfSamples = 0;

/* A text object to draw text on the screen. */ 
Text* text;

bool setupGraphics(int width, int height)
{
    /* Full paths to the shader files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;
    
    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), width, height);
    text->addString(0, 0, "Anti-aliased triangle", 255, 255, 0, 255);

    /* Process shaders. */
    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;
    LOGI("setupGraphics(%d, %d)", width, height);
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    LOGI("vertexShaderID = %d", vertexShaderID);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
    LOGI("fragmentShaderID = %d", fragmentShaderID);

    /* Set up shaders. */
    programID = GL_CHECK(glCreateProgram());
    if (!programID)
    {
        LOGE("Could not create program.");
        return false;
    }
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));
    LOGI("Shaders in use...");

    /* Vertex positions. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "a_v4Position"));
    if(iLocPosition == -1)
    {
        LOGE("Attribute not found at %s:%i\n", __FILE__, __LINE__);
        return false;
    }
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Fill colors. */
    iLocFillColor = GL_CHECK(glGetAttribLocation(programID, "a_v4FillColor"));
    if(iLocFillColor == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Projection matrix. */
    iLocProjection = GL_CHECK(glGetUniformLocation(programID, "u_m4Projection"));
    if(iLocProjection == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, Matrix::identityMatrix.getAsArray()));
    }

    /* Set clear screen color. RGBA format, so opaque blue. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0f));

    return true;
}

void renderFrame(void)
{
    /* Clear the screen on the EGL surface. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL_CHECK(glUseProgram(programID));

    /* Load EGL window-specific projection and modelview matrices. */
    if(iLocProjection != -1)
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, Matrix::identityMatrix.getAsArray()));
    }

    /* Set triangle vertex. */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices));
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Set triangle colors. */
    if(iLocFillColor != -1)
    {
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors));
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Draw the triangle. */
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

    /* Draw fonts. */
    text->draw();
}

extern "C"
{

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_antialias_AntiAlias_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_antialias_AntiAlias_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_antialias_AntiAlias_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
