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
 * \file samples/tutorials/EtcTexture/jni/Native.cpp
 * \brief Demonstration of ETC2 texture compression support in OpenGL ES 3.0.
 *
 * Compressed textures are loaded and displayed on the screen. The internal format of each texture is displayed
 * at the bottom of the screen.
 * The application cycles through all of the texture formats supported by OpenGL ES 3.0.
 *
 * Formats:
 * - GL_COMPRESSED_R11_EAC:                        11 bits for a single channel. Useful for single channel data where
 *                                                 higher than 8 bit precision is needed. For example, heightmaps.
 * - GL_COMPRESSED_SIGNED_R11_EAC:                 Signed version of GL_COMPRESSED_SIGNED_R11_EAC, useful when signed
 *                                                 data is needed.
 * - GL_COMPRESSED_RG11_EAC:                       11 bits for two channels. Useful for two channel data where higher
 *                                                 than 8 bit precision is needed. For example, normalised bump maps,
 *                                                 the third component can be reconstructed from the other two components.
 * - GL_COMPRESSED_SIGNED_RG11_EAC:                Signed version of GL_COMPRESSED_RG11_EAC, useful when signed data is needed.
 * - GL_COMPRESSED_RGB8_ETC2:                      8 bits for three channels. Useful for normal textures without alpha values.
 * - GL_COMPRESSED_SRGB8_ETC2:                     sRGB version of GL_COMPRESSED_RGB8_ETC2.
 * - GL_COMPRESSED_RGBA8_ETC2_EAC:                 8 bits for four channels. Useful for normal textures with varying alpha values.
 * - GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:          sRGB version of GL_COMPRESSED_RGBA8_ETC2_EAC.
 * - GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:  8 bits for three channels and a 1 bit alpha channel. Useful for normal
 *                                                 textures with binary alpha values.
 * - GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2: sRGB version of GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2.
 */
#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include "Common.h"
#include "ETCHeader.h"
#include "EtcTexture.h"
#include "Matrix.h"
#include "Shader.h"
#include "Text.h"
#include "Texture.h"
#include "Timer.h"

using namespace MaliSDK;

/* Structure to hold information about textures:
 * - internal format of image,
 * - path to image file,
 * - texture name,
 * - texture ID, used by OpenGL ES.
 */
struct Image
{
    GLenum  internalformat;
    char*   fileName;
    char*   nameOfImageIneternalformat;
    GLuint  textureId;
};

/* Initialization of assets. */
Image image0 = {GL_COMPRESSED_R11_EAC,
                TEXTURE_GL_COMPRESSED_R11_EAC_FILE_NAME,
                "GL_COMPRESSED_R11_EAC",
                0};
Image image1 = {GL_COMPRESSED_SIGNED_R11_EAC,
                TEXTURE_GL_COMPRESSED_SIGNED_R11_EAC_FILE_NAME,
                "GL_COMPRESSED_SIGNED_R11_EAC",
                0};
Image image2 = {GL_COMPRESSED_RG11_EAC,
                TEXTURE_GL_COMPRESSED_RG11_EAC_FILE_NAME,
                "GL_COMPRESSED_RG11_EAC",
                0};
Image image3 = {GL_COMPRESSED_SIGNED_RG11_EAC,
                TEXTURE_GL_COMPRESSED_SIGNED_RG11_EAC_FILE_NAME,
                "GL_COMPRESSED_SIGNED_RG11_EAC",
                0};
Image image4 = {GL_COMPRESSED_RGB8_ETC2,
                TEXTURE_GL_COMPRESSED_RGB8_ETC2_FILE_NAME,
                "GL_COMPRESSED_RGB8_ETC2",
                0};
Image image5 = {GL_COMPRESSED_SRGB8_ETC2,
                TEXTURE_GL_COMPRESSED_SRGB8_ETC2_FILE_NAME,
                "GL_COMPRESSED_SRGB8_ETC2",
                0};
Image image6 = {GL_COMPRESSED_RGBA8_ETC2_EAC,
                TEXTURE_GL_COMPRESSED_RGBA8_ETC2_EAC_FILE_NAME,
                "GL_COMPRESSED_RGBA8_ETC2_EAC",
                0};
