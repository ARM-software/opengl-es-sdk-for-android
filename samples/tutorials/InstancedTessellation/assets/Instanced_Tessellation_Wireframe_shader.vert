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

/* [Vertex shader source] */
/* Input vertex coordinates. */ 
in vec4 position;

/* Constant transformation matrices. */
uniform mat4 cameraMatrix;
uniform mat4 projectionMatrix;
uniform mat4 scaleMatrix;

/* Coefficients of rotation needed for configuration of rotation matrix. */
uniform vec3 rotationVector;

void main()
{
    mat4 modelViewMatrix;
    mat4 modelViewProjectionMatrix;
    
    /* Matrix rotating Model-View matrix around X axis. */
    mat4 xRotationMatrix = mat4(1.0,  0.0,                            0.0,                            0.0, 
                                0.0,  cos(radians(rotationVector.x)), sin(radians(rotationVector.x)), 0.0, 
                                0.0, -sin(radians(rotationVector.x)), cos(radians(rotationVector.x)), 0.0, 
                                0.0,  0.0,                            0.0,                            1.0);
    
    /* Matrix rotating Model-View matrix around Y axis. */
    mat4 yRotationMatrix = mat4( cos(radians(rotationVector.y)), 0.0, -sin(radians(rotationVector.y)), 0.0, 
                                 0.0,                            1.0,  0.0,                            0.0, 
                                 sin(radians(rotationVector.y)), 0.0,  cos(radians(rotationVector.y)), 0.0, 
                                 0.0,                            0.0,  0.0,                            1.0);
    
    /* Matrix rotating Model-View matrix around Z axis. */
    mat4 zRotationMatrix = mat4( cos(radians(rotationVector.z)), sin(radians(rotationVector.z)), 0.0, 0.0, 
                                -sin(radians(rotationVector.z)), cos(radians(rotationVector.z)), 0.0, 0.0, 
                                 0.0,                            0.0,                            1.0, 0.0, 
                                 0.0,                            0.0,                            0.0, 1.0);
    
    /* Model-View matrix transformations. */
    modelViewMatrix = scaleMatrix;
    modelViewMatrix = xRotationMatrix  * modelViewMatrix;
    modelViewMatrix = yRotationMatrix  * modelViewMatrix;
    modelViewMatrix = zRotationMatrix  * modelViewMatrix;
    modelViewMatrix = cameraMatrix     * modelViewMatrix;
    
    /* Configure Model-View-ProjectionMatrix. */
    modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
    
    /* Set vertex position in Model-View-Projection space. */
    gl_Position = modelViewProjectionMatrix * position;
}
/* [Vertex shader source] */