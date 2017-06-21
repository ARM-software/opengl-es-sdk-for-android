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
 * \file samples/tutorials/ProjectedLights/jni/Native.cpp
 * \brief The application shows the projected lights effect.
 *         There is a spot light effect adjusted to display the texture instead of the normal light colour.
 *         There is also a shadow map technique used to make the scene more realistic by applying some shadows.
 *
 *        The projected lights effect is implemented in two basic steps described as follows:
 *        1. Calculating the shadow map.
 *            a. The scene is rendered from spot light's point of view.
 *            b. The result is stored in the depth texture, which is called a *shadow map*.
 *            c. The shadow map will be used in next steps to verify whether a fragment should be
 *               lit by the spot light or should be obscured by shadow.
 *        2. Scene rendering.
 *            a. The scene (which consists of a plane, on top of which is placed a single cube)
 *               is rendered from the camera's point of view.
 *            b. Directional lighting is implemented to accentuate the 3D scene with the perspective.
 *            c. A spot light effect is implemented, however it is adjusted to display texture rather than
 *               a simple colour.
 *            d. Shadows are computed for the spot lighting (the result of the first step is now used).
 */

#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>
#include "Common.h"
#include "CubeModel.h"
#include "Mathematics.h"
#include "Matrix.h"
#include "PlaneModel.h"
#include "ProjectedLights.h"
#include "Shader.h"
#include "Texture.h"
#include "Timer.h"
#include <cstring>

using namespace MaliSDK;

/**
 * \brief Draw cube and plane model.
 *
 * \note It is assumed that proper program object is made active.
 *
 * \param isCameraPointOfView True, if a scene from camera point of view is supposed to be rendered.
 *                            False in case of rendering a scene from spot light perspective.
 */
static void drawCubeAndPlane(bool isCameraPointOfView);

/**
 * \brief Generate a colour texture object and fill it with data.
 *
 * \note The texture will be projected onto the scene.
 */
static void generateAndPrepareColorTextureObject();

/**
 * \brief Generate a depth texture object, set its properties and bind it to the generated framebuffer object.
 *
 * \note The texture object will be used to store scene depth values calculated from the spot light's point of view.
 */
static void generateAndPrepareDepthTextureObject();

/**
 * \brief Retrieve locations for attributes and uniforms used in a program object
 *        responsible for rendering a scene.
 *
 * \param programObjectId     ID of a program object for which locations are queried.
 * \param locationsStoragePtr Pointer to structure object where retrieved locations will be stored.
 *                            Cannot be NULL.
 */
static void getRenderSceneProgramLocations(GLuint                       programObjectId,
                                           RenderSceneProgramLocations* locationsStoragePtr);

/**
 * \brief Create and compile shader objects.
 *        If successful, they are attached to the program object, which is then linked.
 *
 * \param objectIdsPtr           Deref where generated program and shader objects IDs will be stored.
 *                               Cannot be NULL.
 * \param fragmentShaderFileName Name of fragment shader file.
 *                               Cannot be NULL.
 * \param  vertexShaderFileName  Name of vertex shader file.
 *                               Cannot be NULL.
 */
static void initializeProgramObject(ProgramAndShaderObjectIds* objectIdsPtr,
                                    const char*                fragmentShaderFileName,
                                    const char*                vertexShaderFileName);

/**
 * \brief Initilize data that will be used to translate vertices into eye- and NDC-space.
 */
static void initializeViewMatrices();

/**
 * \brief Function called to render the new frame into the back buffer.
 */
static void renderFrame();

/**
 * \brief Prepare the geometry data that will be used while rendering the scene.
 *
 * Store data in the buffer objects so that it can be used during draw calls.
 */
static void setupGeometryData();

/**
 * \brief Prepare the OpenGLES environment to start the animation.
 *
 * \note There are OpenGLES objects created and filled with data needed for rendering.
 *
 * \param width  Window width.
 * \param height Window height.
 */
static void setupGraphics(int width, int height);

/** \brief Delete created objects and free allocated memory.
 *
 */
static void uninit();

/**
 * \brief Calculate the updated direction of the spot light and update the corresponding
 *        OpenGLES object settings so that the new spot light direction will be used.
 */
static void updateSpotLightDirection();

CameraViewProperties        cameraViewProperties;
GeometryProperties          cubeGeometryProperties;
DirectionalLightProperties  directionalLightProperties;
SpotLightViewProperites     lightViewProperties;
GeometryProperties          planeGeometryProperties;
RenderSceneObjects          renderSceneObjects;
ProgramAndShaderObjectIds   renderSceneProgramAndShadersIds;
RenderSceneProgramLocations renderSceneProgramLocations;
GLsizei                     shadowMapHeight;
GLsizei                     shadowMapWidth;
SpotLightProperties         spotLightProperties;
Timer                       timer;
GLsizei                     windowHeight;
GLsizei                     windowWidth;

