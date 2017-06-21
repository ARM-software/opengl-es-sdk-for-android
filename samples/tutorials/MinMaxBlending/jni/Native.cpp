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

/**
 * \file samples/tutorials/MinMaxBlending/jni/Native.cpp
 * The application demonstrates behaviour of blending in GL_MIN and GL_MAX mode. It renders a 3D texture which consists of a
 * series of greyscaled images obtained from magnetic resonance of a human head. The images are placed one after another in
 * Z axis, so when blending is enabled they imitate a 3D model of the head.
 *
 * Texture coordinates are then rotated, so viewers can see the model from different perspectives and after each 5 seconds,
 * blending equation is changed. Since U/V/W coordinates are taken from interval <0.0, 1.0> and they are clamped to edge,
 * there might occure some distortions for specific angles of rotation. That is why, the application adds a few blank layers
 * behind and in the front of the original images. Now, if rotated coordinates exceed the interval, only the additional edge
 * layers are repeated creating a noiseless background.
 *
 * Because images contain a lot of black color, regular min blending would result in having black square on the screen. Hence,
 * there is a threshold applied in fragment shader which prevents rendering fragments that are not bright enough. Additionally,
 * for both types of blending, contrast of output luminance had to be modified to see more details.
 *
 * To use your own input images, it is check their format and adjust values of min blending threshold,
 * luminance of additional edge layers and contrast modifier.
 */

#include <jni.h>
#include <android/log.h>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "Common.h"
#include "Matrix.h"
#include "Native.h"
#include "Texture.h"
#include "Timer.h"
#include "Shader.h"

using namespace std;
using namespace MaliSDK;

/* Asset directories and filenames */
const string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.minMaxBlending/files/";
const string imagesFilename    = "MRbrain";

/* Number of images in resourceDirectory. */
const int imagesCount = 109;

/* Generic framework timer used to count the time interval for switch of blending equations. */
Timer timer;

/* Dimensions of window. */
int windowWidth  = 0;
int windowHeight = 0;

/* 3D texture dimensions. Although there are 109 images in resourceDirectory, texture depth is extended to 128 for 2 reasons:
 *     1) We require some layers in the front and behind the original ones, to avoid errors while rotating texture coordinates.
 *     2) Setting depth as a half of the other dimensions slightly improves the effect of blending. */
const GLint textureWidth  = 256;
const GLint textureHeight = 256;
const GLint textureDepth  = 128;

/* Emprically determined value of threshold used for min blending. */
const GLfloat minBlendingThreshold = 0.37f;
/* Color value of a 3D texture layer. */
const short fillerLuminance = 4;

/* ID of a 3D texture rendered on the screen. Filled by OpenGL ES. */
GLuint textureID = 0;

/* ID of a program assigned by OpenGL ES. */
GLuint programID = 0;

/* ID of a buffer object storing vertices of a square. */
GLuint verticesBufferID = 0;
/* ID of a buffer object storing U/V/W texture coordinates. */
GLuint uvwBufferID = 0;

/* ID of a vertex array object. */
GLuint vaoID = 0;

/* Locations of changable uniforms. */
GLint isMinBlendingLocation  = -1;
GLint rotationVectorLocation = -1;

/* Since there are additional layers in the front and in the back of original texture images, there
 * are two different functions used to load them. That is why we need this variable to indicate which
 * layer of 3D texture should be filled at the moment. */
GLint textureZOffset = 0;

/* Flag passed to shaders indicating current blending equation. */
GLboolean isMinBlending = GL_FALSE;

/* Amount of time in seconds used by a timer to switch blending equations. */
const float resetTimeInterval = 5.0f;

/* Array storing vertices of a square built from 2 triangles moved in the negative Z direction.
 *
 *   2-3----------------5
 *   | \\               |
 *   |   \\             |
 *   |     \\           |
 *   |       \\         |
 *   |         \\       |
 *   |           \\     |
 *   |             \\   |
 *   |               \\ |
 *   0----------------1-4 */
const float squareVertices[] =
{
    -1.0f,  1.0f, -1.0f, 1.0f,
     1.0f,  1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f, 1.0f,
    -1.0f, -1.0f, -1.0f, 1.0f,
     1.0f,  1.0f, -1.0f, 1.0f,
     1.0f, -1.0f, -1.0f, 1.0f,
};

/* Array storing 3D texture coordinates corresponding to vertices of a square. */
const float uvwCoordinates[] =
{
    0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
};

