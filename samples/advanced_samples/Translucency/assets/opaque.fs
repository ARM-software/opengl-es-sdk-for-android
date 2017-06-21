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

#extension GL_EXT_shader_pixel_local_storage : require
precision highp float;

__pixel_localEXT FragDataLocal {
    layout(rgb10_a2) vec4 lighting;
    layout(rg16f) vec2 minMaxDepth;
    layout(rgb10_a2) vec4 albedo;
    layout(rg16f) vec2 normalXY;
} storage;

in vec4 vPosition;
in vec3 vNormal;

uniform vec3 lightPos0;
uniform vec3 lightPos1;
uniform vec3 lightCol0;
uniform vec3 lightCol1;
uniform float lightInt0;
uniform float lightInt1;

vec3 lambert(vec3 P, vec3 Lp, vec3 N, vec3 Ldiff, float Li)
{
    vec3 L = Lp - P;
    float r = length(L);
    L /= r;
    float NdotL = max(dot(N, L), 0.0);
    float att = Li / (1.0 + 0.4 * r * r);
    return att * NdotL * Ldiff;
}

void main()
{
    vec3 P = vPosition.xyz;
    vec3 N = normalize(vNormal);

    /* Procedural checkerpatterns! :D

    1: Take your floor coordinates modulo some size.
    This will make the coordinates loop in the range [0, size).

    2: Divide the looped coordinates by the size, to normalize
    to the range [0, 1).

    3: Create smooth transitions between edges along both axes.
    The parameter d determines how smooth the transition is.
     ______ ______
    |      |      |   00 ~ fx = 0 and fz = 0
    |  00  |  10  |   01 ~ fx = 0 and fz = 1
    |______|______|   10 ~ fx = 1 and fz = 0
    |      |      |   11 ~ fx = 1 and fz = 1
    |  01  |  11  |
    |______|______|

    Here: fx = 0 when x is on the left, and fx = 1 when x is on the right.
    It is a blend when x is in the middle.

    4: Select out the pattern. We want white where fx and fy are both 0
    or both 1. That is, when fx * fz + (1.0 - fx) * (1.0 - fz) is 1.
    */
    float d = 0.03;
    float size = 0.35;
    float fx = mod(vPosition.x / size, 2.0);
    fx = smoothstep(1.0 - d, 1.0, fx) - smoothstep(2.0 - d, 2.0, fx);
    float fz = mod(vPosition.z / size, 2.0);
    fz = smoothstep(1.0 - d, 1.0, fz) - smoothstep(2.0 - d, 2.0, fz);
    float pattern = fx * fz + (1.0 - fx) * (1.0 - fz);

    // Make floor dark when stuff is above it
    float ao = 1.0 - exp2(-1.0 * dot(vPosition.xz, vPosition.xz));

    vec3 color = vec3(0.0);
    color += lambert(P, lightPos0, N, lightCol0, lightInt0);
    color += lambert(P, lightPos1, N, lightCol1, lightInt1);
    color *= (pattern + 0.2 * (1.0 - pattern)) * ao;

    storage.lighting.rgb = color;
    storage.lighting.a = 1.0;
}