/* Please see the specification above. */
static void drawCubeAndPlane(bool isCameraPointOfView)
{
    /* Draw a cube. */
    {
        /* [Set colour of a cube for geometry] */
        /* Set uniform value specific for rendering a cube. */
        GL_CHECK(glUniform4fv(renderSceneProgramLocations.uniformGeometryColor,
                              1,
                              (GLfloat*)&cubeGeometryProperties.color));
        /* [Set colour of a cube for geometry] */

        if (isCameraPointOfView)
        {
            /* Use matrices specific for rendering a scene from camera perspective. */
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.cubeViewProperties.modelViewMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewProjectionMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.cubeViewProperties.modelViewProjectionMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformNormalMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.cubeViewProperties.normalMatrix.getAsArray()));
        }
        else
        {
            /* [Set uniform values for the spot light point of view: cube]*/
            /* Use matrices specific for rendering a scene from spot light perspective. */
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.cubeViewProperties.modelViewMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewProjectionMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.cubeViewProperties.modelViewProjectionMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformNormalMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.cubeViewProperties.normalMatrix.getAsArray()));
            /* [Set uniform values for the spot light point of view: cube]*/
        }

        /* [Bind VAA for cube] */
        /* Set cube's coordinates to be used within a program object. */
        GL_CHECK(glBindVertexArray(renderSceneObjects.renderCube.vertexArrayObjectId));
        /* [Bind VAA for cube] */

        /* [Draw a cube] */
        GL_CHECK(glDrawArrays(GL_TRIANGLES,
                              0,
                              cubeGeometryProperties.numberOfElementsInCoordinatesArray / NUMBER_OF_POINT_COORDINATES));
        /* [Draw a cube] */
    }

    /* Draw a plane. */
    {
        /* [Set colour of a plane for geometry] */
        /* Set uniform value specific for rendering a plane. */
        GL_CHECK(glUniform4fv(renderSceneProgramLocations.uniformGeometryColor,
                              1,
                              (GLfloat*)&planeGeometryProperties.color));
        /* [Set colour of a plane for geometry] */

        if (isCameraPointOfView)
        {
            /* Use matrices specific for rendering a scene from camera perspective. */
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.planeViewProperties.modelViewMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewProjectionMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.planeViewProperties.modelViewProjectionMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformNormalMatrix,
                                        1,
                                        GL_FALSE,
                                        cameraViewProperties.planeViewProperties.normalMatrix.getAsArray()));
        }
        else
        {
            /* [Set uniform values for the spot light point of view: plane]*/
            /* Use matrices specific for rendering a scene from spot light perspective. */
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.planeViewProperties.modelViewMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformModelViewProjectionMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.planeViewProperties.modelViewProjectionMatrix.getAsArray()));
            GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformNormalMatrix,
                                        1,
                                        GL_FALSE,
                                        lightViewProperties.planeViewProperties.normalMatrix.getAsArray()));
            /* [Set uniform values for the spot light point of view: plane]*/
        }

        /* [Bind VAA for plane] */
        /* Set plane's coordinates to be used within a program object. */
        GL_CHECK(glBindVertexArray(renderSceneObjects.renderPlane.vertexArrayObjectId));
        /* [Bind VAA for plane] */

        /* [Draw a plane] */
        GL_CHECK(glDrawArrays(GL_TRIANGLES,
                              0,
                              planeGeometryProperties.numberOfElementsInCoordinatesArray / NUMBER_OF_POINT_COORDINATES));
        /* [Draw a plane] */
    }
}

/* Please see the specification above. */
static void generateAndPrepareColorTextureObject()
{
    GLsizei        imageHeight = 0;
    GLsizei        imageWidth  = 0;
    unsigned char* textureData = NULL;

    /* [Load BMP image data] */
    Texture::loadBmpImageData(COLOR_TEXTURE_NAME, &imageWidth, &imageHeight, &textureData);
    /* [Load BMP image data] */

    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_FOR_COLOR_TEXTURE));
    /* [Generate and bind colour texture object] */
    GL_CHECK(glGenTextures  (1,
                            &renderSceneObjects.colorTextureObjectId));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             renderSceneObjects.colorTextureObjectId));
    /* [Generate and bind colour texture object] */
    /* [Set colour texture object data] */
    GL_CHECK(glTexStorage2D (GL_TEXTURE_2D,
                             1,
                             GL_RGB8,
                             imageWidth,
                             imageHeight));
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D,
                             0,
                             0,
                             0,
                             imageWidth,
                             imageHeight,
                             GL_RGB,
                             GL_UNSIGNED_BYTE,
                             textureData));
    /* [Set colour texture object data] */
    /* [Set colour texture object parameters] */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_R,
                             GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_REPEAT));
    /* [Set colour texture object parameters] */

    /* Data is already copied to the GPU, we can free the allocated memory now. */
    if (textureData != NULL)
    {
        free(textureData);

        textureData = NULL;
    }

    /* Restore default bindings. */
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
}

/* Please see the specification above. */
static void generateAndPrepareDepthTextureObject()
{
    /* Generate and configure shadow map texture to hold depth values. */
    /* [Generate depth texture object] */
    GL_CHECK(glGenTextures  (1,
                            &renderSceneObjects.depthTextureObjectId));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             renderSceneObjects.depthTextureObjectId));
    /* [Generate depth texture object] */
    /* [Prepare depth texture storage] */
    GL_CHECK(glTexStorage2D(GL_TEXTURE_2D,
                            1,
                            GL_DEPTH_COMPONENT24,
                            shadowMapWidth,
                            shadowMapHeight));
    /* [Prepare depth texture storage] */
    /* [Set depth texture object parameters] */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_R,
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
                                   &renderSceneObjects.framebufferObjectId));
    GL_CHECK(glBindFramebuffer     (GL_FRAMEBUFFER,
                                    renderSceneObjects.framebufferObjectId));
    /* [Generate and bind framebuffer object] */
    /* [Bind depth texture to framebuffer] */
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D,
                                    renderSceneObjects.depthTextureObjectId,
                                    0));
    /* [Bind depth texture to framebuffer] */
}

