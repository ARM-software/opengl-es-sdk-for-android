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

in      vec4  attributePosition;
uniform mat4  projectionMatrix;
uniform vec3  cubePosition;
uniform vec3  cameraPosition;

void main()
{
    /* Create transformation matrices. */
    mat4 modelTranslationMatrix = mat4 (1.0,            0.0,            0.0,            0.0, 
                                        0.0,            1.0,            0.0,            0.0, 
                                        0.0,            0.0,            1.0,            0.0, 
                                        cubePosition.x, cubePosition.y, cubePosition.z, 1.0);
    
    mat4 cameraTranslationMatrix = mat4 (1.0,                0.0,               0.0,              0.0, 
                                         0.0,                1.0,               0.0,              0.0, 
                                         0.0,                0.0,               1.0,              0.0, 
                                         -cameraPosition.x, -cameraPosition.y, -cameraPosition.z, 1.0);
    
    
    /* Multiply model-space coordinates by model-view-projection matrix to bring them into eye-space. */
    gl_Position = projectionMatrix * cameraTranslationMatrix * modelTranslationMatrix * attributePosition;
}