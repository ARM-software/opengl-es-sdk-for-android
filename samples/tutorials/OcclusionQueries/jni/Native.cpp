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
 * \file samples/tutorials/OcclusionQueries/jni/Native.cpp
 *
 * \brief Demonstration of Occlusion Query functionality in OpenGL ES 3.0.
 *
*The main purpose of the application is to show the difference in performance when the occlusion query mode is on or off.
*If the occlusion query mode is on, then only the cubes that are visible to the viewer are rendered.
*In the other case, when the occlusion query mode is off, then all of the cubes are rendered, which leads to
*a massive decrease in performance.
*
* - In the case where occlusion query mode in on:  if there is a small number of objects visible for a viewer,
*                                                  the application runs very smooth; the larger the number of
*                                                  the visible objects, the slower the animation is, but still
*                                                  the performance is better than in the following case.
* - In the case where occlusion query mode in off: the performance is constant (very low), regardless of the
*                                                  number of visible cubes (all of them are always rendered).
*
*                                        
*We are rendering rounded cubes - the objects are more complicated than the normal cubes,
*which means the time needed for rendering this kind of objects is longer. We are using this fact
*to demonstrate the occlusion query mode. When we want to verify whether the object is visible for
*a viewer, we can draw a simpler object (located in the same position as the requested one and being
*almost of the same size and shape), and once we get the results, we are able to render only those
*rounded cubes which passed the test.
*
*There is also text displayed (at the bottom left corner of the screen) showing whether the occlusion
*query mode is currently on or off. The mode changes every 10 seconds.
 *
 */

#include <jni.h>
#include <android/log.h>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

#include "Common.h"
#include "CubeModel.h"
#include "Matrix.h"
#include "Native.h"
#include "PlaneModel.h"
#include "Shader.h"
#include "SuperEllipsoidModel.h"
#include "Text.h"
#include "Texture.h"
#include "Timer.h"

using namespace std;
using namespace MaliSDK;

/* Asset directories and filenames */
const string resourceDirectory = "/data/data/com.arm.malideveloper.openglessdk.occlusionQueries/files/";

/* Window properties. */
int windowWidth  = 0;
int windowHeight = 0;

/* Timer */
Timer timer;

/* Timer variable to calculate FPS. */
Timer fpsTimer;

/* Id of OpenGL program we use for rendering. */
GLuint programId = 0;

/* Pointer to an array that stores vertices of rounded cube. */
float* roundedCubeCoordinates = NULL;

/* Pointer to an array that stores normal vectors of rounded cube. */
float* roundedCubeNormalVectors = NULL;

/* This value represents number of rounded cube vertices. */
int numberOfRoundedCubesVertices = 0;

/* Represents a number of rounded cube's coordinates. */
int numberOfRoundedCubeCoordinates = 0;

/* Represents a number of rounded cube's normal vectors. */
int numberOfRoundedCubeNormalVectors = 0;

/* Array that stores random position of each cube. */
Vec2f randomCubesPositions[NUMBER_OF_CUBES];

/* Minimum distance between cubes. */
const float minimumDistance = ROUNDED_CUBE_SCALE_FACTOR * 2.0f + 0.1f;

/* Array of queries for each of numberOfCubes cubes. */
GLuint cubeQuery[NUMBER_OF_CUBES] = {0};

/* Flag that informs us what mode is turned on (if it's occlusion query mode or not). */
bool occlusionQueriesOn = false;

/* This is the angle that is used to rotate camera around Y axis. */
float angleY = 0.0f;

/* This value informs us how far camera is located from point (0, 0, 0). */
const float cameraRadius = 22.0f;

/* This value is used to translate camera along the Y axis. */
const float yCameraTranslation = 1.25f;

/* Matrices that will be used to setup perspective view. */
Matrix cubeNormalMatrix;
Matrix cubeMvpMatrix;
Matrix cubeModelMatrix;
Matrix cubeWorldInverseMatrix;
Matrix planeNormalMatrix;
Matrix planeMvpMatrix;
Matrix planeModelMatrix;
Matrix planeWorldInverseMatrix;
Matrix projectionMatrix;
Matrix rotatedViewMatrix;
Matrix viewMatrix;

/* Variables that will be used to store locations of the attributes and uniforms.*/
GLint normalMatrixUniformLocation       = -1;
GLint mvpMatrixUniformLocation          = -1;
GLint worldInverseMatrixUniformLocation = -1;
GLint colorUniformLocation              = -1;