/* Please see the specification above. */
static void getRenderSceneProgramLocations(GLuint                       programObjectId,
                                           RenderSceneProgramLocations* locationsStoragePtr)
{
    ASSERT(programObjectId != 0,
           "Cannot use default program object to retrieve attribute/uniform locations.");
    ASSERT(locationsStoragePtr != NULL,
           "Invalid pointer used to store retrieved attribute/uniform locations.");

    /* Retrieve locations of uniforms and attributes used within the program object. */
    /* [Get attribute locations] */
    locationsStoragePtr->attributeVertexCoordinates            = GL_CHECK(glGetAttribLocation   (programObjectId, "vertexCoordinates"));
    locationsStoragePtr->attributeVertexNormals                = GL_CHECK(glGetAttribLocation   (programObjectId, "vertexNormals"));
    /* [Get attribute locations] */
    /* [Get colour texture uniform location] */
    locationsStoragePtr->uniformColorTexture                   = GL_CHECK(glGetUniformLocation  (programObjectId, "colorTexture"));
    /* [Get colour texture uniform location] */
    locationsStoragePtr->uniformDirectionalLightAmbient        = GL_CHECK(glGetUniformLocation  (programObjectId, "directionalLightAmbient"));
    locationsStoragePtr->uniformDirectionalLightColor          = GL_CHECK(glGetUniformLocation  (programObjectId, "directionalLightColor"));
    locationsStoragePtr->uniformDirectionalLightPosition       = GL_CHECK(glGetUniformLocation  (programObjectId, "directionalLightPosition"));
    locationsStoragePtr->uniformGeometryColor                  = GL_CHECK(glGetUniformLocation  (programObjectId, "geometryColor"));
    locationsStoragePtr->uniformModelViewMatrix                = GL_CHECK(glGetUniformLocation  (programObjectId, "modelViewMatrix"));
    locationsStoragePtr->uniformModelViewProjectionMatrix      = GL_CHECK(glGetUniformLocation  (programObjectId, "modelViewProjectionMatrix"));
    locationsStoragePtr->uniformNormalMatrix                   = GL_CHECK(glGetUniformLocation  (programObjectId, "normalMatrix"));
    /* [Get depth texture uniform location] */
    locationsStoragePtr->uniformShadowMap                      = GL_CHECK(glGetUniformLocation  (programObjectId, "shadowMap"));
    /* [Get depth texture uniform location] */
    locationsStoragePtr->uniformSpotLightColor                 = GL_CHECK(glGetUniformLocation  (programObjectId, "spotLightColor"));
    locationsStoragePtr->uniformSpotLightCosAngle              = GL_CHECK(glGetUniformLocation  (programObjectId, "spotLightCosAngle"));
    locationsStoragePtr->uniformSpotLightLookAtPointInEyeSpace = GL_CHECK(glGetUniformLocation  (programObjectId, "spotLightLookAtPointInEyeSpace"));
    locationsStoragePtr->uniformSpotLightPositionInEyeSpace    = GL_CHECK(glGetUniformLocation  (programObjectId, "spotLightPositionInEyeSpace"));
    locationsStoragePtr->uniformViewToColorTextureMatrix       = GL_CHECK(glGetUniformLocation  (programObjectId, "viewToColorTextureMatrix"));
    locationsStoragePtr->uniformViewToDepthTextureMatrix       = GL_CHECK(glGetUniformLocation  (programObjectId, "viewToDepthTextureMatrix"));

    /* Make sure that the data retrieved is valid. */
    ASSERT(locationsStoragePtr->attributeVertexCoordinates            != -1 &&
           locationsStoragePtr->attributeVertexNormals                != -1 &&
           locationsStoragePtr->uniformColorTexture                   != -1 &&
           locationsStoragePtr->uniformDirectionalLightAmbient        != -1 &&
           locationsStoragePtr->uniformDirectionalLightColor          != -1 &&
           locationsStoragePtr->uniformDirectionalLightPosition       != -1 &&
           locationsStoragePtr->uniformGeometryColor                  != -1 &&
           locationsStoragePtr->uniformModelViewMatrix                != -1 &&
           locationsStoragePtr->uniformModelViewProjectionMatrix      != -1 &&
           locationsStoragePtr->uniformNormalMatrix                   != -1 &&
           locationsStoragePtr->uniformShadowMap                      != -1 &&
           locationsStoragePtr->uniformSpotLightColor                 != -1 &&
           locationsStoragePtr->uniformSpotLightCosAngle              != -1 &&
           locationsStoragePtr->uniformSpotLightLookAtPointInEyeSpace != -1 &&
           locationsStoragePtr->uniformSpotLightPositionInEyeSpace    != -1 &&
           locationsStoragePtr->uniformViewToColorTextureMatrix       != -1 &&
           locationsStoragePtr->uniformViewToDepthTextureMatrix       != -1,
           "At least one of uniform/attribute locations retrieved is not valid. The uniform/attribute seems to be inactive.");
}

