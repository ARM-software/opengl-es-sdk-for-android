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

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "Matrix.h"
#include "Texture.h"

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* [vertexShader] */
static const char glVertexShader[] =
        "attribute vec4 vertexPosition;\n"
        "attribute vec2 vertexTextureCord;\n"
        "attribute vec3 vertexNormal;\n"
        "attribute vec3 vertexColor; \n"
        "attribute vec3 vertexTangent;\n"
        "attribute vec3 vertexBiNormal;\n"
        "varying vec2 textureCord;\n"
        "varying vec3 varyingColor; \n"
        "varying vec3 inverseLightDirection;\n"
        "varying vec3 inverseEyeDirection;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 modelView;\n"
        "void main()\n"
        "{\n"
        "   vec3 worldSpaceVertex =(modelView * vertexPosition).xyz;"
        "   vec3 transformedVertexNormal = normalize((modelView *  vec4(vertexNormal, 0.0)).xyz);"

        "   inverseLightDirection = normalize(vec3(0.0, 0.0, 1.0));\n"
        "   inverseEyeDirection = normalize((vec3(0.0, 0.0, 1.0)- worldSpaceVertex ).xyz);\n"

        "   gl_Position = projection * modelView * vertexPosition;\n"
        "   textureCord = vertexTextureCord;\n"
        "   varyingColor = vertexColor;\n"

        "   vec3 transformedTangent = normalize((modelView * vec4(vertexTangent, 0.0)).xyz);\n"
        "   vec3 transformedBinormal = normalize((modelView * vec4(vertexBiNormal, 0.0)).xyz);\n"
        "   mat3 tangentMatrix = mat3(transformedTangent, transformedBinormal, transformedVertexNormal);\n"
        "   inverseLightDirection =inverseLightDirection * tangentMatrix;\n"
        "   inverseEyeDirection = inverseEyeDirection * tangentMatrix;\n"
        "}\n";
/* [vertexShader] */
/* [fragmentShader] */
static const char glFragmentShader[] =
        "precision mediump float;\n"
        "uniform sampler2D texture;\n"
        "varying vec2 textureCord;\n"
        "varying vec3 varyingColor;\n"
        "varying vec3 inverseLightDirection;\n"
        "varying vec3 inverseEyeDirection;\n"
        "varying vec3 transformedVertexNormal;\n"
        "void main()\n"
        "{\n"
        "   vec3 fragColor = vec3(0.0,0.0,0.0); \n"
        "   vec3 normal = texture2D(texture, textureCord).xyz;"
        "   normal = normalize(normal * 2.0 -1.0);"
        /* Calculate the diffuse component. */
        "   vec3 diffuseLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "   float normalDotLight = max(0.0, dot(normal, inverseLightDirection));\n"
        "   fragColor += normalDotLight * varyingColor *diffuseLightIntensity;\n"
        /* Calculate the ambient component. */
        "   vec3 ambientLightIntensity = vec3(0.1, 0.1, 0.1);\n"
        "   fragColor +=  ambientLightIntensity * varyingColor;\n"
        /* Calculate the specular component. */
        "   vec3 specularLightIntensity = vec3(1.0, 1.0, 1.0);\n"
        "   vec3 vertexSpecularReflectionConstant = vec3(1.0, 1.0, 1.0);\n"
        "   float shininess = 2.0;\n"
        "   vec3 lightReflectionDirection = reflect(vec3(0) - inverseLightDirection, normal);\n"
        "   float normalDotReflection = max(0.0, dot(inverseEyeDirection, lightReflectionDirection));\n"
        "   fragColor += pow(normalDotReflection, shininess) * vertexSpecularReflectionConstant * specularLightIntensity;\n"
        "   /* Make sure the fragment colour is between 0 and 1. */"
        "   clamp(fragColor, 0.0, 1.0);\n"
        "   gl_FragColor = vec4(fragColor,1.0);\n"
        "}\n";
/* [fragmentShader] */

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
        glAttachShader(program , vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program , GL_LINK_STATUS, &linkStatus);

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

GLuint glProgram;

/* [LocationVariables] */
GLuint vertexLocation;
GLuint samplerLocation;
GLuint projectionLocation;
GLuint modelViewLocation;
GLuint textureCordLocation;
GLuint colorLocation;
GLuint textureId;
GLuint vertexNormalLocation;
GLuint tangentLocation;
GLuint biNormalLocation;
/* [LocationVariables] */

float projectionMatrix[16];
float modelViewMatrix[16];
float angle = 0;

