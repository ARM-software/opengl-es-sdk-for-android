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

/* [Fragment shader source] */
precision mediump float;

/* Input normal vector. */
in vec3 modelViewProjectionNormalVector;

/* Structure storing directional light parameters. */
struct Light
{
    vec3  lightColor;
    vec3  lightDirection;
    float ambientIntensity;
};

/* Color of the drawn torus. */
uniform vec4  color;
/* Uniform representing light parameters. */
uniform Light light;

/* Output variable. */
out vec4 fragColor;

void main()
{
    /* Calculate the value of diffuse intensity. */
    float diffuseIntensity = max(0.0, -dot(modelViewProjectionNormalVector, normalize(light.lightDirection)));

    /* Calculate the output color value considering the light. */
    fragColor = color * vec4(light.lightColor * (light.ambientIntensity + diffuseIntensity), 1.0);
}
/* [Fragment shader source] */