/* Please see the specification above. */
static void initializeProgramObject(ProgramAndShaderObjectIds* objectIdsPtr,
                                    const char*                fragmentShaderFileName,
                                    const char*                vertexShaderFileName)
{
    ASSERT(objectIdsPtr != NULL,
           "NULL pointer used to store generated object IDs");

    GLint linkStatus = GL_FALSE;

    objectIdsPtr->programObjectId = GL_CHECK(glCreateProgram());

    Shader::processShader(&objectIdsPtr->fragmentShaderObjectId,
                           fragmentShaderFileName,
                           GL_FRAGMENT_SHADER);
    Shader::processShader(&objectIdsPtr->vertexShaderObjectId,
                           vertexShaderFileName,
                           GL_VERTEX_SHADER);

    GL_CHECK(glAttachShader(objectIdsPtr->programObjectId,
                            objectIdsPtr->fragmentShaderObjectId));
    GL_CHECK(glAttachShader(objectIdsPtr->programObjectId,
                            objectIdsPtr->vertexShaderObjectId));
    GL_CHECK(glLinkProgram (objectIdsPtr->programObjectId));
    GL_CHECK(glGetProgramiv(objectIdsPtr->programObjectId,
                            GL_LINK_STATUS,
                           &linkStatus));

    ASSERT(linkStatus == GL_TRUE,
           "Linking program object failed.");
}

/* Please see the specification above. */
static void initializeViewMatrices()
{
    Matrix cubeRotationMatrix    = Matrix::createRotationY(MODEL_Y_ROTATION_ANGLE_IN_DEGREES);
    Matrix cubeTranslationMatrix = Matrix::createTranslation(cubeGeometryProperties.position.x,
                                                             cubeGeometryProperties.position.y,
                                                             cubeGeometryProperties.position.z);
    Matrix cubeModelMatrix        = cubeRotationMatrix * cubeTranslationMatrix;
    Matrix planeTranslationMatrix = Matrix::createTranslation(planeGeometryProperties.position.x,
                                                              planeGeometryProperties.position.y,
                                                              planeGeometryProperties.position.z);

    /* Initialize matrices that will be used to render geometry from camera point of view. */
    {
        cameraViewProperties.lookAtPoint.x                                 = 0.0f;
        cameraViewProperties.lookAtPoint.y                                 = 0.0f;
        cameraViewProperties.lookAtPoint.z                                 = 0.0f;
        cameraViewProperties.position.x                                    = 0.0f;
        cameraViewProperties.position.y                                    = 0.0f;
        cameraViewProperties.position.z                                    = 20.0f;
        cameraViewProperties.projectionMatrix                              = Matrix::matrixPerspective(degreesToRadians(CAMERA_PERSPECTIVE_FOV_IN_DEGREES),
                                                                                                       float(windowWidth) / float(windowHeight),
                                                                                                       NEAR_PLANE,
                                                                                                       FAR_PLANE);
        cameraViewProperties.upVector.x                                    = 0.0f;
        cameraViewProperties.upVector.y                                    = 1.0f;
        cameraViewProperties.upVector.z                                    = 0.0f;
        cameraViewProperties.viewMatrix                                    = Matrix::matrixLookAt(cameraViewProperties.position,
                                                                                                  cameraViewProperties.lookAtPoint,
                                                                                                  cameraViewProperties.upVector);
        cameraViewProperties.cubeViewProperties.modelMatrix                = cubeModelMatrix;
        cameraViewProperties.cubeViewProperties.modelViewMatrix            = cameraViewProperties.viewMatrix *
                                                                             cameraViewProperties.cubeViewProperties.modelMatrix;
        cameraViewProperties.cubeViewProperties.modelViewProjectionMatrix  = cameraViewProperties.projectionMatrix *
                                                                             cameraViewProperties.cubeViewProperties.modelViewMatrix;

        cameraViewProperties.planeViewProperties.modelMatrix               = planeTranslationMatrix;
        cameraViewProperties.planeViewProperties.modelViewMatrix           = cameraViewProperties.viewMatrix *
                                                                             cameraViewProperties.planeViewProperties.modelMatrix;
        cameraViewProperties.planeViewProperties.modelViewProjectionMatrix = cameraViewProperties.projectionMatrix *
                                                                             cameraViewProperties.planeViewProperties.modelViewMatrix;

        Matrix cubeNormalMatrix  = Matrix::matrixInvert(&cameraViewProperties.cubeViewProperties.modelViewMatrix);
        Matrix planeNormalMatrix = Matrix::matrixInvert(&cameraViewProperties.planeViewProperties.modelViewMatrix);
        Vec4f spotLightPosition  = {spotLightProperties.position.x,
                                    spotLightProperties.position.y,
                                    spotLightProperties.position.z,
                                    1.0f};

        Matrix::matrixTranspose(&cubeNormalMatrix);
        Matrix::matrixTranspose(&planeNormalMatrix);

        cameraViewProperties.cubeViewProperties.normalMatrix  = cubeNormalMatrix;
        cameraViewProperties.planeViewProperties.normalMatrix = planeNormalMatrix;

        cameraViewProperties.spotLightPositionInEyeSpace = Matrix::vertexTransform(&spotLightPosition,
                                                                                   &cameraViewProperties.viewMatrix);
    }

    /* Initialize matrices that will be used to render geometry from spot light point of view. */
    {
        lightViewProperties.cubeViewProperties.modelMatrix = cubeModelMatrix;
        lightViewProperties.planeViewProperties.modelMatrix = planeTranslationMatrix;
        lightViewProperties.lookAtPoint.x                   = 0.0f;
        lightViewProperties.lookAtPoint.y                   = 0.0f;
        lightViewProperties.lookAtPoint.z                   = 0.0f;
        lightViewProperties.position                        = spotLightProperties.position;
        /* [Set projection matrix from spot light point of view] */
        lightViewProperties.projectionMatrix                = Matrix::matrixPerspective(degreesToRadians(LIGHT_PERSPECTIVE_FOV_IN_DEGREES),
                                                                                        1.0f,
                                                                                        NEAR_PLANE,
                                                                                        FAR_PLANE);
        /* [Set projection matrix from spot light point of view] */
        lightViewProperties.upVector.x                      = 0.0f;
        lightViewProperties.upVector.y                      = 1.0f;
        lightViewProperties.upVector.z                      = 0.0f;
    }
}

