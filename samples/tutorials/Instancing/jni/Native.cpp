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
 * \file samples/tutorials/Instancing/jni/Native.cpp
 * \brief Demonstration of instanced drawing and uniform buffers in OpenGL ES 3.0.
 *
 * There is only one copy of the cube vertex data in memory, each of the cubes drawn is an instance of that data.
 * This reduces the amount of memory which needs to be transferred to the GPU.
 * By using gl_instanceID in the shader, each of the cubes can have a different position, rotation speed and colour.
 * This technique can be used everywhere repeated geometry is used in a scene.
 */
#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include "Common.h"
#include "CubeModel.h"
#include "Instancing.h"
#include "Shader.h"
#include "Timer.h"
#include <cstring>

using namespace MaliSDK;

/* Instance of a timer. Used for setting positions and rotations of cubes. */
Timer timer;
/* Program used for transforming vertices into world space. */
/* Fragment shader name. */
GLuint fragmentShaderId = 0;
/* Vertex shader name. */
GLuint vertexShaderId = 0;
/* Program name. */
GLuint renderingProgramId = 0;

/* Cubes. */
/* Number of coordinates written to cubeTrianglesCoordinates array*/
int numberOfCubeTriangleCoordinates = 0;
/* Number of vertices that make up a cubic shape. */
int numberOfCubeVertices = 0;
/* Array holding coordinates of triangles which a cube consists of. */
float* cubeTrianglesCoordinates = NULL;
/* Number of values written to vertexColors array. */
int numberOfValuesInVertexColorsArray = 0;
/* Array holding color values for each vertex of cube triangle. */
GLfloat* vertexColors = NULL;
/* Number of values written to cubeColors array. RGBA values for each cube. */
const int numberOfValuesInCubeColorsArray = NUMBER_OF_COLOR_COMPONENTS * NUMBER_OF_CUBES;
/* Array holding color values for each cube. */
GLfloat cubeColors[numberOfValuesInCubeColorsArray] = {0};
/* Scaling factor indicating size of a cube. */
const float cubeSize = 2.5f;

/* Uniform and attribute locations. */
/* "Camera position" shader uniform which is used to set up a view. */
GLint cameraPositionLocation = 0;
/* Shader uniform block index. */
GLuint uniformBlockIndex = 0;
/* "Perspective matrix" shader uniform's location. */
GLint perspectiveMatrixLocation = 0;
/* "Position" shader attribute's location. */
GLint positionLocation = 0;
/* "Color" shader attribute's location. */
GLint cubeVertexColorLocation = 0;
/* "Time" shader uniform that is used to hold timer value. */
GLint timeLocation = 0;

/* Buffer objects. */
/* Constant telling number of buffer objects that should be generated:
*  - buffer object holding cube coordinates,
*  - buffer object holding per-vertex cube colors,
*  - buffer object holding data used in uniform block.
*/
const GLuint numberOfBufferObjectIds = 3;
/* Array of buffer object names. */
GLuint bufferObjectIds[numberOfBufferObjectIds] = {0};
/* Name of buffer object which holds color of triangle vertices. */
GLuint cubeColorsBufferObjectId = 0;
/* Name of buffer object which holds coordinates of triangles making cube. */
GLuint cubeCoordinatesBufferObjectId = 0;
/* Name of buffer object which holds start positions of cubes and values of color for each cube. */
GLuint uniformBlockDataBufferObjectId = 0;

/* Start positions of cubes in 3D space. */
/* Array holding start position of cubes in 3D space which are used to draw cubes for the first time. */
GLfloat startPosition[NUMBER_OF_CUBES] = {0};

/*
 * Arrays holding data used for setting perspective and view.
 * 45.0f - field of view angle (in degrees) in the y direction
 * (float)windowWidth / (float)windowHeight - ratio used to calculate the field of view in the x direction.
 * 0.1f - distance from the camera to the near clipping plane.
 * 1000.0f - distance from the camera to the far clipping plane.
*/
Vec4f perspectiveVector;
/* Array used for view configuration in vertex shader. */
Vec3f cameraVector;

/* [Generate cube start positions data] */
/**
 * \brief Generate positions of cubes which are used during first draw call.
 * \note  Cubes are located on a circular curve.
 */
void generateStartPosition()
{
    float spaceBetweenCubes = (2 * M_PI) / (NUMBER_OF_CUBES);

    /* Fill array with startPosition data. */
    for (int allCubes = 0; allCubes < NUMBER_OF_CUBES; allCubes++)
    {
        startPosition[allCubes] = allCubes * spaceBetweenCubes;
    }
}
/* [Generate cube start positions data] */

/* [Generate cube colours data] */
/**
 * \brief Fill cubeColors array with random color (used for setting random color for each cube).
 */
void fillCubeColorsArray()
{
    for (int allComponents = 0;
             allComponents < numberOfValuesInCubeColorsArray;
             allComponents++)
    {
        /* Get random value from [0.0, 1.0] range. */
        cubeColors[allComponents] = (float)rand() / (float)RAND_MAX;
    }
}
/* [Generate cube colours data] */

