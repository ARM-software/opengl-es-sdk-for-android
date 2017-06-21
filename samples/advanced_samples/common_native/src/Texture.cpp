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

#include "Texture.h"
#include "ETCHeader.h"
#include "Platform.h"

#if GLES_VERSION == 2
#include <GLES2/gl2ext.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using std::string;

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

#if GLES_VERSION == 2
    bool Texture::isETCSupported(bool verbose)
    {
        bool supportETC = false;
        GLint *textureFormat = NULL;
        GLint numberOfTextureFormats = 0;
        getCompressedTextureFormats(&textureFormat, &numberOfTextureFormats);

        if (verbose != 0)
        {
            LOGI("Number of texture formats supported: %d\nFormats:\n", numberOfTextureFormats);
            for(int allTextureFormats = 0; allTextureFormats < numberOfTextureFormats; allTextureFormats++)
            {
                LOGI("0x%.8x\t", textureFormat[allTextureFormats]);
                switch(textureFormat[allTextureFormats])
                {
                    case GL_ETC1_RGB8_OES:
                        LOGI("GL_ETC1_RGB8_OES\n");
                        break;
                    default:
                        LOGI("UNKNOWN\n");
                        break;
                }
            }
        }  

        for(int allTextureFormats = 0; allTextureFormats < numberOfTextureFormats; allTextureFormats++)
        {
            if (textureFormat[allTextureFormats] == GL_ETC1_RGB8_OES)
            {
                supportETC = true;
            }
        }

        free(textureFormat);
        textureFormat = NULL;

        if(!supportETC)
        {
            LOGD("Texture compression format GL_ETC1_RGB8_OES not supported\n");
        }
        return supportETC;
    }
#elif GLES_VERSION == 3
    bool Texture::isETCSupported(bool verbose)
    {
        /* TODO */
        return true;
    }
#endif

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

    void Texture::loadData(const char *filename, unsigned char **textureData)
    {
        LOGD("Texture loadData started for %s...\n", filename);

        FILE *file = fopen(filename, "rb");
        if(file == NULL)
        {
            LOGE("Failed to open '%s'\n", filename);
            exit(1);
        }
        fseek(file, 0, SEEK_END);
        unsigned int length = ftell(file);
        unsigned char *loadedTexture = (unsigned char *)calloc(length, sizeof(unsigned char));
        if(loadedTexture == NULL)
        {
            LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
            exit(1);
        }
        fseek(file, 0, SEEK_SET);
        size_t read = fread(loadedTexture, sizeof(unsigned char), length, file);
        if(read != length)
        {
            LOGE("Failed to read in '%s'\n", filename);
            exit(1);
        }
        fclose(file);

        *textureData = loadedTexture;

        LOGD("Texture loadData for %s done.\n", filename);
    }

    void Texture::loadPKMData(const char *filename, ETCHeader* etcHeader, unsigned char **textureData)
    {
        /* PKM file consists of a header with information about image (stored in 16 first bits) and image data. */
        const int       sizeOfETCHeader = 16; 
        unsigned char*  tempTextureData = NULL;

        loadData(filename, &tempTextureData);

        if (textureData == NULL)
        {
            LOGE("textureData is a NULL pointer.");
            exit(1);
        }
        if (etcHeader == NULL)
        {
            LOGE("etcHeader is a NULL pointer.");
            exit(1);
        }

        if (tempTextureData != NULL)
        {
            ETCHeader tempEtcHeader(tempTextureData);

            *etcHeader   = tempEtcHeader;
            *textureData = tempTextureData + sizeOfETCHeader;
        }
        else
        {
            LOGE("Could not load data from file %s.", filename);
            exit(1);
        }
    }

    void Texture::loadCompressedMipmaps(const char *filenameBase, const char *filenameSuffix, GLuint *textureID)
    {
        /* Allocate texture name. */
        GL_CHECK(glGenTextures(1, textureID));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, *textureID));
        
        /* Load base level Mipmap. */
        /* Construct filename, load and tidy up. */
        string filename = filenameBase + string("0") + filenameSuffix;
        unsigned char *data = NULL;
        loadData(filename.c_str(), &data);
        ETCHeader loadedETCHeader = ETCHeader(data);

        /* Calculate number of Mipmap levels. */
        LOGD("Base level Mipmap loaded: (%i, %i) padded to 4x4 blocks, (%i, %i) actual\n", loadedETCHeader.getPaddedWidth(), loadedETCHeader.getPaddedHeight(), loadedETCHeader.getWidth(), loadedETCHeader.getHeight());
        int width = loadedETCHeader.getWidth();
        int height = loadedETCHeader.getHeight();
        int numberOfMipmaps = 1;
        while((width > 1) || (height > 1))
        {
            numberOfMipmaps ++;
            if(width > 1) width >>= 1;
            if(height > 1) height >>= 1;
        }
        LOGD("Requires %i Mipmap levels in total\n", numberOfMipmaps);

        /* Load base Mipmap level into level 0 of texture.
         * Skip the 16 byte header of the PKM file before passing the data to OpenGL ES.
         * Data size (taken in number of bytes) of the texture is:
         *      Number of pixels = padded width * padded height.
         *      The number of pixels is divided by two as there are 4 bits per pixel in ETC (half a byte)
         */
#if GLES_VERSION == 2
        GL_CHECK(glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, loadedETCHeader.getWidth(), loadedETCHeader.getHeight(), 0, (loadedETCHeader.getPaddedWidth() * loadedETCHeader.getPaddedHeight()) >> 1, data + 16));
#elif GLES_VERSION == 3
        /* TODO */
#endif
        free(data);
        data = NULL;

        /* Load other levels. */
        for(int allMipmaps = 1; allMipmaps < numberOfMipmaps; allMipmaps++)
        {
            /* Construct filename, load and tidy up. */
            char *level = NULL;
            int input = allMipmaps;
            int inputLength = 0;

            /* Calculate the string length of the Mipmap level number (no snprintf in ANSI C). */
            do
            {
                input /= 10;
                inputLength ++;
            }
            while(input != 0);

            level = (char *)calloc(inputLength + 1, sizeof(char));
            if(level == NULL)
            {
                LOGE("Out of memory at %s:%i\n", __FILE__, __LINE__);
                exit(1);
            }
            sprintf(level, "%i", allMipmaps);

            filename = filenameBase + string(level) + filenameSuffix;
            loadData(filename.c_str(), &data);
            free(level);
            level = NULL;
            loadedETCHeader = ETCHeader(data);

            /* Load Mipmap level into texture.
             * Skip the 16 byte header of the PKM file before passing the data to OpenGL ES.
             * Data size (taken in number of bytes) of the texture is:
             *      Number of pixels = padded width * padded height.
             *      The number of pixels is divided by two as there are 4 bits per pixel in ETC (half a byte)
             */
#if GLES_VERSION == 2
            GL_CHECK(glCompressedTexImage2D(GL_TEXTURE_2D, allMipmaps, GL_ETC1_RGB8_OES, loadedETCHeader.getWidth(), loadedETCHeader.getHeight(), 0, (loadedETCHeader.getPaddedWidth() * loadedETCHeader.getPaddedHeight()) >> 1, data + 16));
#elif GLES_VERSION == 3
        /* TODO */
#endif
            free(data);
            data = NULL;
        }
    }

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