/* Please look into header for the specification. */
void initialize3DTexture()
{
    /* Generate and bind 3D texture. */
    /* [Generate texture ID] */
    GL_CHECK(glGenTextures(1, &textureID));
    /* [Generate texture ID] */
    /* [Bind texture object] */
    GL_CHECK(glBindTexture(GL_TEXTURE_3D, textureID));
    /* [Bind texture object] */

    /* [Initialize texture storage] */
    /* Initialize storage space for texture data. */
    GL_CHECK(glTexStorage3D(GL_TEXTURE_3D,
                            1,
                            GL_R16I,
                            textureWidth,
                            textureHeight,
                            textureDepth));
    /* [Initialize texture storage] */

    /* [Set texture object parameters] */
    /* Set texture parameters. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    /* [Set texture object parameters] */

    /* Try loading image data. */
    initializeTextureData();
}

/* Please look into header for the specification. */
void initializeAttribArrays()
{
    /* Location of input variables in vertex shader. */
    GLint positionLocation            = GL_CHECK(glGetAttribLocation(programID, "inputPosition"));
    GLint inputUVWCoordinatesLocation = GL_CHECK(glGetAttribLocation(programID, "inputUVWCoordinates"));

    ASSERT(positionLocation            != -1, "Could not find attribute location for: inputPosition");
    ASSERT(inputUVWCoordinatesLocation != -1, "Could not find attribute location for: inputUVWCoordinates");

    /* Generate and bind a vertex array object. */
    GL_CHECK(glGenVertexArrays(1, &vaoID));
    GL_CHECK(glBindVertexArray(vaoID));

    /* Generate and bind a buffer object storing vertices of a single quad. */
    GL_CHECK(glGenBuffers(1, &verticesBufferID));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, verticesBufferID));

    /* Put data into the buffer. */
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW));

    /* Set a vertex attribute pointer at the beginnig of the buffer. */
    GL_CHECK(glVertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, 0, 0));
    GL_CHECK(glEnableVertexAttribArray(positionLocation));

    /* Generate and bind a buffer object storing UV texture coordinates. */
    GL_CHECK(glGenBuffers(1, &uvwBufferID));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, uvwBufferID));

    /* Put data into the buffer. */
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(uvwCoordinates), uvwCoordinates, GL_STATIC_DRAW));

    /* Set vertex attribute pointer at the beginning of the buffer. */
    GL_CHECK(glVertexAttribPointer(inputUVWCoordinatesLocation, 3, GL_FLOAT, GL_FALSE, 0, 0));
    GL_CHECK(glEnableVertexAttribArray(inputUVWCoordinatesLocation));
}

/* Please look into header for the specification. */
void initializeProgram()
{
    /* Path to vertex shader source. */
    const string vertexShaderPath = resourceDirectory + "Min_Max_Blending_shader.vert";
    /* Path to fragment shader source. */
    const string fragmentShaderPath = resourceDirectory + "Min_Max_Blending_shader.frag";

    /* IDs of shaders. */
    GLuint vertexShaderID   = 0;
    GLuint fragmentShaderID = 0;

    /* Compile shaders and handle possible compilation errors. */
    Shader::processShader(&vertexShaderID,   vertexShaderPath.c_str(),   GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderID, fragmentShaderPath.c_str(), GL_FRAGMENT_SHADER);

    /* Generate ID for a program. */
    programID = GL_CHECK(glCreateProgram());

    /* Attach shaders to the program. */
    GL_CHECK(glAttachShader(programID, vertexShaderID));
    GL_CHECK(glAttachShader(programID, fragmentShaderID));

    /* Link the program. */
    GL_CHECK(glLinkProgram(programID));

    /* Since there is only one program, it is enough to make it current at this stage. */
    GL_CHECK(glUseProgram(programID));
}

/* Please look into header for the specification. */
void initializeTextureData()
{
    /* Number of layers added at the front of a 3D texture. */
    const int frontLayersCount = (textureDepth - imagesCount) / 2;
    /* Number of layers added at the back of a 3D texture. */
    const int backLayersCount = textureDepth - frontLayersCount - imagesCount;

    /* Check if both numbers of additional layers are not negative. */
    ASSERT(frontLayersCount >= 0 && backLayersCount >= 0,
           "Too low textureDepth value or too many images have been tried to be loaded.");

    /* Load front layers. */
    loadUniformTextures(frontLayersCount);
    /* Load imagesCount images. */
    loadImages();
    /* Load back layers. */
    loadUniformTextures(backLayersCount);

    /* Make sure the 3D texture is fully loaded. */
    ASSERT(textureZOffset == textureDepth,
           "3D texture not completely loaded.");
}

