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

/**
 * \file samples/tutorials/InstancedTessellation/jni/Native.cpp
 *
 * \brief The application displays a rotating solid torus with a low-polygon wireframed mesh surrounding it. The torus is drawn
 *        by means of instanced tessellation technique using OpenGL ES 3.0.
 *
 *        To perform instanced tessellation, we need to divide our model into several patches. Each patch is densely packed with
 *        triangles and *improves the effect of round surfaces. In the first stage of tessellation, patches consist of vertices
 *        placed in a form of a square. Once passed to the shader, they are transformed into Bezier surfaces on the basis of control
 *        points stored in uniform blocks. Each instance of a draw call renders next part of the torus.
 *
 *        The following application instantiates 2 classes, these manage both the solid torus model and the wireframe that surrounds it.
 *        The first class is responsible for configuration of a program with shaders capable of instanced drawing, initialization
 *        of data buffers and handling instanced draw calls. To simplify the mathematics and satisfy conditions for C1 continuity
 *        between patches, we assume that torus is constructed by 12 circles, each also defined by 12 points. In that manner, we
 *        are able to divide "big" and "small" circle of torus into four quadrants and build Bezier surfaces that approximate
 *        perfectly round shapes. For that purpose, the control points cannot lay on the surface of the torus, but have to be
 *        distorted as appropriate.
 *
 *        The second class manages components corresponding to the wireframe. It uses vertices placed on the surface of torus and
 *        uses a simple draw call with GL_LINES mode. The size of its "small circle" is slightly bigger than the corresponding dimension
 *        of the solid torus, so there is a space between both the models.
 *
 *        Common elements for both classes are placed in an abstract Torus class.
 */

#include <jni.h>
#include <android/log.h>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "Common.h"
#include "InstancedSolidTorus.h"
#include "Matrix.h"
#include "Shader.h"
#include "Torus.h"
#include "WireframeTorus.h"

using namespace std;
using namespace MaliSDK;

/* Asset directories and filenames */
const string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.instancedTessellation/files/";

/* Window properties. */
int windowWidth  = 0;
int windowHeight = 0;

/* Object managing OpenGL components which draw torus in wireframe. */
Torus* wireframeTorus;
/* Object managing OpenGL components which draw instanced solid torus. */
Torus* solidTorus;

/* Rotation angles. */
static float angleX = 0.0f;
static float angleY = 0.0f;
static float angleZ = 0.0f;

/**
 * \brief Function that sets up shaders, programs, uniforms locations, generates buffer objects and query objects.
 *
 * \param width  Window width.
 * \param height Window height.
 */
void setupGraphics(int width, int height)
{
    /* Store window resolution. */
    windowHeight = height;
    windowWidth  = width;

    Torus::setResourceDirectory(resourceDirectory);

    /* Distance between center of torus and center of its construction circle. */
    const float torusRadius = 1.0f;
    /* Radius of the construction circle. */
    const float circleRadius = 0.4f;
    /* Distance between solid torus and mesh. */
    const float distance = 0.05f;

    /* Construct torus objects. */
    /* [Generate WireframeTorus object] */
    wireframeTorus = new WireframeTorus(torusRadius, circleRadius + distance);
    /* [Generate WireframeTorus object] */
    /* [Generate InstancedSolidTorus object] */
    solidTorus     = new InstancedSolidTorus(torusRadius, circleRadius);
    /* [Generate InstancedSolidTorus object] */

    ASSERT(wireframeTorus != NULL, "Could not instantiate WireframeTorus class.");
    ASSERT(solidTorus     != NULL, "Could not instantiate InstancedSolidTorus class.");

    /* Configure projection matrix. */
    Matrix projectionMatrix = Matrix::matrixPerspective(45.0f, float(windowWidth) / float(windowHeight), 0.1f, 100.0f);

    /* Set projection matrices for each of the torus objects. */
    wireframeTorus->setProjectionMatrix(&projectionMatrix);
    solidTorus->setProjectionMatrix(&projectionMatrix);

    /* Initialize OpenGL-ES. */
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDepthFunc(GL_LEQUAL));
}

/**
 * \brief Render one frame.
 */
void renderFrame()
{
    /* [Update rotations angle] */
    /* Increment rotation angles. */
    angleX += 0.5;
    angleY += 0.5;
    angleZ += 0.5;

    if(angleX >= 360.0f) angleX = 0.0f;
    if(angleY >= 360.0f) angleY = 0.0f;
    if(angleZ >= 360.0f) angleZ = 0.0f;

    float rotationVector[] = {angleX, angleY, angleZ};
    /* [Update rotations angle] */

    /* Clear the screen */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* Draw both toruses. */
    /* [Draw wireframe model] */
    wireframeTorus->draw(rotationVector);
    /* [Draw wireframe model] */
    /* [Draw solid model] */
    solidTorus->draw(rotationVector);
    /* [Draw solid model] */
}

/**
 * \brief Releases all OpenGL objects that were created with glGen*() or glCreate*() functions.
 */
void uninit()
{
    /* Delete torus objects. */
    delete wireframeTorus;
    delete solidTorus;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_init  (JNIEnv*, jobject,
                                                                                                             jint     width,
                                                                                                             jint     height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_step  (JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_uninit(JNIEnv*, jobject);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_init(JNIEnv*, jobject,
                                                                                                       jint     width,
                                                                                                       jint     height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_step(JNIEnv*, jobject)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancedTessellation_NativeLibrary_uninit(JNIEnv*, jobject)
{
    uninit();
}
