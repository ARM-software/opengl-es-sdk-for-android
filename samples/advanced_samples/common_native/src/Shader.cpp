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

#include "Shader.h"
#include "Platform.h"

#include <cstdio>
#include <cstdlib>

namespace MaliSDK
{
    void Shader::processShader(GLuint *shader, const char *filename, GLint shaderType)
    {  
        const char *strings[1] = { NULL };

        /* Create shader and load into GL. */
        *shader = GL_CHECK(glCreateShader(shaderType));
        strings[0] = loadShader(filename);
        GL_CHECK(glShaderSource(*shader, 1, strings, NULL));

        /* Clean up shader source. */
        free((void *)(strings[0]));
        strings[0] = NULL;

        /* Try compiling the shader. */
        GL_CHECK(glCompileShader(*shader));
        GLint status;
        GL_CHECK(glGetShaderiv(*shader, GL_COMPILE_STATUS, &status));

        /* Dump debug info (source and log) if compilation failed. */
        if(status != GL_TRUE) 
        {
            GLint length;
            char *debugSource = NULL;
            char *errorLog = NULL;

            /* Get shader source. */
            GL_CHECK(glGetShaderiv(*shader, GL_SHADER_SOURCE_LENGTH, &length));
            debugSource = (char *)malloc(length);
            GL_CHECK(glGetShaderSource(*shader, length, NULL, debugSource));
            LOGE("Debug source START:\n%s\nDebug source END\n\n", debugSource);
            free(debugSource);

            /* Now get the info log. */
            GL_CHECK(glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length));
            errorLog = (char *)malloc(length);
            GL_CHECK(glGetShaderInfoLog(*shader, length, NULL, errorLog));
            LOGE("Log START:\n%s\nLog END\n\n", errorLog);
            free(errorLog);

            LOGE("Compilation FAILED!\n\n");
            exit(1);
        }
    }

    char* Shader::loadShader(const char *filename)
    {
        FILE *file = fopen(filename, "rb");
        if(file == NULL)
        {
            LOGE("Cannot read file '%s'\n", filename);
            exit(1);
        }
        /* Seek end of file. */
        fseek(file, 0, SEEK_END);
        /* Record the size of the file for memory allocation. */
        long length = ftell(file);
        /* Seek start of file again. */
        fseek(file, 0, SEEK_SET); 
        char *shader = (char *)calloc(length + 1, sizeof(char));
        if(shader == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }
        /* Read in the file */
        size_t numberOfBytesRead = fread(shader, sizeof(char), length, file);
        if (numberOfBytesRead != length) 
        {
            LOGE("Error reading %s (read %zu of %ld)", filename, numberOfBytesRead, length);
            exit(1);
        }
        shader[length] = '\0';
        fclose(file);

        return shader;
    }
}