/* Please see the specification above. */
static void renderFrame()
{
    /* Spot light direction is changing during the rendering process: update it now. */
    updateSpotLightDirection();

    /* Clear the contents of back buffer. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Set uniform values that are common for all the rendering steps. */
    GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformViewToColorTextureMatrix,
                                1,
                                GL_FALSE,
                                cameraViewProperties.viewToColorTextureMatrix.getAsArray()));
    GL_CHECK(glUniformMatrix4fv(renderSceneProgramLocations.uniformViewToDepthTextureMatrix,
                                1,
                                GL_FALSE,
                                cameraViewProperties.viewToDepthTextureMatrix.getAsArray()));
    GL_CHECK(glUniform4fv      (renderSceneProgramLocations.uniformSpotLightLookAtPointInEyeSpace,
                                1,
                                (GLfloat*)&cameraViewProperties.spotLightLookAtPointInEyeSpace));
    GL_CHECK(glUniform4fv      (renderSceneProgramLocations.uniformSpotLightPositionInEyeSpace,
                                1,
                                (GLfloat*)&cameraViewProperties.spotLightPositionInEyeSpace));

    /* 1. Draw a scene from spot light point of view (calculate scene depth). */
    {
        /* Bind framebuffer object.
         * There is a texture attached to depth attachment point for this framebuffer object.
         * By using this framebuffer object, calculated depth values are stored in the texture. */
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, renderSceneObjects.framebufferObjectId));

        /* Clear the depth and colour buffers. */
        GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

        /* [Set viewport for light perspective] */
        /* Set the view port to size of shadow map texture. */
        GL_CHECK(glViewport(0, 0, shadowMapWidth, shadowMapHeight));
        /* [Set viewport for light perspective] */

        /* [Enable shadow map drawing properties] */
        GL_CHECK(glEnable(GL_POLYGON_OFFSET_FILL));
        /* [Enable shadow map drawing properties] */

        /* Enable cull faceing to avoid self shadowing problems: only back faces of the geaometry will be rendered. */
        GL_CHECK(glEnable(GL_CULL_FACE));

        /* [Set colour mask for shadow map rendering] */
        /* Disable writing of each frame buffer colour component. */
        GL_CHECK(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        /* [Set colour mask for shadow map rendering] */

        /* [Draw the scene from spot light point of view] */
        drawCubeAndPlane(false);
        /* [Draw the scene from spot light point of view] */
    } /* 1. */

    /* 2. Draw a scene with lights and shadows from eye point of view. */
    {
        /* Use the default framebuffer object: scene will be rendered on a screen. */
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        /* Clear the depth and colour buffers. */
        GL_CHECK(glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT));

        /* Set the view port to the size of the window. */
        GL_CHECK(glViewport(0, 0, windowWidth, windowHeight));

        GL_CHECK(glDisable(GL_CULL_FACE));
        GL_CHECK(glDisable(GL_POLYGON_OFFSET_FILL));

        /* Enable writing of each frame buffer colour component. */
        GL_CHECK(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

        /* Draw the scene from camera point of view. */
        drawCubeAndPlane(true);
    } /* 2. */
}

