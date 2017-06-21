/* Copyright (c) 2012-2017, ARM Limited and Contributors
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

/**
 * \file Template.cpp
 * \brief A blank sample to use as a basis for OpenGL ES 2.0 applications
 *
 * This is a functioning OpenGL ES 2.0 application which renders nothing to the screen.
 * Add setup code to setupGraphics(), for example, code to load shaders and textures.
 * To use assets (shaders, textures, etc.), place them in the assets folder of the sample.
 * Add code to actually render the scene in renderFrame().
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>

#include <jni.h>
#include <android/log.h>

#include "Template.h"
#include "Text.h"
#include "AndroidPlatform.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.template/";

/* A text object to draw text on the screen.*/
Text* text;

bool setupGraphics(int width, int height)
{
    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), width, height);

    text->addString(0, 0, "Template", 255, 255, 255, 255);

    /* Code to setup shaders, geometry, and to initialize OpenGL ES states. */

    return true;
}

void renderFrame(void)
{
    /* Code to draw one frame. */

    /* Draw fonts. */
    text->draw();
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_template_Template_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_template_Template_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_template_Template_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
