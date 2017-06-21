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
 * \file tutorials/ShadowMapping/jni/Native.cpp
 * \brief Demonstration of shadow mapping functionality using OpenGL ES 3.0.
 *
 * Application displays two cubes on a plane which are lit with directional and spot lights.
 * Location and direction of the spot light source (represented by a small yellow cube flying above the scene)
 * in 3D space are regularly updated.
 * The cube and planes models are shadow receivers, but only the cubes are shadow casters.
 * The application uses shadow mapping for rendering and displaying shadows.
 */

#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include "Common.h"
#include "CubeModel.h"
#include "Mathematics.h"
#include "Matrix.h"
#include "PlaneModel.h"
#include "ShadowMapping.h"
#include "Shader.h"
#include "Texture.h"
#include "Timer.h"
#include <cstring>

using namespace MaliSDK;

/* Structure holding all data needed to initialize mesh-related buffer objects. */
struct GeometryProperties
{
    float* coordinates;                        /* Array holding the geometry's coordinates. */
    float* normals;                            /* Array holding the geometry's normal vectors. */
    float* position;                           /* Array holding position of the geometry in 3D space. */
    int    numberOfElementsInCoordinatesArray; /* Number of elements written to coordinates array. */
    int    numberOfElementsInNormalsArray;     /* Number of elements written to an array holding geometry's normal vectors. */
    int    numberOfElementsInPositionArray;    /* Number of elements written to position array. */
    int    numberOfPoints;                     /* Number of point that are making the geometry. */
    float  scalingFactor;                      /* Factor used for scaling geometry. */
} plane, cube, lightRepresentation;

/* Structure holding window's data. */
struct WindowProperties
{
    int height; /* Height of window. */
    int width; /* Width of window. */
} window;

/* Structure holding properties of a light source. */
struct LightProperties
{
    Vec3f position;
    Vec3f direction;
} light;

/* Structure holding the data describing shadow-map texture. */
struct ShadowMapTextureProperties
{
    GLuint  framebufferObjectName; /* Name of a framebuffer object used for rendering depth texture. */
    GLsizei height;                /* Height of the shadow map texture. */
    GLuint  textureName;           /* Name of a shadow map texture (holds depth values for cubes-plane model from the light's point of view). */
    GLsizei width;                 /* Width of the shadow map texture. */
} shadowMap;

/* Structure holding the data used for configuring the program object that is responsible for drawing cubes and plane and calculating the shadow map. */
struct CubesAndPlaneProgramProperties
{
    GLuint programId;                       /* Program name. Program does the following tasks:
                                             * - moving vertices of a geometry into eye-space,
                                             * - shading the rasterized primitive considering directional and spot lights and the shadow map for the spot light.
                                             */
    GLuint colorOfGeometryLocation;         /* Shader uniform location that is used to hold color of a geometry. Use different color for cubes and plane. */
    GLuint isCameraPointOfViewLocation;     /* Shader uniform location that is used to hold a boolean value indicating whether the point of view of
                                             * the camera (if true) or the light (if false) should be used for rendering the scene.
                                             * The camera's point of view is used for the final pass, the light's point of view is used for calculating the shadow map.
                                             */
    GLint  lightDirectionLocation;          /* Shader uniform location that is used to hold direction of light (from cubes-plane model). */
    GLint  lightPositionLocation;           /* Shader uniform location that is used to hold position of light source (from cubes-plane model). */
    GLuint lightViewMatrixLocation;         /* Shader uniform location that refers to view matrix used for rendering the scene from light point of view. */
    GLuint normalsAttributeLocation;        /* Shader attribute location that is used to hold normal vectors of the cubes or the plane. */
    GLuint positionAttributeLocation;       /* Shader attribute location that is used to hold coordinates of the cubes or the plane to be drawn. */
    GLint  shadowMapLocation;               /* Shader uniform location that is used to hold the shadow map texture unit id. */
    GLuint shouldRenderPlaneLocation;       /* Shader uniform location that is used to hold boolean value indicating whether the plane (if true) or the cubes
                                             * (if false) are being drawn (different data is used for cubes and plane).
                                             */
} cubesAndPlaneProgram;

/* Structure holding data used for configuring the program object that is responsible for drawing representation of a spot light (yellow cube). */
struct LightRepresentationProgramProperties
{
    GLuint programId;        /*
                              * Program name. Program does the following tasks:
                              * - moving vertices of geometry into eye-space,
                              * - setting color of the cube.
                              */
    GLint  positionLocation; /* Shader uniform location that is used to hold position of the light cube. */
} lightRepresentationProgram;

/* Instance of a timer. Used for setting position and direction of light source. */
Timer timer;

/* Buffer object names. */
GLuint cubeCoordinatesBufferObjectId                = 0; /* Name of buffer object which holds coordinates of the triangles making up the scene cubes. */
GLuint cubeNormalsBufferObjectId                    = 0; /* Name of buffer object which holds the scene cubes normal vectors. */
GLuint lightRepresentationCoordinatesBufferObjectId = 0; /* Name of buffer object which holds coordinates of the light cube. */
GLuint planeCoordinatesBufferObjectId               = 0; /* Name of buffer object which holds coordinates of the plane. */
GLuint planeNormalsBufferObjectId                   = 0; /* Name of buffer object which holds plane's normal vectors. */
GLuint uniformBlockDataBufferObjectId               = 0; /* Name of buffer object which holds positions of the scene cubes. */

