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

#include <GLES3/gl3.h>
#include "Common.h"
#include "Texture.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace MaliSDK
{
    void Texture::getCompressedTextureFormats(GLint** textureFormats, int* numberOfTextureFormats)
    {
        GL_CHECK(glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, numberOfTextureFormats));

        *textureFormats = (GLint *)calloc(*numberOfTextureFormats, sizeof(GLint));
        if(*textureFormats == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }

        GL_CHECK(glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, *textureFormats));
    }

    bool Texture::isETCSupported(bool verbose)
    {
        return true;
    }

    void Texture::createTexture(unsigned int width, unsigned int height, GLvoid **textureData)
    {
        unsigned char *randomTexture = new unsigned char [width * height * 4];
        if(randomTexture == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }

        /* Initialize texture with random shades. */
        for(unsigned int allTexels = 0; allTexels < width * height; allTexels ++)
        {
            /* Use 255 (fully opaque) for the alpha value */
            randomTexture[allTexels * 4 + 3] = 255;
            /* Set each colour component (Red, Green, Blue) of the texel to a different random number. */
            for (int allChannels = 0; allChannels < 3; allChannels++)
            {
                /* Generate a random number between 0 and 255 */
                int randomNumber = (int)(255 * (rand() / (float)RAND_MAX));
                randomTexture[allTexels * 4 + allChannels] = randomNumber;
            }
        }

        *textureData = randomTexture;
    }

    void Texture::createTexture(unsigned int width, unsigned int height, unsigned int red, GLvoid **textureData)
    {
        unsigned char* newTexture = new unsigned char [width * height];

        if(newTexture == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }

        for (unsigned int texelIndex = 0; texelIndex < width * height; ++texelIndex)
        {
            newTexture[texelIndex] = red;
        }
        *textureData = newTexture;
    }

    void Texture::createTexture(unsigned int width, unsigned int height, short red, short **textureData)
    {
        *textureData = new short [width * height];

        if (*textureData == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }

        for (unsigned int texelIndex = 0; texelIndex < width * height; ++texelIndex)
        {
            (*textureData)[texelIndex] = red;
        }
    }

    void Texture::deleteTextureData(GLvoid** textureData)
    {
        delete[] (unsigned char*)*textureData;
    }

    /* [Load data from file] */
    void Texture::loadData(const char *filename, unsigned char **textureData)
    {
        FILE *file = fopen(filename, "rb");

        if(file == NULL)
        {
            LOGE("Failed to open '%s'\n", filename);
            exit(1);
        }

        fseek(file, 0, SEEK_END);

        unsigned int   length        = ftell(file);
        unsigned char *loadedTexture = (unsigned char *)calloc(length, sizeof(unsigned char));

        ASSERT(loadedTexture != NULL, "Could not allocate memory to store PKM file data.")

        fseek(file, 0, SEEK_SET);

        size_t read = fread(loadedTexture, sizeof(unsigned char), length, file);

        ASSERT(read == length, "Failed to read PKM file.");

        fclose(file);

        *textureData = loadedTexture;
    }
    /* [Load data from file] */

    void Texture::reversePixelLine(float* destination, const float* source, int lineWidth)
    {
        const int rgbComponentsCount = 3;

        for (int pixelIndex = 0; pixelIndex < lineWidth; ++pixelIndex)
        {
            memcpy(destination + pixelIndex * rgbComponentsCount,
                   source + (lineWidth - pixelIndex - 1) * rgbComponentsCount, 
                   rgbComponentsCount * sizeof(float));
        }
    }
}
