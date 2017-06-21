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
 * \file tutorials/IntegerLogic/jni/Native.cpp
 * \brief The application simulates cellular automata phenomenon following Rule 30. It uses two programs
 *        which operate on two textures used in a ping-pong manner.
 *
 *        The first program takes the ping texture ("ping") as the input and renders the output to a second texture ("pong").
 *        Rendering in this case is performed by drawing one row at a time, with each row having height of 1 pixel and being of screen width.
 *        Excluding the first row, each row is drawn by reading one row above the currently processed one and applying the cellular automata  rule.
 *        The first row's contents are set by the application.
 *        Since we cannot draw and read from the same texture at a single time, the drawing is performed one row at a time.
 *        After a row is drawn to texture A, the application binds texture B for drawing and uses texture A
 *        for reading the previous line. In the end, texture A contains even rows and texture B stores odd rows.
 *
 *        Having finished drawing lines to these two textures, we run another GL program that merges both textures into a single one by
 *        using texture A for even lines and texture B for odd ones.
 *
 *        In order to be able to render to a texture, we use a custom frame-buffer.
 *
 *        For the first run, the input line has only one pixel lit, so it generates
 *        the commonly known Rule 30 pattern. Then, every 5 seconds, textures are reset
 *        and the input is randomly generated.
 */

#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include "Common.h"
#include "CubeModel.h"
#include "Mathematics.h"
#include "Matrix.h"
#include "PlaneModel.h"
#include "IntegerLogic.h"
#include "Shader.h"
#include "Texture.h"
#include "Timer.h"
#include <cstring>

using namespace MaliSDK;

/* Window resolution: height. */
GLsizei windowHeight = 0;
/* Window resolution: width. */
GLsizei windowWidth  = 0;

/* ID assigned by GL ES for "rule 30" program. */
GLuint rule30ProgramID = 0;
/* ID assigned by GL ES for "merge" program. */
GLuint mergeProgramID  = 0;

/* [Define texture units] */
/* Texture unit used for configuring 2D texture binding for ping textures. */
const GLuint pingTextureUnit = 0;
/* Texture unit used for configuring 2D texture binding for pong textures. */
const GLuint pongTextureUnit = 1;
/* [Define texture units] */

/* Data for the initial line of the ping texture. */
GLvoid* pingTextureData = NULL;
/* ID of ping texture that holds the input data. */
GLuint  pingTextureID   = -1;
/* ID of pong texture whose entire input depend on the ping texture. */
GLuint  pongTextureID   = -1;

/* OpenGL ES ID for a frame-buffer we use for off-screen rendering. */
GLuint framebufferID = -1;
/* OpenGL ES ID for a buffer object used for storing line vertex position data. */
GLuint linePositionBOID = -1;
/* OpenGL ES ID for a buffer object used for storing line U/V texture coordinate data. */
GLuint lineUVBOID = -1;
/* OpenGL ES ID for a buffer object used for storing quad vertex position data. */
GLuint quadPositionBOID = 0;
/* OpenGL ES ID for a buffer object used for storing quad U/V texture coordinate data. */
GLuint quadUVBOID = 0;
/* OpengGL ES ID for a Vertex Array object that we use for storing line vertex attribute assignments. */
GLuint lineVAOID = -1;
/* OpenGL ES ID for a Vertex Array object that we use for storing quad vertex attribute assignments. */
GLuint quadVAOID = 0;

/* Cached projection matrix. */
Matrix modelViewProjectionMatrix;

/* Merge program locations. */
MergeProgramLocations  mergeProgramLocations;
/* Merge program locations. */
Rule30ProgramLocations rule30ProgramLocations;
/* Instance of a timer that will be used to switch between textures being displayed. */
Timer timer;
/* Time interval in seconds. */
const float timeInterval = 5.0f;


