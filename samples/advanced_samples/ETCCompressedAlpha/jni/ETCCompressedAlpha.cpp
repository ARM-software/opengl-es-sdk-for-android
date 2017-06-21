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
 * \file ETCCompressedAlpha.cpp
 * \brief A sample to show how to use textures with an seperate compressed image for alpha.
 *
 * ETC does not support alpha channels directly.
 * Here we use a texture which orginally contained an alpha channel but
 * was compressed using the Mali Texture Compression Tool using 
 * the "Create seperate compressed image" option for alpha handling.
 * This makes an ETC compressed image for the RGB channels and a seperate compressed image for 
 * the Alpha channel.
 * In this sample both images are loaded and the RGB and Alpha components are merged back
 * together in the fragment shader.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <sstream>

#include <jni.h>
#include <android/log.h> 
 
#include "ETCCompressedAlpha.h"
#include "Shader.h"
#include "Texture.h"
#include "AndroidPlatform.h"

using std::stringstream;
using std::string;
using namespace MaliSDK;

string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.etccompressedalpha/";
string textureFilename = "good_compressed_mip_";
string imageExtension = ".pkm";
string alphaExtension = "_alpha.pkm";

string vertexShaderFilename = "ETCCompressedAlpha_dualtex.vert";
string fragmentShaderFilename = "ETCCompressedAlpha_dualtex.frag";

/* Texture variables. */
GLuint textureID = 0;
GLuint alphaTextureID = 0;

/* Shader variables. */
GLuint vertexShaderID = 0;
GLuint fragmentShaderID = 0;
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocTexCoord = -1;
GLint iLocSampler = -1;

GLint iLocAlphaSampler= -1;

bool setupGraphics(int w, int h)
{
    LOGD("setupGraphics(%d, %d)", w, h);

    /* Full paths to the shader and texture files */
    string texturePath = resourceDirectory + textureFilename;
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialize OpenGL ES. */
    /* Check which formats are supported. */
    if (!Texture::isETCSupported(true))
    {
        LOGE("ETC1 not supported");
        return false;
    }

    /* Enable alpha blending. */
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize textures using separate files */
    Texture::loadCompressedMipmaps(texturePath.c_str(), imageExtension.c_str(), &textureID);
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    Texture::loadCompressedMipmaps(texturePath.c_str(), alphaExtension.c_str(), &alphaTextureID);

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
        LOGE("Error: Attribute not found: \"a_v4Position\"");
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

    /* Set the alpha sampler to point at the 1st texture unit. */
    iLocAlphaSampler = GL_CHECK(glGetUniformLocation(programID, "u_s2dAlpha"));
    if(iLocAlphaSampler == -1)
    {
        LOGE("Uniform not found at line %i\n", __LINE__);
        exit(1);
    }
    GL_CHECK(glUniform1i(iLocAlphaSampler,1));


    /* Set clear screen color. */
    GL_CHECK(glClearColor(0.125f, 0.25f, 0.5f, 1.0));

    return true;
}

void renderFrame(void)
{
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    GL_CHECK(glUseProgram(programID));

    /* Pass the plane vertices to the shader. */
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, vertices));

    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    if(iLocTexCoord != -1)
    {
        /* Pass the texture coordinates to the shader. */
        GL_CHECK(glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates));
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(indices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, indices));
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etccompressedalpha_ETCCompressedAlpha_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        /* Load all image assets and alpha files from 0 to 8 */
        string texturePathFull = textureFilename + "0" + imageExtension;
        string alphaPathFull = textureFilename + "0" + alphaExtension;
        int numberOfImages = 9;
        for(int allImages = 0; allImages < numberOfImages; allImages++)
        {
            stringstream imageNumber;
            imageNumber << allImages;
            texturePathFull.replace(textureFilename.length(), 1, imageNumber.str());
            alphaPathFull.replace(textureFilename.length(), 1, imageNumber.str());
            AndroidPlatform::getAndroidAsset(env,  resourceDirectory.c_str(), texturePathFull.c_str());
            AndroidPlatform::getAndroidAsset(env,  resourceDirectory.c_str(), alphaPathFull.c_str());
        }

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etccompressedalpha_ETCCompressedAlpha_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etccompressedalpha_ETCCompressedAlpha_uninit
    (JNIEnv *, jclass)
    {

    }
}
