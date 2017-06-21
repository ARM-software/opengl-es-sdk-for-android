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

#include <GLES2/gl2.h>

/**
 * \brief Loads a desired texture into memory at an appropriate mipmap level.
 * \param texture The name of the texture file to be loaded from the system.
 * \param level The mipmap level that the texture should be loaded into.
 * \param width The width of the texture to be loaded
 * \param height The height of the texture to be loaded.
 */
void loadTexture( const char * texture, unsigned int level, unsigned int width, unsigned int height);

/**
 * \brief Loads a compressed texture into memory at an appropriate mipmap level.
 * \param texture The name of the texture file to be loeaded from the system.
 * \param level The mipmap level that the texture should be loaded into.
 */
void loadCompressedTexture( const char * texture, unsigned int level);
#endif
