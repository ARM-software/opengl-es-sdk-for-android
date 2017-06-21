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

#include "common.hpp"
#include "scene.hpp"
#include "common.hpp"
#include <stdlib.h>

#define GLES_VERSION 3
#include "Timer.h"
#include "Text.h"

using namespace std;

int surface_width, surface_height;

static void render_text(Text &text, const char *method, float current_time)
{
    // Enable alpha blending.
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    char method_string[128];
    sprintf(method_string, "Method: %s (%4.1f / 10.0 s)", method, current_time);

    text.clear();
    text.addString(300, surface_height - 20, method_string, 255, 255, 255, 255);

    text.addString(20, surface_height - 40,  "             Legend:", 255, 255, 255, 255);
    text.addString(20, surface_height - 60,  "Green tinted sphere: LOD 0", 255, 255, 0, 255);
    text.addString(20, surface_height - 80,  " Blue tinted sphere: LOD 1 - LOD 3", 255, 255, 0, 255);
    text.addString(20, surface_height - 100, "        Dark sphere: Occluded spheres", 255, 255, 0, 255);

    text.draw();
    GL_CHECK(glDisable(GL_BLEND));
}

Scene *scene = NULL;
Text *text = NULL;

Timer timer;
unsigned phase = 0;
float culling_timer = 0.0f;

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionculling_OcclusionCulling_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
      common_set_basedir("/data/data/com.arm.malideveloper.openglessdk.occlusionculling/files/");
    
      delete scene;
      scene = new Scene;
      scene->set_show_redundant(true);
      scene->set_culling_method(Scene::CullHiZ);

      delete text;
      text = new Text("/data/data/com.arm.malideveloper.openglessdk.occlusionculling/files/", width, height);

      timer.reset();
      surface_width = width;
      surface_height = height;
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionculling_OcclusionCulling_step
    (JNIEnv *env, jclass jcls)
    {
        float delta_time = timer.getInterval();

        // Render scene.
        scene->move_camera(delta_time * 0.1f, 0.0f);
        scene->update(delta_time, surface_width, surface_height);
        scene->render(surface_width, surface_height);

        static const char *methods[] = {
            "Hierarchical-Z occlusion culling with level-of-detail",
            "Hierarchical-Z occlusion culling without level-of-detail",
            "No culling"
        };
        render_text(*text, methods[phase], culling_timer);

        // Don't need depth nor stencil buffers anymore. Just discard them so they are not written out to memory on Mali.
        static const GLenum attachments[] = { GL_DEPTH, GL_STENCIL };
        GL_CHECK(glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, attachments));

        // Change the culling method over time.
        culling_timer += delta_time;
        if (culling_timer > 10.0f)
        {
            culling_timer = 0.0f;
            phase = (phase + 1) % 3;

            switch (phase)
            {
                case 0:
                    scene->set_culling_method(Scene::CullHiZ);
                    break;
                case 1:
                    scene->set_culling_method(Scene::CullHiZNoLOD);
                    break;
                case 2:
                    scene->set_culling_method(Scene::CullNone);
                    break;
            }
        }


    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionculling_OcclusionCulling_uninit
    (JNIEnv *, jclass)
    {
      delete scene;
      scene = NULL;
      delete text;
      text = NULL;
    }
}


