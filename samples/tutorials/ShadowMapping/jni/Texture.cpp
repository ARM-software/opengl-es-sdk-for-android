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

#include "Texture.h"

namespace MaliSDK
{
    /* Please see header for the specification. */
    void Texture::loadBmpImageData(const char*     fileName,
                                   int*            imageWidthPtr,
                                   int*            imageHeightPtr,
                                   unsigned char** textureDataPtrPtr)
    {
        ASSERT(fileName          != NULL,
               "Invalid file name.");
        ASSERT(textureDataPtrPtr != NULL,
               "Cannot use NULL pointer to store image data.");

        tagBITMAPFILEHEADER bitmapFileHeader;
        tagBITMAPINFOHEADER bitmapInfoHeader;
        FILE*               file              = NULL;
        unsigned char*      loadedTexture     = NULL;

        /* Try to open file. */
        file = fopen(fileName, "rb");

        ASSERT(file != NULL, "Failed to open file");

        /* Try to read the bitmap file header. */
        readBitmapFileHeader(file, &bitmapFileHeader);

        /* Try to read the bitmap info header. */
        readBitmapInforHeader(file, &bitmapInfoHeader);

        /* Try to allocate memory to store texture image data. */
        loadedTexture = (unsigned char*) malloc(bitmapInfoHeader.biSizeImage);

        ASSERT(loadedTexture != NULL, "Could not allocate memory to store texture image data.");

        /* Move the file pointer to the begging of the bitmap image data. */
        fseek(file, bitmapFileHeader.bfOffBits, 0);

        /* Read in the image data. */
        fread(loadedTexture, bitmapInfoHeader.biSizeImage, 1, file);

        unsigned char tempElement;

        /* As data in bmp file is stored in BGR, we need to convert it into RGB. */
        for (unsigned int imageIdx  = 0;
                          imageIdx  < bitmapInfoHeader.biSizeImage;
                          imageIdx += 3)
        {
            tempElement                 = loadedTexture[imageIdx];
            loadedTexture[imageIdx]     = loadedTexture[imageIdx + 2];
            loadedTexture[imageIdx + 2] = tempElement;
        }

        /* At the end, close the file. */
        fclose(file);

        /* Return retrieved data. */
        *textureDataPtrPtr = loadedTexture;

        /* Store the image dimensions if requested. */
        if (imageHeightPtr != NULL)
        {
            *imageHeightPtr = bitmapInfoHeader.biHeight;
        }

        if (imageWidthPtr != NULL)
        {
            *imageWidthPtr = bitmapInfoHeader.biWidth;
        }
    }

    /* Please see header for the specification. */
    void Texture::readBitmapFileHeader(FILE* filePtr, tagBITMAPFILEHEADER* bitmapFileHeaderPtr)
    {
        ASSERT(filePtr             != NULL &&
               bitmapFileHeaderPtr != NULL,
               "Invalid arguments used to read bitmap file header.");

        fread(&bitmapFileHeaderPtr->bfType,      sizeof(bitmapFileHeaderPtr->bfType),      1, filePtr);
        fread(&bitmapFileHeaderPtr->bfSize,      sizeof(bitmapFileHeaderPtr->bfSize),      1, filePtr);
        fread(&bitmapFileHeaderPtr->bfReserved1, sizeof(bitmapFileHeaderPtr->bfReserved1), 1, filePtr);
        fread(&bitmapFileHeaderPtr->bfReserved2, sizeof(bitmapFileHeaderPtr->bfReserved2), 1, filePtr);
        fread(&bitmapFileHeaderPtr->bfOffBits,   sizeof(bitmapFileHeaderPtr->bfOffBits),   1, filePtr);

        /* Make sure that file type is valid. */
        ASSERT(bitmapFileHeaderPtr->bfType == 0x4D42,
               "Invalid file type read");
    }

    /* Please see header for the specification. */
    void Texture::readBitmapInforHeader(FILE* filePtr, tagBITMAPINFOHEADER* bitmapInfoHeaderPtr)
    {
        ASSERT(filePtr != NULL &&
               bitmapInfoHeaderPtr != NULL,
               "Invalid arguments used to read bitmap info header.");

        fread(&bitmapInfoHeaderPtr->biSize,          sizeof(bitmapInfoHeaderPtr->biSize),          1, filePtr);
        fread(&bitmapInfoHeaderPtr->biWidth,         sizeof(bitmapInfoHeaderPtr->biWidth),         1, filePtr);
        fread(&bitmapInfoHeaderPtr->biHeight,        sizeof(bitmapInfoHeaderPtr->biHeight),        1, filePtr);
        fread(&bitmapInfoHeaderPtr->biPlanes,        sizeof(bitmapInfoHeaderPtr->biPlanes),        1, filePtr);
        fread(&bitmapInfoHeaderPtr->biBitCount,      sizeof(bitmapInfoHeaderPtr->biBitCount),      1, filePtr);
        fread(&bitmapInfoHeaderPtr->biCompression,   sizeof(bitmapInfoHeaderPtr->biCompression),   1, filePtr);
        fread(&bitmapInfoHeaderPtr->biSizeImage,     sizeof(bitmapInfoHeaderPtr->biSizeImage),     1, filePtr);
        fread(&bitmapInfoHeaderPtr->biXPelsPerMeter, sizeof(bitmapInfoHeaderPtr->biXPelsPerMeter), 1, filePtr);
        fread(&bitmapInfoHeaderPtr->biYPelsPerMeter, sizeof(bitmapInfoHeaderPtr->biYPelsPerMeter), 1, filePtr);
        fread(&bitmapInfoHeaderPtr->biClrUsed,       sizeof(bitmapInfoHeaderPtr->biClrUsed),       1, filePtr);
        fread(&bitmapInfoHeaderPtr->biClrImportant,  sizeof(bitmapInfoHeaderPtr->biClrImportant),  1, filePtr);
    }
}
