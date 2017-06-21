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

/* Coordinates of the drawn lines. */
in vec4  position;
/* Input UV coordinates. */
in vec2  vertexTexCoord;

/* Model-View-Projection matrix. */
uniform mat4  mvpMatrix;
/* A uniform used to determine the distance from the bottom of the drawn geometry of the currently drawn line. */
uniform float verticalOffset;

/* UV coordinates passed to fragment shader. */
out vec2 fragmentTexCoord;

void main()
{
    /* Pass texture coordinates to fragment shader. */
    fragmentTexCoord = vertexTexCoord;

    /* Determine gl_Position modified by verticalOffset. */
    gl_Position = (mvpMatrix * position) - vec4(0.0, verticalOffset, 0.0, 0.0);
}
