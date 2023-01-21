/* Copyright (c) 2014-2017, ARM Limited and Contributors
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

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Matrix.h"

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)



static const char  glVertexShader[] =
        "attribute vec4 vertexPosition;\n"
        "attribute vec3 vertexColour;\n"
        "varying vec3 fragColour;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 modelView;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * modelView * vertexPosition;\n"
        "    fragColour = vertexColour;\n"
        "}\n";

static const char  glFragmentShader[] =
        "precision mediump float;\n"
        "varying vec3 fragColour;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(fragColour, 1.0);\n"
        "}\n";


static GLuint loadShader(GLenum shaderType, const char* shaderSource)
{
    GLuint shader = glCreateShader(shaderType);
    if (shader != 0)
    {
        glShaderSource(shader, 1, &shaderSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE)
        {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

            if (infoLen > 0)
            {
                char * logBuffer = (char*) malloc(infoLen);

                if (logBuffer != NULL)
                {
                    glGetShaderInfoLog(shader, infoLen, NULL, logBuffer);
                    LOGE("Could not Compile Shader %d:\n%s\n", shaderType, logBuffer);
                    free(logBuffer);
                    logBuffer = NULL;
                }

                glDeleteShader(shader);
                shader = 0;
            }
        }
    }

    return shader;
}

static GLuint createProgram(const char* vertexSource, const char * fragmentSource)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0)
    {
        return 0;
    }

    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0)
    {
        return 0;
    }

    GLuint program = glCreateProgram();

    if (program != 0)
    {
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if(linkStatus != GL_TRUE)
        {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength > 0)
            {
                char* logBuffer = (char*) malloc(bufLength);

                if (logBuffer != NULL)
                {
                    glGetProgramInfoLog(program, bufLength, NULL, logBuffer);
                    LOGE("Could not link program:\n%s\n", logBuffer);
                    free(logBuffer);
                    logBuffer = NULL;
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

static GLuint glProgram;
static GLuint vertexLocation;
static GLuint vertexColourLocation;
static GLuint projectionLocation;
static GLuint modelViewLocation;
/* [vboIDDefinition] */
static GLuint vboBufferIds[2];
/* [vboIDDefinition] */

static float projectionMatrix[16];
static float modelViewMatrix[16];
static float angle = 0;

/* [vboVertexData] */
static GLfloat cubeVertices[] = { -1.0f,  1.0f, -1.0f,  /* Back Face First Vertex Position */
                            1.0f, 0.0f, 0.0f,           /* Back Face First Vertex Colour */
                            1.0f,  1.0f, -1.0f,         /* Back Face Second Vertex Position */
                            1.0f, 0.0f, 0.0f,           /* Back Face Second Vertex Colour */
                            -1.0f, -1.0f, -1.0f,        /* Back Face Third Vertex Position */
                            1.0f, 0.0f, 0.0f,           /* Back Face Third Vertex Colour */
                            1.0f, -1.0f, -1.0f,         /* Back Face Fourth Vertex Position */
                            1.0f, 0.0f, 0.0f,           /* Back Face Fourth Vertex Colour */
                            -1.0f,  1.0f,  1.0f,        /* Front. */
                            0.0f, 1.0f, 0.0f,
                            1.0f,  1.0f,  1.0f,
                            0.0f, 1.0f, 0.0f,
                            -1.0f, -1.0f,  1.0f,
                            0.0f, 1.0f, 0.0f,
                            1.0f, -1.0f,  1.0f,
                            0.0f, 1.0f, 0.0f,
                            -1.0f,  1.0f, -1.0f,        /* Left. */
                            0.0f, 0.0f, 1.0f,
                            -1.0f, -1.0f, -1.0f,
                            0.0f, 0.0f, 1.0f,
                            -1.0f, -1.0f,  1.0f,
                            0.0f, 0.0f, 1.0f,
                            -1.0f,  1.0f,  1.0f,
                            0.0f, 0.0f, 1.0f,
                            1.0f,  1.0f, -1.0f,         /* Right. */
                            1.0f, 1.0f, 0.0f,
                            1.0f, -1.0f, -1.0f,
                            1.0f, 1.0f, 0.0f,
                            1.0f, -1.0f,  1.0f,
                            1.0f, 1.0f, 0.0f,
                            1.0f,  1.0f,  1.0f,
                            1.0f, 1.0f, 0.0f,
                            -1.0f, -1.0f, -1.0f,         /* Top. */
                            0.0f, 1.0f, 1.0f,
                            -1.0f, -1.0f,  1.0f,
                            0.0f, 1.0f, 1.0f,
                            1.0f, -1.0f,  1.0f,
                            0.0f, 1.0f, 1.0f,
                            1.0f, -1.0f, -1.0f,
                            0.0f, 1.0f, 1.0f,
                            -1.0f,  1.0f, -1.0f,         /* Bottom. */
                            1.0f, 0.0f, 1.0f,
                            -1.0f,  1.0f,  1.0f,
                            1.0f, 0.0f, 1.0f,
                            1.0f,  1.0f,  1.0f,
                            1.0f, 0.0f, 1.0f,
                            1.0f,  1.0f, -1.0f,
                            1.0f, 0.0f, 1.0f,
};
/* [vboVertexData] */

