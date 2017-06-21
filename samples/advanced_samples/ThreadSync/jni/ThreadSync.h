/* Copyright (c) 2013-2017, ARM Limited and Contributors
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

#ifndef THREADSYNC_H
#define THREADSYNC_H

#include <GLES3/gl3.h>
#include "AndroidPlatform.h"
#include <stdlib.h>


/* These indices describe the cube triangle strips, separated by degenerate triangles where necessary. */
static const GLubyte cubeIndices[] =
{
    0, 1, 2, 3,   3, 4,   4, 5, 6, 7,   7, 8,   8, 9, 10, 11,   11, 12,   12, 13, 14, 15,   15, 16,   16, 17, 18, 19,   19, 20,   20, 21, 22, 23,
};

/* Tri strips, so quads are in this order:
 *
 * 2 ----- 3
 * | \     |
 * |   \   |6 - 7
 * |     \ || \ |
 * 0 ----- 14 - 5
 */
static const float cubeVertices[] =
{
    /* Front. */
    -0.5f, -0.5f,  0.5f, /* 0 */
     0.5f, -0.5f,  0.5f, /* 1 */
    -0.5f,  0.5f,  0.5f, /* 2 */
     0.5f,  0.5f,  0.5f, /* 3 */

    /* Right. */
     0.5f, -0.5f,  0.5f, /* 4 */
     0.5f, -0.5f, -0.5f, /* 5 */
     0.5f,  0.5f,  0.5f, /* 6 */
     0.5f,  0.5f, -0.5f, /* 7 */

    /* Back. */
     0.5f, -0.5f, -0.5f, /* 8 */
    -0.5f, -0.5f, -0.5f, /* 9 */
     0.5f,  0.5f, -0.5f, /* 10 */
    -0.5f,  0.5f, -0.5f, /* 11 */

    /* Left. */
    -0.5f, -0.5f, -0.5f, /* 12 */
    -0.5f, -0.5f,  0.5f, /* 13 */
    -0.5f,  0.5f, -0.5f, /* 14 */
    -0.5f,  0.5f,  0.5f, /* 15 */

    /* Top. */
    -0.5f,  0.5f,  0.5f, /* 16 */
     0.5f,  0.5f,  0.5f, /* 17 */
    -0.5f,  0.5f, -0.5f, /* 18 */
     0.5f,  0.5f, -0.5f, /* 19 */

    /* Bottom. */
    -0.5f, -0.5f, -0.5f, /* 20 */
     0.5f, -0.5f, -0.5f, /* 21 */
    -0.5f, -0.5f,  0.5f, /* 22 */
     0.5f, -0.5f,  0.5f, /* 23 */
};

static const float cubeTextureCoordinates[] =
{
    /* Front. */
    0.0f, 0.0f, /* 0 */
    1.0f, 0.0f, /* 1 */
    0.0f, 1.0f, /* 2 */
    1.0f, 1.0f, /* 3 */

    /* Right. */
    0.0f, 0.0f, /* 4 */
    1.0f, 0.0f, /* 5 */
    0.0f, 1.0f, /* 6 */
    1.0f, 1.0f, /* 7 */

    /* Back. */
    0.0f, 0.0f, /* 8 */
    1.0f, 0.0f, /* 9 */
    0.0f, 1.0f, /* 10 */
    1.0f, 1.0f, /* 11 */

    /* Left. */
    0.0f, 0.0f, /* 12 */
    1.0f, 0.0f, /* 13 */
    0.0f, 1.0f, /* 14 */
    1.0f, 1.0f, /* 15 */

    /* Top. */
    0.0f, 0.0f, /* 16 */
    1.0f, 0.0f, /* 17 */
    0.0f, 1.0f, /* 18 */
    1.0f, 1.0f, /* 19 */

    /* Bottom. */
    0.0f, 0.0f, /* 20 */
    1.0f, 0.0f, /* 21 */
    0.0f, 1.0f, /* 22 */
    1.0f, 1.0f, /* 23 */
};

