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
/* [Define attributes] */
/* ATTRIBUTES */
in vec4 vertexCoordinates; /* Attribute: holding coordinates of triangles that make up a geometry. */
in vec3 vertexNormals;     /* Attribute: holding normals. */
/* [Define attributes] */

/* UNIFORMS */
uniform mat4 modelViewMatrix;           /* Model * View matrix */
uniform mat4 modelViewProjectionMatrix; /* Model * View * Projection matrix */
uniform mat4 normalMatrix;              /* transpose(inverse(Model * View)) matrix */

/* OUTPUTS */
out vec3 normalInEyeSpace; /* Normal vector for the coordinates. */
out vec4 vertexInEyeSpace; /* Vertex coordinates expressed in eye space. */

void main()
{
    /* Calculate and set output vectors. */
    normalInEyeSpace = mat3x3(normalMatrix) * vertexNormals;
    vertexInEyeSpace = modelViewMatrix      * vertexCoordinates;

    /* Multiply model-space coordinates by model-view-projection matrix to bring them into eye-space. */
    gl_Position = modelViewProjectionMatrix * vertexCoordinates;
}
 /* [Vertex shader source] */
