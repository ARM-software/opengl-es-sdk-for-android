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

#ifndef COMMON_H
#define COMMON_H

#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <GLES3/gl3.h>

    #ifndef NUMBER_OF_CUBE_FACES
        /** \brief Number of faces which make up a cubic shape. */
        #define NUMBER_OF_CUBE_FACES (6)
    #endif /* NUMBER_OF_CUBE_FACES */

    #ifndef NUMBER_OF_POINT_COORDINATES
        /** \brief Number of coordinates for a point in 3D space. */
        #define NUMBER_OF_POINT_COORDINATES (3)
    #endif /* NUMBER_OF_POINT_COORDINATES */

    #ifndef NUMBER_OF_TRIANGLE_VERTICES
        /** \brief Number of vertices which make up a triangle shape. */
        #define NUMBER_OF_TRIANGLE_VERTICES (3)
    #endif /* NUMBER_OF_TRIANGLE_VERTICES */

    #ifndef NUMBER_OF_TRIANGLES_IN_QUAD
       /** \brief Number of triangles which make up a quad. */
        #define NUMBER_OF_TRIANGLES_IN_QUAD (2)
    #endif /* NUMBER_OF_TRIANGLES_IN_QUAD */

    #define LOG_TAG "libNative"
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBBUG, LOG_TAG, __VA_ARGS__)
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,  LOG_TAG, __VA_ARGS__)
    #define LOGI(...) __android_log_print(ANDROID_LOG_INFO,   LOG_TAG, __VA_ARGS__)
    #define ASSERT(x, s)                                                    \
        if (!(x))                                                           \
        {                                                                   \
            LOGE("Assertion failed at %s:%i\n%s\n", __FILE__, __LINE__, s); \
            exit(1);                                                        \
        }

    #define GL_CHECK(x)                                                                              \
        x;                                                                                           \
        {                                                                                            \
            GLenum glError = glGetError();                                                           \
            if(glError != GL_NO_ERROR) {                                                             \
                LOGE("glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
                exit(1);                                                                             \
            }                                                                                        \
        }
#endif /* COMMON_H */