/**
 * \brief Generates input for Rule 30 Cellular Automaton, setting a white dot in the top line of the texture
 *        on the given horizontal offset.
 *
 * \param [in]  xoffset     Horizontal position of the stripe.
 * \param [in]  width       Width of the texture.
 * \param [in]  height      Height of the texture.
 * \param [in]  nComponents Number of components defining the colors in the texture.
 * \param [out] textureData Output texture.
 */
void generateRule30Input(unsigned int xoffset,
                         unsigned int width,
                         unsigned int height,
                         unsigned int nComponents,
                         GLvoid**     textureData);
/**
 * \brief Generates random input for Rule 30 Cellular Automaton, setting random white dots in the top line of the texture.
 *
 * \param [in]  width       Width of the texture.
 * \param [in]  height      Height of the texture.
 * \param [in]  nComponents Number of components defining the colors in the texture.
 * \param [out] textureData Output texture.
 */
void generateRule30Input(unsigned int width, unsigned int height, unsigned int nComponents, GLvoid** textureData);

/* Renders to the texture attached to a custom framebuffer, following the Rule 30. */
void performOffscreenRendering();

/* Perform rendering on a single frame. */
void renderFrame();

/* Renders to the back buffer. */
void renderToBackBuffer();

/* Reset the textures, so a new pattern can be generated. */
void resetTextures();

/* Initializes all the required components.
 * \param width  Width of a rendering window.
 * \param height Height of a rendering window.
 */
void setupGraphics(int width, int height);

/* Perform a clean up. */
void uninit();

/* [Generate input texture data] */
/* Please see the specification above. */
void generateRule30Input(unsigned int xoffset,
                         unsigned int width,
                         unsigned int height,
                         unsigned int nComponents,
                         GLvoid** textureData)
{
    ASSERT(textureData != NULL, "Null data passed");

    for(unsigned int channelIndex = 0; channelIndex < nComponents; ++channelIndex)
    {
        (*(unsigned char**)textureData)[(height - 1) * width * nComponents + xoffset * nComponents + channelIndex] = 255;
    }
}
/* [Generate input texture data] */

/* [Generate input texture data for next steps] */
/* Please see the specification above. */
void generateRule30Input(unsigned int width,
                         unsigned int height,
                         unsigned int nComponents,
                         GLvoid**     textureData)
{
    ASSERT(textureData != NULL, "Null data passed");

    for (unsigned int texelIndex = 0; texelIndex < width; ++texelIndex)
    {
        bool setWhite = (rand() % 2 == 0) ? true : false;

        if (setWhite)
        {
            for (unsigned int channelIndex = 0; channelIndex < nComponents; ++channelIndex)
            {
                (*(unsigned char**)textureData)[(height - 1) * width * nComponents + texelIndex * nComponents + channelIndex] = 255;
            }
        }
    }
}
/* [Generate input texture data for next steps] */