/* [Setup geometry data] */
/* Please see the specification above. */
static void setupGeometryData()
{
    /* Get triangular representation of the scene cube. Store the data in the cubeCoordinates array. */
    CubeModel::getTriangleRepresentation(&cubeGeometryProperties.coordinates,
                                         &cubeGeometryProperties.numberOfElementsInCoordinatesArray,
                                          CUBE_SCALING_FACTOR);

    /* Calculate normal vectors for the scene cube created above. */
    CubeModel::getNormals(&cubeGeometryProperties.normals,
                          &cubeGeometryProperties.numberOfElementsInNormalsArray);

    /* Get triangular representation of a square to draw plane in XZ space. Store the data in the planeCoordinates array. */
    PlaneModel::getTriangleRepresentation(&planeGeometryProperties.coordinates,
                                          &planeGeometryProperties.numberOfElementsInCoordinatesArray,
                                           PLANE_SCALING_FACTOR);

    /* Calculate normal vectors for the plane. Store the data in the planeNormals array. */
    PlaneModel::getNormals(&planeGeometryProperties.normals,
                           &planeGeometryProperties.numberOfElementsInNormalsArray);

    /* Fill buffer objects with data. */
    /* Buffer holding coordinates of triangles which make up the scene cubes. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          renderSceneObjects.renderCube.coordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          cubeGeometryProperties.numberOfElementsInCoordinatesArray * sizeof(GLfloat),
                          cubeGeometryProperties.coordinates,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of normal vectors for each vertex of the scene cubes. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          renderSceneObjects.renderCube.normalsBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          cubeGeometryProperties.numberOfElementsInNormalsArray * sizeof(GLfloat),
                          cubeGeometryProperties.normals,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of triangles which make up the plane. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          renderSceneObjects.renderPlane.coordinatesBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          planeGeometryProperties.numberOfElementsInCoordinatesArray * sizeof(GLfloat),
                          planeGeometryProperties.coordinates,
                          GL_STATIC_DRAW));

    /* Buffer holding coordinates of the plane's normal vectors. */
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          renderSceneObjects.renderPlane.normalsBufferObjectId));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          planeGeometryProperties.numberOfElementsInNormalsArray * sizeof(GLfloat),
                          planeGeometryProperties.normals,
                          GL_STATIC_DRAW));
}
/* [Setup geometry data] */