/* Please look into header for the specification. */
void initializeUniformData()
{
    /* Locations in shaders of uniform variables whose values are set only once. */
    GLint cameraMatrixLocation         = GL_CHECK(glGetUniformLocation(programID, "cameraMatrix"));
    GLint projectionMatrixLocation     = GL_CHECK(glGetUniformLocation(programID, "projectionMatrix"));
    /* [Get 3D sampler uniform location] */
    GLint textureSamplerLocation       = GL_CHECK(glGetUniformLocation(programID, "textureSampler"));
    /* [Get 3D sampler uniform location] */
    GLint instancesCountLocation       = GL_CHECK(glGetUniformLocation(programID, "instancesCount"));
    GLint minBlendingThresholdLocation = GL_CHECK(glGetUniformLocation(programID, "minBlendingThreshold"));

    /* Locations in shaders of uniform variables whose values are going to be modified. */
    isMinBlendingLocation  = GL_CHECK(glGetUniformLocation(programID, "isMinBlending"));
    rotationVectorLocation = GL_CHECK(glGetUniformLocation(programID, "rotationVector"));

    ASSERT(cameraMatrixLocation         != -1, "Could not find location for uniform: cameraMatrix");
    ASSERT(projectionMatrixLocation     != -1, "Could not find location for uniform: projectionMatrix");
    /* [Verify 3D sampler uniform location value] */
    ASSERT(textureSamplerLocation       != -1, "Could not find location for uniform: textureSampler");
    /* [Verify 3D sampler uniform location value] */
    ASSERT(instancesCountLocation       != -1, "Could not find location for uniform: instancesCount");
    ASSERT(minBlendingThresholdLocation != -1, "Could not find location for uniform: minBlendingThreshold");
    ASSERT(isMinBlendingLocation        != -1, "Could not find location for uniform: isMinBlending");
    ASSERT(rotationVectorLocation       != -1, "Could not find location for uniform: rotationVector");

    /* Value of translation of camera in Z axis. */
    const float cameraTranslation = -2.0f;
    /* Matrix representing translation of camera. */
    Matrix      cameraMatrix      = Matrix::createTranslation(0.0f,
                                                              0.0f,
                                                              cameraTranslation);
    /* Perspective matrix used as projection matrix. */
    Matrix      projectionMatrix  = Matrix::matrixPerspective(45.0f,
                                                              (float) windowWidth / (float) windowHeight,
                                                              0.01f,
                                                              10.0f);

    /* Pass matrix to the program. */
    GL_CHECK(glUniformMatrix4fv(cameraMatrixLocation,
                                1,
                                GL_FALSE,
                                cameraMatrix.getAsArray()));
    /* Pass matrix to the program. */
     GL_CHECK(glUniformMatrix4fv(projectionMatrixLocation,
                                 1,
                                 GL_FALSE,
                                 projectionMatrix.getAsArray()));

     /* Pass default texture unit ID to the program. */
    GL_CHECK(glUniform1i(textureSamplerLocation, 0));

    /* Pass the number of instances to be drawn, which is equal to the depth of texture. */
    GL_CHECK(glUniform1i(instancesCountLocation, textureDepth));

    /* Pass the value of threshold used for min blending. */
    GL_CHECK(glUniform1f(minBlendingThresholdLocation, minBlendingThreshold));
}

/* Please look into header for the specification. */
void loadImages()
{
    /* Indices of images start with 1. */
    for (int currentImageIndex = 1; currentImageIndex <= imagesCount; ++currentImageIndex)
    {
        GLvoid* textureData = 0;
        /* Maximum number of digits representing extensions. */
        const int digitsCount = 3;

        /* Convert index of the current image, to a string. */
        std::stringstream stringStream;
        stringStream << currentImageIndex;
        string numericExtension = stringStream.str();

        /* Path to the image. */
        const string filePath = resourceDirectory + imagesFilename + "." + numericExtension;

        /* Load data from a file. */
        Texture::loadData(filePath.c_str(), (unsigned char**) &textureData);

        /* Push loaded data to the next layer of a 3D texture that has not been filled yet. */
        setNextTextureImage(textureData);

        /* Free allocated memory space. */
        free(textureData);
    }
}

/* Please look into header for the specification. */
void loadUniformTextures(int count)
{
    GLvoid* textureData = 0;

    /* Create texture with short data type. */
    Texture::createTexture(textureWidth,
                           textureHeight,
                           fillerLuminance,
                           (short**) &textureData);

    /* Load created texture count times. */
    for (int i = 0; i < count; ++i)
    {
        setNextTextureImage(textureData);
    }

    /* Free allocated memory space. */
    delete [] (unsigned char *)textureData;
}