bool setupGraphics(int width, int height)
{
    glProgram = createProgram(glVertexShader, glFragmentShader);

    if (!glProgram)
    {
        LOGE ("Could not create program");
        return false;
    }

    /* [setLocation] */
    vertexLocation = glGetAttribLocation(glProgram, "vertexPosition");
    textureCordLocation = glGetAttribLocation(glProgram, "vertexTextureCord");
    projectionLocation = glGetUniformLocation(glProgram, "projection");
    modelViewLocation = glGetUniformLocation(glProgram, "modelView");
    samplerLocation = glGetUniformLocation(glProgram, "texture");
    vertexNormalLocation = glGetAttribLocation(glProgram, "vertexNormal");
    colorLocation = glGetAttribLocation(glProgram, "vertexColor");
    tangentLocation = glGetAttribLocation(glProgram, "vertexTangent");
    biNormalLocation = glGetAttribLocation(glProgram, "vertexBiNormal");
    /* [setLocation] */

    /* Setup the perspective. */
    matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 100);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    /* Load the Texture. */
    textureId = loadTexture();
    if(textureId == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
/* [vertexColourTangentNormal] */
GLfloat cubeVertices[] = {-1.0f,  1.0f, -1.0f, /* Back. */
                           1.0f,  1.0f, -1.0f,
                          -1.0f, -1.0f, -1.0f,
                           1.0f, -1.0f, -1.0f,
                          -1.0f,  1.0f,  1.0f, /* Front. */
                           1.0f,  1.0f,  1.0f,
                          -1.0f, -1.0f,  1.0f,
                           1.0f, -1.0f,  1.0f,
                          -1.0f,  1.0f, -1.0f, /* Left. */
                          -1.0f, -1.0f, -1.0f,
                          -1.0f, -1.0f,  1.0f,
                          -1.0f,  1.0f,  1.0f,
                           1.0f,  1.0f, -1.0f, /* Right. */
                           1.0f, -1.0f, -1.0f,
                           1.0f, -1.0f,  1.0f,
                           1.0f,  1.0f,  1.0f,
                          -1.0f, 1.0f, -1.0f, /* Top. */
                          -1.0f, 1.0f,  1.0f,
                           1.0f, 1.0f,  1.0f,
                           1.0f, 1.0f, -1.0f,
                          -1.0f, - 1.0f, -1.0f, /* Bottom. */
                          -1.0f,  -1.0f,  1.0f,
                           1.0f, - 1.0f,  1.0f,
                           1.0f,  -1.0f, -1.0f
                         };

GLfloat normals[] =     {0.0f, 0.0f, -1.0f,            /* Back */
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, 1.0f,            /* Front */
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        -1.0f, 0.0, 0.0f,            /* Left */
                        -1.0f, 0.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,             /* Right */
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,             /* Top */
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f,            /* Bottom */
                        0.0f, -1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f,
                        0.0f, -1.0f, 0.0f
};

GLfloat colour[] =      {1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        1.0f, 1.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        1.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 1.0f,
                        0.0f, 1.0f, 1.0f,
                        1.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 1.0f,
                        1.0f, 0.0f, 1.0f
};

GLfloat tangents[] =    {-1.0f, 0.0f, 0.0f,            /* Back */
                        -1.0f, 0.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,                /* Front */
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f,                /* Left */
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, -1.0f,                /* Right */
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        1.0f, 0.0f, 0.0f,                /* Top */
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,                /* Bottom */
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f
};

GLfloat biNormals[] = {    0.0f, 1.0f, 0.0f,                /* Back */
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,                /* Front */
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,                /* Left */
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,                 /* Right */
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, -1.0f,                /* Top */
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, -1.0f,
                        0.0f, 0.0f, 1.0f,                /* Bottom */
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f,
                        0.0f, 0.0f, 1.0f

};
GLfloat textureCords[] = {1.0f, 1.0f, /* Back. */
                        0.0f, 1.0f,
                        1.0f, 0.0f,
                        0.0f, 0.0f,
                        0.0f, 1.0f, /* Front. */
                        1.0f, 1.0f,
                        0.0f, 0.0f,
                        1.0f, 0.0f,
                        0.0f, 1.0f, /* Left. */
                        0.0f, 0.0f,
                        1.0f, 0.0f,
                        1.0f, 1.0f,
                        1.0f, 1.0f, /* Right. */
                        1.0f, 0.0f,
                        0.0f, 0.0f,
                        0.0f, 1.0f,
                        0.0f, 1.0f, /* Top. */
                        0.0f, 0.0f,
                        1.0f, 0.0f,
                        1.0f, 1.0f,
                        0.0f, 0.0f, /* Bottom. */
                        0.0f, 1.0f,
                        1.0f, 1.0f,
                        1.0f, 0.0f
};

GLushort indicies[] = {0, 3, 2, 0, 1, 3, 4, 6, 7, 4, 7, 5,  8, 9, 10, 8, 11, 10, 12, 13, 14, 15, 12, 14, 16, 17, 18, 16, 19, 18, 20, 21, 22, 20, 23, 22};
/* [vertexColourTangentNormal] */

void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    matrixIdentityFunction(modelViewMatrix);

    matrixRotateX(modelViewMatrix, angle);
    matrixRotateY(modelViewMatrix, angle);

    matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -10.0f);

    glUseProgram(glProgram);

    /* [supplyData] */
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, cubeVertices);
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(textureCordLocation, 2, GL_FLOAT, GL_FALSE, 0, textureCords);
    glEnableVertexAttribArray(textureCordLocation);
    glVertexAttribPointer(colorLocation, 3, GL_FLOAT, GL_FALSE, 0, colour);
    glEnableVertexAttribArray(colorLocation);
    glVertexAttribPointer(vertexNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, normals);
    glEnableVertexAttribArray(vertexNormalLocation);
    glVertexAttribPointer(biNormalLocation, 3, GL_FLOAT, GL_FALSE, 0, biNormals);
    glEnableVertexAttribArray(biNormalLocation);
    glVertexAttribPointer(tangentLocation, 3, GL_FLOAT, GL_FALSE, 0, tangents);
    glEnableVertexAttribArray(tangentLocation);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE,projectionMatrix);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);
    /* [supplyData] */

    /* Set the sampler texture unit to 0. */
    glUniform1i(samplerLocation, 0);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, indicies);

    angle += 1;
    if (angle > 360)
    {
        angle -= 360;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_normalmapping_NativeLibrary_init (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_normalmapping_NativeLibrary_step(
            JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_normalmapping_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_normalmapping_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