/* Vertex array objects. */
GLuint cubesVertexArrayObjectId                          = 0; /* Name of vertex array object used when rendering the scene cubes. */
GLuint lightRepresentationCoordinatesVertexArrayObjectId = 0; /* Name of vertex array object used when rendering the light cube. */
GLuint planeVertexArrayObjectId                          = 0; /* Name of vertex array object used when rendering the scene cubes. */

/* View and Projection configuration. */
/* We use different projection matrices for both passes. */
Matrix cameraProjectionMatrix;
Matrix lightProjectionMatrix;

const Vec3f cameraPosition   = {0.0f, 0.0f, 30.0f}; /* Array holding position of camera. */
Vec3f       lookAtPoint      = {0.0f, 0.0f,  0.0f}; /* Coordinates of a point the camera should look at (from light point of view). */
Matrix      viewMatrixForShadowMapPass;             /* Matrix used for translating geometry relative to light position. This is used for shadow map rendering pass. */

/* Arrays holding RGBA values of the scene cubes and the plane. */
const float cubesColor[] = {0.8f, 0.1f, 0.2f, 0.6f};
const float planeColor[] = {0.2f, 0.4f, 0.8f, 0.6f};

/**
 * \brief Initialize structure data.
 */
void initializeStructureData()
{
    /* Each scene cube is placed on the same altitude described by this value. */
    const float cubesYPosition = -3.0f;

    /* Set up cube properties. */
    cube.coordinates                        = NULL;
    cube.numberOfElementsInCoordinatesArray = 0;
    cube.normals                            = NULL;
    cube.numberOfElementsInNormalsArray     = 0;
    cube.numberOfElementsInPositionArray    = 2 * 4;   /* There are 2 cubes and 4 coordinates describing position (x, y, z, w). */
    cube.scalingFactor                      = 2.0f;

    /* Allocate memory for array holding position of cubes. */
    cube.position = (float*) malloc (cube.numberOfElementsInPositionArray * sizeof(float));

    ASSERT(cube.position != NULL, "Could not allocate memory for cube position array.");

    /* Array holding position of cubes. Y coordinate value is the same for both cubes - they are lying on the same surface. */
    /* First cube. */
    cube.position[0] = -3.0f;           /* x */
    cube.position[1] =  cubesYPosition; /* y */
    cube.position[2] =  5.0f;           /* z */
    cube.position[3] =  1.0f;           /* w */
    /* Second cube. */
    cube.position[4] =  5.0f;           /* x */
    cube.position[5] =  cubesYPosition; /* y */
    cube.position[6] =  3.0f;           /* z */
    cube.position[7] =  1.0f;           /* w */

    /* Set up plane properties. */
    plane.coordinates                        = NULL;
    plane.normals                            = NULL;
    plane.numberOfElementsInCoordinatesArray = 0;
    plane.numberOfElementsInNormalsArray     = 0;
    plane.numberOfElementsInPositionArray    = 3; /* There are 3 coordinates describing position (x, y, z). */
    plane.scalingFactor                      = 15.0; /* plane.scalingFactor * 2 indicates size of square's side. */
    plane.position                           = (float*) malloc (plane.numberOfElementsInPositionArray * sizeof(float));

    ASSERT(plane.position != NULL, "Could not allocate memory for plane position array.");

    /* Array holding position of plane. Y coordinate value is calculated so that cubes are sitting on the plane. */
    plane.position[0] = 0.0f;
    plane.position[1] = cubesYPosition - cube.scalingFactor;
    plane.position[2] = 0.0f;

    /* Set up light representation properties. */
    lightRepresentation.coordinates                        = NULL;
    lightRepresentation.numberOfElementsInCoordinatesArray = 0;
    lightRepresentation.scalingFactor                      = 0.3f; /* lightRepresentation.scalingFactor * 2 indicates size of cube's side. */

    /* Set up shadow map properties. */
    shadowMap.framebufferObjectName = 0;
    /* [Declare shadow map texture resolution] */
    shadowMap.height                = window.height * 2;
    shadowMap.width                 = window.width  * 2;
    /* [Declare shadow map texture resolution] */
    shadowMap.textureName           = 0;

    /* We use different projection matrices for both passes. */
    cameraProjectionMatrix = Matrix::matrixPerspective(degreesToRadians(60.0f),
                                                       float(window.width) / float(window.height),
                                                       1.0f,
                                                       50.0f);
    /* [Calculate projection matrix from spot light point of view] */
    lightProjectionMatrix  = Matrix::matrixPerspective(degreesToRadians(90.0f),
                                                       1.0f,
                                                       1.0f,
                                                       50.0f);
    /* [Calculate projection matrix from spot light point of view] */
}

/**
 * \brief Creates GL objects.
 */
