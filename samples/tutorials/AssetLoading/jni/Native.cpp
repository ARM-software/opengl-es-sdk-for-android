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

/* [New includes and global variables.] */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <vector>

/* The global Assimp scene object. */
const struct aiScene* scene = NULL;

std::vector<GLfloat> vertices;
std::vector<GLushort> indices;
/* [New includes and global variables.] */

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

GLuint glProgram;
GLuint vertexLocation;
GLuint vertexColourLocation;
GLuint projectionLocation;
GLuint modelViewLocation;

float projectionMatrix[16];
float modelViewMatrix[16];
float angle = 0;

bool setupGraphics(int width, int height)
{
    glProgram = createProgram(glVertexShader, glFragmentShader);

    if (glProgram == 0)
    {
        LOGE ("Could not create program");
        return false;
    }

    vertexLocation = glGetAttribLocation(glProgram, "vertexPosition");
    vertexColourLocation = glGetAttribLocation(glProgram, "vertexColour");
    projectionLocation = glGetUniformLocation(glProgram, "projection");
    modelViewLocation = glGetUniformLocation(glProgram, "modelView");

    /* Setup the perspective */
    matrixPerspective(projectionMatrix, 45, (float)width / (float)height, 0.1f, 100);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    /* [Load a model into the Open Asset Importer.] */
    std::string sphere = "s 0 0 0 10";
    scene = aiImportFileFromMemory(sphere.c_str(), sphere.length(), 0, ".nff");

    if(!scene)
    {
        LOGE("Open Asset Importer could not load scene. \n");
        return false;
    }
    /* [Load a model into the Open Asset Importer.] */

    /* [Accumulate the model vertices and indices.] */
    int vertices_accumulation = 0;
    /* Go through each mesh in the scene. */
    for (int i = 0; i < scene->mNumMeshes; i++)
    {
        /* Add all the vertices in the mesh to our array. */
        for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
        {
            const aiVector3D& vector = scene->mMeshes[i]->mVertices[j];
            vertices.push_back(vector.x);
            vertices.push_back(vector.y);
            vertices.push_back(vector.z);
        }

        /*
         * Add all the indices in the mesh to our array.
         * Indices are listed in the Open Asset importer relative to the mesh they are in.
         * Because we are adding all vertices from all meshes to one array we must add an offset
         * to the indices to correct for this.
         */
        for (unsigned int j = 0 ; j < scene->mMeshes[i]->mNumFaces ; j++)
        {
            const aiFace& face = scene->mMeshes[i]->mFaces[j];
            indices.push_back(face.mIndices[0] + vertices_accumulation);
            indices.push_back(face.mIndices[1] + vertices_accumulation);
            indices.push_back(face.mIndices[2] + vertices_accumulation);
        }

        /* Keep track of number of vertices loaded so far to use as an offset for the indices. */
        vertices_accumulation += scene->mMeshes[i]->mNumVertices;
    }
    /* [Accumulate the model vertices and indices.] */

    return true;
}

void renderFrame()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    matrixIdentityFunction(modelViewMatrix);

    matrixRotateX(modelViewMatrix, angle);
    matrixRotateY(modelViewMatrix, angle);

    matrixTranslate(modelViewMatrix, 0.0f, 0.0f, -10.0f);

    /* [Pass the the model vertices and indices to OpenGL ES.] */
    glUseProgram(glProgram);
    /* Use the vertex data loaded from the Open Asset Importer. */
    glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, 0, &vertices[0]);
    glEnableVertexAttribArray(vertexLocation);
    /* We're using vertices as the colour data here for simplicity. */
    glVertexAttribPointer(vertexColourLocation, 3, GL_FLOAT, GL_FALSE, 0, &vertices[0]);
    glEnableVertexAttribArray(vertexColourLocation);

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, projectionMatrix);
    glUniformMatrix4fv(modelViewLocation, 1, GL_FALSE, modelViewMatrix);

    /* Use the index data loaded from the Open Asset Importer. */
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, &indices[0]);
    /* [Pass the the model vertices and indices to OpenGL ES.] */

    angle += 1;
    if (angle > 360)
    {
        angle -= 360;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_assetloading_NativeLibrary_init(
            JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_assetloading_NativeLibrary_step(
            JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_assetloading_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_assetloading_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