/* Array to store sorted positions of the cubes. Each cube has 2 coordinates. */
float sortedCubesPositions[2 * NUMBER_OF_CUBES] = {0.0f};

/* Scaling factor to scale up the plane. */
const float planeScalingFactor = 40.0f;

/* Determines how many times an area (where the cubes are located) should be smaller than original plane. */
const float planeDividend = 3.0f;

const float cubeColor[]     = {0.0f, 0.75f, 0.0f, 1.0f};
const float planeColor[]    = {1.0f, 0.8f,  0.0f, 1.0f};
const float planeLocation[] = {0.0f, 0.0f,  0.0f};

/* Vertex Array Objects for plane, normal cube and rounded cube. */
GLuint planeVertexArrayObjectId       = 0;
GLuint normalCubeVertexArrayObjectId  = 0;
GLuint roundedCubeVertexArrayObjectId = 0;

/* This buffer object holds the plane's vertices. */
GLuint planeVerticesBufferId = 0;

/* This buffer object holds the plan's normal vectors. */
GLuint planeNormalVectorsBufferId = 0;

/* This buffer object holds the normal cubes' vertices. */
GLuint normalCubeBufferId = 0;

/* This buffer object holds the rounded cubes' vertices. */
GLuint roundedCubeVerticesBufferId = 0;

/* This buffer object holds the rounded cubes' normal vectors. */
GLuint roundedCubeNormalVectorsBufferId = 0;

/* Pointer to an array that stores the cubes' vertices. */
float* normalCubeVertices = NULL;

/* Determines the size of the dynamically allocated normalCubeVertices array. */
int numberOfCubeCoordinates = 0;
int numberOfCubeVertices    = 0;

/* Pointer to an array that stores the plane's vertices. */
float* planeVertices = NULL;

/* Number of plane vertices and vertex coordinates that make up a plane shape. */
int numberOfPlaneVertices          = 0;
int numberOfPlaneVertexCoordinates = 0;

/* Pointer to an array that stores the plane's normal vectors. */
float* planeNormalVectors = NULL;

/* Determines the size of the dynamically allocated plane Normals array. */
int sizeOfPlaneNormalsArray = 0;

/* Flag indicating that the rendering mode has changed (occlusion query OFF => ON) */
bool modeChanged = false;

/* Counter for the number of rounded cubes drawn each frame. */
int numberOfRoundedCubesDrawn = 0;

/* Text object to indicate whether occlusion queries are turned on or not. */
Text* text;

/**
 * \brief Compute Euclidean 2-dimensional distance between two points on XY plane.
 *
 * \param[in] point1 First point.
 * \param[in] point2 Second point.
 *
 * \return Distance between points on XY plane.
 */
inline float distanceBetweenPoints(const Vec2f& point1, const Vec2f& point2)
{
    return sqrtf((point2.x - point1.x) * (point2.x - point1.x) + (point2.y - point1.y) * (point2.y - point1.y));
}

/**
 * \brief Check if the rounded cubes are a proper distance from each other to prevent cubes overlapping.
 *
 * \param[in] point       The point for which we check distance.
 * \param[in] minDistance Determines minimum distance between points.
 * \param[in] j           Tells how many points are already saved in the randomCubesPositions array.
 *
 * \return True if the point is in neighbourhood of any other point that is already saved in randomCubesPotisitions array and false otherwise.
 */
inline bool inNeighbourhood(const Vec2f& point, float minDistance, int j)
{
    for (int i = 0; i < j; i++)
    {
        if (distanceBetweenPoints(point, randomCubesPositions[i]) < minDistance)
        {
            return true;
        }
    }
    return false;
}

/**
 * \brief Generate random number in the 0.0 to 1.0 range.
 *
 * \return Random number in the range 0.0 to 1.0.
 */
inline float uniformRandomNumber()
{
    return rand() / float(RAND_MAX);
}

/**
 * \brief Generate random cubes' center locations.
 *
 * This algorithm ensures that cube will be the required distance apart.
 *
 * \param[in] planeWidth  Width of the plane.
 * \param[in] planeHeight Height of the plane.
 * \param[in] minDistance Determines minimum distance between cubes.
 */