void createObjects()
{
    GLuint bufferObjectIds[6]         = {0};
    GLuint vertexArrayObjectsNames[3] = {0};

    /* [Generate objects for rendering the geometry] */
    /* Generate buffer objects. */
    GL_CHECK(glGenBuffers(6, bufferObjectIds));

    /* Store buffer object names in global variables.
     * The variables have more friendly names, so that using them is easier. */
    cubeCoordinatesBufferObjectId                = bufferObjectIds[0];
    lightRepresentationCoordinatesBufferObjectId = bufferObjectIds[1];
    cubeNormalsBufferObjectId                    = bufferObjectIds[2];
    planeCoordinatesBufferObjectId               = bufferObjectIds[3];
    planeNormalsBufferObjectId                   = bufferObjectIds[4];
    uniformBlockDataBufferObjectId               = bufferObjectIds[5];

    /* Generate vertex array objects. */
    GL_CHECK(glGenVertexArrays(3, vertexArrayObjectsNames));

    /* Store vertex array object names in global variables.
     * The variables have more friendly names, so that using them is easier. */
    cubesVertexArrayObjectId                          = vertexArrayObjectsNames[0];
    lightRepresentationCoordinatesVertexArrayObjectId = vertexArrayObjectsNames[1];
    planeVertexArrayObjectId                          = vertexArrayObjectsNames[2];
    /* [Generate objects for rendering the geometry] */

    /* Generate and configure shadow map texture to hold depth values. */
    /* [Generate depth texture object] */
    GL_CHECK(glGenTextures  (1,
                            &shadowMap.textureName));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             shadowMap.textureName));
    /* [Generate depth texture object] */
    /* [Prepare depth texture storage] */
    GL_CHECK(glTexStorage2D(GL_TEXTURE_2D,
                            1,
                            GL_DEPTH_COMPONENT24,
                            shadowMap.width,
                            shadowMap.height));
    /* [Prepare depth texture storage] */
    /* [Set depth texture object parameters] */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_COMPARE_FUNC,
                             GL_LEQUAL));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_COMPARE_MODE,
                             GL_COMPARE_REF_TO_TEXTURE));
    /* [Set depth texture object parameters] */

    /* Attach texture to depth attachment point of a framebuffer object. */
    /* [Generate and bind framebuffer object] */
    GL_CHECK(glGenFramebuffers     (1,
                                   &shadowMap.framebufferObjectName));
    GL_CHECK(glBindFramebuffer     (GL_FRAMEBUFFER,
                                    shadowMap.framebufferObjectName));
    /* [Generate and bind framebuffer object] */
    /* [Bind depth texture to framebuffer] */
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D,
                                    shadowMap.textureName,
                                    0));
    /* [Bind depth texture to framebuffer] */
}

/**
 * \brief Deletes all created GL objects.
 */
void deleteObjects()
{
    /* Delete buffers. */
    GL_CHECK(glDeleteBuffers(1, &cubeCoordinatesBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &cubeNormalsBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &lightRepresentationCoordinatesBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &planeCoordinatesBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &planeNormalsBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &uniformBlockDataBufferObjectId));

    /* Delete framebuffer object. */
    GL_CHECK(glDeleteFramebuffers(1, &shadowMap.framebufferObjectName));

    /* Delete texture. */
    GL_CHECK(glDeleteTextures(1, &shadowMap.textureName));

    /* Delete vertex arrays. */
    GL_CHECK(glDeleteVertexArrays(1, &cubesVertexArrayObjectId));
    GL_CHECK(glDeleteVertexArrays(1, &lightRepresentationCoordinatesVertexArrayObjectId));
    GL_CHECK(glDeleteVertexArrays(1, &planeVertexArrayObjectId));
}

/**
 * brief Cleans up. Frees all allocated memory.
 */
void deallocateMemory()
{
    /* If triangular representation of a cube was created successfully, make sure memory is deallocated. */
    if (cube.coordinates != NULL)
    {
        free (cube.coordinates);

        cube.coordinates = NULL;
    }

    /* If representation of a cube's normal was created successfully, make sure memory is deallocated. */
    if (cube.normals != NULL)
    {
        free (cube.normals);

        cube.normals = NULL;
    }

    /* If triangular representation of a cube simulating a light source was created successfully, make sure memory is deallocated. */
    if (lightRepresentation.coordinates != NULL)
    {
        free (lightRepresentation.coordinates);

        lightRepresentation.coordinates = NULL;
    }

    /* If triangular representation of a plane was created successfully, make sure memory is deallocated. */
    if (plane.coordinates != NULL)
    {
        free (plane.coordinates);

        plane.coordinates = NULL;
    }

    /* If representation of a plane's normal was created successfully, make sure memory is deallocated. */
    if (plane.normals != NULL)
    {
        free (plane.normals);

        plane.normals = NULL;
    }

    /* If position of plane was created successfully, make sure memory is deallocated. */
    if (plane.position != NULL)
    {
        free (plane.position);

        plane.position = NULL;
    }

    /* If position of cube was created successfully, make sure memory is deallocated. */
    if (cube.position != NULL)
    {
        free (cube.position);

        cube.position = NULL;
    }
}

/**
 * \brief Calculates depth values written to shadow map texture.
 *
 * Fills viewMatrixForShadowMapPass matrix with data. That matrix is then used to set the viewMatrix in the shader from the light's point of view.
 */
