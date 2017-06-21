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

    struct tagBITMAPFILEHEADER
    {
        short bfType;
        int   bfSize;
        short bfReserved1;
        short bfReserved2;
        int   bfOffBits;
    };

    struct tagBITMAPINFOHEADER
    {
        int   biSize;
        int   biWidth;
        int   biHeight;
        short biPlanes;
        short biBitCount;
        int   biCompression;
        int   biSizeImage;
        int   biXPelsPerMeter;
        int   biYPelsPerMeter;
        int   biClrUsed;
        int   biClrImportant;
    };

    /** 
     * \brief Functions for working with textures.
     */
    class Texture
    {
    private:
        /**
         * \brief Read BMP file header.
         *
         * \param filePtr             File pointer where BMP file header data is stored.
         *                            Cannot be NULL.
         * \param bitmapFileHeaderPtr Deref will be used to store loaded data.
         *                            Cannot be NULL.
         */
        static void readBitmapFileHeader(FILE* filePtr, tagBITMAPFILEHEADER* bitmapFileHeaderPtr);
        /**
         * \brief Read BMP info header.
         *
         * \param filePtr             File pointer where BMP info header data is stored.
         *                            Cannot be NULL.
         * \param bitmapInfoHeaderPtr Deref will be used to store loaded data.
         *                            Cannot be NULL.
         */
        static void readBitmapInforHeader(FILE* filePtr, tagBITMAPINFOHEADER* bitmapInfoHeaderPtr);

    public:
        /**
         * \brief Load BMP texture data from a file into memory.
         *
         * \param fileName          The filename of the texture to be loaded.
         *                          Cannot be NULL.
         * \param imageWidthPtr     Deref will be used to store image width.
         * \param imageHeightPtr    Deref will be used to store image height.
         * \param textureDataPtrPtr Pointer to a memory where loaded texture data will be stored.
         *                          Cannot be NULL.
         */
        static void loadBmpImageData(const char*     fileName,
                                     int*            imageWidthPtr,
                                     int*            imageHeightPtr,
                                     unsigned char** textureDataPtrPtr);
    };
}
#endif /* TEXTURE_H */