void generateCubesLocations(float planeWidth, float planeHeight, float minDistance)
{
    /*
     * xRange and zRange are both minimum (-xRange, -zRange) and maximum (+xRange, +zRange) values respectively for X axis and Z axes.
     * These two values ensure that cubes will not partially land outside the plane. To prevent such a situation we have to
     * restrict the bounds of our plane. We also want camera to fly around the cubes and
     * have the plane still visible (we don't want to see the edges of the plane - "end of the world").
     * That's why we divide planeWidth and planeHeight by planeDividend - this will give us a smaller plane
     * that a part of the bigger, original plane.
     */
    float xRange = planeWidth / planeDividend;
    float zRange = planeHeight / planeDividend;

    /* This variable is used to prevent the situation that this function will iterate for
     * infinite number of times to find locations for a cube - we can't locate infinite number of cubes
     * that don't overlap on a finite plane. */
    float loopsCounter = 0;

    /* We generate the first point which can be randomly chosen from points on the plane
     * and we save it to the array.
     * Generation of this point is based on equation for generating random numbers
     * in given interval. If we want to get a random number in an interval [a, b]
     * we can use equation: rand()%(b - a) + a, where rand() generates a number
     * between 0 and 1. */
    Vec2f firstRandomPoint = {(xRange + xRange) * uniformRandomNumber() - xRange,
                              (zRange + zRange) * uniformRandomNumber() - zRange};

    randomCubesPositions[0] = firstRandomPoint;

    for (int i = 1; i < NUMBER_OF_CUBES; i++)
    {
        /* Check if the loop has not iterated too many times. */
        if (loopsCounter > NUMBER_OF_CUBES * NUMBER_OF_CUBES)
        {
            return;
        }

        /* We choose another random point on a plane. */
        Vec2f randomPoint = {(xRange + xRange) * uniformRandomNumber() - xRange,
                             (zRange + zRange) * uniformRandomNumber() - zRange};

        /* And check if it's a proper distance from any other point that is already stored in the array. */
        if (!inNeighbourhood(randomPoint, minDistance, i))
        {
            /* If it is, we can save it to the array of random cubes positions. */
            randomCubesPositions[i] = randomPoint;
        }
        else
        {
            /* Otherwise, decrement the i counter and try again. */
            i -= 1;
        }

        /* Increase loop counter. */
        loopsCounter++;
    }
}

/**
 * \brief Function that is used to sort cubes' center positions from the nearest to the furthest, relative to the camera position.
 *
 * We use bubble sort algorithm that checks whether an array is sorted or not.
 *
 * \param[in,out] arrayToSort An array to be sorted.
 */
void sortCubePositions(float* arrayToSort)
{
    /* The upper limit position of sorted elements. We subtract 4 because we will refer to the 4 array's
     * cells ahead and we don't want to go out of bounds of the array. */
    int max = (NUMBER_OF_CUBES * 2) - 4;

    bool swapped = true;

    while(swapped)
    {
        swapped = false;

        for (int i = 0; i <= max; i += 2)
        {
            /* Temporarily store location points. */
            Vec2f firstCubeLocation, secondCubeLocation;

            firstCubeLocation.x = arrayToSort[i];
            firstCubeLocation.y = arrayToSort[i + 1];

            secondCubeLocation.x = arrayToSort[i + 2];
            secondCubeLocation.y = arrayToSort[i + 3];

            Vec3f first_cube_location              = {firstCubeLocation.x,  1, firstCubeLocation.y};
            Vec3f second_cube_location             = {secondCubeLocation.x, 1, secondCubeLocation.y};
            Vec3f transformed_first_cube_location  = Matrix::vertexTransform(&first_cube_location,  &rotatedViewMatrix);
            Vec3f transformed_second_cube_location = Matrix::vertexTransform(&second_cube_location, &rotatedViewMatrix);

            if (transformed_first_cube_location.z < transformed_second_cube_location.z)
            {
                /* Swap coordinates' positions. */
                arrayToSort[i]     = secondCubeLocation.x;
                arrayToSort[i + 1] = secondCubeLocation.y;

                arrayToSort[i + 2] = firstCubeLocation.x;
                arrayToSort[i + 3] = firstCubeLocation.y;

                swapped = true;
            }
        } /* for (int i = 0; i <= max; i += 2) */
    }/* while(swapped) */
}

/**
 * \brief convert Vec2f array to an array of floats.
 *
 * Rewrite coordinates from randomCubesPositions array which type is Vec2f
 * to a simple array of floats (sortedCubesPositions) that we can easily pass to the uniform.
 */
