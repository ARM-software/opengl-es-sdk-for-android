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

/**
 * \file FrameBufferObject.cpp
 * \brief A sample which shows how to use frame buffer objects.
 *
 * A cube is rendered into a frame buffer object rather than to the 
 * default frame buffer. This frame buffer object is then used as a texture
 * for another spinning cube.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

#include <jni.h>
#include <android/log.h> 
 
#include "FrameBufferObject.h"
#include "Text.h"
#include "Shader.h"
#include "Texture.h"
#include "Matrix.h"
#include "AndroidPlatform.h"

using std::string;
using namespace MaliSDK;

/* Asset directories and filenames. */
string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.framebufferobject/";
string vertexShaderFilename = "FrameBufferObject_cube.vert";
string fragmentShaderFilename = "FrameBufferObject_cube.frag";

#define FBO_WIDTH    256
#define FBO_HEIGHT    256

/* Shader variables. */
GLuint vertexShaderID = 0;
GLuint fragmentShaderID = 0;
GLuint programID = 0;
GLint iLocPosition = -1;
GLint iLocTextureMix = -1;
GLint iLocTexture = -1;
GLint iLocFillColor = -1;
GLint iLocTexCoord = -1;
GLint iLocProjection = -1;
GLint iLocModelview = -1;

/* Animation variables. */
static float angleX = 0;
static float angleY = 0;
static float angleZ = 0;
Matrix rotationX;
Matrix rotationY;
Matrix rotationZ;
Matrix translation;
Matrix modelView;
Matrix projection;
Matrix projectionFBO;

/* Framebuffer variables. */
GLuint iFBO = 0;

/* Application textures. */
GLuint iFBOTex = 0;

int windowWidth = -1;
int windowHeight = -1;

/* A text object to draw text on the screen. */
Text* text;

bool setupGraphics(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    /* Full paths to the shader files */
    string vertexShaderPath = resourceDirectory + vertexShaderFilename; 
    string fragmentShaderPath = resourceDirectory + fragmentShaderFilename;

    /* Initialize matrices. */
    projection = Matrix::matrixPerspective(45.0f, windowWidth/(float)windowHeight, 0.01f, 100.0f);
    projectionFBO = Matrix::matrixPerspective(45.0f, (FBO_WIDTH / (float)FBO_HEIGHT), 0.01f, 100.0f);
    /* Move cube 2 further away from camera. */
    translation = Matrix::createTranslation(0.0f, 0.0f, -2.0f);

    /* Initialize OpenGL ES. */
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glCullFace(GL_BACK));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_BLEND));
    /* Should do src * (src alpha) + dest * (1-src alpha). */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Initialize the Text object and add some text. */
    text = new Text(resourceDirectory.c_str(), windowWidth, windowHeight);
    text->addString(0, 0, "Simple FrameBuffer Object (FBO) Example", 255, 255, 0, 255);

    /* Initialize FBO texture. */
    GL_CHECK(glGenTextures(1, &iFBOTex));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, iFBOTex));
    /* Set filtering. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));

    /* Initialize FBOs. */
    GL_CHECK(glGenFramebuffers(1, &iFBO));

    /* Render to framebuffer object. */
    /* Bind our framebuffer for rendering. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, iFBO));

    /* Attach texture to the framebuffer. */
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iFBOTex, 0));

    /* Check FBO is OK. */
    GLenum iResult = GL_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    if(iResult != GL_FRAMEBUFFER_COMPLETE)
    {
        LOGE("Framebuffer incomplete at %s:%i\n", __FILE__, __LINE__);
        return false;
    }

    /* Unbind framebuffer. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    /* Process shaders. */
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

    /* Texture mix. */
    iLocTextureMix = GL_CHECK(glGetUniformLocation(programID, "u_fTex"));
    if(iLocTextureMix == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glUniform1f(iLocTextureMix, 0.0));
    }

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

    /* Vertex colors. */
    iLocFillColor = GL_CHECK(glGetAttribLocation(programID, "a_v4FillColor"));
    if(iLocFillColor == -1)
    {
        LOGD("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
    }

    /* Texture coords. */
    iLocTexCoord = GL_CHECK(glGetAttribLocation(programID, "a_v2TexCoord"));
    if(iLocTexCoord == -1)
    {
        LOGD("Warning: Attribute not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
    }

    /* Projection matrix. */
    iLocProjection = GL_CHECK(glGetUniformLocation(programID, "u_m4Projection"));
    if(iLocProjection == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    else 
    {
        GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, projection.getAsArray()));
    }

    /* Modelview matrix. */
    iLocModelview = GL_CHECK(glGetUniformLocation(programID, "u_m4Modelview"));
    if(iLocModelview == -1)
    {
        LOGD("Warning: Uniform not found at %s:%i\n", __FILE__, __LINE__);
    }
    /* We pass this for each object, below. */

    return true;
}

