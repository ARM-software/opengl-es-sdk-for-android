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

#ifndef SKYBOX_H
#define SKYBOX_H

#include <android/log.h>
#include <GLES3/gl3.h>

#define LOG_TAG "libNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)

#define MALLOC_CHECK(ptr_type, ptr, size)                                        \
{                                                                                \
    ptr = (ptr_type) malloc(size);                                               \
    if (ptr == NULL)                                                             \
    {                                                                            \
        LOGF("Memory allocation error FILE: %s LINE: %i\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE);                                                      \
    }                                                                            \
}

#define REALLOC_CHECK(ptr_type, ptr, size)                                       \
{                                                                                \
    ptr = (ptr_type) realloc(ptr, size);                                         \
    if (ptr == NULL)                                                             \
    {                                                                            \
        LOGF("Memory allocation error FILE: %s LINE: %i\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE);                                                      \
    }                                                                            \
}

#define FREE_CHECK(ptr) \
{                       \
    free((void*) ptr);  \
    ptr = NULL;         \
}

#define GL_CHECK(x)                                                                             \
    x;                                                                                          \
    {                                                                                           \
        GLenum glError = glGetError();                                                          \
        if(glError != GL_NO_ERROR)                                                              \
        {                                                                                       \
            LOGE("glGetError() = %i (%#.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    }

/* Vertex shader source code. */
const char skybox_vertex_shader_source[] =
{
    "#version 300 es\n"
    "out     vec3 texCoord;\n"
    "uniform mat4 viewMat;\n"
    "void main(void) {\n"
    "     const vec3 vertices[4] = vec3[4](vec3(-1.0f, -1.0f, 1.0f),\n"
    "                                      vec3( 1.0f, -1.0f, 1.0f),\n"
    "                                      vec3(-1.0f,  1.0f, 1.0f),\n"
    "                                      vec3( 1.0f,  1.0f, 1.0f));\n"
    "    texCoord = mat3(viewMat) * vertices[gl_VertexID];\n"
    "    gl_Position = vec4(vertices[gl_VertexID], 1.0f);\n"
    "}\n"
};

/* Fragment shader source code. */
const char skybox_fragment_shader_source[] =
{
    "#version 300 es\n"
    "precision mediump float;\n"
    "in      vec3        texCoord;\n"
    "out     vec4        color;\n"
    "uniform samplerCube texCubemap; \n"
    "void main(void) {\n"
    "    color = texture(texCubemap, texCoord);\n"
    "}\n"
};

/**
 * \brief Create shader object and compile its source code.
 *
 * \param[in] shader_type   Vertex or fragment shader.
 * \param[in] shader_source Shader source code.
 * \return    Shader object ID.
 */
GLuint load_shader(GLenum shader_type, const char* shader_source);

/**
 * \brief Create program object, attach vertex and fragment shader to it.
 *        Link program object and check whether it has succeeded.
 *
 * \param[in] vertex_source   Vertex or fragment shader.
 * \param[in] fragment_source Shader source code.
 * \return    Program object ID.
 */
GLuint create_program(const char* vertex_source, const char* fragment_source);

#endif /* SKYBOX_H */