void rewriteVec2fArrayToFloatArray()
{
    for (int i = 0; i < NUMBER_OF_CUBES; i++)
    {
        sortedCubesPositions[i * 2]     = randomCubesPositions[i].x;
        sortedCubesPositions[i * 2 + 1] = randomCubesPositions[i].y;
    }
}

/**
 * \brief Sends center position of a cube to the vertex shader's uniform.
 *
 * \param[in] whichCube Determines which cube's center position we should send to the vertex shader.
 */
inline void sendCubeLocationVectorToUniform(int whichCube)
{
    /* Array to be sent to the vertex shader.*/
    float tempArray[3];

    /* We send roundedCubeScaleFactor to translate cubes a little bit up so they won't intersect with the plane.*/
    tempArray[0] = sortedCubesPositions[2 * whichCube];
    tempArray[1] = ROUNDED_CUBE_SCALE_FACTOR;
    tempArray[2] = sortedCubesPositions[2 * whichCube + 1];

    cubeModelMatrix        = Matrix::createTranslation(tempArray[0],
                                                       tempArray[1],
                                                       tempArray[2]);
    cubeMvpMatrix          = projectionMatrix * rotatedViewMatrix * cubeModelMatrix;
    cubeWorldInverseMatrix = Matrix::matrixInvert(&cubeMvpMatrix);
    cubeNormalMatrix       = Matrix::matrixInvert     (&cubeModelMatrix);

    Matrix::matrixTranspose(&cubeNormalMatrix);

    /* Send array to the shader. */
    GL_CHECK(glUniformMatrix4fv(normalMatrixUniformLocation,
                                1,
                                GL_FALSE,
                                cubeNormalMatrix.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(worldInverseMatrixUniformLocation,
                                1,
                                GL_FALSE,
                                cubeWorldInverseMatrix.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(mvpMatrixUniformLocation,
                                1,
                                GL_FALSE,
                                cubeMvpMatrix.getAsArray()));
}

/**
 * \brief Function that sets up shaders, programs, uniforms locations, generates buffer objects and query objects.
 *
 * \param width  Window width.
 * \param height Window height.
 */
void setupGraphics(int width, int height)
{
    /* This line ensures that everytime we run the new instance of the program, rand() will generate different numbers. */
    srand((unsigned int)time(NULL));

    /* Store window resolution. */
    windowHeight = height;
    windowWidth  = width;

    /* Initialize scaling matrix. */
    Matrix scaling = Matrix::createScaling(planeScalingFactor,
                                           planeScalingFactor,
                                           planeScalingFactor);

    /* Initialize vectors that will be passed to matrixCameralookAt function. */
    const Vec3f eyeVector   = {0.0f, yCameraTranslation, cameraRadius};
    const Vec3f lookAtPoint = {0.0f, 0.0f,               0.0f};
    const Vec3f upVector    = {0.0f, 1.0f,               0.0f};

    /* Calculate matrices. */
    projectionMatrix  = Matrix::matrixPerspective(degreesToRadians(45.0f),
                                                  float(windowWidth)/float(windowHeight),
                                                  0.1f,
                                                  50.0f);
    viewMatrix        = Matrix::matrixCameraLookAt(eyeVector,
                                                   lookAtPoint,
                                                   upVector);
    planeModelMatrix  = Matrix::createTranslation(planeLocation[0],
                                                  planeLocation[1],
                                                  planeLocation[2]);
    planeNormalMatrix = Matrix::matrixInvert(&planeModelMatrix);

    Matrix::matrixTranspose(&planeNormalMatrix);

    /* Set up the text object. */
    text = new Text(resourceDirectory.c_str(), windowWidth, windowHeight);
    text->addString(0, 0, "Occlusion query OFF", 255, 0, 0, 255);

    /* Set clear color. */
    GL_CHECK(glClearColor(0.3f, 0.6f, 0.70f, 1.0f));

    /* Enable depth test and set depth function to GL_LEQUAL. */
    GL_CHECK(glEnable   (GL_DEPTH_TEST));
    GL_CHECK(glDepthFunc(GL_LEQUAL));

    /* Set shaders up. */
    GLuint fragmentShaderId   = 0;
    string fragmentShaderPath = resourceDirectory + "fragment.frag";
    GLuint vertexShaderId     = 0;
    string vertexShaderPath   = resourceDirectory + "vertex.vert";

    Shader::processShader(&vertexShaderId,
                          vertexShaderPath.c_str(),
                          GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderId,
                          fragmentShaderPath.c_str(),
                          GL_FRAGMENT_SHADER);

    /* [Create program object] */
    programId = GL_CHECK(glCreateProgram());
    /* [Create program object] */

    /* [Attach vertex and fragment shader objects to rendering program] */
    GL_CHECK(glAttachShader(programId, vertexShaderId));
    GL_CHECK(glAttachShader(programId, fragmentShaderId));
    /* [Attach vertex and fragment shader objects to rendering program] */

    /* [Link the program object] */
    GL_CHECK(glLinkProgram(programId));
    /* [Link the program object] */
    /* [Set active program object] */
    GL_CHECK(glUseProgram (programId));
    /* [Set active program object] */

    /* Retrieve program uniform and attribute locations. */
    /* [Get program attrib locations] */
    GLuint normalAttributeLocation   = GL_CHECK(glGetAttribLocation (programId, "normal"));
    GLuint verticesAttributeLocation = GL_CHECK(glGetAttribLocation (programId, "vertex"));
    /* [Get program attrib locations] */

    colorUniformLocation              = GL_CHECK(glGetUniformLocation(programId, "color"));
    normalMatrixUniformLocation       = GL_CHECK(glGetUniformLocation(programId, "normalMatrix"));
    worldInverseMatrixUniformLocation = GL_CHECK(glGetUniformLocation(programId, "worldInverseMatrix"));
    mvpMatrixUniformLocation          = GL_CHECK(glGetUniformLocation(programId, "mvpMatrix"));

    ASSERT(colorUniformLocation              != -1, "Could not retrieve uniform location:   color");
    ASSERT(verticesAttributeLocation         != -1, "Could not retrieve attribute location: vertex");
    ASSERT(normalAttributeLocation           != -1, "Could not retrieve attribute location: normal");
    ASSERT(normalMatrixUniformLocation       != -1, "Could not retrieve uniform location:   normalMatrix");
    ASSERT(worldInverseMatrixUniformLocation != -1, "Could not retrieve uniform location:   worldInverseMatrix");
    ASSERT(mvpMatrixUniformLocation          != -1, "Could not retrieve uniform location:   mvpMatrix");

    /* Generate super ellipsoid. */
    SuperEllipsoidModel::create(NUMBER_OF_SAMPLES,
                                SQUARENESS_1,
                                SQUARENESS_2,
                                ROUNDED_CUBE_SCALE_FACTOR,
                                &roundedCubeCoordinates,
                                &roundedCubeNormalVectors,
                                &numberOfRoundedCubesVertices,
                                &numberOfRoundedCubeCoordinates,
                                &numberOfRoundedCubeNormalVectors);
    /* Generate triangular representation of a cube. */
    CubeModel::getTriangleRepresentation(NORMAL_CUBE_SCALE_FACTOR,
                                         &numberOfCubeVertices,
                                         &numberOfCubeCoordinates,
                                         &normalCubeVertices);
    /* [Generate triangular representation of a plane] */
    PlaneModel::getTriangleRepresentation(&numberOfPlaneVertices,
                                          &numberOfPlaneVertexCoordinates,
                                          &planeVertices);
    /* [Generate triangular representation of a plane] */
    /* [Get plane normals] */
    PlaneModel::getNormals(&sizeOfPlaneNormalsArray,
                           &planeNormalVectors);
    /* [Get plane normals] */

    /* Make sure the models'coordinates were created successfully. */
    ASSERT(roundedCubeCoordinates   != NULL, "Could not create super ellipsoid's coordinates.");
    ASSERT(roundedCubeNormalVectors != NULL, "Could not create super ellipsoid's normal vectors.");
    ASSERT(normalCubeVertices       != NULL, "Could not create triangular representation of a cube.");
    ASSERT(planeVertices            != NULL, "Could not create triangular representation of a plane.");
    ASSERT(planeNormalVectors       != NULL, "Could not create plane's normal vector.");

    /* Scale the plane up to fill the screen. */
    PlaneModel::transform(scaling, numberOfPlaneVertexCoordinates, &planeVertices);

    /* Generate cubes' center locations. */
    generateCubesLocations(planeScalingFactor, planeScalingFactor, minimumDistance);

    /* Rewrite Vec2f randomCubesPosition array to simple array of floats. */
    rewriteVec2fArrayToFloatArray();

    /* Generate buffer objects. */
    /* [Generate buffer objects for plane geometry] */
    GL_CHECK(glGenBuffers(1, &planeVerticesBufferId));
    GL_CHECK(glGenBuffers(1, &planeNormalVectorsBufferId));
    /* [Generate buffer objects for plane geometry] */
    GL_CHECK(glGenBuffers(1, &normalCubeBufferId));
    GL_CHECK(glGenBuffers(1, &roundedCubeVerticesBufferId));
    GL_CHECK(glGenBuffers(1, &roundedCubeNormalVectorsBufferId));

    /* Generate vertex array objects. */
    /* [Generate Vertex array object for plane geometry] */
    GL_CHECK(glGenVertexArrays(1, &planeVertexArrayObjectId));
    /* [Generate Vertex array object for plane geometry] */
    GL_CHECK(glGenVertexArrays(1, &normalCubeVertexArrayObjectId));
    GL_CHECK(glGenVertexArrays(1, &roundedCubeVertexArrayObjectId));

    /* This vertex array object stores the plane's vertices and normal vectors. */
    /* [Bind vertex array object] */
    GL_CHECK(glBindVertexArray(planeVertexArrayObjectId));
    /* [Bind vertex array object] */
    {
        /* [Copy plane coordinates to the buffer object] */
        GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                       planeVerticesBufferId));
        GL_CHECK(glBufferData         (GL_ARRAY_BUFFER,
                                       numberOfPlaneVertexCoordinates * sizeof(float),
                                       planeVertices,
                                       GL_STATIC_DRAW));
        /* [Copy plane coordinates to the buffer object] */
        /* [Set vertex attrib pointer for plane coordinates] */
        GL_CHECK(glVertexAttribPointer(verticesAttributeLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
        /* [Set vertex attrib pointer for plane coordinates] */

        /* [Copy plane normals to the buffer object] */
        GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                       planeNormalVectorsBufferId));
        GL_CHECK(glBufferData         (GL_ARRAY_BUFFER,
                                       sizeOfPlaneNormalsArray * sizeof(float),
                                       planeNormalVectors,
                                       GL_STATIC_DRAW));
        /* [Copy plane normals to the buffer object] */
        /* [Set vertex attrib pointer for plane normals] */
        GL_CHECK(glVertexAttribPointer(normalAttributeLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
        /* [Set vertex attrib pointer for plane normals] */

        /* [Enable plane vertex attrib arrays] */
        GL_CHECK(glEnableVertexAttribArray(verticesAttributeLocation));
        GL_CHECK(glEnableVertexAttribArray(normalAttributeLocation));
        /* [Enable plane vertex attrib arrays] */
    }

    /* This vertex array object stores the normal cubes' vertices. */
    GL_CHECK(glBindVertexArray(normalCubeVertexArrayObjectId));
    {
        GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                       normalCubeBufferId));
        GL_CHECK(glBufferData         (GL_ARRAY_BUFFER,
                                       numberOfCubeCoordinates * sizeof(float),
                                       normalCubeVertices,
                                       GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(verticesAttributeLocation,
                                       3,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));

        GL_CHECK(glEnableVertexAttribArray(verticesAttributeLocation));
    }

    /* This vertex array object stores rounded cube's vertices and normal vectors. */
    GL_CHECK(glBindVertexArray(roundedCubeVertexArrayObjectId));
    {
        GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                       roundedCubeVerticesBufferId));
        GL_CHECK(glBufferData         (GL_ARRAY_BUFFER,
                                       numberOfRoundedCubeCoordinates * sizeof(float),
                                       roundedCubeCoordinates,
                                       GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(verticesAttributeLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));

        GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                       roundedCubeNormalVectorsBufferId));
        GL_CHECK(glBufferData         (GL_ARRAY_BUFFER,
                                       numberOfRoundedCubeNormalVectors * sizeof(float),
                                       roundedCubeNormalVectors,
                                       GL_STATIC_DRAW));
        GL_CHECK(glVertexAttribPointer(normalAttributeLocation,
                                       4,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));

        GL_CHECK(glEnableVertexAttribArray(verticesAttributeLocation));
        GL_CHECK(glEnableVertexAttribArray(normalAttributeLocation));
    }

    /* [Generate query objects] */
    GL_CHECK(glGenQueries(NUMBER_OF_CUBES, cubeQuery));
    /* [Generate query objects] */

    /* Define blending function that will be used when enabled. */
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Release allocated memory. */
    if (normalCubeVertices != NULL)
    {
        free(normalCubeVertices);
        normalCubeVertices = NULL;
    }

    if (planeVertices != NULL)
    {
        free(planeVertices);
        planeVertices = NULL;
    }

    if (planeNormalVectors != NULL)
    {
        free(planeNormalVectors);
        planeNormalVectors = NULL;
    }

    if(roundedCubeCoordinates != NULL)
    {
        delete[] roundedCubeCoordinates;
        roundedCubeCoordinates = NULL;
    }

    if(roundedCubeNormalVectors != NULL)
    {
        delete[] roundedCubeNormalVectors;
        roundedCubeNormalVectors = NULL;
    }

    fpsTimer.reset();
    timer.reset();
}

