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

#ifndef MIN_MAX_BLENDING_H
#define MIN_MAX_BLENDING_H

#include <GLES3/gl3.h>

    /**
     * \brief Initializes OpenGL ES texture components.
     */
    void initialize3DTexture();

    /**
     * \brief Initializes input vertex data for shaders.
     */
    void initializeAttribArrays();

    /**
     * \brief Creates program and attaches shaders to it.
     */
    void initializeProgram();

    /**
     * \brief Fills 3D texture with images.
     */
    void initializeTextureData();

    /**
     * \brief Initializes uniform variables in program.
     */
    void initializeUniformData();

    /**
     * \brief Loads imagesCount images located in resourceDirectory.
     */
    void loadImages();

    /**
     * \brief Creates and loads count unicolor layers to a 3D texture.
     *
     * \param count Number of layers to be filled.
     */
    void loadUniformTextures(int count);

    /**
     * \brief Sets current blending equation.
     *
     * \param isMinBlending True, if GL_MIN blending mode should be used.
     */
    void setBlendEquation(GLboolean isMinBlending);

    /**
     * \brief Fills next empty 3D texture layer with textureData.
     *
     * It is called by the functions which prepare texture data either
     * by creating it inside the application or loading it from from a file.
     *
     * \param textureData Data the 3D texture is filled with.
     */
    void setNextTextureImage(GLvoid* textureData);
#endif /* MIN_MAX_BLENDING_H */