/*
 * \brief Fill vertexColors array with random color for each vertex of a cube triangular representation.
 */
void fillVertexColorsArray()
{
    /* Calculate number of colour components for each cube vertex. */
    numberOfValuesInVertexColorsArray = numberOfCubeVertices * NUMBER_OF_COLOR_COMPONENTS;

    /* Allocate memory for vertexColors array. */
    vertexColors = (float*) malloc (numberOfValuesInVertexColorsArray * sizeof(float));

    ASSERT(vertexColors != NULL, "Could not allocate memory for vertexColors array.");

    for (int allComponents = 0;
             allComponents < numberOfValuesInVertexColorsArray;
             allComponents++)
    {
        vertexColors[allComponents] = (float)rand() / (float)RAND_MAX;
    }
}

/**
 * Initialize data for cubes.
 */
void createCubesData()
{
    /* [Get cube coordinates] */
    /* Get triangular representation of a cube. Save data in cubeTrianglesCoordinates array. */
    CubeModel::getTriangleRepresentation(&cubeTrianglesCoordinates,
                                         &numberOfCubeTriangleCoordinates,
                                         &numberOfCubeVertices,
                                          cubeSize);
    /* [Get cube coordinates] */

    /* Make sure triangular representation of a cube was created successfully. */
    ASSERT(cubeTrianglesCoordinates != NULL,
           "Could not retrieve triangle representation of a cube");

    /* Set start values of positions. */
    generateStartPosition();

    /* Calculate color for each cube. */
    fillCubeColorsArray();

    /* Calculate color for each vertex of a cube. */
    fillVertexColorsArray();
}

/*
 * Initializes data used for rendering.
 */
void initializeData()
{
    /* Create all data needed to draw cube. */
    createCubesData();

    /* Settings for 3D shape drawing. */
    GL_CHECK(glEnable(GL_DEPTH_TEST));

    /* [Generate buffer objects] */
    /* Generate buffers. */
    GL_CHECK(glGenBuffers(numberOfBufferObjectIds, bufferObjectIds));

    cubeCoordinatesBufferObjectId  = bufferObjectIds[0];
    cubeColorsBufferObjectId       = bufferObjectIds[1];
    uniformBlockDataBufferObjectId = bufferObjectIds[2];
    /* [Generate buffer objects] */

    /* [Fill buffer object with vertex data] */
    /* Buffer holding coordinates of triangles which create a cube. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          cubeCoordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          numberOfCubeTriangleCoordinates * sizeof(GLfloat),
                          cubeTrianglesCoordinates,
                          GL_STATIC_DRAW));
    /* [Fill buffer object with vertex data] */

    /* Buffer holding RGBA values of color for each vertex. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          cubeColorsBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          numberOfValuesInVertexColorsArray * sizeof(GLfloat),
                          vertexColors,
                          GL_STATIC_DRAW));

    /* [Set uniform block buffer data] */
    /* Buffer holding coordinates of start positions of cubes and RGBA values of colors. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          uniformBlockDataBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          sizeof(startPosition) + sizeof(cubeColors),
                          NULL,
                          GL_STATIC_DRAW));

    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER,
                             0,
                             sizeof(startPosition),
                             startPosition));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER,
                             sizeof(startPosition),
                             sizeof(cubeColors),
                             cubeColors));
    /* [Set uniform block buffer data] */

    /* Deallocate memory (data is now stored in buffer objects). */
    if (vertexColors != NULL)
    {
        free(vertexColors);

        vertexColors = NULL;
    }

    if (cubeTrianglesCoordinates != NULL)
    {
        free(cubeTrianglesCoordinates);

        cubeTrianglesCoordinates = NULL;
    }
}

/*
 * Create programs that will be used to rasterize the geometry of cubes.
 */
