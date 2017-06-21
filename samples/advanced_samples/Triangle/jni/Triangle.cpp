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
 * \file Triangle.cpp
 * \brief A sample which shows how to draw a simple triangle to the screen.
 *
 * Uses a simple shader to fill the the triangle with a gradient color.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
 
#include <jni.h>
#include <android/log.h>

#include <cstdio>
#include <cstdlib>
#include <cmath> 

#include "AndroidPlatform.h"
#include "Triangle.h"
#include "Text.h"
#include "Shader.h"
#include "Timer.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.triangle/";
string vertexShaderFilename = "Triangle_triangle.vert";
string fragmentShaderFilename = "Triangle_triangle.frag";

/* Shader variables. */
GLuint programID;
GLint iLocPosition = -1;
GLint iLocFillColor = -1;

/* A text object to draw text on the screen. */ 
Text *text;

bool setupGraphics(int width, int height)
{
    LOGD("setupGraphics(%d, %d)", width, height);

    /* Full paths to the shader files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;

    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthFunc(GL_LEQUAL));
    
    
    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do: src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), width, height);
    text->addString(0, 0, "Simple triangle", 255, 255, 0, 255);

    /* Process shaders. */
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    LOGD("vertexShaderID = %d", vertexShaderID);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
    LOGD("fragmentShaderID = %d", fragmentShaderID);

    programID = GL_CHECK(glCreateProgram());
    if (programID == 0)
    {
        LOGE("Could not create program.");
        return false;
    }

    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));

    /* Positions. */
    GL_CHECK(iLocPosition = glGetAttribLocation(programID, "a_v4Position"));
    LOGD("glGetAttribLocation(\"a_v4Position\") = %d\n", iLocPosition);

    /* Fill colors. */
    GL_CHECK(iLocFillColor = glGetAttribLocation(programID, "a_v4FillColor"));
    LOGD("glGetAttribLocation(\"a_v4FillColor\") = %d\n", iLocFillColor);

    GL_CHECK(glViewport(0, 0, width, height));

    /* Set clear screen color. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0f));
    GL_CHECK(glClearDepthf(1.0f));
    return true;
}

void renderFrame(void)
{
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL_CHECK(glUseProgram(programID));

    /* Pass the triangle vertex positions to the shader */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, triangleVertices));

    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    if(iLocFillColor != -1)
    {
        /* Pass the vertex colours to the shader */
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, triangleColors));
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));

    /* Draw fonts. */
    text->draw();
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_triangle_Triangle_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_triangle_Triangle_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_triangle_Triangle_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
