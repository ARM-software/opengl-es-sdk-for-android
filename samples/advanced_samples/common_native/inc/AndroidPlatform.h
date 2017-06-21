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

#ifndef ANDROIDPLATFORM_H
#define ANDROIDPLATFORM_H

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    __FILE__

#define  LOGI(format, args...) { fprintf(stderr, format, ##args); __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, format, ##args); }
#define  LOGE(format, args...) { fprintf(stderr, format, ##args); __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, format, ##args); }
#define  LOGD(format, args...) { fprintf(stderr, format, ##args); __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, format, ##args); }

#define GL_CHECK(x) \
        x; \
        AndroidPlatform::checkGlesError(#x);

namespace MaliSDK
{
    /**
     * \brief Functions specific to the Android Platform
     */
    class AndroidPlatform
    {
    public:
        /**
         * \brief Extract an asset file from the APK.
         *
         * If the file specified by filename is not avaliable in destinationDirectory, this method will attempt to extract it from the APK into destinationDirectory.
         * Typically used in OpenGL ES applications to extract shader files and textures.
         * \param[in] JNIEnvironment  A pointer to the JNI environment which allows interfacing with the Java Virtual Machine (JVM).
         *                            Allows extensive interaction with the JVM including accessing Java classes, fields and methods.
         *                            This pointer is provided as part of a JNI call from Java to C++.
         * \param[in] destinationDirectory The destination directory where the file should be placed.
         * \param[in] filename Name of the file to extract from the APK. Can be any file placed inside the "assets" directory of the Android project when the APK is built.
         * \return Returns true if the file is avaliable in the destinationDirectory.
         */
        static bool getAndroidAsset(JNIEnv* JNIEnvironment, const char destinationDirectory[], const char filename[]);

        /**
         * \brief Checks if OpenGL ES has reported any errors.
         * \param[in] operation The OpenGL ES function that has been called.
         */
        static void checkGlesError(const char* operation);

        /**
         * \brief Converts OpenGL ES error codes into the readable strings.
         * \param[in] glErrorCode The OpenGL ES error code to convert.
         * \return The string form of the error code.
         */
        static const char* glErrorToString(int glErrorCode);

        /**
         * \brief Deep copy a string using memcopy().
         * \param[in] string The string to make a copy of.
         * \return A deep copy of the string.
         */
        static char* copyString(const char* string);
    };
}
#endif /* ANDROIDPLATFORM_H */