/* [Perform the offscreen rendering] */
/* Please see the specification above. */
void performOffscreenRendering()
{
    /* Offset of the input line passed to the appropriate uniform. */
    float inputVerticalOffset = 0.0f;

    /* Activate the first program. */
    GL_CHECK(glUseProgram(rule30ProgramID));
    GL_CHECK(glBindVertexArray(lineVAOID));

    /* [Bind the framebuffer object] */
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferID));
    /* [Bind the framebuffer object] */

    /* Render each line, beginning from the 2nd one, using the input from the previous line.*/
    for (unsigned int y = 1; y <= windowHeight; ++y)
    {
        /* Even lines should be taken from the ping texture, odd from the pong. */
        bool  isEvenLineBeingRendered = (y % 2 == 0) ? (true) : (false);
        /* Vertical offset of the currently rendered line. */
        float verticalOffset          = (float) y / (float) windowHeight;

        /* Pass data to uniforms. */
        GL_CHECK(glUniform1f(rule30ProgramLocations.verticalOffsetLocation,      verticalOffset));
        GL_CHECK(glUniform1f(rule30ProgramLocations.inputVerticalOffsetLocation, inputVerticalOffset));

        if (isEvenLineBeingRendered)
        {
            /* [Bind ping texture to the framebuffer] */
            GL_CHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                            GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D,
                                            pingTextureID,
                                            0) );
            /* [Bind ping texture to the framebuffer] */
            GL_CHECK(glUniform1i           (rule30ProgramLocations.inputTextureLocation,
                                            pongTextureUnit) );
        }
        else
        {
            /* [Bind pong texture to the framebuffer] */
            GL_CHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                            GL_COLOR_ATTACHMENT0,
                                            GL_TEXTURE_2D,
                                            pongTextureID,
                                            0));
            /* [Bind pong texture to the framebuffer] */
            GL_CHECK(glUniform1i           (rule30ProgramLocations.inputTextureLocation,
                                            pingTextureUnit) );
        }

        /* Drawing a horizontal line defined by 2 vertices. */
        GL_CHECK(glDrawArrays(GL_LINES, 0, 2));

        /* Update the input vertical offset after the draw call, so it points to the previous line. */
        inputVerticalOffset = verticalOffset;
    }

    /* Unbind the framebuffer. */
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}
/* [Perform the offscreen rendering] */


/* [Perform textures' merging] */
/* Please see the specification above. */
void renderToBackBuffer()
{
    /* Activate the second program. */
    GL_CHECK(glUseProgram     (mergeProgramID));
    GL_CHECK(glBindVertexArray(quadVAOID));

    /* Draw a quad as a triangle strip defined by 4 vertices. */
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
}
/* [Perform textures' merging] */

