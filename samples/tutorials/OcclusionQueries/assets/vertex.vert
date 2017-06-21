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

/* Vertex and normal vector attributes sent from the program. */
in vec4 vertex;
in vec4 normal;

/* This matrix is used to set the perspective up and is sent from the program. */
uniform mat4 projectionMatrix;

/* This matrix is used to set the view up and is sent from the program. */
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 normalMatrix;
uniform mat4 mvpMatrix;

/* Output normal vector (used to compute light). */
out vec3 normalOut;

/* Vertex position in world space that we pass to fragment shader. */
out vec4 modelPosition;

/* Inverted model-view-projection matrix passed to the fragment shader to compute light. */
out mat4 worldInverse;

void main()
{
    /* Calculate normal vector in world space (used to calculate light). */
    normalOut = vec3(normalMatrix * normal);

    modelPosition = modelMatrix * vertex;
	
    gl_Position = mvpMatrix * vertex;
}
/* [Vertex shader code] */