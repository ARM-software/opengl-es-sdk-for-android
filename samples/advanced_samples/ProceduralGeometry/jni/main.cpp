/* Copyright (c) 2015-2017, ARM Limited and Contributors
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
#include <stdlib.h>
#include <time.h>

#include <android/log.h>
#define LOG_TAG "ProceduralGeometry"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define GL_GLEXT_PROTOTYPES
#define GL_GEOMETRY_SHADER GL_GEOMETRY_SHADER_EXT
#include <GLES3/gl3.h>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#include "geometry.cpp"

#define BASE_ASSET_PATH    "/data/data/com.arm.malideveloper.openglessdk.proceduralgeometry/files/"
#define TEXTURE_PATH(name) BASE_ASSET_PATH name
#define SHADER_PATH(name)  BASE_ASSET_PATH name
#include "loader.cpp"

#include <sys/time.h>
static timeval start_time;
static App app;

const char *get_gl_error_msg(GLenum code)
{
    switch (code)
    {
    case 0: return "NO_ERROR";
    case 0x0500: return "INVALID_ENUM";
    case 0x0501: return "INVALID_VALUE";
    case 0x0502: return "INVALID_OPERATION";
    case 0x0503: return "STACK_OVERFLOW";
    case 0x0504: return "STACK_UNDERFLOW";
    case 0x0505: return "OUT_OF_MEMORY";
    case 0x0506: return "INVALID_FRAMEBUFFER_OPERATION";
    default: return "UNKNOWN";
    }
}

void gl_check_error()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        LOGD("An OpenGL error occurred %s", get_gl_error_msg(error));
        exit(1);
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_proceduralgeometry_ProceduralGeometry_init(JNIEnv* env, jobject obj)
    {
        LOGD("Init\n");
        start_time.tv_sec = 0;
        start_time.tv_usec = 0;
        gettimeofday(&start_time, NULL);

        LOGD("Load assets\n");
        load_assets(&app);
        app_initialize(&app);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_proceduralgeometry_ProceduralGeometry_resize(JNIEnv* env, jobject obj, jint width, jint height)
    {
        app.window_width = width;
        app.window_height = height;
        app.elapsed_time = 0.0f;
        glViewport(0, 0, width, height);
        LOGD("Resizing %d %d\n", width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_proceduralgeometry_ProceduralGeometry_step(JNIEnv* env, jobject obj)
    {
        timeval now;
        gettimeofday(&now, NULL);
        float seconds  = (now.tv_sec - start_time.tv_sec);
        float milliseconds = (float(now.tv_usec - start_time.tv_usec)) / 1000000.0f;
        float elapsed_time = seconds + milliseconds;
        app.frame_time = elapsed_time - app.elapsed_time;
        app.elapsed_time = elapsed_time;

        app_update_and_render(&app);
        gl_check_error();

        LOGD("%.2f ms\n", app.frame_time * 1000.0f);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_proceduralgeometry_ProceduralGeometry_onpointerdown(JNIEnv* env, jobject obj, jfloat x, jfloat y)
    {
        app.pointer_x = x;
        app.pointer_y = y;
        app.pointer_down = true;
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_proceduralgeometry_ProceduralGeometry_onpointerup(JNIEnv* env, jobject obj, jfloat x, jfloat y)
    {
        app.pointer_down = false;
        if (y < 0.2f * app.window_height)
            app.voxel_mode = 1.0f - app.voxel_mode;
    }
};
