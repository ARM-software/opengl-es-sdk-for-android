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

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Matrix.h"

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* [vertexShader] */
static const char  glVertexShader[] =
        "attribute vec4 vertexPosition;\n"
        "attribute vec3 vertexColour;\n"
        /* [Add a vertex normal attribute.] */
        "attribute vec3 vertexNormal;\n"
        /* [Add a vertex normal attribute.] */
        "varying vec3 fragColour;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 modelView;\n"
        "void main()\n"
        "{\n"
             /* [Setup scene vectors.] */
        "    vec3 transformedVertexNormal = normalize((modelView * vec4(vertexNormal, 0.0)).xyz);"
        "    vec3 inverseLightDirection = normalize(vec3(0.0, 1.0, 1.0));\n"
        "    fragColour = vec3(0.0);\n"
             /* [Setup scene vectors.] */
        "\n"
             /* [Calculate the diffuse component.] */
        "    vec3 diffuseLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "    vec3 vertexDiffuseReflectionConstant = vertexColour;\n"
        "    float normalDotLight = max(0.0, dot(transformedVertexNormal, inverseLightDirection));\n"
        "    fragColour += normalDotLight * vertexDiffuseReflectionConstant * diffuseLightIntensity;\n"
             /* [Calculate the diffuse component.] */
        "\n"
             /* [Calculate the ambient component.] */
        "    vec3 ambientLightIntensity = vec3(0.1, 0.1, 0.1);\n"
        "    vec3 vertexAmbientReflectionConstant = vertexColour;\n"
        "    fragColour += vertexAmbientReflectionConstant * ambientLightIntensity;\n"
             /* [Calculate the ambient component.] */
        "\n"
             /* [Calculate the specular component.] */
        "    vec3 inverseEyeDirection = normalize(vec3(0.0, 0.0, 1.0));\n"
        "    vec3 specularLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "    vec3 vertexSpecularReflectionConstant = vec3(1.0, 1.0, 1.0);\n"
        "    float shininess = 2.0;\n"
        "    vec3 lightReflectionDirection = reflect(vec3(0) - inverseLightDirection, transformedVertexNormal);\n"
        "    float normalDotReflection = max(0.0, dot(inverseEyeDirection, lightReflectionDirection));\n"
        "    fragColour += pow(normalDotReflection, shininess) * vertexSpecularReflectionConstant * specularLightIntensity;\n"
             /* [Calculate the specular component.] */
        "\n"
        "    /* Make sure the fragment colour is between 0 and 1. */"
        "    clamp(fragColour, 0.0, 1.0);\n"
        "\n"
        "    gl_Position = projection * modelView * vertexPosition;\n"
        "}\n";
/* [vertexShader] */

static const char  glFragmentShader[] =
        "precision mediump float;\n"
        "varying vec3 fragColour;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(fragColour, 1.0);\n"
        "}\n";

GLuint loadShader(GLenum shaderType, const char* shaderSource)
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

GLuint createProgram(const char* vertexSource, const char * fragmentSource)
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

GLuint lightingProgram;
GLuint vertexLocation;
GLuint vertexColourLocation;
/* [Global variable to hold vertex normal attribute location.] */
GLuint vertexNormalLocation;
/* [Global variable to hold vertex normal attribute location.] */
GLuint projectionLocation;
GLuint modelViewLocation;

float projectionMatrix[16];
float modelViewMatrix[16];
float angle = 0;

bool setupGraphics(int width, int height)
{
    lightingProgram = createProgram(glVertexShader, glFragmentShader);

    if (lightingProgram == 0)
    {
        LOGE ("Could not create program");
        return false;
    }

    vertexLocation = glGetAttribLocation(lightingProgram, "vertexPosition");
    vertexColourLocation = glGetAttribLocation(lightingProgram, "vertexColour");
    /* [Get vertex normal attribute location.] */
    vertexNormalLocation = glGetAttribLocation(lightingProgram, "vertexNormal");
    /* [Get vertex normal attribute location.] */
    projectionLocation = glGetUniformLocation(lightingProgram, "projection");
    modelViewLocation = glGetUniformLocation(lightingProgram, "modelView");

    /* Setup the perspective */
    matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 100);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    return true;
}

/* [Geometry] */
GLfloat verticies[] = { 1.0f,  1.0f, -1.0f, /* Back. */
                       -1.0f,  1.0f, -1.0f,
                        1.0f, -1.0f, -1.0f,
                       -1.0f, -1.0f, -1.0f,
                        0.0f,  0.0f, -2.0f,
                       -1.0f,  1.0f,  1.0f, /* Front. */
                        1.0f,  1.0f,  1.0f,
                       -1.0f, -1.0f,  1.0f,
                        1.0f, -1.0f,  1.0f,
                        0.0f,  0.0f,  2.0f,
                       -1.0f,  1.0f, -1.0f, /* Left. */
                       -1.0f,  1.0f,  1.0f,
                       -1.0f, -1.0f, -1.0f,
                       -1.0f, -1.0f,  1.0f,
                       -2.0f,  0.0f,  0.0f,
                        1.0f,  1.0f,  1.0f, /* Right. */
                        1.0f,  1.0f, -1.0f,
                        1.0f, -1.0f,  1.0f,
                        1.0f, -1.0f, -1.0f,
                        2.0f,  0.0f,  0.0f,
                       -1.0f, -1.0f,  1.0f, /* Bottom. */
                        1.0f, -1.0f,  1.0f,
                       -1.0f, -1.0f, -1.0f,
                        1.0f, -1.0f, -1.0f,
                        0.0f, -2.0f,  0.0f,
                       -1.0f,  1.0f, -1.0f, /* Top. */
                        1.0f,  1.0f, -1.0f,
                       -1.0f,  1.0f,  1.0f,
                        1.0f,  1.0f,  1.0f,
                        0.0f,  2.0f,  0.0f
                      };