/**
 * \brief Renders single frame.
 */
void renderFrame()
{
    /* Switch blending each resetTimeInterval seconds passed. */
    if (timer.getTime() > resetTimeInterval)
    {
        isMinBlending = !isMinBlending;

        setBlendEquation(isMinBlending);

        timer.reset();
    }

    /* Rotation angles. */
    static float angleX = 0.0f;
    static float angleY = 0.0f;
    static float angleZ = 0.0f;

    /* Arbitrary angle incremental values. */
    const float angleXIncrement = 0.75f;
    const float angleYIncrement = 1.0f;
    const float angleZIncrement = 0.5f;

    /* Vector storing rotation angles that is going to be passed to shader. */
    float rotationVector[] = {angleX, angleY, angleZ};

    /* Clear the screen. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Pass the rotation vector to shader. */
    GL_CHECK(glUniform3fv(rotationVectorLocation, 1, rotationVector));

    /* [Draw 3D texture] */
    /* Draw a single square layer consisting of 6 vertices for textureDepth times. */
    GL_CHECK(glDrawArraysInstanced(GL_TRIANGLES, 0, 6, textureDepth));
    /* [Draw 3D texture] */

    /* Increment rotation angles.*/
    angleX += angleXIncrement;
    angleY += angleYIncrement;
    angleZ += angleZIncrement;

    /* Make sure rotation angles do not go over 360 degrees. */
    if(angleX >= 360.0f) angleX = 0.0f;
    if(angleY >= 360.0f) angleY = 0.0f;
    if(angleZ >= 360.0f) angleZ = 0.0f;
}

/* Please look into header for the specification. */
void setBlendEquation(GLboolean isMinBlending)
{
    if (isMinBlending)
    {
        /* [Set new blend equation : GL_MIN] */
        /* Set new blend equation. */
        GL_CHECK(glBlendEquation(GL_MIN));
        /* [Set new blend equation : GL_MIN] */
        /* Set white color for min blending. */
        GL_CHECK(glClearColor(1.0f, 1.0f, 1.0f, 1.0f));
    }
    else
    {
        /* [Set new blend equation : GL_MAX] */
        /* Set new blend equation. */
        GL_CHECK(glBlendEquation(GL_MAX));
        /* [Set new blend equation : GL_MAX] */
        /* Set black color for max blending. */
        GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
    }

    /* Pass boolean value informing shader about current blending mode. */
    GL_CHECK(glUniform1i(isMinBlendingLocation, isMinBlending));
}

/* Please look into header for the specification. */
void setNextTextureImage(GLvoid* textureData)
{
    /* [Fill texture layer with data] */
    /* Set 2D image at the current textureZOffset. */
    GL_CHECK(glTexSubImage3D(GL_TEXTURE_3D,
                             0,
                             0,
                             0,
                             textureZOffset,
                             textureWidth,
                             textureHeight,
                             1,
                             GL_RED_INTEGER,
                             GL_SHORT,
                             textureData));
    /* [Fill texture layer with data] */

    /* Increment textureZOffset. */
    textureZOffset++;
}

/**
 * \brief Initializes OpenGL ES context.
 *
 * \param width  Window resolution: width.
 * \param height Window resolution: height.
 */
void setupGraphics(int width, int height)
{
    windowHeight = height;
    windowWidth  = width;

    initializeProgram();

    /* Try initializing 3D texture. Log error if it fails. */
    initialize3DTexture();

    /* Try initializing attribute arrays. Log error if it fails. */
    initializeAttribArrays();

    initializeUniformData();

    /* [Enable blending] */
    /* Enable blending. */
    GL_CHECK(glEnable(GL_BLEND));
    /* [Enable blending] */

    /* Set initial blending equation. */
    setBlendEquation(isMinBlending);

    /* Start counting time. */
    timer.reset();
}

/**
 * \brief Releases OpenGL ES objects.
 *
 * It should be called before leaving the application.
 */
void uninit()
{
    GL_CHECK(glDeleteTextures    (1, &textureID       ));
    GL_CHECK(glDeleteBuffers     (1, &verticesBufferID));
    GL_CHECK(glDeleteBuffers     (1, &uvwBufferID     ));
    GL_CHECK(glDeleteVertexArrays(1, &vaoID           ));
    GL_CHECK(glDeleteProgram     (programID           ));
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_step(JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_uninit(JNIEnv*, jobject);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_step(JNIEnv*, jobject)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_minMaxBlending_NativeLibrary_uninit(JNIEnv*, jobject)
{
    uninit();
}