void calculateLookAtMatrix()
{
    Vec3f upVector = {0.0f, 1.0f, 0.0f};

    /*
     * Position of light is very close to the scene. From this point of view, we are not able to see the whole model.
     * We have to see whole model to calculate depth values in a final pass. To do that, we have to move camera away from the model.
     */
    Vec3f cameraTranslation = {0.0f, 0.0f, -20.0f};

    /* Get lookAt matrix from the light's point of view, directed at the center of a plane. Store result in viewMatrixForShadowMapPass. */
    viewMatrixForShadowMapPass = Matrix::matrixLookAt(light.position,
                                                      lookAtPoint,
                                                      upVector);
    viewMatrixForShadowMapPass = Matrix::createTranslation(cameraTranslation.x,
                                                           cameraTranslation.y,
                                                           cameraTranslation.z) *
                                 viewMatrixForShadowMapPass;
}

/* [Generate geometry data] */
/**
 * \brief Initialize data used for drawing the scene.
 *
 * Retrieve the coordinates of the triangles that make up the cubes and the plane and their normal vectors.
 */
void createDataForObjectsToBeDrawn()
{
    /* Get triangular representation of the scene cube. Store the data in the cubeCoordinates array. */
    CubeModel::getTriangleRepresentation(&cube.coordinates,
                                         &cube.numberOfElementsInCoordinatesArray,
                                         &cube.numberOfPoints,
                                          cube.scalingFactor);
    /* Calculate normal vectors for the scene cube created above. */
    CubeModel::getNormals(&cube.normals,
                          &cube.numberOfElementsInNormalsArray);
    /* Get triangular representation of a square to draw plane in XZ space. Store the data in the planeCoordinates array. */
    PlaneModel::getTriangleRepresentation(&plane.coordinates,
                                          &plane.numberOfElementsInCoordinatesArray,
                                          &plane.numberOfPoints,
                                           plane.scalingFactor);
    /* Calculate normal vectors for the plane. Store the data in the planeNormals array. */
    PlaneModel::getNormals(&plane.normals,
                           &plane.numberOfElementsInNormalsArray);
    /* Get triangular representation of the light cube. Store the data in the lightRepresentationCoordinates array. */
    CubeModel::getTriangleRepresentation(&lightRepresentation.coordinates,
                                         &lightRepresentation.numberOfElementsInCoordinatesArray,
                                         &lightRepresentation.numberOfPoints,
                                          lightRepresentation.scalingFactor);

    ASSERT(cube.coordinates                != NULL, "Could not retrieve cube coordinates.");
    ASSERT(cube.normals                    != NULL, "Could not retrieve cube normals.");
    ASSERT(lightRepresentation.coordinates != NULL, "Could not retrieve cube coordinates.");
    ASSERT(plane.coordinates               != NULL, "Could not retrieve plane coordinates.");
    ASSERT(plane.normals                   != NULL, "Could not retrieve plane normals.");
}
/* [Generate geometry data] */

/**
 * \brief Initialize the data used for rendering.
 *        Store data in buffer objects so that it can be used during draw calls.
 */
void initializeData()
{
    /* Create all needed GL objects. */
    createObjects();

    /* Create all data needed to draw scene. */
    createDataForObjectsToBeDrawn();

    /* [Fill buffer objects with data] */
    /* Buffer holding coordinates of triangles which make up the scene cubes. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          cubeCoordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          cube.numberOfElementsInCoordinatesArray * sizeof(float),
                          cube.coordinates,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of normal vectors for each vertex of the scene cubes. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          cubeNormalsBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          cube.numberOfElementsInNormalsArray * sizeof(float),
                          cube.normals,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of triangles which make up the plane. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          planeCoordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          plane.numberOfElementsInCoordinatesArray * sizeof(float),
                          plane.coordinates,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of the plane's normal vectors. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          planeNormalsBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          plane.numberOfElementsInNormalsArray * sizeof(float),
                          plane.normals,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of the light cube. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          lightRepresentationCoordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          lightRepresentation.numberOfElementsInCoordinatesArray * sizeof(float),
                          lightRepresentation.coordinates,
                          GL_STATIC_DRAW));
    /* [Fill buffer objects with data] */

    /* Buffer holding the positions coordinates of the scene cubes.
     * Data is then used by a uniform buffer object to set the position of each cube drawn using glDrawArraysInstanced(). */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          uniformBlockDataBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          cube.numberOfElementsInPositionArray * sizeof(float),
                          cube.position,
                          GL_STATIC_DRAW));
}

/**
 * \brief Create, compile and attach vertex and fragment shaders to previously created program object, link and set program object to be used.
 * \param[in] programId              Previously created program object name to be used.
 * \param[in] fragmentShaderFileName File name of a fragment shader to be attached to the  program object.
 * \param[in] vertexShaderFileName   File name of a vertex shader to be attached to the program object.
 */
void setUpAndUseProgramObject(GLint programId, const char* fragmentShaderFileName, const char*  vertexShaderFileName)
{
    /* Initialize vertex and shader. */
    GLuint vertexShaderId   = 0;
    GLuint fragmentShaderId = 0;

    Shader::processShader(&vertexShaderId,   vertexShaderFileName,   GL_VERTEX_SHADER);
    Shader::processShader(&fragmentShaderId, fragmentShaderFileName, GL_FRAGMENT_SHADER);

    /* Attach the vertex and fragment shaders to the rendering program. */
    GL_CHECK(glAttachShader(programId, vertexShaderId));
    GL_CHECK(glAttachShader(programId, fragmentShaderId));

    /* Link and use the rendering program object. */
    GL_CHECK(glLinkProgram(programId));
    GL_CHECK(glUseProgram (programId));
}