Image image7 = {GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,
                TEXTURE_GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC_FILE_NAME,
                "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC",
                0};
Image image8 = {GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,
                TEXTURE_GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2_FILE_NAME,
                "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2",
                0};
Image image9 = {GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2,
                TEXTURE_GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2_FILE_NAME,
                "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2",
                0};

/* Array of asset objects that will be used for displaying the images and text on screen. */
Image imageArray[] = {image0,
                      image1,
                      image2,
                      image3,
                      image4,
                      image5,
                      image6,
                      image7,
                      image8,
                      image9};

GLuint      bufferObjectIds[2]          = {0};   /* Array of buffer objects names. Buffer objects hold quad and texture coordinates. */
GLuint      currentAssetIndex           = 0;     /* Index of imageArray to currently displayed image. */
const float displayInterval             = 5.0f;  /* Number of seconds to display one image. */
GLuint      fragmentShaderId            = 0;     /* Fragment shader name. */
Text*       internalformatTextDisplayer = NULL;  /* Instance of a class that holds text with internalformat of displayed image. */
GLint       modelViewMatrixLocation     = 0;     /* Default shader uniform model view location. */
const int   numberOfTextures            = sizeof(imageArray) / sizeof(imageArray[0]);
GLint       positionLocation            = 0;     /* Default shader attribute position location. */
GLuint      programId                   = 0;     /* Program name. */
float       scalingFactor               = 0.75f; /* Scale factor for displaying texture image. */
GLint       textureCoordinateLocation   = 0;     /* Default shader attribute texture coordinate location. */
GLint       textureLocation             = 0;     /* Default shader uniform sampler2D location.*/
Timer       timer;                               /* Instance of a timer that is used to change displayed image every a couple of seconds.*/
int         windowHeight                = 0;     /* Height of window */
int         windowWidth                 = 0;     /* Width of window */
GLuint      vertexShaderId              = 0;     /* Vertex shader name. */
GLuint      vertexArrayId               = 0;

/* [Array of coordinates describing quad] */
float vertexData[] = {-1.0f, -1.0f, 0.0f,
                       1.0f, -1.0f, 0.0f,
                      -1.0f,  1.0f, 0.0f,
                      -1.0f,  1.0f, 0.0f,
                       1.0f, -1.0f, 0.0f,
                       1.0f,  1.0f, 0.0f};
/* [Array of coordinates describing quad] */

/* [Array of texture coordinates used for mapping texture to a quad] */
float textureCoordinatesData[] = {0.0f, 1.0f,
                                  1.0f, 1.0f,
                                  0.0f, 0.0f,
                                  0.0f, 0.0f,
                                  1.0f, 1.0f,
                                  1.0f, 0.0f};
/* [Array of texture coordinates used for mapping texture to a quad] */

/** Generate and fill texture objects with data.
 *
 *  \param textureIndex holds index of an imageArray (indicates which texture is to be created).
 */
void initializeTexture(int textureIndex)
{
    ASSERT(textureIndex >= 0 && textureIndex < numberOfTextures,
           "Incorrect value of index of imageArray.");

    /* Loads the image data and information about the image. */
    char*          fileName = imageArray[textureIndex].fileName;
    ETCHeader      etcHeader;
    unsigned char* imageData = NULL;

    /* [Load compressed image data] */
    Texture::loadPKMData(fileName, &etcHeader, &imageData);
    /* [Load compressed image data] */

    ASSERT(imageData != NULL, "Could not load image data.")

    /* Get size of compressed image with padding included. */
    GLenum internalformat = imageArray[textureIndex].internalformat;
    /* [Get image properties] */
    int     imageHeight = etcHeader.getHeight();
    int     imageWidth  = etcHeader.getWidth();
    GLsizei imageSize   = etcHeader.getSize(internalformat);
    /* [Get image properties] */

    /* Generate and bind texture. Generated texture name is written to imageArray at a given index. */
    /* [Generate texture object ID] */
    GL_CHECK(glGenTextures(1,            &imageArray[textureIndex].textureId));
    /* [Generate texture object ID] */
    /* [Bind texture object] */
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, imageArray[textureIndex].textureId));
    /* [Bind texture object] */

    /* [Fill texture object with data] */
    /* Call CompressedTexImage2D() function which specifies texture with compressed image. */
    GL_CHECK(glCompressedTexImage2D(GL_TEXTURE_2D,
                                    0,
                                    internalformat,
                                    imageWidth,
                                    imageHeight,
                                    0,
                                    imageSize,
                                    imageData));
    /* [Fill texture object with data] */

    /* [Set texture object parameters] */
    /* Set parameters for a texture. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    /* [Set texture object parameters] */
}

