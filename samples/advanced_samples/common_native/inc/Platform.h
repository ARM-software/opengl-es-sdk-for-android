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

#ifndef PLATFORM_H
#define PLATFORM_H

#if !defined(ANDROID)

#include "EGLRuntime.h"
#include "VectorTypes.h"

#include <cstdio>

namespace MaliSDK
{
    /**
     * \brief Abstract class to hide the complexity of multiple build targets.
     */
    class Platform
    {
    public:
        /**
         * \brief An enum to define the status of a window.
         */
        enum WindowStatus {/** Window has nothing to report */
                           WINDOW_IDLE, 
                           /** The window has been closed by the user */
                           WINDOW_EXIT, 
                           /** The user has clicked on the window */
                           WINDOW_CLICK};
        Vec2 mouseClick;
        /*
         * The following variables are platform specific handles/pointers to 
         * displays/devices/windows. Used to create and manage the window 
         * to which OpenGL ES 2.0 graphics are rendered.
         */
    #if defined(_WIN32)
        HWND window;
        HDC deviceContext;
    #elif defined(__arm__) && defined(__linux__)
        fbdev_window *window;
    #elif defined(__linux__)
        Window window;
        Display* display;
    #endif    
        /**
         * \brief Create a native window on the target device.
         * \param[in] width The required width of the window.
         * \param[in] height The required height of the window.
         */
        virtual void createWindow(int width, int height) = 0;
        
        /**
         * \brief Check status of the window.
         * \return The status of the window.
         */
        virtual WindowStatus checkWindow(void) = 0;
        
        /**
         * \brief Close and clean-up the native window.
         */
        virtual void destroyWindow(void) = 0;
        
        /**
         * \brief Print a log message to the terminal.
         * \param[in] format The format the log message should take. Equivilent to printf.
         * \param[in] ... Variable length input to specify variables to print. They will be formatted as specified in format.
         */
        static void log(const char* format, ...);
        
        /**
         * Get the instance of Platform specific to the target.
         * \return An instance of a subclass of Platform which will work on the target platform.
         */
        static Platform* getInstance(void);
    };
}
#if defined(_WIN32)
#include "WindowsPlatform.h"

#elif defined(__arm__) && defined(__linux__)
#include "LinuxOnARMPlatform.h"

#elif defined(__linux__)
#include "DesktopLinuxPlatform.h"

#endif

#define GL_CHECK(x) \
    x; \
    { \
        GLenum glError = glGetError(); \
        if(glError != GL_NO_ERROR) { \
            LOGD("glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
            exit(1); \
        } \
    }
    
#define LOGI Platform::log 
#define LOGE fprintf (stderr, "Error: "); Platform::log
#ifdef DEBUG
#define LOGD fprintf (stderr, "Debug: "); Platform::log
#else
#define LOGD
#endif

#else
#include "AndroidPlatform.h"
#endif /* !defined(__android__) */
#endif /* PLATFORM_H */