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

#ifndef SHADER_H
#define SHADER_H

#include <GLES3/gl3.h>

namespace MaliSDK
{
    /**
    * \brief Functions for working with OpenGL ES shaders.
    */
    class Shader
    {
    private:
        /**
        * \brief Load shader source from a file into memory.
        *
        * \param filename File name of the shader to load.
        *
        * \return A character array containing the contents of the shader source file.
        */
        static char *loadShader(const char *filename);

    public:
        /**
        * \brief Create shader, load in source, compile, and dump debug as necessary.
        *
        *  \note Loads the OpenGL ES Shading Language code into memory.
        *        Creates a shader using with the required shaderType using glCreateShader(shaderType) and then compiles it using glCompileShader.
        *        The output from the compilation is checked for success and a log of the compilation errors is printed in the case of failure.
        *
        * \param shaderObjectIdPtr Deref will be used to store generated shader object ID.
        *                          Cannot be NULL.
        * \param filename          Name of a file containing OpenGL ES SL source code.
        * \param shaderType        Passed to glCreateShader to define the type of shader being processed.
        *                          Must be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
        */
        static void processShader(GLuint *shaderObjectIdPtr, const char *filename, GLint shaderType);
    };
}
#endif /* SHADER_H */
