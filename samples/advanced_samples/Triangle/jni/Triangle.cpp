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
#define STB_IMAGE_IMPLEMENTATION
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
#include <stb_image.h>

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.triangle/";

//#define CUBIC
#ifdef CUBIC
string vertexShaderFilename = "cubic.vert";
string fragmentShaderFilename = "cubic.frag";
#else
string vertexShaderFilename = "gsr.vert";
string fragmentShaderFilename = "gsr.frag";
#endif

const GLfloat gTriangleVertices[] = {
        -1.0f, 1.0f,
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
};

const GLfloat gTriangleTexcoords[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
};

/* Shader variables. */
GLuint programID;

GLint gvPositionHandle;
GLint gTexSamplerHandle;
GLint gvTexcoordHandle;
GLuint gTex;

#ifdef CUBIC
int gUid, gVid;
int gTexWidth, gTexHeight;
#endif
/* A text object to draw text on the screen. */ 
//Text *text;

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
    //text = new Text(resourceDirectory.c_str(), width, height);
    //text->addString(0, 0, "Simple triangle", 255, 255, 0, 255);

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

    gvPositionHandle = glGetAttribLocation(programID, "aPos");
    LOGE("glGetAttribLocation(\"aPos\") = %d\n",
            gvPositionHandle);
    gvTexcoordHandle = glGetAttribLocation(programID, "aTexcoord");
    LOGE("glGetAttribLocation(\"aTexcoord\") = %d\n",
            gvTexcoordHandle);
    gTexSamplerHandle = glGetUniformLocation(programID, "u_InputTexture");
    LOGE("glGetUniformLocation(\"gTexSampler\") = %d\n",
            gTexSamplerHandle);
#ifdef CUBIC
    gUid = glGetUniformLocation(programID, "uUnit");
    gVid = glGetUniformLocation(programID, "vUnit");
#endif
    glGenTextures(1, &gTex);
    glBindTexture(GL_TEXTURE_2D, gTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int tex_width, tex_height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    std::string tex_path = resourceDirectory + "font.png";//"onepiece_medium.png";
    unsigned char *data = stbi_load(tex_path.c_str(), &tex_width, &tex_height, &nrChannels, 0);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
        LOGE("load texture %d x %d \n", tex_width, tex_height);
    }
    else
    {
        LOGE("Failed to load texture\n");
    }
#ifdef CUBIC
    gTexWidth = tex_width;
    gTexHeight = tex_height;
#endif
    stbi_image_free(data);

    glViewport(0, 0, width, height);
    return true;
}

void renderFrame(void)
{
    LOGI("begin...");
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    GL_CHECK(glUseProgram(programID));
    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleVertices);
    glEnableVertexAttribArray(gvPositionHandle);
    glVertexAttribPointer(gvTexcoordHandle, 2, GL_FLOAT, GL_FALSE, 0, gTriangleTexcoords);
    glEnableVertexAttribArray(gvTexcoordHandle);
#ifdef CUBIC
    float uUnit = (float)gTexWidth;
    float vUnit = (float)gTexHeight;
    glUniform1f(gUid, uUnit);
    glUniform1f(gVid, vUnit);
#endif
    glUniform1i(gTexSamplerHandle, 0);
    glBindTexture(GL_TEXTURE_2D, gTex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glFinish();
    LOGI("end...");
    /* Draw fonts. */
    //text->draw();
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
        //delete text;
    }
}
