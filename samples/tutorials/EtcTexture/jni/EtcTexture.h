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

#ifndef ETCTEXTURE_H
#define ETCTEXTURE_H

namespace MaliSDK
{
    /** Name of a fragment shader file. */
    #define FRAGMENT_SHADER_FILE_NAME                                      ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/fragment_shader_source.frag")
    /** Name of a fragment shader file that will be used to render text. */
    #define FONT_FRAGMENT_SHADER_FILE_NAME                                 ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/font.frag")
    /** Name of a image file that will be used to render text. */
    #define FONT_TEXTURE_FILE_NAME                                         ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/font.raw")
    /** Name of a vertex shader file that will be used to render text. */
    #define FONT_VERTEX_SHADER_FILE_NAME                                   ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/font.vert")
    /** Name of a GL_COMPRESSED_R11_EAC texture file. */
    #define TEXTURE_GL_COMPRESSED_R11_EAC_FILE_NAME                        ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/HeightMap.pkm")
    /** Name of a GL_COMPRESSED_SIGNED_R11_EAC texture file. */
    #define TEXTURE_GL_COMPRESSED_SIGNED_R11_EAC_FILE_NAME                 ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/HeightMapSigned.pkm")
    /** Name of a GL_COMPRESSED_RG11_EAC texture file. */
    #define TEXTURE_GL_COMPRESSED_RG11_EAC_FILE_NAME                       ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/BumpMap.pkm")
    /** Name of a GL_COMPRESSED_SIGNED_RG11_EAC texture file. */
    #define TEXTURE_GL_COMPRESSED_SIGNED_RG11_EAC_FILE_NAME                ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/BumpMapSigned.pkm")
    /** Name of a GL_COMPRESSED_RGB8_ETC2 texture file. */
    #define TEXTURE_GL_COMPRESSED_RGB8_ETC2_FILE_NAME                      ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/Texture.pkm")
    /** Name of a GL_COMPRESSED_SRGB8_ETC2 texture file. */
    #define TEXTURE_GL_COMPRESSED_SRGB8_ETC2_FILE_NAME                     ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/Texture.pkm")
    /** Name of a GL_COMPRESSED_RGBA8_ETC2_EAC texture file. */
    #define TEXTURE_GL_COMPRESSED_RGBA8_ETC2_EAC_FILE_NAME                 ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/SemiAlpha.pkm")
    /** Name of a GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EA texture file. */
    #define TEXTURE_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC_FILE_NAME          ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/SemiAlpha.pkm")
    /** Name of a GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 texture file. */
    #define TEXTURE_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2_FILE_NAME  ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/BinaryAlpha.pkm")
    /** Name of a GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 texture file. */
    #define TEXTURE_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/BinaryAlpha.pkm")
    /** Name of a vertex shader file. */
    #define VERTEX_SHADER_FILE_NAME                                        ("/data/data/com.arm.malideveloper.openglessdk.etcTexture/files/vertex_shader_source.vert")
}
#endif /* ETCTEXTURE_H */
