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
 * \file ETCAtlasAlpha.cpp
 * \brief A sample to show how to use textures with the alpha channel as part of the texture atlas.
 *
 * ETC does not support alpha channels directly.
 * Here we use a texture which orginally contained an alpha channel but
 * was compressed using the Mali Texture Compression Tool using 
 * the "Create atlas" option for alpha handling.
 * This makes an ETC compressed image containing both the RGB and alpha channels. However, the 
 * alpha channel is stored below the RGB image forming a texture atlas.
 * In this sample the atlas images are loaded and the RGB and Alpha components are merged back
 * together in the fragment shader.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <sstream>

#include <jni.h>
#include <android/log.h> 
 
#include "ETCAtlasAlpha.h"
#include "Shader.h"
#include "Texture.h"
#include "ETCHeader.h"
#include "AndroidPlatform.h"

using std::stringstream;
using std::string;
using namespace MaliSDK;
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.etcatlasalpha/";
string textureFilename = "good_atlas_mip_";
string imageExtension = ".pkm";

string vertexShaderFilename = "ETCAtlasAlpha_atlastex.vert";
string fragmentShaderFilename = "ETCAtlasAlpha_atlastex.frag";

/* Texture variables. */
GLuint textureID = 0;

/* Shader variables. */
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocTexCoord = -1;
GLint iLocSampler = -1;

bool setupGraphics(int width, int height)
{
    LOGD("setupGraphics(%d, %d)", width, height);

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

    /* Initialize textures. Using an atlas, load atlas and all mipmap levels from files. */
    Texture::loadCompressedMipmaps(texturePath.c_str(), imageExtension.c_str(), &textureID);

    /* Process shaders. */
    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;
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
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcatlasalpha_ETCAtlasAlpha_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        /* Load all "ATLAS" assets from 0 to 8 */
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

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcatlasalpha_ETCAtlasAlpha_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcatlasalpha_ETCAtlasAlpha_uninit
    (JNIEnv *, jclass)
    {

    }
}
