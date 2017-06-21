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

#ifndef TEXTURE_H
#define TEXTURE_H

#include "ETCHeader.h"

#if GLES_VERSION == 2
#include <GLES2/gl2.h>
#elif GLES_VERSION == 3
#include <GLES3/gl3.h>
#else 
#error "GLES_VERSION must be defined as either 2 or 3"
#endif

namespace MaliSDK
{
    /** 
     * \brief Functions for working with textures.
     */
    class Texture
    {
    private:
        /**
         * \brief Uses glGetIntegerv to get the number of compressed texture formats and the formats themselves.
         
         * Calls glGetIntegerv with GL_NUM_COMPRESSED_TEXTURE_FORMATS and GL_COMPRESSED_TEXTURE_FORMATS.
         * \param[out] textureFormats Pointer to the array of texture formats.
         * \param[out] numberOfTextureFormats Pointer to the number of compressed texture formats.
         */
        static void getCompressedTextureFormats(GLint **textureFormats, int* numberOfTextureFormats);
    public:
        /**
         * \brief Reports whether or not ETC (Ericsson Texture Compression) is supported.
         *
         * Uses getCompressedTextureFormats to get the list of supported compression 
         * formats and then checks to see if any of them are GL_ETC1_RGB8_OES.
         * \param[in] verbose If true, prints out the number of supported texture compression formats and then lists the formats supported.
         */
        static bool isETCSupported(bool verbose = false);

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

        /**
         * \brief Load texture data from a file into memory.
         *
         * \param[in] filename The filename of the texture to load.
         * \param[out] textureData Pointer to the texture that has been loaded.
         */
        static void loadData(const char *filename, unsigned char **textureData);

        /**
         * \brief Load header and texture data from a pkm file into memory.
         *
         * \param[in] filename The filename of the texture to load.
         * \param[out] etcHeader Pointer to the header that has been loaded.
         * \param[out] textureData Pointer to the texture that has been loaded.      
         */
        static void loadPKMData(const char *filename, ETCHeader* etcHeader, unsigned char **textureData);

        /**
         * \brief Load compressed mipmaps into memory
         *
         * Load the base level, calculate how many Mipmap levels there are.
         * Load the PKM files into memory.
         * Load the data into the texture mipmap levels.
         * \param[in] filenameBase The base filename of the texture mipmap levels. Will have the mipmap level number appended to it to load all of the mipmap levels.
         * For example, if filenameBase = "texture_", this method will try to load the files "texture_0", "texture_1", "texture_2", etc..
         * \param[in] filenameSuffix Any suffix to the mipmap filenames. Most commonly used for file extensions.
         * For example, if filenameSuffix = ".pkm", this method will append ".pkm" to all the files it tries to load.
         * \param[out] textureID The texture ID of the texture that has been loaded.
         */
        static void loadCompressedMipmaps(const char *filenameBase, const char *filenameSuffix, GLuint *textureID);

        /**
         * \brief Copies float pixel data of one line of the image from source to
         * destination in the reverse direction.
         *
         * \param[out] destination Place in memory where the reversed data will be copied to. Cannot be NULL.
         * \param[in] source Place from which the copying should start from. Cannot be NULL.
         * \param[in] lineWidth Number of RGB pixels that will be copied.
         */
        static void reversePixelLine(float* destination, const float* source, int lineWidth);
    };
}
#endif /* TEXTURE_H */