/**
 * \brief Create a program that will be used to convert vertices into eye-space and then rasterize cubes and plane.
 */
void setupCubesAndPlaneProgram()
{
    /* Create program object. */
    cubesAndPlaneProgram.programId = GL_CHECK(glCreateProgram());

    /* Call function to prepare program object to be used. */
    setUpAndUseProgramObject(cubesAndPlaneProgram.programId,
                             FRAGMENT_SHADER_FILE_NAME,
                             VERTEX_SHADER_FILE_NAME);

    /* Get uniform and attribute locations from current program.
     * Values for those uniforms and attributes will be set later. */
    /* [Get attribute locations] */
    cubesAndPlaneProgram.positionAttributeLocation   = GL_CHECK(glGetAttribLocation  (cubesAndPlaneProgram.programId, "attributePosition"));   /* Attribute that is fed with the vertices of triangles that make up geometry (cube or plane). */
    cubesAndPlaneProgram.normalsAttributeLocation    = GL_CHECK(glGetAttribLocation  (cubesAndPlaneProgram.programId, "attributeNormals"));    /* Attribute that is fed with the normal vectors for geometry (cube or plane). */
    /* [Get attribute locations] */
    cubesAndPlaneProgram.isCameraPointOfViewLocation = GL_CHECK(glGetUniformLocation (cubesAndPlaneProgram.programId, "isCameraPointOfView")); /* Uniform holding boolean value that is used for setting camera or light point of view.
                                                                                                                                                * If true: the camera's point of view is used for drawing scene with light and shadows.
                                                                                                                                                * If false: the light's point of view-used for drawing scene to calculate depth values (create shadow map).
                                                                                                                                                * Vertex shader is used for both calculating depth values to create shadow map and for drawing the scene.
                                                                                                                                                */
    /* [Get uniform location: shouldRenderPlane] */
    cubesAndPlaneProgram.shouldRenderPlaneLocation   = GL_CHECK(glGetUniformLocation (cubesAndPlaneProgram.programId, "shouldRenderPlane"));   /* Uniform holding a boolean value indicating which geometry is being drawn: cube or plane. */
    /* [Get uniform location: shouldRenderPlane] */
    cubesAndPlaneProgram.lightViewMatrixLocation     = GL_CHECK(glGetUniformLocation (cubesAndPlaneProgram.programId, "lightViewMatrix"));     /* Uniform holding the calculated view matrix used to render the scene from the light's point of view. */
    cubesAndPlaneProgram.colorOfGeometryLocation     = GL_CHECK(glGetUniformLocation (cubesAndPlaneProgram.programId, "colorOfGeometry"));     /* Uniform holding the color of a geometry. */
    cubesAndPlaneProgram.lightDirectionLocation      = GL_CHECK(glGetUniformLocation(cubesAndPlaneProgram.programId, "lightDirection"));
    cubesAndPlaneProgram.lightPositionLocation       = GL_CHECK(glGetUniformLocation(cubesAndPlaneProgram.programId, "lightPosition"));
    /* [Get depth texture uniform location] */
    cubesAndPlaneProgram.shadowMapLocation           = GL_CHECK(glGetUniformLocation(cubesAndPlaneProgram.programId, "shadowMap"));
    /* [Get depth texture uniform location] */

    /* Get uniform locations and uniform block index (index of "cubesDataUniformBlock" uniform block) for the current program.
     * Values for those uniforms will be set now (only once, because they are constant).
     */
    GLuint uniformBlockIndex              = GL_CHECK(glGetUniformBlockIndex(cubesAndPlaneProgram.programId, "cubesDataUniformBlock"));    /* Uniform block that holds the position the scene cubes. */
    GLuint planePositionLocation          = GL_CHECK(glGetUniformLocation  (cubesAndPlaneProgram.programId, "planePosition"));            /* Uniform holding the position of plane. */
    GLuint cameraPositionLocation         = GL_CHECK(glGetUniformLocation  (cubesAndPlaneProgram.programId, "cameraPosition"));           /* Uniform holding the position of camera (which is used to render the scene from the camera's point of view). */
    GLuint cameraProjectionMatrixLocation = GL_CHECK(glGetUniformLocation  (cubesAndPlaneProgram.programId, "cameraProjectionMatrix"));   /* Uniform holding the projection matrix (which is used to render the scene from the camera's point of view). */
    GLuint lightProjectionMatrixLocation  = GL_CHECK(glGetUniformLocation  (cubesAndPlaneProgram.programId, "lightProjectionMatrix"));    /* Uniform holding the projection matrix (which is used to render the scene from the light's point of view). */

    /* Check if uniform and attribute locations were found in the shaders. */
    ASSERT(cubesAndPlaneProgram.positionAttributeLocation   != -1,               "Could not retrieve attribute location: positionAttributeLocation.");
    ASSERT(cubesAndPlaneProgram.normalsAttributeLocation    != -1,               "Could not retrieve attribute location: normalsAttributeLocation.");
    ASSERT(cubesAndPlaneProgram.isCameraPointOfViewLocation != -1,               "Could not retrieve uniform location: isCameraPointOfViewLocation.");
    ASSERT(cubesAndPlaneProgram.shouldRenderPlaneLocation   != -1,               "Could not retrieve uniform location: shouldRenderPlaneLocation.")
    ASSERT(cubesAndPlaneProgram.lightViewMatrixLocation     != -1,               "Could not retrieve uniform location: lightViewMatrixLocation.");
    ASSERT(cubesAndPlaneProgram.colorOfGeometryLocation     != -1,               "Could not retrieve uniform location: colorOfGeometryLocation.");
    ASSERT(cubesAndPlaneProgram.lightDirectionLocation      != -1,               "Could not retrieve uniform location: lightDirectionLocation");
    ASSERT(cubesAndPlaneProgram.lightPositionLocation       != -1,               "Could not retrieve uniform location: lightPositionLocation");
    ASSERT(cubesAndPlaneProgram.shadowMapLocation           != -1,               "Could not retrieve uniform location: shadowMapLocation");
    ASSERT(uniformBlockIndex                                != GL_INVALID_INDEX, "Could not retrieve uniform block index: uniformBlockIndex");
    ASSERT(planePositionLocation                            != -1,               "Could not retrieve uniform location: planePositionLocation");
    ASSERT(cameraPositionLocation                           != -1,               "Could not retrieve uniform location: cameraPositionLocation");
    ASSERT(cameraProjectionMatrixLocation                   != -1,               "Could not retrieve uniform location: cameraProjectionMatrixLocation");
    ASSERT(lightProjectionMatrixLocation                    != -1,               "Could not retrieve uniform location: lightProjectionMatrixLocation");

    /*
     * Set the binding point for the uniform block. The uniform block holds the position of the scene cubes.
     * The binding point is then used by glBindBufferBase() to bind buffer object (for GL_UNIFORM_BUFFER).
     */
    GL_CHECK(glUniformBlockBinding(cubesAndPlaneProgram.programId,
                                   uniformBlockIndex,
                                   0));
    GL_CHECK(glUniform3fv         (planePositionLocation,
                                   1,
                                   plane.position));
    GL_CHECK(glUniform3fv         (cameraPositionLocation,
                                   1,
                                   (float*)&cameraPosition));
    GL_CHECK(glUniformMatrix4fv   (cameraProjectionMatrixLocation,
                                   1,
                                   GL_FALSE,
                                   cameraProjectionMatrix.getAsArray()));
    GL_CHECK(glUniformMatrix4fv   (lightProjectionMatrixLocation,
                                   1,
                                   GL_FALSE,
                                   lightProjectionMatrix.getAsArray()));
}

