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

/** 
 * \file ETCMipmap.cpp
 * \brief A sample to show how to use the various mipmap options
 * in OpenGL ES 2.0.
 *
 * The sample demonstrates three techniques controlled by 
 * the LOAD_MIPMAPS and DISABLE_MIPMAPS defines: 
 * - loading compressed mipmaps from a file (\#define LOAD_MIPMAPS \#undef DISABLE_MIPMAPS),
 * - loading a compressed base image from a file and using glGenerateMipmap() to generate the mipmap levels (\#undef LOAD_MIPMAPS \#undef DISABLE_MIPMAPS),
 * - loading a compressed base image from a file and disabling mipmaps (\#undef LOAD_MIPMAPS \#define DISABLE_MIPMAPS).
 */

#define LOAD_MIPMAPS
#undef DISABLE_MIPMAPS

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <sstream>

#include <jni.h>
#include <android/log.h>

#include "ETCMipmap.h"
#include "Shader.h"
#include "Text.h"
#include "Texture.h"
#include "ETCHeader.h"
#include "AndroidPlatform.h"

using std::stringstream;
using std::string;
using namespace MaliSDK;

string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.etcmipmap/";
string textureFilename = "good_mip_";
string imageExtension = ".pkm";

string vertexShaderFilename = "ETCMipmap_texture.vert";
string fragmentShaderFilename = "ETCMipmap_texture.frag";

/* Texture variables. */
GLuint textureID = 0;

/* Shader variables. */
GLuint vertexShaderID = 0;
GLuint fragmentShaderID = 0;
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocTexCoord = -1;
GLint iLocSampler = -1;

/* A text object to draw text on the screen. */
Text *text;

bool setupGraphics(int w, int h)
{
    LOGD("setupGraphics(%d, %d)", w, h);

    /* Full paths to the shader and texture files */
    string texturePath = resourceDirectory + textureFilename;
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glCullFace(GL_BACK));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Check which formats are supported. */
    if (!Texture::isETCSupported(true))
    {
        LOGE("ETC1 not supported");
        return false;
    }

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), w, h);
    text->addString(0, 0, "Mipmapped ETC1 compressed texture", 255, 255, 0, 255);

    /* 
     * Initialize textures. 
     * For a texture to be considered 'complete' by OpenGL-ES you must either:
     * auto-generate the Mipmap levels using glGenerateMipmap(GL_TEXTURE_2D), or
     * disable Mipmap for minimised textures using
     * glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST)
     * as Mipmap is on by default.
     */
#ifdef LOAD_MIPMAPS
    /* Load all Mipmap levels from files. */
    Texture::loadCompressedMipmaps(texturePath.c_str(), imageExtension.c_str(), &textureID);
#else /* LOAD_MIPMAPS */
    /* Load just base level texture data. */
    GL_CHECK(glGenTextures(1, &textureID));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));
    string mainTexturePath = texturePath + "0" + imageExtension;
    unsigned char *textureData;
    Texture::loadData(mainTexturePath.c_str(), &textureData);
    ETCHeader loadedETCHeader = ETCHeader(textureData);
    GL_CHECK(glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES,
             loadedETCHeader.getWidth(), loadedETCHeader.getHeight(), 0,
             loadedETCHeader.getPaddedWidth() * loadedETCHeader.getPaddedHeight() >> 1,
             textureData + 16));
    free(textureData);

#    ifdef DISABLE_MIPMAPS
    /* Disable Mipmaps. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
#    else /* DISABLE_MIPMAPS */
    /* Auto generate Mipmaps. */
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
#    endif /* DISABLE_MIPMAPS */
#endif /* LOAD_MIPMAPS */

    /* Process shaders. */
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    LOGD("vertexShaderID = %d", vertexShaderID);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);
    LOGD("fragmentShaderID = %d", fragmentShaderID);
    programID = GL_CHECK(glCreateProgram());
    if (!programID) 
    {
        LOGE("Could not create program.");
        return false;
    }
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));

    /* Vertex positions. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "a_v4Position"));
    if(iLocPosition == -1)
    {
        LOGE("Attribute not found: \"a_v4Position\"");
        exit(1);
    }
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Texture coordinates. */
    iLocTexCoord = GL_CHECK(glGetAttribLocation(programID, "a_v2TexCoord"));
    if(iLocTexCoord == -1)
    {
        LOGD("Warning: Attribute not found: \"a_v2TexCoord\"");
    }
    else
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    /* Set the sampler to point at the 0th texture unit. */
    iLocSampler = GL_CHECK(glGetUniformLocation(programID, "u_s2dTexture"));
    if(iLocSampler == -1)
    {
        LOGD("Warning: Uniform not found: \"u_s2dTexture\"");
    }
    else
    {
        GL_CHECK(glUniform1i(iLocSampler, 0));
    }

    /* Set clear screen color. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0));

    return true;
}

void renderFrame(void)
{
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL_CHECK(glUseProgram(programID));

    /* Pass the plane vertices to the shader. */
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices));

    if(iLocTexCoord != -1)
    {
        /* Pass the texture coordinates to the shader. */
        GL_CHECK(glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates));
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    /* Ensure the correct texture is bound to texture unit 0. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));

    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(indices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, indices));

    /* Draw fonts. */
    text->draw();
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcmipmap_ETCMipmap_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());
        
        /* Load all image assets from 0 to 8 */
        string texturePathFull = textureFilename + "0" + imageExtension;        
        int numberOfImages = 9;
        for(int allImages = 0; allImages < numberOfImages; allImages++)
        {
            stringstream imageNumber;
            imageNumber << allImages;
            texturePathFull.replace(textureFilename.length(), 1, imageNumber.str());
            AndroidPlatform::getAndroidAsset(env,  resourceDirectory.c_str(), texturePathFull.c_str());
        }

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcmipmap_ETCMipmap_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcmipmap_ETCMipmap_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