/**
 * \brief Draw the plane and cubes.
 *
 * Renders all rounded cubes if occlusion queries turned on,
 * otherwise just draws those visible.
 */
void draw(void)
{
    numberOfRoundedCubesDrawn = 0;

    /* Set active program object. */
    GL_CHECK(glUseProgram(programId));

    /* Draw the cubes. */
    {
        GL_CHECK(glUniform4fv(colorUniformLocation,
                              1,
                              cubeColor));

        if (occlusionQueriesOn)
        {
            /* [Bind normal cubes vertex array object] */
            GL_CHECK(glBindVertexArray(normalCubeVertexArrayObjectId));
            /* [Bind normal cubes vertex array object] */
            /* [Set color mask to GL_FALSE] */
            GL_CHECK(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
            /* [Set color mask to GL_FALSE] */

            /* [Issue the occlusion test] */
            for (int i = 0; i < NUMBER_OF_CUBES; i++)
            {
                sendCubeLocationVectorToUniform(i);
                /* Begin occlusion query. */
                GL_CHECK(glBeginQuery(GL_ANY_SAMPLES_PASSED, cubeQuery[i]));
                {
                    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, numberOfCubeVertices));
                }
                GL_CHECK(glEndQuery(GL_ANY_SAMPLES_PASSED));
                /* End occlusion query. */
            }
            /* [Issue the occlusion test] */

            /* [Set color mask to GL_TRUE] */
            /* Clear depth buffer and enable color mask to make rounded cubes visible. */
            GL_CHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
            /* [Set color mask to GL_TRUE] */

            if (modeChanged)
            {
                GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT));
            }

            /* Clear color and depth buffers. */
            GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

            /* [Bind rounded cubes vertex array object] */
            /* Draw rounded cubes. */
            GL_CHECK(glBindVertexArray(roundedCubeVertexArrayObjectId));
            /* [Bind rounded cubes vertex array object] */

            for(int i = 0; i < NUMBER_OF_CUBES; i++)
            {
                GLuint queryResult = GL_FALSE;

                /* [Check query result] */
                GL_CHECK(glGetQueryObjectuiv(cubeQuery[i], GL_QUERY_RESULT, &queryResult));
                /* [Check query result] */

                /* If the cube has become visible in this frame, render it again as a rounded cube. */
                if (queryResult == GL_TRUE)
                {
                    sendCubeLocationVectorToUniform(i);

                    /* [Draw rounded cube] */
                    GL_CHECK(glDrawArrays(GL_TRIANGLES,
                                          0,
                                          numberOfRoundedCubesVertices));
                    /* [Draw rounded cube] */

                    numberOfRoundedCubesDrawn++;
                }
            }
        }
        else
        {
            /* [Draw for disabled occlusion query mode] */
            /* Draw all rounded cubes without using occlusion queries. */
            GL_CHECK(glBindVertexArray(roundedCubeVertexArrayObjectId));

            for(int i = 0; i < NUMBER_OF_CUBES; i++)
            {
                sendCubeLocationVectorToUniform(i);

                GL_CHECK(glDrawArrays(GL_TRIANGLES,
                                      0,
                                      numberOfRoundedCubesVertices));
            }

            numberOfRoundedCubesDrawn = NUMBER_OF_CUBES;
            /* [Draw for disabled occlusion query mode] */
        }
    } /* Draw the cubes. */

    /* [Draw the plane] */
    {
        GL_CHECK(glBindVertexArray (planeVertexArrayObjectId));
        GL_CHECK(glUniform4fv      (colorUniformLocation,
                                    1,
                                    planeColor));
        GL_CHECK(glUniformMatrix4fv(normalMatrixUniformLocation,
                                    1,
                                    GL_FALSE,
                                    planeNormalMatrix.getAsArray()));
        GL_CHECK(glUniformMatrix4fv(worldInverseMatrixUniformLocation,
                                    1,
                                    GL_FALSE,
                                    planeWorldInverseMatrix.getAsArray()));
        GL_CHECK(glUniformMatrix4fv(mvpMatrixUniformLocation,
                                    1,
                                    GL_FALSE,
                                    planeMvpMatrix.getAsArray()));
        GL_CHECK(glDrawArrays      (GL_TRIANGLES,
                                    0,
                                    numberOfPlaneVertices));
    }
    /* [Draw the plane] */
}