/* [Geometry] */

/* [Colours] */
GLfloat colour[] = {1.0f, 0.0f, 0.0f, /* Back. */
                    1.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 0.0f,
                    1.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, /* Front. */
                    0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, /* Left. */
                    0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 1.0f,
                    1.0f, 1.0f, 0.0f, /* Right. */
                    1.0f, 1.0f, 0.0f,
                    1.0f, 1.0f, 0.0f,
                    1.0f, 1.0f, 0.0f,
                    1.0f, 1.0f, 0.0f,
                    0.0f, 1.0f, 1.0f, /* Bottom. */
                    0.0f, 1.0f, 1.0f,
                    0.0f, 1.0f, 1.0f,
                    0.0f, 1.0f, 1.0f,
                    0.0f, 1.0f, 1.0f,
                    1.0f, 0.0f, 1.0f, /* Top. */
                    1.0f, 0.0f, 1.0f,
                    1.0f, 0.0f, 1.0f,
                    1.0f, 0.0f, 1.0f,
                    1.0f, 0.0f, 1.0f
                   };
/* [Colours] */

/* [Normals] */
GLfloat normals[] = { 1.0f,  1.0f, -1.0f, /* Back. */
                     -1.0f,  1.0f, -1.0f,
                      1.0f, -1.0f, -1.0f,
                     -1.0f, -1.0f, -1.0f,
                      0.0f,  0.0f, -1.0f,
                     -1.0f,  1.0f,  1.0f, /* Front. */
                      1.0f,  1.0f,  1.0f,
                     -1.0f, -1.0f,  1.0f,
                      1.0f, -1.0f,  1.0f,
                      0.0f,  0.0f,  1.0f,
                     -1.0f,  1.0f, -1.0f, /* Left. */
                     -1.0f,  1.0f,  1.0f,
                     -1.0f, -1.0f, -1.0f,
                     -1.0f, -1.0f,  1.0f,
                     -1.0f,  0.0f,  0.0f,
                      1.0f,  1.0f,  1.0f, /* Right. */
                      1.0f,  1.0f, -1.0f,
                      1.0f, -1.0f,  1.0f,
                      1.0f, -1.0f, -1.0f,
                      1.0f,  0.0f,  0.0f,
                     -1.0f, -1.0f,  1.0f, /* Bottom. */
                      1.0f, -1.0f,  1.0f,
                     -1.0f, -1.0f, -1.0f,
                      1.0f, -1.0f, -1.0f,
                      0.0f, -1.0f,  0.0f,
                     -1.0f,  1.0f, -1.0f, /* Top. */
                      1.0f,  1.0f, -1.0f,
                     -1.0f,  1.0f,  1.0f,
                      1.0f,  1.0f,  1.0f,
                      0.0f,  1.0f,  0.0f
                     };
/* [Normals] */

/* [Indices] */
GLushort indices[] = {0,  2,  4,  0,  4,  1,  1,  4,  3,  2,  3,  4,  /* Back. */
                      5,  7,  9,  5,  9,  6,  6,  9,  8,  7,  8,  9,  /* Front. */
                      10, 12, 14, 10, 14, 11, 11, 14, 13, 12, 13, 14, /* Left. */
                      15, 17, 19, 15, 19, 16, 16, 19, 18, 17, 18, 19, /* Right. */
                      20, 22, 24, 20, 24, 21, 21, 24, 23, 22, 23, 24, /* Bottom. */
                      25, 27, 29, 25, 29, 26, 26, 29, 28, 27, 28, 29  /* Top. */
                     };
/* [Indices] */

void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    matrixIdentityFunction(modelViewMatrix);

    matrixRotateX(modelViewMatrix, angle);
    matrixRotateY(modelViewMatrix, angle);

    matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -10.0f);

    glUseProgram(lightingProgram);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, verticies);
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexColourLocation, 3, GL_FLOAT, GL_FALSE, 0, colour);
    glEnableVertexAttribArray(vertexColourLocation);

    /* [Upload vertex normals.] */
    glVertexAttribPointer(vertexNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, normals);
    glEnableVertexAttribArray(vertexNormalLocation);
    /* [Upload vertex normals.] */

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);

    /* [Draw the object.] */
    glDrawElements(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, indices);
    /* [Draw the object.] */

    angle += 1;
    if (angle > 360)
    {
        angle -= 360;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_lighting_NativeLibrary_init(
            JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_lighting_NativeLibrary_step(
            JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_lighting_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_lighting_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
