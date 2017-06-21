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
#include <stdlib.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "app.h"
#include "timer.h"
#include "common.h"

float last_tick;

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

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_computeparticles_ComputeParticles_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        ASSERT(load_app(), "Failed to load content");
        init_app(width, height);

        last_tick = 0.0f;
        timer_init();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_computeparticles_ComputeParticles_step
    (JNIEnv *env, jclass jcls)
    {
        double curr_tick = get_elapsed_time();
        double dt = curr_tick - last_tick;
        last_tick = curr_tick;

        update_app(dt);
        render_app(dt);

        GLenum error = glGetError();
        bool were_errors = false;
        while (error != GL_NO_ERROR)
        {
            LOGD("An OPENGL error occured %s", get_gl_error_msg(error));
            were_errors = true;
            error = glGetError();
        }
        if (were_errors) exit(1);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_computeparticles_ComputeParticles_uninit
    (JNIEnv *, jclass)
    {
        free_app();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_computeparticles_ComputeParticles_onpointerup
    (JNIEnv * env, jobject obj, jfloat x, jfloat y)
    {
        on_pointer_up(x, y);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_computeparticles_ComputeParticles_onpointerdown
    (JNIEnv * env, jobject obj, jfloat x, jfloat y)
    {
        on_pointer_down(x, y);
    }
}
