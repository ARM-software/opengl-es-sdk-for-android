#version 310 es

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

in vec4 position;

out vec4 mask0;

uniform mat4 projection;
uniform mat4 view;
const float scale = 0.7;

void main()
{
    vec4 viewPos = view * vec4(position.xyz, 1.0);
    gl_Position = projection * viewPos;
    gl_PointSize = scale * (16.0 - 6.0 * (length(viewPos.xyz) - 1.0) / (3.0 - 1.0));

    float z = 0.5 + 0.5 * gl_Position.z;
    mask0 = clamp(floor( mod(vec4(z) + vec4(1.00, 0.75, 0.50, 0.25), vec4(1.25)) ), vec4(0.0), vec4(1.0));
}