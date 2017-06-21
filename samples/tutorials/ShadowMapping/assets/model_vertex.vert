#version 300 es
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

/* [Vertex shader code] */
/* Number of cubes to be drawn. */ 
#define numberOfCubes 2

/* [Define attributes] */
in vec4 attributePosition; /* Attribute: holding coordinates of triangles that make up a geometry. */
in vec3 attributeNormals;  /* Attribute: holding normals. */
/* [Define attributes] */

uniform mat4 cameraProjectionMatrix; /* Projection matrix from camera point of view. */
uniform mat4 lightProjectionMatrix;  /* Projection matrix from light  point of view. */

uniform mat4 lightViewMatrix; /* View matrix from light point of view. */
uniform vec3 cameraPosition;  /* Camera position which we use to calculate view matrix for final pass. */

uniform vec3 lightPosition; /* Vector of position of spot light source. */

uniform bool isCameraPointOfView; /* If true: perform calculations from camera point of view, else: from light point of view. */
uniform bool shouldRenderPlane;   /* If true: draw plane, else: draw cubes. */

uniform vec3 planePosition; /* Position of plane used to calculate translation matrix for a plane. */

/* Uniform block holding data used for rendering cubes (position of cubes) - used to calculate translation matrix for each cube in world space. */
uniform cubesDataUniformBlock
{
    vec4 cubesPosition[numberOfCubes];
};

out vec4 outputLightPosition;       /* Output variable: vector of position of spot light source translated into eye-space. */
out vec3 outputNormal;              /* Output variable: normal vector for the coordinates. */
out vec4 outputPosition;            /* Output variable: vertex coordinates expressed in eye space. */
out mat4 outputViewToTextureMatrix; /* Output variable: matrix we will use in the fragment shader to sample the shadow map for given fragment. */

void main()
{
    /* View matrix calculated from camera point of view. */
    mat4 cameraViewMatrix;

    /* Matrices and vectors used for calculating output variables. */
    vec3 modelPosition;
    mat4 modelViewMatrix;
    mat4 modelViewProjectionMatrix;

    /* Model consists of plane and cubes (each of them has different colour and position). */
    /* [Use different position for a specific geometry] */
    if (shouldRenderPlane)
    {
        modelPosition = planePosition;
    }
    else
    {
        modelPosition = vec3(cubesPosition[gl_InstanceID].x, cubesPosition[gl_InstanceID].y, cubesPosition[gl_InstanceID].z);
    }
    /* [Use different position for a specific geometry] */

    /* Create transformation matrix (translation of a model). */
    mat4 translationMatrix = mat4 (1.0,             0.0,             0.0,             0.0, 
                                   0.0,             1.0,             0.0,             0.0, 
                                   0.0,             0.0,             1.0,             0.0, 
                                   modelPosition.x, modelPosition.y, modelPosition.z, 1.0);

    /* Compute matrices for camera point of view. */
    if (isCameraPointOfView == true)
    {
        cameraViewMatrix = mat4 ( 1.0,                 0.0,               0.0,              0.0, 
                                  0.0,                 1.0,               0.0,              0.0, 
                                  0.0,                 0.0,               1.0,              0.0, 
                                  -cameraPosition.x,  -cameraPosition.y, -cameraPosition.z, 1.0);
                                            
        /* Compute model-view matrix. */
        modelViewMatrix = cameraViewMatrix * translationMatrix;
        /* Compute  model-view-perspective matrix. */
        modelViewProjectionMatrix = cameraProjectionMatrix * modelViewMatrix;
    
    }
    /* Compute matrices for light point of view. */
    else
    {
        /* Compute model-view matrix. */
        modelViewMatrix = lightViewMatrix * translationMatrix;
        /* Compute model-view-perspective matrix. */
        modelViewProjectionMatrix = lightProjectionMatrix * modelViewMatrix;
    }

    /* [Define bias matrix] */
    /* Bias matrix used to map values from a range <-1, 1> (eye space coordinates) to <0, 1> (texture coordinates). */
    const mat4 biasMatrix = mat4(0.5, 0.0, 0.0, 0.0,
                                 0.0, 0.5, 0.0, 0.0,
                                 0.0, 0.0, 0.5, 0.0,
                                 0.5, 0.5, 0.5, 1.0);
    /* [Define bias matrix] */
    
    /* Calculate normal matrix. */
    mat3 normalMatrix = transpose(inverse(mat3x3(modelViewMatrix)));

    /* Calculate and set output vectors. */
    outputLightPosition = modelViewMatrix * vec4(lightPosition, 1.0);
    outputNormal        = normalMatrix    * attributeNormals;
    outputPosition      = modelViewMatrix * attributePosition;

    if (isCameraPointOfView)
    {
        /* [Calculate matrix that will be used to convert camera to eye space] */
        outputViewToTextureMatrix = biasMatrix * lightProjectionMatrix * lightViewMatrix * inverse(cameraViewMatrix);
        /* [Calculate matrix that will be used to convert camera to eye space] */
    }

    /* Multiply model-space coordinates by model-view-projection matrix to bring them into eye-space. */
    gl_Position = modelViewProjectionMatrix * attributePosition;
}
/* [Vertex shader code] */