/* [vboStrideSize] */
static GLushort strideLength = 6 * sizeof(GLfloat);
/* [vboStrideSize] */

/* [vboColourOffset] */
static GLushort vertexColourOffset = 3  * sizeof (GLfloat);
/* [vboColourOffset] */

/* [vboBufferSize] */
static GLushort vertexBufferSize = 48 * 3 * sizeof (GLfloat);
/* [vboBufferSize] */
/* [vboElementSize] */
static GLushort elementBufferSize = 36 * sizeof(GLushort);
/* [vboElementSize] */

static GLushort indices[] = {0, 2, 3, 0, 1, 3, 4, 6, 7, 4, 5, 7, 8, 9, 10, 11, 8, 10, 12, 13, 14, 15, 12, 14, 16, 17, 18, 16, 19, 18, 20, 21, 22, 20, 23, 22};

static bool setupGraphics(int width, int height)
{
    glProgram = createProgram(glVertexShader, glFragmentShader);

    if (glProgram == 0)
    {
        LOGE ("Could not create program");
        return false;
    }
    /* [vboCreation] */
    glGenBuffers(2, vboBufferIds);
    glBindBuffer(GL_ARRAY_BUFFER, vboBufferIds[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboBufferIds[1]);
    /* [vboCreation] */

    /* [vboAllocateSpace] */
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, cubeVertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementBufferSize, indices, GL_STATIC_DRAW);
    /* [vboAllocateSpace] */

    vertexLocation = glGetAttribLocation(glProgram, "vertexPosition");
    vertexColourLocation = glGetAttribLocation(glProgram, "vertexColour");
    projectionLocation = glGetUniformLocation(glProgram, "projection");
    modelViewLocation = glGetUniformLocation(glProgram, "modelView");

    matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 100);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    return true;
}

static void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    matrixIdentityFunction(modelViewMatrix);

    matrixRotateX(modelViewMatrix, angle);
    matrixRotateY(modelViewMatrix, angle);

    matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -10.0f);

    glUseProgram(glProgram);

    /* [vboVertexAttribPointer] */
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, strideLength, 0);
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexColourLocation, 3, GL_FLOAT, GL_FALSE, strideLength, (const void *) vertexColourOffset);
    glEnableVertexAttribArray(vertexColourLocation);
    /* [vboVertexAttribPointer] */

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);

    /* [vboDrawElements] */
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
    /* [vboDrawElements] */

    angle += 1;
    if (angle > 360)
    {
        angle -= 360;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_vbo_NativeLibrary_init(
            JNIEnv * env, jclass obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_vbo_NativeLibrary_step(
            JNIEnv * env, jclass obj);
};

extern "C" JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_vbo_NativeLibrary_init(
        JNIEnv * env, jclass obj, jint width, jint height)
{
    setupGraphics(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_vbo_NativeLibrary_step(
        JNIEnv * env, jclass obj)
{
    renderFrame();
}