/**
 * \brief Render one frame.
 */
void renderFrame()
{
    if(fpsTimer.isTimePassed(1.0f))
    {
        /* Calculate FPS. */
        float FPS = fpsTimer.getFPS();

        LOGI("FPS:\t%.1f", FPS);
        LOGI("Number of Cubes drawn: %d\n", numberOfRoundedCubesDrawn);
    }

    /* Clear color and depth buffers. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Increase angleY. */
    angleY += 0.25f;

    if(angleY >= 360)
    {
        angleY = 0.0f;
    }

    /* Rotation matrix used to rotate the view. */
    Matrix yRotationMatrix = Matrix::createRotationY(-angleY);

    /* Multiply viewMatrix and yRotationMatrix to make camera rotate around the scene. */
    rotatedViewMatrix       = viewMatrix       * yRotationMatrix;
    planeMvpMatrix          = projectionMatrix * rotatedViewMatrix * planeModelMatrix;
    planeWorldInverseMatrix = Matrix::matrixInvert(&planeMvpMatrix);

    /* [Sort positions] */
    /* Sort the cubes' positions. We have to do it in every frame because camera constantly moves around the scene.
     * It is important that the cubes are rendered front to back because the occlusion test is done per draw call.
     * If the cubes are draw out of order then some cubes may pass the occlusion test even when they end up being
     * occluded by geometry drawn later. */
    sortCubePositions(sortedCubesPositions);
    /* [Sort positions] */

    modeChanged = false;

    /* Check timer to know if we should turn off/on occlusion queries. */
    if(timer.getTime() > TIME_INTERVAL)
    {
        occlusionQueriesOn = !occlusionQueriesOn;

        if(occlusionQueriesOn)
        {
            /* Mark that mode has changed */
            modeChanged = true;

            LOGI("\nOcclusion query ON\n");
            text->clear();
            text->addString(0, 0, "Occlusion query ON", 255, 0, 0, 255);
        }
        else
        {
            LOGI("\nOcclusion query OFF\n");
            text->clear();
            text->addString(0, 0, "Occlusion query OFF", 255, 0, 0, 255);
        }

        timer.reset();
    }

    draw();

    /* Enable blending (required for Text drawing). */
    GL_CHECK(glEnable(GL_BLEND));

    text->draw();

    /* Enable blending (required for Text drawing). */
    GL_CHECK(glDisable(GL_BLEND));
}