static const float cubeColors[] =
{
    /* Front. */
    0.0f, 0.0f, 0.0f, 1.0f, /* 0 */
    1.0f, 0.0f, 0.0f, 1.0f, /* 1 */
    0.0f, 1.0f, 0.0f, 1.0f, /* 2 */
    1.0f, 1.0f, 0.0f, 1.0f, /* 3 */

    /* Right. */
    1.0f, 0.0f, 0.0f, 1.0f, /* 4 */
    0.0f, 0.0f, 1.0f, 1.0f, /* 5 */
    1.0f, 1.0f, 0.0f, 1.0f, /* 6 */
    0.0f, 1.0f, 1.0f, 1.0f, /* 7 */

    /* Back. */
    0.0f, 0.0f, 1.0f, 1.0f, /* 8 */
    1.0f, 0.0f, 1.0f, 1.0f, /* 9 */
    0.0f, 1.0f, 1.0f, 1.0f, /* 10 */
    1.0f, 1.0f, 1.0f, 1.0f, /* 11 */

    /* Left. */
    1.0f, 0.0f, 1.0f, 1.0f, /* 12 */
    0.0f, 0.0f, 0.0f, 1.0f, /* 13 */
    1.0f, 1.0f, 1.0f, 1.0f, /* 14 */
    0.0f, 1.0f, 0.0f, 1.0f, /* 15 */

    /* Top. */
    0.0f, 1.0f, 0.0f, 1.0f, /* 16 */
    1.0f, 1.0f, 0.0f, 1.0f, /* 17 */
    1.0f, 1.0f, 1.0f, 1.0f, /* 18 */
    0.0f, 1.0f, 1.0f, 1.0f, /* 19 */

    /* Bottom. */
    1.0f, 0.0f, 1.0f, 1.0f, /* 20 */
    0.0f, 0.0f, 1.0f, 1.0f, /* 21 */
    0.0f, 0.0f, 0.0f, 1.0f, /* 22 */
    1.0f, 0.0f, 0.0f, 1.0f, /* 23 */
};

EGLint configAttributes[] =
{
    /* DO NOT MODIFY. */
    /* These attributes are in a known order and may be re-written at initialization according to application requests. */
    EGL_SAMPLES,             4,

    EGL_ALPHA_SIZE,          0,

    EGL_RED_SIZE,            8,
    EGL_GREEN_SIZE,          8,
    EGL_BLUE_SIZE,           8,
    EGL_BUFFER_SIZE,         32,

    EGL_STENCIL_SIZE,        0,
    EGL_RENDERABLE_TYPE,     EGL_OPENGL_ES2_BIT,    /* This field will be completed according to application request. */

    EGL_SURFACE_TYPE,        EGL_PBUFFER_BIT ,

    EGL_DEPTH_SIZE,          16,

    /* MODIFY BELOW HERE. */
    /* If you need to add or override EGL attributes from above, do it below here. */

    EGL_NONE
};

EGLint contextAttributes[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 3, /* GLES 3 version is requested. */
    EGL_NONE
};

/* [Pixel buffer attributes] */
EGLint pBufferAttributes[] =
{
    EGL_WIDTH, 2,
    EGL_HEIGHT, 2,
    EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
    EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
    EGL_NONE
};
/* [Pixel buffer attributes] */