void setupProgram()
{
    /* [Create a program object] */
    renderingProgramId = GL_CHECK(glCreateProgram());
    /* [Create a program object] */

    /* Initialize rendering program. */
    /* [Initialize shader objects] */
    Shader::processShader(&vertexShaderId,   VERTEX_SHADER_FILE_NAME,   GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderId, FRAGMENT_SHADER_FILE_NAME, GL_FRAGMENT_SHADER);
    /* [Initialize shader objects] */

    /* [Attach vertex and fragment shader objects to rendering program] */
    GL_CHECK(glAttachShader(renderingProgramId, vertexShaderId));
    GL_CHECK(glAttachShader(renderingProgramId, fragmentShaderId));
    /* [Attach vertex and fragment shader objects to rendering program] */

    /* Link and use rendering program object. */
    /* [Link the program object] */
    GL_CHECK(glLinkProgram(renderingProgramId));
    /* [Link the program object] */
    /* [Set active program object] */
    GL_CHECK(glUseProgram(renderingProgramId));
    /* [Set active program object] */

    /* Get uniform, attribute and uniform block locations from current program. */
    /* [Get position attribute location] */
    positionLocation          = GL_CHECK(glGetAttribLocation   (renderingProgramId, "attributePosition"));
    /* [Get position attribute location] */
    cubeVertexColorLocation   = GL_CHECK(glGetAttribLocation   (renderingProgramId, "attributeColor"));
    perspectiveMatrixLocation = GL_CHECK(glGetUniformLocation  (renderingProgramId, "perspectiveVector"));
    cameraPositionLocation    = GL_CHECK(glGetUniformLocation  (renderingProgramId, "cameraVector"));
    /* [Get uniform block index] */
    uniformBlockIndex         = GL_CHECK(glGetUniformBlockIndex(renderingProgramId, "CubesUniformBlock"));
    /* [Get uniform block index] */
    timeLocation              = GL_CHECK(glGetUniformLocation  (renderingProgramId, "time"));

    /* [Check position attribute location] */
    ASSERT(positionLocation          != -1,               "Could not retrieve attribute location: attributePosition");
    /* [Check position attribute location] */
    ASSERT(cubeVertexColorLocation   != -1,               "Could not retrieve attribute location: attributeColor");
    ASSERT(perspectiveMatrixLocation != -1,               "Could not retrieve uniform location: perspectiveVector");
    ASSERT(cameraPositionLocation    != -1,               "Could not retrieve uniform location: cameraVector");
    ASSERT(timeLocation              != -1,               "Could not retrieve uniform location: time");
    /* [Check uniform block index] */
    ASSERT(uniformBlockIndex         != GL_INVALID_INDEX, "Could not retrieve uniform block index: CubesUniformBlock");
    /* [Check uniform block index] */
}

/**
 * \brief Render new frame's contents into back buffer.
 */
void renderFrame()
{
    /* Clear contents of back buffer. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Value of time returned by timer used for setting cubes rotations and positions. */
    const float time = timer.getTime();

    /* [Set time uniform value] */
    GL_CHECK(glUniform1f(timeLocation, time));
    /* [Set time uniform value] */

    /* [Instanced drawing command] */
    /* Draw cubes on a screen. */
    GL_CHECK(glDrawArraysInstanced(GL_TRIANGLES,
                                   0,
                                   numberOfCubeVertices,
                                   NUMBER_OF_CUBES));
    /* [Instanced drawing command] */
}

void setupGraphics(int width, int height)
{
    perspectiveVector.x = 45.0f;
    perspectiveVector.y = (float)width / (float)height;
    perspectiveVector.z = 0.1f;
    perspectiveVector.w = 1000.0f;

    cameraVector.x = 0.0f;
    cameraVector.y = 0.0f;
    cameraVector.z = -60.0f;

    /* Initialize data used for rendering. */
    initializeData();
    /* Create program. */
    setupProgram();
    /* Start counting time. */
    timer.reset();

    GL_CHECK(glUseProgram(renderingProgramId));

    /* Enable VAAa. */
    /* [Set Vertex Attrib Array for cube coordinates] */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       cubeCoordinatesBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(positionLocation));
    GL_CHECK(glVertexAttribPointer    (positionLocation,
                                       NUMBER_OF_POINT_COORDINATES,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
    /* [Set Vertex Attrib Array for cube coordinates] */

    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       cubeColorsBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(cubeVertexColorLocation));
    GL_CHECK(glVertexAttribPointer    (cubeVertexColorLocation,
                                       NUMBER_OF_COLOR_COMPONENTS,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));

    /* Set uniform values which are constant during the rendering process. */
    /* [Set perspective vector uniform value] */
    GL_CHECK(glUniform4fv(perspectiveMatrixLocation,
                          1,
                          (GLfloat*)&perspectiveVector));
    /* [Set perspective vector uniform value] */
    /* [Set camera position uniform value] */
    GL_CHECK(glUniform3fv(cameraPositionLocation,
                          1,
                          (GLfloat*)&cameraVector));
    /* [Set camera position uniform value] */

    /* [Set uniform block data] */
    /* Set binding point for uniform block. */
    GL_CHECK(glUniformBlockBinding(renderingProgramId,
                                   uniformBlockIndex,
                                   0));
    GL_CHECK(glBindBufferBase     (GL_UNIFORM_BUFFER,
                                   0,
                                   uniformBlockDataBufferObjectId));
    /* [Set uniform block data] */
}

void uninit()
{
    /* Delete buffers. */
    GL_CHECK(glDeleteBuffers(numberOfBufferObjectIds, bufferObjectIds));

    /* Free allocated memory. */
    if (cubeTrianglesCoordinates != NULL)
    {
        free(cubeTrianglesCoordinates);

        cubeTrianglesCoordinates = NULL;
    }

    if (vertexColors != NULL)
    {
        free(vertexColors);

        vertexColors = NULL;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_step  (JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_instancing_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
