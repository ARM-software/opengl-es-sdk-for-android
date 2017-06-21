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

#ifndef TEXTURE_H
#define TEXTURE_H

#include "Common.h"

#include <cstdio>
#include <cstdlib>

namespace MaliSDK
{
    /** 
     * \brief Functions for working with textures.
     */
    class Texture
    {

    public:
        /**
         * \brief Create a texture using random data.
         *
         * \param[in] width The required width of the texture.
         * \param[in] height The required height of the texture.
         * \param[in] textureData A pointer to the created texture data.
         */
        static void createTexture(unsigned int width, unsigned int height, GLvoid **textureData);

        /**
         * \brief Create a 1 component texture of uniform colour.
         *
         * \param [in]  width       The required width of the texture.
         * \param [in]  height      The required height of the texture.
         * \param [in]  red         The required red channel.
         * \param [out] textureData Output texture.
         */
        static void createTexture(unsigned int width, unsigned int height, unsigned int red, GLvoid **textureData);

        /**
         * \brief Create uniform texture using given color with 1 short integer components
         *
         * \param [in]  width       The required width of the texture.
         * \param [in]  height      The required height of the texture.
         * \param [in]  red         The required red channel.
         * \param [out] textureData Output texture.
         */
        static void createTexture(unsigned int width, unsigned int height, short red, short **textureData);

        /**
         * \brief Deletes previously created texture.
         *
         * \param [in] textureData Texture to be deleted.
         */
        static void deleteTextureData(GLvoid **textureData);
    };
}
#endif /* TEXTURE_H */
