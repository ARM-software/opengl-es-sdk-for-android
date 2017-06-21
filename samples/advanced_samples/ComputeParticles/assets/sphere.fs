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

precision mediump float;
precision mediump sampler2D;

in vec2 vShadowTexel;
in vec3 vNormal;

out vec4 outColor;

uniform vec3 color;
uniform vec3 lightDir;
uniform sampler2D shadowMap0;

float sampleShadow()
{
    vec4 shadow0 = texture(shadowMap0, vShadowTexel);
    return clamp(dot(shadow0, vec4(1.0)), 0.0, 1.0);
}

void main()
{
    outColor.rgb = color * max(dot(normalize(vNormal), lightDir), 0.0);
    outColor.a = 1.0;

    // shadow from particles
    float shadow = sampleShadow();
    outColor.rgb = mix(outColor.rgb, color * vec3(0.1, 0.12, 0.15), shadow);

    // gamma correction
    outColor.rgb = sqrt(outColor.rgb);
}