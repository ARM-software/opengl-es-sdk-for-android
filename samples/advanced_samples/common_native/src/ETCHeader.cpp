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

#include "ETCHeader.h"

namespace MaliSDK
{
    ETCHeader::ETCHeader()
    {
    
    }

    ETCHeader::ETCHeader(unsigned char *data)
    {
        /*
         * Load from a ETC compressed pkm image file. 
         * First 6 bytes are the name of the file format and version/packing type.
         * Bytes 7 and 8 are blank.
         */
        /* Beware endianess issues with most/least significant bits of the height and width. */
        paddedWidthMSB = data[8];
        paddedWidthLSB = data[9];
        paddedHeightMSB = data[10];
        paddedHeightLSB = data[11];
        widthMSB = data[12];
        widthLSB = data[13];
        heightMSB = data[14];
        heightLSB = data[15];
    }

    unsigned short ETCHeader::getWidth(void)
    {
        return (widthMSB << 8) | widthLSB;
    }

    unsigned short ETCHeader::getHeight(void)
    {
        return (heightMSB << 8) | heightLSB;
    }

    unsigned short ETCHeader::getPaddedWidth(void)
    {
        return (paddedWidthMSB << 8) | paddedWidthLSB;
    }

    unsigned short ETCHeader::getPaddedHeight(void)
    {
        return (paddedHeightMSB << 8) | paddedHeightLSB;
    }

#if GLES_VERSION == 2
    GLsizei ETCHeader::getSize(GLenum internalFormat)
    {
        return (getPaddedWidth() * getPaddedHeight());
    }
#elif GLES_VERSION == 3
    GLsizei ETCHeader::getSize(GLenum internalFormat)
    {
        if (internalFormat != GL_COMPRESSED_RG11_EAC       && internalFormat != GL_COMPRESSED_SIGNED_RG11_EAC &&
            internalFormat != GL_COMPRESSED_RGBA8_ETC2_EAC && internalFormat != GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
        {
            return (getPaddedWidth() * getPaddedHeight()) >> 1;
        }
        else
        {
            return (getPaddedWidth() * getPaddedHeight());
        }
    }
#endif
}