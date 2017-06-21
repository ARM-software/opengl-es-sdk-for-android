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

#include <stdio.h>
#include <stdlib.h>

// If the sample does not run appropriately
// do run
//     adb logcat jni/main.cpp:* jni/loader.cpp:* *:S
// If something bad happened in main, or if any shaders
// or textures failed to load, you will be notified.

#include <GLES3/gl3.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#define GL_PATCHES GL_PATCHES_EXT
#define GL_PATCH_VERTICES GL_PATCH_VERTICES_EXT
#define GL_TESS_CONTROL_SHADER GL_TESS_CONTROL_SHADER_EXT
#define GL_TESS_EVALUATION_SHADER GL_TESS_EVALUATION_SHADER_EXT
#define glPatchParameteri glPatchParameteriEXT
#include "tessellation.cpp"

#include <jni.h>
#include <android/log.h>
#include <sys/time.h>

#include "EGLRuntime.h"
#include "Platform.h"
using namespace MaliSDK;

#define BASE_ASSET_PATH         "/data/data/com.arm.malideveloper.openglessdk.tessellation/files/"
#define HEIGHTMAP_PATH(name)    BASE_ASSET_PATH name "_heightmap.png"
#define DIFFUSEMAP_PATH(name)   BASE_ASSET_PATH name "_diffusemap.png"
#define SHADER_PATH(name)       BASE_ASSET_PATH name
#include "loader.cpp"

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
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_tessellation_NativeLibrary_init
    (JNIEnv *env, jclass jcls)
    {
        start_time.tv_sec = 0;
        start_time.tv_usec = 0;
        gettimeofday(&start_time, NULL);

        LOGD("Loading assets");
        load_assets(&app);
        app_initialize(&app);
        LOGD("App successfully initialized");
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_tessellation_NativeLibrary_resize
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        app.window_width = width;
        app.window_height = height;
        glViewport(0, 0, width, height);
        LOGD("Resizing %d %d\n", width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_tessellation_NativeLibrary_step
    (JNIEnv *env, jclass jcls)
    {
        timeval now;
        gettimeofday(&now, NULL);
        float seconds  = (now.tv_sec - start_time.tv_sec);
        float milliseconds = (float(now.tv_usec - start_time.tv_usec)) / 1000000.0f;
        app.elapsed_time = seconds + milliseconds;

        glClearColor(1.0f, 0.3f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        app_update_and_render(&app);
        gl_check_error();
    }
}