/**
 * \brief Releases all OpenGL objects that were created with glGen*() or glCreate*() functions.
 */
void uninit()
{
    /* Delete the program. */
    GL_CHECK(glDeleteProgram(programId));

    /* Delete the buffer objects. */
    GL_CHECK(glDeleteBuffers(1, &planeVerticesBufferId));
    GL_CHECK(glDeleteBuffers(1, &planeNormalVectorsBufferId));
    GL_CHECK(glDeleteBuffers(1, &normalCubeBufferId));
    GL_CHECK(glDeleteBuffers(1, &roundedCubeVerticesBufferId));
    GL_CHECK(glDeleteBuffers(1, &roundedCubeNormalVectorsBufferId));

    /* Delete the vertex array objects. */
    GL_CHECK(glDeleteVertexArrays(1, &planeVertexArrayObjectId));
    GL_CHECK(glDeleteVertexArrays(1, &normalCubeVertexArrayObjectId));
    GL_CHECK(glDeleteVertexArrays(1, &roundedCubeVertexArrayObjectId));

    /* Delete the query objects. */
    GL_CHECK(glDeleteQueries(NUMBER_OF_CUBES, cubeQuery));
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_init  (JNIEnv*, jobject,
                                                                                                        jint     width,
                                                                                                        jint     height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_step  (JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_uninit(JNIEnv*, jobject);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_init(JNIEnv*, jobject,
                                                                                                  jint     width,
                                                                                                  jint     height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_step(JNIEnv*, jobject)
{
    renderFrame();
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_occlusionQueries_NativeLibrary_uninit(JNIEnv*, jobject)
{
    uninit();
}