EGLConfig findConfig(EGLDisplay display, bool strictMatch, bool offscreen)
{
    EGLConfig *configsArray = NULL;
    EGLint numberOfConfigs = 0;
    EGLBoolean success = EGL_FALSE;

    /* Enumerate available EGL configurations which match or exceed our required attribute list. */
    success = eglChooseConfig(display, configAttributes, NULL, 0, &numberOfConfigs);
    if(success != EGL_TRUE)
    {
        EGLint error = eglGetError();
        LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
        LOGE("Failed to enumerate EGL configs at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }

    LOGD("Number of configs found is %d\n", numberOfConfigs);

    if (numberOfConfigs == 0)
    {
        LOGD("Disabling AntiAliasing to try and find a config.\n");
        configAttributes[1] = EGL_DONT_CARE;
        success = eglChooseConfig(display, configAttributes, NULL, 0, &numberOfConfigs);
        if(success != EGL_TRUE)
        {
            EGLint error = eglGetError();
            LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
            LOGE("Failed to enumerate EGL configs at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }

        if (numberOfConfigs == 0)
        {
            LOGE("No configs found with the requested attributes.\n");
            exit(1);
        }
        else
        {
            LOGD("Configs found when antialiasing disabled.\n ");
        }
    }

    /* Allocate space for all EGL configs available and get them. */
    configsArray = (EGLConfig *)calloc(numberOfConfigs, sizeof(EGLConfig));
    if(configsArray == NULL)
    {
        LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }
    success = eglChooseConfig(display, configAttributes, configsArray, numberOfConfigs, &numberOfConfigs);
    if(success != EGL_TRUE)
    {
        EGLint error = eglGetError();
        LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
        LOGE("Failed to enumerate EGL configs at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }

    bool matchFound = false;
    int matchingConfig = -1;

    if (strictMatch)
    {
        /*
         * Loop through the EGL configs to find a color depth match.
         * Note: This is necessary, since EGL considers a higher color depth than requested to be 'better'
         * even though this may force the driver to use a slow color conversion blitting routine.
         */
        EGLint redSize = configAttributes[5];
        EGLint greenSize = configAttributes[7];
        EGLint blueSize = configAttributes[9];

        for(int configsIndex = 0; (configsIndex < numberOfConfigs) && !matchFound; configsIndex++)
        {
            EGLint attributeValue = 0;

            success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_RED_SIZE, &attributeValue);
            if(success != EGL_TRUE)
            {
                EGLint error = eglGetError();
                LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
                LOGE("Failed to get EGL attribute at %s:%i\n", __FILE__, __LINE__);
                exit(1);
            }

            if(attributeValue == redSize)
            {
                success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_GREEN_SIZE, &attributeValue);
                if(success != EGL_TRUE)
                {
                    EGLint error = eglGetError();
                    LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
                    LOGE("Failed to get EGL attribute at %s:%i\n", __FILE__, __LINE__);
                    exit(1);
                }

                if(attributeValue == greenSize)
                {
                    success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_BLUE_SIZE, &attributeValue);
                    if(success != EGL_TRUE)
                    {
                        EGLint error = eglGetError();
                        LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
                        LOGE("Failed to get EGL attribute at %s:%i\n", __FILE__, __LINE__);
                        exit(1);
                    }

                    if(attributeValue == blueSize)
                    {
                        if(offscreen)
                        {
                            success = eglGetConfigAttrib(display, configsArray[configsIndex], EGL_SURFACE_TYPE, &attributeValue);
                            if(success != EGL_TRUE)
                            {
                                EGLint error = eglGetError();
                                LOGE("eglGetError(): %i (0x%.4x)\n", (int)error, (int)error);
                                LOGE("Failed to get EGL attribute at %s:%i\n", __FILE__, __LINE__);
                                exit(1);
                            }
                            if(attributeValue & EGL_PBUFFER_BIT)
                            {
                                matchFound = true;
                                matchingConfig = configsIndex;
                            }

                        }
                        else
                        {
                            matchFound = true;
                            matchingConfig = configsIndex;
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* If not strict we can use any matching config. */
        matchingConfig = 0;
        matchFound = true;
    }

    if(!matchFound)
    {
        LOGE("Failed to find matching EGL config at %s:%i\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Return the matching config. */
    EGLConfig configToReturn = configsArray[matchingConfig];

    free(configsArray);
    configsArray = NULL;

    return configToReturn;
}

#endif /* THREADSYNC_H */