/** Initializes data used for rendering.
 */
void setupTextures()
{
    /* [Set pixel storage mode] */
    /* Set OpenGL to use right alignment when reading texture images. */
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    /* [Set pixel storage mode] */

    /* Generate textures and fill them with data. */
    for (int textureIndex = 0; textureIndex < numberOfTextures; textureIndex++)
    {
        initializeTexture(textureIndex);
    }

    /* Generate and bind vertex array. */
    GL_CHECK(glGenVertexArrays(1,
                              &vertexArrayId));
    GL_CHECK(glBindVertexArray(vertexArrayId));

    /* [Generate and initialize buffer objects] */
    /* Generate buffers. */
    GL_CHECK(glGenBuffers(2,
                          bufferObjectIds));

    /* Fill buffer object with vertex data. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          bufferObjectIds[0]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          sizeof(vertexData),
                          vertexData,
                          GL_STATIC_DRAW));

    /* Fill buffer object with texture coordinates data. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          bufferObjectIds[1]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          sizeof(textureCoordinatesData),
                          textureCoordinatesData,
                          GL_STATIC_DRAW));
    /* [Generate and initialize buffer objects] */
}

/** Draw image and text into back buffer.
 */
void draw()
{
    /* Draw text. */
    if (internalformatTextDisplayer != NULL)
    {
        internalformatTextDisplayer->clear();
        internalformatTextDisplayer->addString(0,
                                               0,
                                               imageArray[currentAssetIndex].nameOfImageIneternalformat,
                                               255,
                                               255,
                                               255,
                                               255);
        internalformatTextDisplayer->draw();
    }

    /* [Bind the texture object] */
    /* Draw texture-mapped quad. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             imageArray[currentAssetIndex].textureId));
    /* [Bind the texture object] */
    GL_CHECK(glUseProgram   (programId));

    GL_CHECK(glBindVertexArray(vertexArrayId));

    /* [Draw quad with texture image] */
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));
    /* [Draw quad with texture image] */
}

/** Create program that will be used to rasterize the geometry.
 */
