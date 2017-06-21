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

 
#ifndef ETCHEADER_H
#define ETCHEADER_H

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
     * \brief Class to extract information from the ETC headers of compressed textures.
     */
    class ETCHeader
    {
    private:
        unsigned char paddedWidthMSB;
        unsigned char paddedWidthLSB;
        unsigned char paddedHeightMSB;
        unsigned char paddedHeightLSB;
        unsigned char widthMSB;
        unsigned char widthLSB;
        unsigned char heightMSB;
        unsigned char heightLSB;
    public:
        /**
         * \brief Default constructor.
         */
        ETCHeader();

        /**
         * \brief Extract the ETC header information from a loaded ETC compressed texture.
         */
        ETCHeader(unsigned char *data);
        
        /**
         * \brief The width of the original texture.
         * 
         * The width of a compressed texture is padded to 4x4 blocks by the compression method.
         * The resulting width of the compressed texture may therefore be larger if it's original width was not a multiple of 4.
         * By using the unpadded width, the original texture can be drawn.
         * \return The width of the original texture without padding.
         */
        unsigned short getWidth(void);

        /**
         * \brief The height of the original texture.
         * 
         * The height of a compressed texture is padded to 4x4 blocks by the compression method.
         * The resulting height of the compressed texture may therefore be larger if it's original height was not a multiple of 4.
         * By using the unpadded height, the original texture can be drawn.
         * \return The height of the original texture without padding.
         */
        unsigned short getHeight(void);

        /**
         * \brief The width of the compressed texture with the padding added.
         * 
         * The width of a compressed texture is padded to 4x4 blocks by the compression method.
         * The resulting width of the compressed texture may therefore be larger if it's original width was not a multiple of 4.
         * \return The width of the compressed texture with padding included.
         */
        unsigned short getPaddedWidth(void);

        /**
         * \brief The height of the compressed texture with the padding added.
         * 
         * The height of a compressed texture is padded to 4x4 blocks by the compression method.
         * The resulting height of the compressed texture may therefore be larger if it's original height was not a multiple of 4.
         * \return The height of the compressed texture with padding included.
         */
        unsigned short getPaddedHeight(void);

        /**
         * \brief The size of the compressed texture with the padding added.
         * 
         * The size is computed as padded width multiplied by padded height.
         * \param[in] internalFormat The internal format of the compressed texture.
         * \return The size of the compressed texture with padding included.
         */
        GLsizei getSize(GLenum internalFormat);
    };
}
#endif /* ETCHEADER_H */