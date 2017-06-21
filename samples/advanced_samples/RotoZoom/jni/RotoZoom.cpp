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

 
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>

#include <jni.h>
#include <android/log.h>
 
#include "RotoZoom.h"
#include "Timer.h"
#include "Text.h"
#include "Texture.h"
#include "Shader.h"
#include "Matrix.h"
#include "Platform.h"
#include "Mathematics.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.rotozoom/";
string textureFilename = "RotoZoom.raw";
string vertexShaderFilename = "RotoZoom_cube.vert";
string fragmentShaderFilename = "RotoZoom_cube.frag";

/* Texture variables. */
GLuint textureID = 0;

/* Shader variables. */
GLuint programID = 0;
GLint iLocTextureMatrix = -1;
GLint iLocPosition = -1;
GLint iLocTextureMix = -1;
GLint iLocTexture = -1;
GLint iLocTexCoord = -1;

/* Animation variables. */
Matrix translation;
Matrix scale;
Matrix negativeTranslation;

int windowWidth = -1;
int windowHeight = -1;

/* A text object to draw text on the screen. */
Text* text;

bool setupGraphics(int width, int height)
{
    /* Height and width of the texture being used. */
    int textureWidth = 256;
    int textureHeight = 256;
    
    windowWidth = width;
    windowHeight = height;
    
    /* Full paths to the shader and texture files */
    string texturePath = resourceDirectory + textureFilename;   
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialize matrices. */
    /* Make scale matrix to centre texture on screen. */
    translation = Matrix::createTranslation(0.5f, 0.5f, 0.0f);
    scale = Matrix::createScaling(width / (float)textureWidth, height / (float)textureHeight, 1.0f); /* 2.0 makes it smaller, 0.5 makes it bigger. */
    negativeTranslation = Matrix::createTranslation(-0.5f, -0.5f, 0.0f);

    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glCullFace(GL_BACK));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), windowWidth, windowHeight);
    text->addString(0, 0, "Simple RotoZoom Example", 255, 255, 255, 255);

    /* Load just base level texture data. */
    GL_CHECK(glGenTextures(1, &textureID));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));
    unsigned char *textureData = NULL;
    Texture::loadData(texturePath.c_str(), &textureData);
        
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData));        

    /* Set texture mode. */
    GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)); /* Default anyway. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

    /* Process shaders. */
    GLuint vertexShaderID = 0;
    GLuint fragmentShaderID = 0;
    Shader::processShader(&vertexShaderID, vertexShaderPath.c_str(), GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

    /* Set up shaders. */
    programID = GL_CHECK(glCreateProgram());
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));
    GL_CHECK(glLinkProgram(programID));
    GL_CHECK(glUseProgram(programID));

    /* Vertex positions. */
    iLocPosition = GL_CHECK(glGetAttribLocation(programID, "a_v4Position"));
    if(iLocPosition == -1)
    {
        LOGE("Attribute not found at %s:%i\n", __FILE__, __LINE__);
        return false;
    }
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));

    /* Texture. */
    iLocTexture = GL_CHECK(glGetUniformLocation(programID, "u_s2dTexture"));
    if(iLocTexture == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glUniform1i(iLocTexture, 0));
    }

    /* Texture coordinates. */
    iLocTexCoord = GL_CHECK(glGetAttribLocation(programID, "a_v2TexCoord"));
    if(iLocTexCoord == -1)
    {
        LOGD("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    /* Texture matrix. */
    iLocTextureMatrix = GL_CHECK(glGetUniformLocation(programID, "u_m4Texture"));
    if(iLocTextureMatrix == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else
    {
        GL_CHECK(glUniformMatrix4fv(iLocTextureMatrix, 1, GL_FALSE, scale.getAsArray()));
    }
    
    return true;
}

void renderFrame(void)
{
    static float angleZTexture = 0.0f;
    static float angleZOffset = 0.0f;
    static float angleZoom = 0.0f;
    static Vec4f radius = {0.0f, 1.0f, 0.0f, 1.0f};

    /* Select our shader program. */
    GL_CHECK(glUseProgram(programID));

    /* Set up vertex positions. */
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, quadVertices));

    /* And texture coordinate data. */
    if(iLocTexCoord != -1)
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
        GL_CHECK(glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, quadTextureCoordinates));
    }

    /* Reset viewport to the EGL window surface's dimensions. */
    GL_CHECK(glViewport(0, 0, windowWidth, windowHeight));

    /* Clear the screen on the EGL surface. */
    GL_CHECK(glClearColor(1.0f, 1.0f, 0.0f, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Construct a rotation matrix for rotating the texture about its centre. */
    Matrix rotateTextureZ = Matrix::createRotationZ(angleZTexture);

    Matrix rotateOffsetZ = Matrix::createRotationZ(angleZOffset);

    Vec4f offset = Matrix::vertexTransform(&radius, &rotateOffsetZ);

    /* Construct offset translation. */
    Matrix translateTexture = Matrix::createTranslation(offset.x, offset.y, offset.z);

    /* Construct zoom matrix. */
    Matrix zoom = Matrix::createScaling(sinf(degreesToRadians(angleZoom)) * 0.75f + 1.25f, sinf(degreesToRadians(angleZoom)) * 0.75f + 1.25f, 1.0f);

    /* Create texture matrix. Operations happen bottom-up order. */
    Matrix textureMovement = Matrix::identityMatrix * translation; /* Translate texture back to original position. */
    textureMovement = textureMovement * rotateTextureZ;            /* Rotate texture about origin. */
    textureMovement = textureMovement * translateTexture;          /* Translate texture away from origin. */
    textureMovement = textureMovement * zoom;                      /* Zoom the texture. */
    textureMovement = textureMovement * scale;                     /* Scale texture down in size from fullscreen to 1:1. */
    textureMovement = textureMovement * negativeTranslation;       /* Translate texture to be centred on origin. */

    GL_CHECK(glUniformMatrix4fv(iLocTextureMatrix, 1, GL_FALSE, textureMovement.getAsArray()));

    /* Ensure the correct texture is bound to texture unit 0. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, textureID));

    /* And draw. */
    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(quadIndices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, quadIndices));

    /* Draw any text. */
    text->draw();

    /* Update rotation angles for animating. */
    angleZTexture += 1;
    angleZOffset += 1;
    angleZoom += 1;

    if(angleZTexture >= 360) angleZTexture -= 360;
    if(angleZTexture < 0) angleZTexture += 360;

    if(angleZOffset >= 360) angleZOffset -= 360;
    if(angleZOffset < 0) angleZOffset += 360;

    if(angleZoom >= 360) angleZoom -= 360;
    if(angleZoom < 0) angleZoom += 360;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_rotozoom_RotoZoom_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), textureFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_rotozoom_RotoZoom_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_rotozoom_RotoZoom_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
