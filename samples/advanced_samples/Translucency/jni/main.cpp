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

#include "app.h"

#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include <android/log.h>
#include "common.h"
#include "timer.h"

double last_tick = 0.0;

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
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_translucency_NativeLibrary_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        ASSERT(init_app(width, height), "Failed to initialize app");
        LOGD("OK!");
        timer_init();
        last_tick = 0.0;
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_translucency_NativeLibrary_step
    (JNIEnv *env, jclass jcls)
    {
        double now = get_elapsed_time();
        double dt = now - last_tick;
        last_tick = now;
        update_app(dt);
        render_app(dt);
        gl_check_error();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_translucency_NativeLibrary_uninit
    (JNIEnv *, jclass)
    {
        free_app();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_translucency_NativeLibrary_onpointerdown
    (JNIEnv * env, jobject obj, jfloat x, jfloat y)
    {
        on_pointer_down(x, y);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_translucency_NativeLibrary_onpointerup
    (JNIEnv * env, jobject obj, jfloat x, jfloat y)
    {
        on_pointer_up(x, y);
    }
}