/* Please see the specification above. */
static void setupGraphics(int width, int height)
{
    /* [Declare window resolution] */
    /* Store window size. */
    windowHeight = height;
    windowWidth  = width;
    /* [Declare window resolution] */
    /* [Declare shadow map texture resolution] */
    /* Calculate size of a shadow map texture that will be used. */
    shadowMapHeight = 3 * windowHeight;
    shadowMapWidth  = 3 * windowWidth;
    /* [Declare shadow map texture resolution] */

    /* Set up a directional light properties. */
    directionalLightProperties.ambient    = 0.9f;
    directionalLightProperties.color.x    = 1.0f;
    directionalLightProperties.color.y    = 1.0f;
    directionalLightProperties.color.z    = 1.0f;
    directionalLightProperties.position.x = 0.0f;
    directionalLightProperties.position.y = 0.0f;
    directionalLightProperties.position.z = -1.0f;

    /* Set up a spot light properties. */
    spotLightProperties.color.x    = 1.0f;
    spotLightProperties.color.y    = 1.0f;
    spotLightProperties.color.z    = 1.0f;
    spotLightProperties.position.x = 15.0f;
    spotLightProperties.position.y = 15.0f;
    spotLightProperties.position.z = 15.0f;

    /* Set up properties of the geometry that will be rendered. */
    cubeGeometryProperties.color.x     = 0.8f;
    cubeGeometryProperties.color.y     = 0.1f;
    cubeGeometryProperties.color.z     = 0.2f;
    cubeGeometryProperties.color.w     = 1.0f;
    cubeGeometryProperties.position.x  = 0.0f;
    cubeGeometryProperties.position.y  = MODEL_Y_POSITION;
    cubeGeometryProperties.position.z  = 0.0f;
    planeGeometryProperties.color.x    = 0.2f;
    planeGeometryProperties.color.y    = 0.4f;
    planeGeometryProperties.color.z    = 0.8f;
    planeGeometryProperties.color.w    = 1.0f;
    planeGeometryProperties.position.x = 0.0f;
    planeGeometryProperties.position.y = MODEL_Y_POSITION - CUBE_SCALING_FACTOR;
    planeGeometryProperties.position.z = 0.0f;

    /* Initialize the matrices that are used to translatevertices into specific space. */
    initializeViewMatrices();

    /* [Generate objects for rendering the geometry] */
    /* Generate buffer objects. */
    GL_CHECK(glGenBuffers(1, &renderSceneObjects.renderCube.coordinatesBufferObjectId));
    GL_CHECK(glGenBuffers(1, &renderSceneObjects.renderCube.normalsBufferObjectId));
    GL_CHECK(glGenBuffers(1, &renderSceneObjects.renderPlane.coordinatesBufferObjectId));
    GL_CHECK(glGenBuffers(1, &renderSceneObjects.renderPlane.normalsBufferObjectId));

    /* Generate vertex array objects. */
    GL_CHECK(glGenVertexArrays(1, &renderSceneObjects.renderCube.vertexArrayObjectId));
    GL_CHECK(glGenVertexArrays(1, &renderSceneObjects.renderPlane.vertexArrayObjectId));
    /* [Generate objects for rendering the geometry] */

    /* Initialize program object responsible for rendering the scene. */
    initializeProgramObject(&renderSceneProgramAndShadersIds,
                            FRAGMENT_SHADER_FILE_NAME,
                            VERTEX_SHADER_FILE_NAME);

    /* Initialize OpenGLES objects. */
    generateAndPrepareColorTextureObject();
    generateAndPrepareDepthTextureObject();
    setupGeometryData();

    GL_CHECK(glUseProgram(renderSceneProgramAndShadersIds.programObjectId));

    /* Retrieve the program attribute and uniform locations. */
    getRenderSceneProgramLocations(renderSceneProgramAndShadersIds.programObjectId,
                                  &renderSceneProgramLocations);

    /* Set the uniform data which is constant during the rendering process. */
    /* [Set texture object for colour texture sampler] */
    GL_CHECK(glUniform1i       (renderSceneProgramLocations.uniformColorTexture,
                                TEXTURE_UNIT_FOR_COLOR_TEXTURE));
    /* [Set texture object for colour texture sampler] */
    GL_CHECK(glUniform1f       (renderSceneProgramLocations.uniformDirectionalLightAmbient,
                                directionalLightProperties.ambient));
    GL_CHECK(glUniform3fv      (renderSceneProgramLocations.uniformDirectionalLightColor,
                                1,
                                (GLfloat*)&directionalLightProperties.color));
    GL_CHECK(glUniform3fv      (renderSceneProgramLocations.uniformDirectionalLightPosition,
                                1,
                                (GLfloat*)&directionalLightProperties.position));
    GL_CHECK(glUniform1f       (renderSceneProgramLocations.uniformSpotLightCosAngle,
                                cosf(degreesToRadians(SPOT_LIGHT_ANGLE_IN_DEGREES))));
    /* [Set texture object for depth texture sampler] */
    GL_CHECK(glUniform1i       (renderSceneProgramLocations.uniformShadowMap,
                                TEXTURE_UNIT_FOR_SHADOW_MAP_TEXTURE));
    /* [Set texture object for depth texture sampler] */
    GL_CHECK(glUniform4fv      (renderSceneProgramLocations.uniformSpotLightColor,
                                1,
                                (GLfloat*)&spotLightProperties.color));

    /* [Set up Vertex Attrib Arrays] */
    /* Enable cube VAAs. */
    GL_CHECK(glBindVertexArray        (renderSceneObjects.renderCube.vertexArrayObjectId));
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       renderSceneObjects.renderCube.coordinatesBufferObjectId));
    GL_CHECK(glVertexAttribPointer    (renderSceneProgramLocations.attributeVertexCoordinates,
                                       NUMBER_OF_POINT_COORDINATES,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       NULL));
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       renderSceneObjects.renderCube.normalsBufferObjectId));
    GL_CHECK(glVertexAttribPointer    (renderSceneProgramLocations.attributeVertexNormals,
                                       NUMBER_OF_POINT_COORDINATES,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       NULL));
    GL_CHECK(glEnableVertexAttribArray(renderSceneProgramLocations.attributeVertexCoordinates));
    GL_CHECK(glEnableVertexAttribArray(renderSceneProgramLocations.attributeVertexNormals));

    /* Enable plane VAAs. */
    GL_CHECK(glBindVertexArray        (renderSceneObjects.renderPlane.vertexArrayObjectId));
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       renderSceneObjects.renderPlane.coordinatesBufferObjectId));
    GL_CHECK(glVertexAttribPointer    (renderSceneProgramLocations.attributeVertexCoordinates,
                                       NUMBER_OF_POINT_COORDINATES,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       NULL));
    GL_CHECK(glBindBuffer             (GL_ARRAY_BUFFER,
                                       renderSceneObjects.renderPlane.normalsBufferObjectId));
    GL_CHECK(glVertexAttribPointer    (renderSceneProgramLocations.attributeVertexNormals,
                                       NUMBER_OF_POINT_COORDINATES,
                                       GL_FLOAT,
                                       GL_FALSE,
                                       0,
                                       NULL));
    GL_CHECK(glEnableVertexAttribArray(renderSceneProgramLocations.attributeVertexCoordinates));
    GL_CHECK(glEnableVertexAttribArray(renderSceneProgramLocations.attributeVertexNormals));
    /* [Set up Vertex Attrib Arrays] */

    /* Bind the depth texture so that it can be used as a fragment shader input data. */
    /* [Bind depth texture to specific binding point] */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_FOR_SHADOW_MAP_TEXTURE));
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             renderSceneObjects.depthTextureObjectId));
    /* [Bind depth texture to specific binding point] */
    /* [Set active texture unit for colour texture] */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_FOR_COLOR_TEXTURE));
    /* [Set active texture unit for colour texture] */
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             renderSceneObjects.colorTextureObjectId));

    /* [Set shadow map drawing properties] */
    /* Set the Polygon offset, used when rendering the into the shadow map
     * to eliminate z-fighting in the shadows (if enabled). */
    GL_CHECK(glPolygonOffset(1.0f, 0.0f));
    /* Set back faces to be culled (only when GL_CULL_FACE mode is enabled). */
    GL_CHECK(glCullFace(GL_BACK));
    /* [Set shadow map drawing properties] */
    /* [Enable depth test] */
    /* Enable depth test to do comparison of depth values. */
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    /* [Enable depth test] */
}