/**
 * \brief Create a program that will be used to rasterize the geometry of light cube.
 */
void setupLightRepresentationProgram()
{
    /* Create program object. */
    lightRepresentationProgram.programId = GL_CHECK(glCreateProgram());

    /* Call function to prepare the program object to be used. */
    setUpAndUseProgramObject(lightRepresentationProgram.programId,
                             SPOT_LIGHT_CUBE_FRAGMENT_SHADER_FILE_NAME,
                             SPOT_LIGHT_CUBE_VERTEX_SHADER_FILE_NAME);

    /* Get the uniform locations for the current program.
     * Values for those uniforms will be set later. */
    lightRepresentationProgram.positionLocation  = GL_CHECK(glGetUniformLocation(lightRepresentationProgram.programId, "cubePosition"));   /* Uniform holding position of a cube (used to calculate translation matrix). */

    /* Get uniform and attribute locations for current program.
     * Values for those uniforms and attribute will be set now (only once, because their are constant). */
    GLuint positionLocation         = GL_CHECK(glGetAttribLocation (lightRepresentationProgram.programId, "attributePosition")); /* Attribute holding coordinates of the triangles that make up the light cube. */
    GLuint projectionMatrixLocation = GL_CHECK(glGetUniformLocation(lightRepresentationProgram.programId, "projectionMatrix"));  /* Uniform holding projection matrix (cube is drawn only from camera point of view,
                                                                                                                                  * not present in shadow map calculation). Camera is static, so the value for this
                                                                                                                                  * uniform can be set just once. */
    GLuint cameraPositionLocation    = GL_CHECK(glGetUniformLocation(lightRepresentationProgram.programId, "cameraPosition"));   /* Uniform holding the position of camera. */

    /* Check if the uniform and attribute locations were found in the shaders. Set their values. */
    ASSERT(positionLocation                            != -1, "Could not retrieve attribute location: positionLocation");
    ASSERT(projectionMatrixLocation                    != -1, "Could not retrieve uniform location: projectionMatrixLocation");
    ASSERT(cameraPositionLocation                      != -1, "Could not retrieve uniform location: cameraPositionLocation");
    ASSERT(lightRepresentationProgram.positionLocation != -1, "Could not retrieve uniform location: lightRepresentationPositionLocation");

    GL_CHECK(glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, cameraProjectionMatrix.getAsArray()));
    GL_CHECK(glUniform3fv      (cameraPositionLocation,   1, (float*)&cameraPosition));

    GL_CHECK(glBindVertexArray        (lightRepresentationCoordinatesVertexArrayObjectId));
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       lightRepresentationCoordinatesBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(positionLocation));
    GL_CHECK(glVertexAttribPointer    (positionLocation,
                                       3,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       0));
}

