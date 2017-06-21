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
 * \file ListEGLConfigs.cpp
 * \brief A sample which prints out the list of available
 * EGL configurations on the current platform.
 *
 * Shows how to use eglGetConfigs to view the availiable
 * configurations on a system.
 */

#include <EGL/egl.h>
#include <cstdlib>
#include <cstdio>

#include <jni.h>
#include <android/log.h>

#include "Platform.h"
#include "AndroidPlatform.h"

using namespace MaliSDK;

/* 
 * Function pointer type, taking an EGL token and value
 * returning a textual meaning
 */
typedef const char*(*decodeToken)(EGLint);
const char* decodeColorBuffer(EGLint value);
const char* decodeCaveat(EGLint value);
const char* decodeSurface(EGLint value);
const char* decodeAPISupport(EGLint value);

/* List of EGL attributes to query, text to be printed and function to decode meaning */
typedef struct {
    EGLint attribute;
    const char* text;
    decodeToken meaning;
} query;

/* 
 * Additional attributes to be inspected can be added here
 * The first member is the attribute as passed to eglGetConfigAttrib.
 * The value of the attribute can either be interpreted as 
 *   (i) an integer in which case the string to be printed should
 *       contain a single integer format specifier e.g. %d or %x
 *       and the third member should be NULL
 * or
 *  (ii) a value to be decoded into a human readable form e.g. a bitfield.
 *       In this case the third member is a pointer to a function of type
 *       decodeToken which will decode the attribute value. The string
 *       to be printed should contain a single string format specifier
 *       i.e. %s which will take the return value of the decode function.
 */
query queryList[] = {
    {EGL_CONFIG_ID,         " EGL_CONFIG_ID %d\n",       NULL},
    {EGL_CONFIG_CAVEAT,     " Config caveat: %s ",       decodeCaveat},
    {EGL_COLOR_BUFFER_TYPE, " Colour buffer type: %s\n", decodeColorBuffer},
    {EGL_BUFFER_SIZE,       " Colour depth: %d",         NULL},
    {EGL_RED_SIZE,          "     Red   %d ",            NULL},
    {EGL_GREEN_SIZE,        "     Green %d ",            NULL},
    {EGL_BLUE_SIZE,         "     Blue  %d ",            NULL},
    {EGL_ALPHA_SIZE,        "     Alpha %d\n",           NULL},
    {EGL_DEPTH_SIZE,        " Z-buffer bits: %d\n",      NULL},
    {EGL_SAMPLES,           " %dx anti-aliasing\n",      NULL},
    {EGL_SURFACE_TYPE,      " Surfaces: %s\n ",          decodeSurface},
    {EGL_RENDERABLE_TYPE,   " API support: %s\n",        decodeAPISupport},
    {EGL_NONE,              NULL,                        NULL}
};

/* 
 * Decipher an EGLConfig into more human readable terms.
 * EGLDisplay display : EGLDisplay handle required to call other EGL functions
 * EGLConfig* configs: Pointer to array of EGLConfigs
 * int configIndex: Element of list to inspect
 */
void describeConfig(EGLDisplay display, EGLConfig* configs, int configIndex) 
{
    EGLint attribute;
    EGLint value;
    EGLint allAttributes = 0;

    LOGI("Config number %d in returned configs\n", configIndex);

    /* Loop through all the attributes listed in queryList */
    while((attribute = queryList[allAttributes].attribute) != EGL_NONE)
    {
        /* Get the value of the attribute */
        EGLBoolean result = eglGetConfigAttrib(display, configs[configIndex], attribute, &value);
        
        if(result == EGL_FALSE)
        {
            LOGE("eglGetConfigAttrib failed (%d, %d)\n", configIndex, allAttributes);
        } 
        else 
        {
            /*
             * Display the information, calling a function to decode
             * EGL tokens into human readable forms if available
             */
            if(queryList[allAttributes].meaning)
            {
                LOGI(queryList[allAttributes].text, queryList[allAttributes].meaning(value));
            } 
            else
            {
                LOGI(queryList[allAttributes].text, value);
            }
        }
        allAttributes++;
    }
    LOGI("\n");
}