/* Please see the specification above. */
static void uninit()
{
    /* Use default program object. */
    GL_CHECK(glUseProgram(0));
    /* Bind default objects. */
    GL_CHECK(glBindBuffer     (GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,  0));
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,   0));

    /* Delete buffers. */
    GL_CHECK(glDeleteBuffers(1, &renderSceneObjects.renderCube.coordinatesBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &renderSceneObjects.renderCube.normalsBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &renderSceneObjects.renderPlane.coordinatesBufferObjectId));
    GL_CHECK(glDeleteBuffers(1, &renderSceneObjects.renderPlane.normalsBufferObjectId));

    /* Delete framebuffer object. */
    GL_CHECK(glDeleteFramebuffers(1, &renderSceneObjects.framebufferObjectId));

    /* Delete textures. */
    GL_CHECK(glDeleteTextures(1, &renderSceneObjects.colorTextureObjectId));
    GL_CHECK(glDeleteTextures(1, &renderSceneObjects.depthTextureObjectId));

    /* Delete vertex arrays. */
    GL_CHECK(glDeleteVertexArrays(1, &renderSceneObjects.renderCube.vertexArrayObjectId));
    GL_CHECK(glDeleteVertexArrays(1, &renderSceneObjects.renderPlane.vertexArrayObjectId));

    /* Delete program and shader objects. */
    GL_CHECK(glDeleteShader (renderSceneProgramAndShadersIds.fragmentShaderObjectId));
    GL_CHECK(glDeleteShader (renderSceneProgramAndShadersIds.vertexShaderObjectId));
    GL_CHECK(glDeleteProgram(renderSceneProgramAndShadersIds.programObjectId));

    /* Free the memory allocated. */
    if (cubeGeometryProperties.coordinates != NULL)
    {
        free(cubeGeometryProperties.coordinates);

        cubeGeometryProperties.coordinates = NULL;
    }

    if (cubeGeometryProperties.normals != NULL)
    {
        free(cubeGeometryProperties.normals);

        cubeGeometryProperties.normals = NULL;
    }

    if (planeGeometryProperties.coordinates != NULL)
    {
        free(planeGeometryProperties.coordinates);

        planeGeometryProperties.coordinates = NULL;
    }

    if (planeGeometryProperties.normals != NULL)
    {
        free(planeGeometryProperties.normals);

        planeGeometryProperties.normals = NULL;
    }
}

/* [Update spot light direction] */
/* Please see the specification above. */
static void updateSpotLightDirection()
{
    /* Time used to set light direction and position. */
    const float currentAngle = timer.getTime() / 4.0f;

    /* Update the look at point coordinates. */
    lightViewProperties.lookAtPoint.x = SPOT_LIGHT_TRANSLATION_RADIUS * sinf(currentAngle);
    lightViewProperties.lookAtPoint.y = -1.0f;
    lightViewProperties.lookAtPoint.z = SPOT_LIGHT_TRANSLATION_RADIUS * cosf(currentAngle);

    /* Update all the view, projection matrixes athat are connected with updated look at point coordinates. */
    Vec4f lookAtPoint = {lightViewProperties.lookAtPoint.x,
                         lightViewProperties.lookAtPoint.y,
                         lightViewProperties.lookAtPoint.z,
                         1.0f};

    /* Get lookAt matrix from the light's point of view, directed at the center of a plane.
     * Store result in viewMatrixForShadowMapPass. */
    lightViewProperties.viewMatrix = Matrix::matrixLookAt(lightViewProperties.position,
                                                          lightViewProperties.lookAtPoint,
                                                          lightViewProperties.upVector);

    lightViewProperties.cubeViewProperties.modelViewMatrix            = lightViewProperties.viewMatrix * lightViewProperties.cubeViewProperties.modelMatrix;
    lightViewProperties.planeViewProperties.modelViewMatrix           = lightViewProperties.viewMatrix * lightViewProperties.planeViewProperties.modelMatrix;
    lightViewProperties.cubeViewProperties.modelViewProjectionMatrix  = lightViewProperties.projectionMatrix * lightViewProperties.cubeViewProperties.modelViewMatrix;
    lightViewProperties.planeViewProperties.modelViewProjectionMatrix = lightViewProperties.projectionMatrix * lightViewProperties.planeViewProperties.modelViewMatrix;
    cameraViewProperties.spotLightLookAtPointInEyeSpace               = Matrix::vertexTransform(&lookAtPoint, &cameraViewProperties.viewMatrix);

    Matrix inverseCameraViewMatrix       = Matrix::matrixInvert(&cameraViewProperties.viewMatrix);
    /* [Define colour texture translation matrix] */
    Matrix colorTextureTranslationMatrix = Matrix::createTranslation(COLOR_TEXTURE_TRANSLATION,
                                                                     0.0f,
                                                                     COLOR_TEXTURE_TRANSLATION);
    /* [Define colour texture translation matrix] */

    /* [Calculate matrix for shadow map sampling: colour texture] */
    cameraViewProperties.viewToColorTextureMatrix = Matrix::biasMatrix                   *
                                                    lightViewProperties.projectionMatrix *
                                                    lightViewProperties.viewMatrix       *
                                                    colorTextureTranslationMatrix        *
                                                    inverseCameraViewMatrix;
    /* [Calculate matrix for shadow map sampling: colour texture] */
    /* [Calculate matrix for shadow map sampling: depth texture] */
    cameraViewProperties.viewToDepthTextureMatrix = Matrix::biasMatrix                   *
                                                    lightViewProperties.projectionMatrix *
                                                    lightViewProperties.viewMatrix       *
                                                    inverseCameraViewMatrix;
    /* [Calculate matrix for shadow map sampling: depth texture] */
}
/* [Update spot light direction] */

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_step  (JNIEnv * env, jobject obj);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_projectedLights_NativeLibrary_step(
        JNIEnv * env, jobject obj)
{
    renderFrame();
}
