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

#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static int PRIVATE_FILE_SIZE = 82;
static int PUBLIC_FILE_SIZE = 105;
static int CACHE_FILE_SIZE = 146;

extern "C"
{
    /* [initFunctionNativePrototype] */
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_fileloading_NativeLibrary_init(
            JNIEnv * env, jobject obj, jstring privateFile, jstring publicFile, jstring cacheFile);
    /* [initFunctionNativePrototype] */
};

/* [readFile] */
void readFile(const char * fileName, int size)
{
    FILE * file = fopen(fileName, "r");
    char * fileContent =(char *) malloc(sizeof(char) * size);

    if(file == NULL)
    {
        LOGE("Failure to load the file");
        return;
    }
    fread(fileContent, size, 1, file);
    LOGI("%s",fileContent);
    free(fileContent);
    fclose(file);
}
/* [readFile] */

/*[nativeInitFunction]*/
JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_fileloading_NativeLibrary_init(
        JNIEnv * env, jobject obj, jstring privateFile, jstring publicFile, jstring cacheFile )
{
    const char* privateFileC = env->GetStringUTFChars(privateFile, NULL);
    const char* publicFileC = env->GetStringUTFChars(publicFile, NULL);
    const char* cacheFileC = env->GetStringUTFChars(cacheFile, NULL);

    readFile(privateFileC, PRIVATE_FILE_SIZE);
    readFile(publicFileC, PUBLIC_FILE_SIZE);
    readFile(cacheFileC, CACHE_FILE_SIZE);

    env->ReleaseStringUTFChars(privateFile, privateFileC);
    env->ReleaseStringUTFChars(publicFile, publicFileC);
    env->ReleaseStringUTFChars(cacheFile, cacheFileC);
}
/*[nativeInitFunction]*/
