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

#include <GLES2/gl2ext.h>
#include <cstdio>
#include <cstdlib>
#include <android/log.h>

#define CHANNELS_PER_PIXEL  3

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* [loadTexture] */
void loadTexture( const char * texture, unsigned int level, unsigned int width, unsigned int height)
{
    GLubyte * theTexture;
    theTexture = (GLubyte *)malloc(sizeof(GLubyte) * width * height * CHANNELS_PER_PIXEL);

    FILE * theFile = fopen(texture, "r");

    if(theFile == NULL)
    {
        LOGE("Failure to load the texture");
        return;
    }

    fread(theTexture, width * height * CHANNELS_PER_PIXEL, 1, theFile);

    /* Load the texture. */
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, theTexture);

    /* Set the filtering mode. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    free(theTexture);
}
/* [loadTexture] */

/* [loadCompressedTexture] */
void loadCompressedTexture( const char * texture, unsigned int level)
{
    GLushort paddedWidth;
    GLushort paddedHeight;
    GLushort width;
    GLushort height;
    GLubyte textureHead[16];
    GLubyte * theTexture;

    FILE * theFile = fopen(texture, "rb");

    if(theFile == NULL)
    {
        LOGE("Failure to load the texture");
        return;
    }

    fread(textureHead, 16, 1, theFile);

    paddedWidth = (textureHead[8] << 8) | textureHead[9];
    paddedHeight = (textureHead[10] << 8) | textureHead[11];
    width = (textureHead[12] << 8) | textureHead[13];
    height = (textureHead[14] << 8) | textureHead[15];

    theTexture = (GLubyte *)malloc(sizeof(GLubyte) * ((paddedWidth * paddedHeight) >> 1));
    fread(theTexture, (paddedWidth * paddedHeight) >> 1, 1, theFile);

    /* Load the texture. */
    glCompressedTexImage2D(GL_TEXTURE_2D, level, GL_ETC1_RGB8_OES, width, height, 0, (paddedWidth * paddedHeight) >> 1, theTexture);

    /* Set the filtering mode. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    free(theTexture);
    fclose(theFile);
}
/* [loadCompressedTexture] */
