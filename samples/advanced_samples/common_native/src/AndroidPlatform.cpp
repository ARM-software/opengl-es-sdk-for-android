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

#include <string>
 
#include <android/log.h>

#include <GLES2/gl2.h>

#include "AndroidPlatform.h"
#include "JavaClass.h"

#include <unistd.h>

using std::string;

namespace MaliSDK
{
    /**
     * Calls glGetError() until no more errors are reported.
     * GL stores all the errors since the last call to glGetError().
     * These are returned one-by-one with calls to getGetError(). Each call clears an error flag from GL.
     * If GL_NO_ERROR is returned no errors have occured since the last call to glGetError().
     * Prints the original error code the operation which caused the error (passed in) and the string version of the error.
     */
    void AndroidPlatform::checkGlesError(const char* operation)
    {
        GLint error = 0;
        for (error = glGetError(); error != GL_NO_ERROR; error = glGetError())
        {
            LOGE("glError (0x%x) after `%s` \n", error, operation);
            LOGE("glError (0x%x) = `%s` \n", error, glErrorToString(error));
        }
    }

    /**
     * Converts the error codes into strings using the definitions found in <GLES2/gl2.h>
     */
    const char* AndroidPlatform::glErrorToString(int glErrorCode)
    {
        switch(glErrorCode)
        {
            case GL_NO_ERROR: 
                return "GL_NO_ERROR"; 
                break;
            case GL_INVALID_ENUM:
                return "GL_INVALID_ENUM"; 
                break;
            case GL_INVALID_VALUE: 
                return "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION: 
                return "GL_INVALID_OPERATION"; 
                break;
            case GL_OUT_OF_MEMORY: 
                return "GL_OUT_OF_MEMORY"; 
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                return "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            default:   
                return "unknown"; 
                break;
        }
    }

    char* AndroidPlatform::copyString(const char* string)
    {
        int length = 0;
        char *newString = NULL;
        if (string == NULL)
        {
            return NULL;
        }
        length = strlen(string) + 1;
        newString = (char*)malloc(length);
        if (newString == NULL)
        {
            LOGE("copyString(): Failed to allocate memory using malloc().\n");
            return NULL;
        }
        memcpy(newString, string, length);
        return newString;
    }

    bool AndroidPlatform::getAndroidAsset(JNIEnv* JNIEnvironment, const char destinationDirectory[], const char filename[])
    {   
        if (JNIEnvironment == NULL || destinationDirectory == NULL || filename == NULL)
        {
            LOGE("getAndroidAsset(): NULL argument is not acceptable.\n");
            return false;
        }
        
        /* Create the full path to where we want the file to be found. */
        string resourceFilePath = string(destinationDirectory) + string(filename);

        /* Try and find the file in the file system. */
        FILE * file = NULL;
        file = fopen(resourceFilePath.c_str(), "r");
        if (file != NULL)
        {    
            /* 
             * The file does exist on the target device's file system,
             * The program can use this file as normal.
             */
            fclose (file);
        }
        else
        {
            /* The file does not exist and needs to be extracted from the APK package */
            
            /* Use the MaliSamplesActivity.extractAsset() Java method to extract the file */
            JavaClass javaClass(JNIEnvironment, "com/arm/malideveloper/openglessdk/MaliSamplesActivity");

            /* Extract the file from the asset folder embedded in the APK to the destination directory. */
            if (!javaClass.staticMethod("extractAsset", destinationDirectory, filename))
            {
                LOGE("getAndroidAsset(): Failed to call MaliSamplesActivity.extractAsset() for %s\n", filename);
                return false;
            }    
        }
        return true;
    }
}