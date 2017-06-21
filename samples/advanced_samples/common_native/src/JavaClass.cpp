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

#include <android/log.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>

#include "JavaClass.h"
#include "AndroidPlatform.h"

namespace MaliSDK
{
    JavaClass::JavaClass(JNIEnv* env, const char* requiredClassPath):
        classPath(AndroidPlatform::copyString(requiredClassPath)),
        JNIEnvironment(env),
        intialized(false)
    {
        if (JNIEnvironment == NULL) 
        { 
            LOGE("ERROR - JavaClass: Environment cannot be NULL.\n"); 
            return; 
        }
        jClass = JNIEnvironment->FindClass(classPath);
        if (jClass == NULL) 
        { 
            LOGE("ERROR - JavaClass: Java class not found."); 
            return; 
        }
        intialized = true;
    }

    JavaClass::~JavaClass()
    {
        if (classPath != NULL) 
        {
            free(classPath);
        }
    }

    bool JavaClass::staticField(const char* fieldName, char** result)
    {
        if (!intialized || fieldName == NULL)
        {
            return false;
        }
        jfieldID fieldID = JNIEnvironment->GetStaticFieldID(jClass, fieldName, TJString);
        if (fieldID == NULL) 
        { 
            LOGE("ERROR - JavaClass: Field %s not found in %s\n", fieldName, classPath); 
            return false; 
        }
        jstring jValue = (jstring)JNIEnvironment->GetStaticObjectField(jClass, fieldID);
        const char *value = JNIEnvironment->GetStringUTFChars(jValue, 0);
        char * returnValue = AndroidPlatform::copyString(value);
        JNIEnvironment->ReleaseStringUTFChars(jValue, value);
        *result = returnValue;
        return true;
    }

    bool JavaClass::staticField(const char* fieldName, int* result)
    {
        if (!intialized || fieldName == NULL) return false;
        jfieldID fieldID = JNIEnvironment->GetStaticFieldID(jClass, fieldName, TJInt);
        if (fieldID == NULL) 
        { 
            LOGE("ERROR - JavaClass: Field %s not found in %s\n", fieldName, classPath); 
            return false; 
        }
        *result = JNIEnvironment->GetStaticIntField(jClass, fieldID);
        return true;
    }

    bool JavaClass::staticMethod(const char* methodName, int** returnValue, const char* param01)
    {
        if (!intialized || methodName == NULL || returnValue == NULL) 
        {
            return false;
        }
        jmethodID methodID = JNIEnvironment->GetStaticMethodID(jClass, methodName, JM(TJIntArr, TJString));
        if (methodID == NULL) 
        { 
            LOGE("ERROR - JavaClass: Method %s not found in %s.\n", methodName, classPath); 
            return false; 
        }
        jintArray javaReturn = (jintArray)JNIEnvironment->CallStaticObjectMethod(jClass, methodID, JNIEnvironment->NewStringUTF(param01));
        if (javaReturn == NULL) 
        { 
            LOGE("ERROR - JavaClass: A call to static method %s in %s failed.\n", methodName, classPath); 
            return false; 
        }
        jboolean copy; /* copy flag */
        *returnValue = (int*)JNIEnvironment->GetIntArrayElements(javaReturn, &copy);
        if (*returnValue == NULL) 
        { 
            LOGE("ERROR - JavaClass: An attempt to retrieve array data in method %s in %s failed.\n", methodName, classPath); 
            return false; 
        }
        return true;
    }

    bool JavaClass::staticMethod(const char* methodName, const char* param01, const char* param02)
    {
        if (!intialized || methodName == NULL)
        {
            return false;
        }
        jmethodID methodID = JNIEnvironment->GetStaticMethodID(jClass, methodName, JM(TJVoid, TJString TJString));
        if (methodID == NULL) 
        { 
            LOGE("ERROR - JavaClass: Method %s not found in %s.\n", methodName, classPath); 
            return false; 
        }
        JNIEnvironment->CallStaticVoidMethod(jClass, methodID, JNIEnvironment->NewStringUTF(param01), JNIEnvironment->NewStringUTF(param02));
        return true;
    }
}