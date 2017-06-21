/* Copyright (c) 2013-2017, ARM Limited and Contributors
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

/* [includeTextureDefinition] */
#include "Texture.h"

#include <GLES2/gl2ext.h>
#include <cstdio>
#include <cstdlib>

GLuint loadSimpleTexture()
{
    /* Texture Object Handle. */
    GLuint textureId;

    /* 3 x 3 Image,  R G B A Channels RAW Format. */
    GLubyte pixels[9 * 4] =
    {
            18,  140, 171, 255, /* Some Colour Bottom Left. */
            143, 143, 143, 255, /* Some Colour Bottom Middle. */
            255, 255, 255, 255, /* Some Colour Bottom Right. */
            255, 255, 0,   255, /* Yellow Middle Left. */
            0,   255, 255, 255, /* Some Colour Middle. */
            255, 0,   255, 255, /* Some Colour Middle Right. */
            255, 0,   0,   255, /* Red Top Left. */
            0,   255, 0,   255, /* Green Top Middle. */
            0,   0,   255, 255, /* Blue Top Right. */
    };
    /* [includeTextureDefinition] */

    /* [placeTextureInMemory] */
    /* Use tightly packed data. */
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* Generate a texture object. */
    glGenTextures(1, &textureId);

    /* Activate a texture. */
    glActiveTexture(GL_TEXTURE0);

    /* Bind the texture object. */
    glBindTexture(GL_TEXTURE_2D, textureId);

    /* Load the texture. */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    /* Set the filtering mode. */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureId;
}
/* [placeTextureInMemory] */