/**
 * \brief Draw geometry.
 * \param[in] hasShadowMapBeenCalculated If true, will draw the whole scene from the camera's point of view.
 *                                       If false, will draw only the scene cubes and the plane from the light's point of view.
 */
void draw(bool hasShadowMapBeenCalculated)
{
    /* Clear the depth and color buffers. */
    GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

    /* Let's focus on drawing the model. */
    GL_CHECK(glUseProgram(cubesAndPlaneProgram.programId));

     /* Values are the same for both passes (creating shadow map and drawing the scene) - can be set just once. */
    if (!hasShadowMapBeenCalculated)
    {
        /* Set light and shadow values for uniforms. */
        /* [Set uniform values for light position and direction] */
        GL_CHECK(glUniform3fv(cubesAndPlaneProgram.lightDirectionLocation, 1, (float*)&light.direction));
        GL_CHECK(glUniform3fv(cubesAndPlaneProgram.lightPositionLocation,  1, (float*)&light.position));
        /* [Set uniform values for light position and direction] */
        /* [Set texture object for depth texture sampler] */
        GL_CHECK(glUniform1i (cubesAndPlaneProgram.shadowMapLocation,      0));
        /* [Set texture object for depth texture sampler] */
    }

    /* Set uniform value indicating point of view: camera or light. */
    GL_CHECK(glUniform1i(cubesAndPlaneProgram.isCameraPointOfViewLocation, hasShadowMapBeenCalculated));

    /* If the light's point of view, set calculated view matrix. (View is static from camera point of view - no need to change that value). */
    if (!hasShadowMapBeenCalculated)
    {
        GL_CHECK(glUniformMatrix4fv(cubesAndPlaneProgram.lightViewMatrixLocation,
                                    1,
                                    GL_FALSE,
                                    viewMatrixForShadowMapPass.getAsArray()));
    }

     /*Draw cubes. */
    /*Set uniform value indicating which geometry to draw: cubes or plane. Set to draw cubes. */
    /*[Set boolean value to false if cubes are being rendered] */
    GL_CHECK(glUniform1i(cubesAndPlaneProgram.shouldRenderPlaneLocation,
                         false));
    /*[Set boolean value to false if cubes are being rendered] */
    /* Set uniform value indicating the color of geometry (set the color for cubes). */
    GL_CHECK(glUniform4f(cubesAndPlaneProgram.colorOfGeometryLocation,
                         cubesColor[0],
                         cubesColor[1],
                         cubesColor[2],
                         cubesColor[3]));

    /* [Bind VAA for cube] */
    GL_CHECK(glBindVertexArray(cubesVertexArrayObjectId));
    /* [Bind VAA for cube] */

    /* Draw two cubes. */
    /* [Draw cubes] */
    GL_CHECK(glDrawArraysInstanced(GL_TRIANGLES, 0, cube.numberOfPoints, 2));
    /* [Draw cubes] */

    /* Draw plane. */
    /* Set uniform value indicating which shape to draw: cubes or plane. Now set to draw plane. */
    /* [Set boolean value to true if plane is being rendered] */
    GL_CHECK(glUniform1i(cubesAndPlaneProgram.shouldRenderPlaneLocation,
                         true));
    /* [Set boolean value to true if plane is being rendered] */
    /* Set uniform value indicating color of the geometry (set color for plane). */
    GL_CHECK(glUniform4f(cubesAndPlaneProgram.colorOfGeometryLocation,
                         planeColor[0],
                         planeColor[1],
                         planeColor[2],
                         planeColor[3]));

    /* [Bind VAA for plane] */
    GL_CHECK(glBindVertexArray(planeVertexArrayObjectId));
    /* [Bind VAA for plane] */

    /* Draw plane. */
    /* [Draw a plane] */
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, plane.numberOfPoints));
    /* [Draw a plane] */

    if (hasShadowMapBeenCalculated)
    {
        /* Let's focus on drawing the light cube. */
        GL_CHECK(glUseProgram(lightRepresentationProgram.programId));

        /* Set the values for the uniforms (values used for translation and rotation of a cube). */
        GL_CHECK(glUniform3fv(lightRepresentationProgram.positionLocation, 1, (float*)&light.position));

        GL_CHECK(glBindVertexArray(lightRepresentationCoordinatesVertexArrayObjectId));

         /* Draw cube. */
         GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, lightRepresentation.numberOfPoints));
    }
}

/**
 * \brief Draw the scene from the light's point of view to calculate depth values (calculated values are held in shadow map texture).
 */