void setupGraphics(int width, int height)
{
    windowHeight = height;
    windowWidth  =  width;

    setupTextures();

    internalformatTextDisplayer = new Text(FONT_VERTEX_SHADER_FILE_NAME,
                                           FONT_FRAGMENT_SHADER_FILE_NAME,
                                           FONT_TEXTURE_FILE_NAME,
                                           windowWidth,
                                           windowHeight);

    glClearColor(0.1f, 0.3f, 0.2f, 1.0f);

    Matrix scaleMatrix;
    float  scaleMultimplier = 0.0f;

    /* Create scale matrix and orthographic matrix. */
    if (height > width)
    {
        scaleMultimplier = (float) width / (float) height;
        scaleMatrix      = Matrix::createScaling(scalingFactor * windowWidth,
                                                 scalingFactor * scaleMultimplier * windowHeight,
                                                 1.0f);
    }
    else
    {
        scaleMultimplier = (float) height / (float) width;
        scaleMatrix      = Matrix::createScaling(scalingFactor * scaleMultimplier * windowWidth,
                                                 scalingFactor * windowHeight,
                                                 1.0f);
    }

    Matrix ortographicMatrix = Matrix::matrixOrthographic(float(-windowWidth), float(windowWidth), float(-windowHeight),
                                                          float(windowHeight), -1.0f,              1.0f);

    /* Enable blending because it is needed for text drawing. */
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Do everything to create program. */
    Shader::processShader(&vertexShaderId,   VERTEX_SHADER_FILE_NAME,   GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderId, FRAGMENT_SHADER_FILE_NAME, GL_FRAGMENT_SHADER);

    /*[Create program object] */
    programId = GL_CHECK(glCreateProgram());
    /*[Create program object] */

    /* [Attach shaders to program object] */
    GL_CHECK(glAttachShader(programId, vertexShaderId));
    GL_CHECK(glAttachShader(programId, fragmentShaderId));
    /* [Attach shaders to program object] */

    /* [Link program object] */
    GL_CHECK(glLinkProgram(programId));
    /* [Link program object] */
    /* [Select active program object] */
    GL_CHECK(glUseProgram(programId));
    /* [Select active program object] */

    /* [Get uniform and attribute locations] */
    /* Get attributes and uniforms locations from shaders attached to the program. */
    modelViewMatrixLocation   = GL_CHECK(glGetUniformLocation(programId, "modelViewMatrix"));
    positionLocation          = GL_CHECK(glGetAttribLocation (programId, "attributePosition"));
    textureCoordinateLocation = GL_CHECK(glGetAttribLocation (programId, "attributeTextureCoordinate"));
    textureLocation           = GL_CHECK(glGetUniformLocation(programId, "uniformTexture"));
    /* [Get uniform and attribute locations] */

    /* [Check uniform and attribute location values] */
    ASSERT(modelViewMatrixLocation   != -1, "Could not retrieve uniform location: modelViewMatrix.");
    ASSERT(positionLocation          != -1, "Could not retrieve attribute location: attributePosition.");
    ASSERT(textureCoordinateLocation != -1, "Could not retrieve attribute location: attributeTextureCoordinate.");
    ASSERT(textureLocation           != -1, "Could not retrieve uniform location: uniformTexture.");
    /* [Check uniform and attribute location values] */

    /* Set up model-view matrix. */
    Matrix resultMatrix = ortographicMatrix * scaleMatrix;

    /* [Enable VAAs] */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       bufferObjectIds[0]));
    GL_CHECK(glVertexAttribPointer    (positionLocation,
                                       3,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(positionLocation));

    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       bufferObjectIds[1]));
    GL_CHECK(glVertexAttribPointer    (textureCoordinateLocation,
                                       2,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(textureCoordinateLocation));
    /* [Enable VAAs] */

    GL_CHECK(glUniformMatrix4fv(modelViewMatrixLocation,
                                1,
                                GL_FALSE,
                                resultMatrix.getAsArray()));
    /* [Set sample uniform value] */
    GL_CHECK(glUniform1i       (textureLocation,
                                0));
    /* [Set sample uniform value] */

    /* Start counting time. */
    timer.reset();
}

/** Render new frame's contents into back buffer.
 */
void renderFrame(void)
{
    /* Clear contents of back buffer. */
    glClear(GL_COLOR_BUFFER_BIT);

    /* Check if time for displaying one image has passed. */
    if(timer.getTime() > displayInterval)
    {
        /* If last picture available is displayed, move to the first one. */
        if (currentAssetIndex < numberOfTextures - 1)
        {
            currentAssetIndex ++;
        }
        else
        {
            currentAssetIndex = 0;
        }

        /* Reset time counter. */
        timer.reset();
    }

    draw();
}

void uninit()
{
    /* Delete textures. */
    for (int i = 0; i < numberOfTextures; i++)
    {
        GL_CHECK(glDeleteTextures(1, &imageArray[i].textureId));
    }

    /* Delete buffers. */
    GL_CHECK(glDeleteBuffers(2, bufferObjectIds));

    /* Delete vertex array object. */
    GL_CHECK(glDeleteVertexArrays(1, &vertexArrayId));

    /* Release text object instance. */
    if (internalformatTextDisplayer != NULL)
    {
        delete internalformatTextDisplayer;

        internalformatTextDisplayer = NULL;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_step  (JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_etcTexture_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