/* Please see the specification above. */
void setupGraphics(int width, int height)
{
    /* Store window resolution. */
    windowHeight = height;
    windowWidth  = width;

    /* Array specifying the draw buffers to which render. */
    const GLuint offscreenFBODrawBuffers[] = {GL_COLOR_ATTACHMENT0};

    /* Initialize matrices. */
    Matrix scale        = Matrix::createScaling     ( (float) windowWidth,
                                                      (float) windowHeight,
                                                      1.0f);
    /* Multiplication by 2 for vertical boundaries are caused by setting 0.5 as w coordinate in vertices array. */
    Matrix orthographic = Matrix::matrixOrthographic(-(float) windowWidth,
                                                      (float) windowWidth,
                                                     -(float) windowHeight * 2,
                                                      (float) windowHeight * 2,
                                                     -1.0f,
                                                      1.0f);

    modelViewProjectionMatrix = orthographic * scale;

    /* Create Input data for the ping texture. */
    Texture::createTexture(windowWidth,
                           windowHeight,
                           0,
                          &pingTextureData);
    /* [Generate the original data for ping texture] */
    generateRule30Input   (windowWidth / 2,
                           windowWidth,
                           windowHeight,
                           1,
                          &pingTextureData);
    /* [Generate the original data for ping texture] */

    /* Generate textures. */
    GLuint textureIDs[] = {0, 0};

    /* [Generate texture objects] */
    GL_CHECK(glGenTextures(2, textureIDs));

    pingTextureID = textureIDs[0];
    pongTextureID = textureIDs[1];
    /* [Generate texture objects] */

    /* [Ping texture: Bind texture object to specific texture unit and set its property] */
    /* Load ping texture data. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + pingTextureUnit));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             pingTextureID));
    GL_CHECK(glTexImage2D   (GL_TEXTURE_2D,
                             0,
                             GL_R8UI,
                             windowWidth,
                             windowHeight,
                             0,
                             GL_RED_INTEGER,
                             GL_UNSIGNED_BYTE,
                             pingTextureData));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST));
    /* [Ping texture: Bind texture object to specific texture unit and set its property] */

    /* [Pong texture: Bind texture object to specific texture unit and set its property] */
    /* Prepare pong texture object. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + pongTextureUnit));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             pongTextureID));
    GL_CHECK(glTexStorage2D (GL_TEXTURE_2D,
                             1,
                             GL_R8UI,
                             windowWidth,
                             windowHeight));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST));
    /* [Pong texture: Bind texture object to specific texture unit and set its property] */

    /* Code to setup shaders, geometry, and to initialise OpenGL ES states. */
    GLuint fragmentMergeShaderID  = 0;
    GLuint fragmentRule30ShaderID = 0;
    GLuint vertexRule30ShaderID   = 0;
    GLuint vertexMergeShaderID    = 0;

    /* Process shaders. */
    Shader::processShader(&vertexRule30ShaderID,   VERTEX_RULE_30_SHADER_FILENAME,   GL_VERTEX_SHADER);
    Shader::processShader(&vertexMergeShaderID,    VERTEX_MERGE_SHADER_FILENAME,     GL_VERTEX_SHADER);
    Shader::processShader(&fragmentRule30ShaderID, FRAGMENT_RULE_30_SHADER_FILENAME, GL_FRAGMENT_SHADER);
    Shader::processShader(&fragmentMergeShaderID,  FRAGMENT_MERGE_SHADER_FILENAME,   GL_FRAGMENT_SHADER);

    /* Create programs. */
    /* [Create program object]*/
    rule30ProgramID = GL_CHECK(glCreateProgram());
    /* [Create program object]*/
    mergeProgramID  = GL_CHECK(glCreateProgram());

    /* Attach shaders. */
    /* [Attach shaders to program object] */
    GL_CHECK(glAttachShader(rule30ProgramID, vertexRule30ShaderID));
    GL_CHECK(glAttachShader(rule30ProgramID, fragmentRule30ShaderID));
    /* [Attach shaders to program object] */
    GL_CHECK(glAttachShader(mergeProgramID,  vertexMergeShaderID));
    GL_CHECK(glAttachShader(mergeProgramID,  fragmentMergeShaderID));

    /* Link programs. */
    /* [Link program object] */
    GL_CHECK(glLinkProgram(rule30ProgramID));
    /* [Link program object] */
    GL_CHECK(glLinkProgram(mergeProgramID) );

    /* Set up buffer objects. */
    GLuint boIDs[] = {0, 0, 0, 0};

    /* [Generate buffer objects] */
    GL_CHECK(glGenBuffers(4, boIDs));

    linePositionBOID = boIDs[0];
    lineUVBOID       = boIDs[1];
    quadPositionBOID = boIDs[2];
    quadUVBOID       = boIDs[3];
    /* [Generate buffer objects] */

    /* [Set up framebuffer object] */
    GL_CHECK(glGenFramebuffers(1,
                              &framebufferID));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                               framebufferID));
    GL_CHECK(glDrawBuffers    (1,
                               offscreenFBODrawBuffers));
    /* [Set up framebuffer object] */

    /* Set up VAO for line data. */
    /* [Generate vertex array object] */
    GL_CHECK(glGenVertexArrays(1, &lineVAOID));
    /* [Generate vertex array object] */
    /* [Bind vertex array object] */
    GL_CHECK(glBindVertexArray(lineVAOID));
    /* [Bind vertex array object] */

    /* Retrieve vertex attributes and uniforms locations in "rule30" program. */
    /* [Select active program object] */
    GL_CHECK(glUseProgram(rule30ProgramID) );
    /* [Select active program object] */

    /* [Retrieve attrib and uniform locations] */
    rule30ProgramLocations.inputNeighbourLocation      = GL_CHECK(glGetUniformLocation(rule30ProgramID, "inputNeighbour")      );
    rule30ProgramLocations.inputTextureLocation        = GL_CHECK(glGetUniformLocation(rule30ProgramID, "inputTexture")        );
    rule30ProgramLocations.inputVerticalOffsetLocation = GL_CHECK(glGetUniformLocation(rule30ProgramID, "inputVerticalOffset") );
    rule30ProgramLocations.mvpMatrixLocation           = GL_CHECK(glGetUniformLocation(rule30ProgramID, "mvpMatrix")           );
    rule30ProgramLocations.positionLocation            = GL_CHECK(glGetAttribLocation (rule30ProgramID, "position")            );
    rule30ProgramLocations.texCoordLocation            = GL_CHECK(glGetAttribLocation (rule30ProgramID, "vertexTexCoord")      );
    rule30ProgramLocations.verticalOffsetLocation      = GL_CHECK(glGetUniformLocation(rule30ProgramID, "verticalOffset")      );
    /* [Retrieve attrib and uniform locations] */

    /* [Check location values] */
    ASSERT(rule30ProgramLocations.inputNeighbourLocation      != -1, "Could not find location of a uniform in rule30 program: inputNeighbour"     );
    ASSERT(rule30ProgramLocations.inputTextureLocation        != -1, "Could not find location of a uniform in rule30 program: inputTexture"       );
    ASSERT(rule30ProgramLocations.inputVerticalOffsetLocation != -1, "Could not find location of a uniform in rule30 program: inputVerticalOffset");
    ASSERT(rule30ProgramLocations.mvpMatrixLocation           != -1, "Could not find location of a uniform in rule30 program: mvpMatrix"          );
    ASSERT(rule30ProgramLocations.positionLocation            != -1, "Could not find location of an attribute in rule30 program: position"        );
    ASSERT(rule30ProgramLocations.texCoordLocation            != -1, "Could not find location of an attribute in rule30 program: vertexTexCoord"  );
    ASSERT(rule30ProgramLocations.verticalOffsetLocation      != -1, "Could not find location of a uniform in rule30 program: verticalOffset"     );
    /* [Check location values] */

    /* [Pass data to uniforms] */
    GL_CHECK(glUniformMatrix4fv(rule30ProgramLocations.mvpMatrixLocation,
                                1,
                                GL_FALSE,
                                modelViewProjectionMatrix.getAsArray()));
    GL_CHECK(glUniform1f       (rule30ProgramLocations.inputNeighbourLocation,
                                1.0f / windowWidth) );
    /* [Pass data to uniforms] */

    /* [Fill buffer object with quad coordinates data] */
    /* Fill buffers with line vertices attribute data. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       linePositionBOID));
    GL_CHECK(glBufferData             (GL_ARRAY_BUFFER,
                                       sizeof(lineVertices),
                                       lineVertices,
                                       GL_STATIC_DRAW));
    GL_CHECK(glVertexAttribPointer    (rule30ProgramLocations.positionLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(rule30ProgramLocations.positionLocation));
    /* [Fill buffer object with quad coordinates data] */

    /* [Fill buffer object with texture UVs data] */
    /* Fill buffers with line U/V attribute data. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       lineUVBOID));
    GL_CHECK(glBufferData             (GL_ARRAY_BUFFER,
                                       sizeof(lineTextureCoordinates),
                                       lineTextureCoordinates,
                                       GL_STATIC_DRAW));
    GL_CHECK(glVertexAttribPointer    (rule30ProgramLocations.texCoordLocation,
                                       2,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(rule30ProgramLocations.texCoordLocation));
    /* [Fill buffer object with texture UVs data] */

    /* Set up VAO for quad data. */
    GL_CHECK(glGenVertexArrays(1, &quadVAOID));
    GL_CHECK(glBindVertexArray(quadVAOID));

    /* Retrieve vertex attributes and uniforms locations in "merge" program. */
    GL_CHECK(glUseProgram(mergeProgramID) );

    mergeProgramLocations.mvpMatrixLocation   = GL_CHECK(glGetUniformLocation(mergeProgramID, "mvpMatrix")      );
    mergeProgramLocations.pingTextureLocation = GL_CHECK(glGetUniformLocation(mergeProgramID, "pingTexture")    );
    mergeProgramLocations.pongTextureLocation = GL_CHECK(glGetUniformLocation(mergeProgramID, "pongTexture")    );
    mergeProgramLocations.positionLocation    = GL_CHECK(glGetAttribLocation (mergeProgramID, "position")       );
    mergeProgramLocations.texCoordLocation    = GL_CHECK(glGetAttribLocation (mergeProgramID, "vertexTexCoord") );

    ASSERT(mergeProgramLocations.mvpMatrixLocation   != -1, "Could not find location of a uniform in merge program: mvpMatrix");
    ASSERT(mergeProgramLocations.pingTextureLocation != -1, "Could not find location of a uniform in merge program: pingTexture");
    ASSERT(mergeProgramLocations.pongTextureLocation != -1, "Could not find location of a uniform in merge program: pongTexture");
    ASSERT(mergeProgramLocations.positionLocation    != -1, "Could not find location of an attribute in merge program: position");
    ASSERT(mergeProgramLocations.texCoordLocation    != -1, "Could not find location of an attribute in merge program: vertexTexCoord");

    /* Pass data to uniforms. */
    GL_CHECK(glUniformMatrix4fv(mergeProgramLocations.mvpMatrixLocation,
                                1,
                                GL_FALSE,
                                modelViewProjectionMatrix.getAsArray()));
    GL_CHECK(glUniform1i       (mergeProgramLocations.pingTextureLocation,
                                0) );
    GL_CHECK(glUniform1i       (mergeProgramLocations.pongTextureLocation,
                                1) );

    /* Fill buffers with quad vertices attribute data. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       quadPositionBOID));
    GL_CHECK(glBufferData             (GL_ARRAY_BUFFER,
                                       sizeof(quadVertices),
                                       quadVertices,
                                       GL_STATIC_DRAW));
    GL_CHECK(glVertexAttribPointer    (mergeProgramLocations.positionLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(mergeProgramLocations.positionLocation));

    /* Fill buffers with quad U/V attribute data. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       quadUVBOID));
    GL_CHECK(glBufferData             (GL_ARRAY_BUFFER,
                                       sizeof(quadTextureCoordinates),
                                       quadTextureCoordinates,
                                       GL_STATIC_DRAW));
    GL_CHECK(glVertexAttribPointer    (mergeProgramLocations.texCoordLocation,
                                       2,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    GL_CHECK(glEnableVertexAttribArray(mergeProgramLocations.texCoordLocation));

    /* Set line width to 1.5, to avoid rounding errors. */
    GL_CHECK(glLineWidth(1.5));

    timer.reset();
}

/* Please see the specification above. */
void renderFrame()
{
    performOffscreenRendering();
    renderToBackBuffer();

    if (timer.getTime()> timeInterval)
    {
        resetTextures();
        timer.reset();
    }
}

/* Please see the specification above. */
void resetTextures()
{
    /* Delete existing texture data. */
    Texture::deleteTextureData(&pingTextureData);

    /* Create new texture data. */
    Texture::createTexture(windowWidth, windowHeight, 0, &pingTextureData);
    generateRule30Input   (windowWidth, windowHeight, 1, &pingTextureData);

    /* [Substitute ping texture data] */
    /* Since texture objects have already been created, we can substitute ping image using glTexSubImage2D call.
     * Pong texture does not require reset, because its content depends entirely on the first line of the ping texture. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D,
                             0,
                             0,
                             0,
                             windowWidth,
                             windowHeight,
                             GL_RED_INTEGER,
                             GL_UNSIGNED_BYTE,
                             pingTextureData));
    /* [Substitute ping texture data] */
}

/* Please see the specification above. */
void uninit()
{
    /* Delete texture data. */
    Texture::deleteTextureData(&pingTextureData);
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_step  (JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_integerLogic_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
