#version 300 es

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

/* [Vertex shader code] */
/* [Define number of cubes] */
const int   numberOfCubes = 10;
/* [Define number of cubes] */
const float pi            = 3.14159265358979323846;
const float radius        = 20.0;

in      vec4 attributeColor;
in      vec4 attributePosition;
out     vec4 vertexColor;
uniform vec3 cameraVector;
uniform vec4 perspectiveVector;

uniform float time; /* Time value used for determining positions and rotations. */

/* [Define uniform block] */
/*
 * We use uniform block in order to reduce amount of memory transfers to minimum. 
 * The uniform block uses data taken directly from a buffer object. 
 */
uniform CubesUniformBlock
{
    float startPosition[numberOfCubes];
    vec4  cubeColor[numberOfCubes];
};
/* [Define uniform block] */

void main()
{
    float fieldOfView = 1.0 / tan(perspectiveVector.x * 0.5);
    
    /* Vector data used for translation of cubes (each cube is placed on and moving around a circular curve). */
    vec3 locationOfCube = vec3(radius * cos(startPosition[gl_InstanceID] + (time/3.0)),
                               radius * sin(startPosition[gl_InstanceID] + (time/3.0)),
                               1.0);

    /* 
     * Vector data used for setting rotation of cube. Each cube has different speed of rotation,
     * first cube has the slowest rotation, the last one has the fastest. 
     */
    vec3 rotationOfube = vec3 (float(gl_InstanceID + 1) * 5.0 * time);
    
    /* 
     * Set different random colours for each cube. 
     * There is one colour passed in per cube set for each cube (cubeColor[gl_InstanceID]).
     * There are also different colours per vertex of a cube (attributeColor).
     */
    vertexColor = attributeColor * cubeColor[gl_InstanceID];
    
    /* Create transformation matrices. */
    mat4 translationMatrix = mat4 (1.0,             0.0,             0.0,             0.0, 
                                   0.0,             1.0,             0.0,             0.0, 
                                   0.0,             0.0,             1.0,             0.0, 
                                   locationOfCube.x, locationOfCube.y, locationOfCube.z, 1.0);
                                  
    mat4 cameraMatrix = mat4 (1.0,           0.0,           0.0,           0.0, 
                              0.0,              1.0,           0.0,           0.0, 
                              0.0,           0.0,           1.0,           0.0, 
                              cameraVector.x, cameraVector.y, cameraVector.z, 1.0);
    
    mat4 xRotationMatrix = mat4 (1.0,  0.0,                               0.0,                                0.0, 
                                 0.0,  cos(pi * rotationOfube.x / 180.0), sin(pi * rotationOfube.x / 180.0),  0.0, 
                                 0.0, -sin(pi * rotationOfube.x / 180.0), cos(pi * rotationOfube.x / 180.0),  0.0, 
                                 0.0,  0.0,                               0.0,                                1.0);
                                
    mat4 yRotationMatrix = mat4 (cos(pi * rotationOfube.y / 180.0), 0.0, -sin(pi * rotationOfube.y / 180.0), 0.0, 
                                 0.0,                               1.0, 0.0,                                0.0, 
                                 sin(pi * rotationOfube.y / 180.0), 0.0, cos(pi * rotationOfube.y / 180.0),  0.0, 
                                 0.0,                               0.0, 0.0,                                1.0);
                                
    mat4 zRotationMatrix = mat4 ( cos(pi * rotationOfube.z / 180.0), sin(pi * rotationOfube.z / 180.0), 0.0, 0.0, 
                                 -sin(pi * rotationOfube.z / 180.0), cos(pi * rotationOfube.z / 180.0), 0.0, 0.0, 
                                  0.0,                               0.0,                               1.0, 0.0, 
                                  0.0,                               0.0,                               0.0, 1.0);
                                 
    mat4 perspectiveMatrix = mat4 (fieldOfView/perspectiveVector.y, 0.0,        0.0,                                                                                              0.0, 
                                   0.0,                            fieldOfView, 0.0,                                                                                              0.0, 
                                   0.0,                            0.0,        -(perspectiveVector.w + perspectiveVector.z) / (perspectiveVector.w - perspectiveVector.z),        -1.0, 
                                   0.0,                            0.0,        (-2.0 * perspectiveVector.w * perspectiveVector.z) / (perspectiveVector.w - perspectiveVector.z), 0.0);

    /* Compute rotation. */
    mat4 tempMatrix = xRotationMatrix;
    
    tempMatrix = yRotationMatrix * tempMatrix;
    tempMatrix = zRotationMatrix * tempMatrix;
    
    /* Compute translation. */
    tempMatrix = translationMatrix * tempMatrix;
    tempMatrix = cameraMatrix      * tempMatrix;
                
    /* Compute perspective. */
    tempMatrix = perspectiveMatrix * tempMatrix;
                
    /* Return gl_Position. */
    gl_Position = tempMatrix * attributePosition;
}
/* [Vertex shader code] */