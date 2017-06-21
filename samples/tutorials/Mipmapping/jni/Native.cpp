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
#include "Texture.h"

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const char glVertexShader[] =
        "attribute vec4 vertexPosition;\n"
        "attribute vec2 vertexTextureCord;\n"
        "varying vec2 textureCord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 modelView;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * modelView * vertexPosition;\n"
        "    textureCord = vertexTextureCord;\n"
        "}\n";

static const char glFragmentShader[] =
        "precision mediump float;\n"
        "uniform sampler2D texture;\n"
        "varying vec2 textureCord;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, textureCord);\n"
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
GLuint vertexLocation;
GLuint samplerLocation;
GLuint projectionLocation;
GLuint modelViewLocation;
GLuint textureCordLocation;
GLuint textureIds[2];

float projectionMatrix[16];
float modelViewMatrix[16];
/* [newGlobals] */
float distance = 1;
float velocity = 0.1;
GLuint textureModeToggle = 0;
/* [newGlobals] */

bool setupGraphics(int width, int height)
{
    glProgram = createProgram(glVertexShader, glFragmentShader);

    if (!glProgram)
    {
        LOGE ("Could not create program");
        return false;
    }

    vertexLocation = glGetAttribLocation(glProgram, "vertexPosition");
    textureCordLocation = glGetAttribLocation(glProgram, "vertexTextureCord");
    projectionLocation = glGetUniformLocation(glProgram, "projection");
    modelViewLocation = glGetUniformLocation(glProgram, "modelView");
    samplerLocation = glGetUniformLocation(glProgram, "texture");

    /* Setup the perspective. */
    /* [matrixPerspective] */
    matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 170);
    /* [matrixPerspective] */
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    /* Code has been pulled out of the texture function as each of load texture calls for both compressed
     * and uncompressed textures need to use the same textureId
     */
    /* [mipmapRegularTextures] */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* Generate a texture object. */
    glGenTextures(2, textureIds);
    /* Activate a texture. */
    glActiveTexture(GL_TEXTURE0);
    /* Bind the texture object. */
    glBindTexture(GL_TEXTURE_2D, textureIds[0]);


    /* Load the Texture. */
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level0.raw", 0, 512, 512);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level1.raw", 1, 256, 256);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level2.raw", 2, 128, 128);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level3.raw", 3, 64, 64);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level4.raw", 4, 32, 32);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level5.raw", 5, 16, 16);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level6.raw", 6, 8, 8);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level7.raw", 7, 4, 4);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level8.raw", 8, 2, 2);
    loadTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level9.raw", 9, 1, 1);
    /* [mipmapRegularTextures] */

    /* [mipmapCompressedTextures] */
    /* Activate a texture. */
    glActiveTexture(GL_TEXTURE1);

    /* Bind the texture object. */
    glBindTexture(GL_TEXTURE_2D, textureIds[1]);

    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level0.pkm", 0);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level1.pkm", 1);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level2.pkm", 2);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level3.pkm", 3);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level4.pkm", 4);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level5.pkm", 5);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level6.pkm", 6);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level7.pkm", 7);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level8.pkm", 8);
    loadCompressedTexture("/data/data/com.arm.malideveloper.openglessdk.mipmapping/files/level9.pkm", 9);
    /* [mipmapCompressedTextures] */
    return true;
}


/* [vertexIndiceCode] */
GLfloat squareVertices[] = { -1.0f,  1.0f,  1.0f,
                           1.0f,  1.0f,  1.0f,
                          -1.0f, -1.0f,  1.0f,
                           1.0f, -1.0f,  1.0f,
                         };

GLfloat textureCords[] = { 0.0f, 1.0f,
                           1.0f, 1.0f,
                           0.0f, 0.0f,
                           1.0f, 0.0f,
};

GLushort indicies[] = {0, 2, 3, 0, 3, 1};
/* [vertexIndiceCode] */

void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear (GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    matrixIdentityFunction(modelViewMatrix);

    /* [matrixTranslate] */
    matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -distance);
    /* [matrixTranslate] */

    glUseProgram(glProgram);
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, squareVertices);
    glEnableVertexAttribArray(vertexLocation);

    glVertexAttribPointer(textureCordLocation, 2, GL_FLOAT, GL_FALSE, 0, textureCords);
    glEnableVertexAttribArray(textureCordLocation);
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE,projectionMatrix);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);

    /* [rangeOfMovement] */
    glUniform1i(samplerLocation, textureModeToggle);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indicies);

    distance += velocity;
    if (distance > 160 || distance < 1)
    {
        velocity *= -1;
        textureModeToggle = !textureModeToggle;
    }
    /* [rangeOfMovement] */
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_mipmapping_NativeLibrary_init (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_mipmapping_NativeLibrary_step(
            JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_mipmapping_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_mipmapping_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