void renderFrame(void)
{
    /* Both main window surface and FBO use the same shader program. */
    GL_CHECK(glUseProgram(programID));

    /* Both drawing surfaces also share vertex data. */
    GL_CHECK(glEnableVertexAttribArray(iLocPosition));
    GL_CHECK(glVertexAttribPointer(iLocPosition, 3, GL_FLOAT, GL_FALSE, 0, cubeVertices));

    /* Including color data. */
    if(iLocFillColor != -1)
    {
        GL_CHECK(glEnableVertexAttribArray(iLocFillColor));
        GL_CHECK(glVertexAttribPointer(iLocFillColor, 4, GL_FLOAT, GL_FALSE, 0, cubeColors));
    }

    /* And texture coordinate data. */
    if(iLocTexCoord != -1)
    {
        GL_CHECK(glEnableVertexAttribArray(iLocTexCoord));
        GL_CHECK(glVertexAttribPointer(iLocTexCoord, 2, GL_FLOAT, GL_FALSE, 0, cubeTextureCoordinates));
    }

    /* Bind the FrameBuffer Object. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, iFBO));

    /* Set the viewport according to the FBO's texture. */
    GL_CHECK(glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT));

    /* Clear screen on FBO. */
    GL_CHECK(glClearColor(0.5f, 0.5f, 0.5f, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Create rotation matrix specific to the FBO's cube. */
    rotationX = Matrix::createRotationX(-angleZ);
    rotationY = Matrix::createRotationY(-angleY);
    rotationZ = Matrix::createRotationZ(-angleX);

    /* Rotate about origin, then translate away from camera. */
    modelView = translation * rotationX;
    modelView = modelView * rotationY;
    modelView = modelView * rotationZ;

    /* Load FBO-specific projection and modelview matrices. */
    GL_CHECK(glUniformMatrix4fv(iLocModelview, 1, GL_FALSE, modelView.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, projectionFBO.getAsArray()));

    /* The FBO cube doesn't get textured so zero the texture mix factor. */
    if(iLocTextureMix != -1)
    {
        GL_CHECK(glUniform1f(iLocTextureMix, 0.0));
    }

    /* Now draw the colored cube to the FrameBuffer Object. */
    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(cubeIndices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, cubeIndices));

    /* And unbind the FrameBuffer Object so subsequent drawing calls are to the EGL window surface. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,0));

    /* Reset viewport to the EGL window surface's dimensions. */
    GL_CHECK(glViewport(0, 0, windowWidth, windowHeight));

    /* Clear the screen on the EGL surface. */
    GL_CHECK(glClearColor(0.0f, 0.0f, 1.0f, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Construct different rotation for main cube. */
    rotationX = Matrix::createRotationX(angleX);
    rotationY = Matrix::createRotationY(angleY);
    rotationZ = Matrix::createRotationZ(angleZ);

    /* Rotate about origin, then translate away from camera. */
    modelView = translation * rotationX;
    modelView = modelView * rotationY;
    modelView = modelView * rotationZ;

    /* Load EGL window-specific projection and modelview matrices. */
    GL_CHECK(glUniformMatrix4fv(iLocModelview, 1, GL_FALSE, modelView.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(iLocProjection, 1, GL_FALSE, projection.getAsArray()));

    /* For the main cube, we use texturing so set the texture mix factor to 1. */
    if(iLocTextureMix != -1)
    {
        GL_CHECK(glUniform1f(iLocTextureMix, 1.0));
    }

    /* Ensure the correct texture is bound to texture unit 0. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, iFBOTex));

    /* And draw the cube. */
    GL_CHECK(glDrawElements(GL_TRIANGLE_STRIP, sizeof(cubeIndices) / sizeof(GLubyte), GL_UNSIGNED_BYTE, cubeIndices));

    /* Draw any text. */
    text->draw();

    /* Update cube's rotation angles for animating. */
    angleX += 3;
    angleY += 2;
    angleZ += 1;

    if(angleX >= 360) angleX -= 360;
    if(angleY >= 360) angleY -= 360;
    if(angleZ >= 360) angleZ -= 360;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_framebufferobject_FrameBufferObject_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Make sure that all resource files are in place. */
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), vertexShaderFilename.c_str());
        AndroidPlatform::getAndroidAsset(env, resourceDirectory.c_str(), fragmentShaderFilename.c_str());

        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_framebufferobject_FrameBufferObject_step
    (JNIEnv *env, jclass jcls)
    {
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_framebufferobject_FrameBufferObject_uninit
    (JNIEnv *, jclass)
    {
        delete text;
    }
}