void createShadowMap()
{
    /* Bind framebuffer object.
     * There is a texture attached to depth attachment point for this framebuffer object.
     * By using this framebuffer object, calculated depth values are stored in the texture. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.framebufferObjectName));

    /* Set the view port to size of shadow map texture. */
    /* [Set viewport for light perspective] */
    GL_CHECK(glViewport(0, 0, shadowMap.width, shadowMap.height));
    /* [Set viewport for light perspective] */

    /* Set back face to be culled */
    GL_CHECK(glEnable(GL_CULL_FACE));

    /* [Enable depth test] */
    /* Enable depth test to do comparison of depth values. */
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    /* [Enable depth test] */

    /* [Set colour mask for shadow map rendering] */
    /* Disable writing of each frame buffer color component. */
    GL_CHECK(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
    /* [Set colour mask for shadow map rendering] */

    /* Update the lookAt matrix that we use for view matrix (to look at scene from the light's point of view). */
    calculateLookAtMatrix();

    /* Draw the scene.
     * Value of parameter indicates that shadow map should now be calculated.
     * Only the plane and scene cubes should be drawn (without cube representing light source).
     * Scene should be rendered from the light's point of view.
     * Enable a polygon offset to ensure eliminate z-fighting in the resulting shadow map. */
    /* [Enable shadow map drawing properties] */
    GL_CHECK(glEnable(GL_POLYGON_OFFSET_FILL));
    /* [Enable shadow map drawing properties] */
    /* [Draw the scene from spot light point of view] */
    draw(false);
    /* [Draw the scene from spot light point of view] */
    GL_CHECK(glDisable(GL_POLYGON_OFFSET_FILL));
}

/**
 * \brief Draw the lit shadow-mapped scene from the camera's point of view.
 */
void drawScene()
{
    /* Use the default framebuffer object. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    /* Set the view port to the size of the window. */
    GL_CHECK(glViewport(0, 0, window.width, window.height));

    /* Disable culling. */
    GL_CHECK(glDisable(GL_CULL_FACE));

    /* Enable writing of each frame buffer color component. */
    GL_CHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    /* Draw the scene.
     * Value of parameter indicates that shadow map has been already calculated and the normal scene should now be drawn.
     * All elements should be drawn (scene cubes, plane, and the light cube).
     * Scene should be rendered from the camera's point of view. */
    draw(true);
}

/**
 * \brief Function called to render the new frame into the back buffer.
 */
void renderFrame()
{
    /* Time used to set light direction and position. */
    const float time = timer.getTime();

    /* Set position of the light. The light is moving on a circular curve (radius of a circle is now equal to 5).*/
    const float radius = 5.0f;

    /* [Update spot light position and direction] */
    light.position.x = radius * sinf(time / 2.0f);
    light.position.y = 2.0f;
    light.position.z = radius * cosf(time / 2.0f);

    /* Direction of light. */
    light.direction.x = lookAtPoint.x - light.position.x;
    light.direction.y = lookAtPoint.y - light.position.y;
    light.direction.z = lookAtPoint.z - light.position.z;

    /* Normalize the light direction vector. */
    light.direction.normalize();
    /* [Update spot light position and direction] */

    /* Clear the contents of back buffer. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Fill the shadow map texture with the calculated depth values. */
    createShadowMap();

    /* Draw the scene consisting of all objects, light and shadow. Use created shadow map to display shadows. */
    drawScene();
}

void setupGraphics(int width, int height)
{
    /* Set up window properties. */
    /* [Declare window resolution] */
    window.height = height;
    window.width  = width;
    /* [Declare window resolution] */

    initializeStructureData();
    initializeData();

    /* Set up the program for rendering the scene. */
    setupCubesAndPlaneProgram();
    setupLightRepresentationProgram();

    /* [Set shadow map drawing properties] */
    /* Set the Polygon offset, used when rendering the into the shadow map to eliminate z-fighting in the shadows. */
    GL_CHECK(glPolygonOffset(1.0, 0.0));
    GL_CHECK(glCullFace(GL_BACK));
    /* [Set shadow map drawing properties] */

    /* [Bind depth texture to specific binding point] */
    /* Set active texture. Shadow map texture will be passed to shader. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D, shadowMap.textureName));
    /* [Bind depth texture to specific binding point] */

    /* [Set up Vertex Attrib Arrays] */
    GL_CHECK(glBindVertexArray(cubesVertexArrayObjectId));
    /* Set values for cubes' normal vectors. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER, cubeNormalsBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(cubesAndPlaneProgram.normalsAttributeLocation));
    GL_CHECK(glVertexAttribPointer    (cubesAndPlaneProgram.normalsAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0));
    /* Set values for the cubes' coordinates. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER, cubeCoordinatesBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(cubesAndPlaneProgram.positionAttributeLocation));
    GL_CHECK(glVertexAttribPointer    (cubesAndPlaneProgram.positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindVertexArray(planeVertexArrayObjectId));
    /* Set values for plane's normal vectors. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER, planeNormalsBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(cubesAndPlaneProgram.normalsAttributeLocation));
    GL_CHECK(glVertexAttribPointer    (cubesAndPlaneProgram.normalsAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0));
    /* Set values for plane's coordinates. */
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER, planeCoordinatesBufferObjectId));
    GL_CHECK(glEnableVertexAttribArray(cubesAndPlaneProgram.positionAttributeLocation));
    GL_CHECK(glVertexAttribPointer    (cubesAndPlaneProgram.positionAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0));
    /* [Set up Vertex Attrib Arrays] */

    /* Bind buffer with uniform data. Used to set the locations of the cubes. */
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBlockDataBufferObjectId));

    /* Start counting time. */
    timer.reset();
}

void uninit()
{
    /* Delete all created GL objects. */
    deleteObjects();
    /* Deallocate memory. */
    deallocateMemory();
}
extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_step  (JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_shadowMapping_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
