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

#include "ClipmapApplication.h"
#include "EGLRuntime.h"
#include "Platform.h"

#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include <android/log.h>

using namespace MaliSDK;


// Implements a geometry clipmap algorithm.
// Paper: http://research.microsoft.com/en-us/um/people/hoppe/geomclipmap.pdf

// Sets the size of clipmap blocks, NxN vertices per block. Should be power-of-two and no bigger than 64.
// A clipmap-level is organized roughly as 4x4 blocks with some padding. A clipmap level is a (4N-1) * (4N-1) grid.
#define CLIPMAP_SIZE 64

// Number of LOD levels for clipmap.
#define CLIPMAP_LEVELS 10

// Distance between vertices.
#define CLIPMAP_SCALE 0.25f

ClipmapApplication* app = NULL;
int surface_width, surface_height;

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_terrain_Terrain_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
      delete app;
      app = new ClipmapApplication(CLIPMAP_SIZE, CLIPMAP_LEVELS, CLIPMAP_SCALE);
      surface_width = width;
      surface_height = height;
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_terrain_Terrain_step
    (JNIEnv *env, jclass jcls)
    {
      app->render(surface_width, surface_height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_terrain_Terrain_uninit
    (JNIEnv *, jclass)
    {
      delete app;
      app = NULL;
    }
}