/* Decode EGL_SURFACE_TYPE */
const char* decodeSurfaceStrings[] = {
    "None!",
    "PBuffer",
    "Pixmap",
    "PBuffer+Pixmap",
    "Window",
    "Window+PBuffer",
    "Window+Pixmap",
    "Window+Pixmap+PBuffer"
};

const char* decodeSurface(EGLint value)
{
    /* TODO consider more than just bits 0-2 */
    return decodeSurfaceStrings[value&7];
}

/* Decode EGL_CONFIG_CAVEAT */
const char* decodeCaveat( EGLint value)
{
    switch(value)
    {
        case EGL_NONE:
            return "Normal";
            break;
        case EGL_SLOW_CONFIG:
            return "Slow";
            break;
        case EGL_NON_CONFORMANT_CONFIG:
            return "Non-conformant";
            break;
        default:
            return "Unknown EGL_CONFIG_CAVEAT";
            break;
    }
}

/* Decode EGL_COLOR_BUFFER_TYPE */
const char* decodeColorBuffer(EGLint value)
{
    switch(value)
    {
        case EGL_RGB_BUFFER:
            return "RGB colour buffer";
            break;
        case EGL_LUMINANCE_BUFFER:
            return "Luminance buffer";
            break;
        default:
            return "Unknown EGL_COLOR_BUFFER_TYPE";
            break;
    }
}

/* Decode EGL_RENDERABLE_TYPE */
const char* decodeAPISupportStrings[] = {
    "No API support(?)",
    "OpenGL ES",
    "OpenVG",
    "OpenGL ES, OpenVG",
    "OpenGL ES 2.0",
    "OpenGL ES, OpenGL ES 2.0",
    "OpenGL ES 2.0, OpenVG",
    "OpenGL ES, OpenGL ES 2.0, OpenVG"
};

const char* decodeAPISupport(EGLint value)
{
    /* Ignores OpenGL (desktop) support (bit 3 set) */
    return decodeAPISupportStrings[value&7];
}    

bool listConfigs(void) 
{    
    EGLBoolean success;

    /* Get a display handle and initalize EGL */
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(display == EGL_NO_DISPLAY) 
    {
        LOGD("eglGetDisplay returned EGL_NO_DISPLAY\n");
        return false;
    }
        
    EGLint major, minor;
    success = eglInitialize(display, &major, &minor);
    if(success == EGL_FALSE) 
    {
        LOGD("eglInitialize failed\n");
        return false;
    }
    /* Read the vendor string */
    const char *vendor = eglQueryString(display, EGL_VENDOR);
    if(vendor == NULL)
    {
        LOGD("eglQueryString failed\n");
        return false;
    }

    LOGI("EGL_VENDOR = %s, version %d.%d\n", vendor, major, minor);

    /* 
     * Find out how many configs are available in total,
     * allocate some memory to hold them, and then
     * get all of the configs. 
     * In the first call to eglGetConfigs 'numberOfConfigs' is an 
     * output telling us the total number of configs
     * available (total because we passed a NULL pointer
     * to the memory to be filled with configs). 
     * In the second call to eglGetConfigs 'numberOfConfigs' is an
     * input (telling EGL the size of the 'configs' buffer)
     * and also an output telling us how many configs are
     * in the configs.
     * It should have the same value after both calls.
     */
    EGLint numberOfConfigs;
    success = eglGetConfigs(display, NULL, 0, &numberOfConfigs);
    EGLConfig *configs = (EGLConfig*)malloc(sizeof(EGLConfig)*numberOfConfigs);
    success &= eglGetConfigs(display, configs, numberOfConfigs, &numberOfConfigs);
    if(success == EGL_FALSE) 
    {
        LOGD("eglGetConfigs failed\n");
        return false;
    }
    
    /* Look at each config */
    for(int allConfigs = 0; allConfigs < numberOfConfigs; allConfigs++) 
    {
        describeConfig(display, configs, allConfigs);
    }

    free(configs);

    fflush(stdout);

    /* Tell EGL we have finished */
    eglTerminate(display);
    
    return true;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_listeglconfigs_ListEGLConfigs_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        listConfigs();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_listeglconfigs_ListEGLConfigs_step
    (JNIEnv *env, jclass jcls)
    {
        /* We don't need to render frames. */
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_listeglconfigs_ListEGLConfigs_uninit
    (JNIEnv *, jclass)
    {
    
    }
}
