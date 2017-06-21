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

#ifndef JAVACLASS_H
#define JAVACLASS_H

#include <jni.h>

#include <cstdio>
#include <cstdlib>

// Java method signatures
#define JM(ret,params) "(" params ")" ret
// Java types
#define TJString "Ljava/lang/String;"
#define TJInt "I"
#define TJVoid "V"
#define TJIntArr "[I"

namespace MaliSDK
{
    /**
     * \brief Wraps a Java class to allow access to it's static fields and methods using JNI.
     */
    class JavaClass
    {
    private:
        char* classPath;
        jclass jClass;
        JNIEnv* JNIEnvironment;
        bool intialized;

    public:
        /**
         * \brief Constructor taking the Java environment and the required class path.
         *
         * Checks for the existance of the class and prints an error if it can't be found.
         * \param[in] JNIEnvironment  A pointer to the JNI environment which allows interfacing with the Java Virtual Machine (JVM).
         *                            Allows extensive interaction with the JVM including accessing Java classes, fields and methods.
         *                            This pointer is provided as part of a JNI call from Java to C++.
         * \param[in] classPath The class path to the Java class to wrap.
         */
        JavaClass(JNIEnv* JNIEnvironment, const char* classPath);

        /**
         * Default destructor.
         */
        virtual ~JavaClass();

        /**
         * \brief Access a static String field of the Java class.
         * \param[in] fieldName The name of the static field to access.
         * \param[out] result Pointer to where the String value of the static field will be placed.
         * \return True if the operation was successful.
         */
        bool staticField(const char* fieldName, char** result);
        
        /**
         * \brief Access a static integer field of the Java class.
         * \param[in] fieldName The name of the static field to access.
         * \param[out] result Pointer to where the integer value of the static field will be placed.
         * \return True if the operation was successful.
         */
        bool staticField(const char* fieldName, int* result);

        /**
         * \brief Call a static method with one parameter which returns an integer array within the Java class.
         * \param[in] methodName The name of the static method to call.
         * \param[out] returnValue Pointer to an integer array where the return value of the static method will be placed.
         * \param[in] param01 A parameter that will be passed to the static method of the Java class.
         * \return True if the operation was successful.
         */
        bool staticMethod(const char* methodName, int** returnValue, const char* param01);
        
        /**
         * \brief Call a static method with two parameters which doesn't return a value within the Java class.
         * \param[in] methodName The name of the static method to call.
         * \param[in] param01 The first parameter that will be passed to the static method of the Java class.
         * \param[in] param02 The second parameter that will be passed to the static method of the Java class.       
         * \return True if the operation was successful.
         */
        bool staticMethod(const char* methodName, const char* param01, const char* param02);
    };
}
#endif /* JAVACLASS_H */