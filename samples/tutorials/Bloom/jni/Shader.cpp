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

#include "Shader.h"

#define LOG_TAG "libNative"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace MaliSDK
{
    /* Please see header for the specification. */
    void Shader::processShader(GLuint* shaderPtr, const char* shaderSourcePtr, GLint shaderType)
    {
        GLint compilationStatus = GL_FALSE;

        /* Create shader and load into GL. */
        *shaderPtr = GL_CHECK(glCreateShader(shaderType));

        GL_CHECK(glShaderSource(*shaderPtr, 1, &shaderSourcePtr, NULL));

        /* Try compiling the shader. */
        GL_CHECK(glCompileShader(*shaderPtr));

        GL_CHECK(glGetShaderiv(*shaderPtr, GL_COMPILE_STATUS, &compilationStatus));

        /* Dump debug info (source and log) if compilation failed. */
        if(compilationStatus != GL_TRUE)
        {
            GLint length;
            char *debugSource = NULL;
            char *errorLog    = NULL;

            /* Get shader source. */
            GL_CHECK(glGetShaderiv(*shaderPtr, GL_SHADER_SOURCE_LENGTH, &length));

            debugSource = (char *)malloc(length);

            GL_CHECK(glGetShaderSource(*shaderPtr, length, NULL, debugSource));

            LOGE("Debug source START:\n%s\nDebug source END\n\n", debugSource);

            free(debugSource);

            /* Now get the info log. */
            GL_CHECK(glGetShaderiv(*shaderPtr, GL_INFO_LOG_LENGTH, &length));

            errorLog = (char *)malloc(length);

            GL_CHECK(glGetShaderInfoLog(*shaderPtr, length, NULL, errorLog));

            LOGE("Log START:\n%s\nLog END\n\n", errorLog);

            free(errorLog);

            LOGE("Compilation FAILED!\n\n");
            exit(1);
        }
    }
}
