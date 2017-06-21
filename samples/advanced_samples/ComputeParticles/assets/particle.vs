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
 
precision mediump sampler2D;

in vec4 position;

out float lifetime;
out vec3 color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 projectionViewLight;
uniform vec3 smokeColor;
uniform vec3 smokeShadow;
uniform sampler2D shadowMap0;

const float scale = 0.85;

void main()
{
    vec4 viewPos = view * vec4(position.xyz, 1.0);
    float viewDist = length(viewPos.xyz);
    gl_PointSize = scale * (20.0 - 6.0 * (viewDist - 1.0) / (3.0 - 1.0));
    lifetime = position.w;
    gl_Position = projection * viewPos;

    // Project particle position onto lightmap
    vec4 lightPos = projectionViewLight * vec4(position.xyz, 1.0);
    vec3 shadowTexel = vec3(0.5) + 0.5 * lightPos.xyz;
    vec4 shadow0 = texture(shadowMap0, shadowTexel.xy);

    // Mask out shadow layers that are ahead of the particle's own layer
    // and linearly interpolate between them based on its depth
    vec4 mask0 = clamp( (vec4(shadowTexel.z) - vec4(0.00, 0.25, 0.50, 0.75)) * 4.0, vec4(0.0), vec4(1.0));
    shadow0 *= mask0;

    // Sum up the components weighted by the mask
    float shadow = shadow0.x + shadow0.y + shadow0.z + shadow0.w;
    shadow = clamp(shadow, 0.0, 1.0);

    color = smokeColor * (1.0 - shadow) + shadow * smokeShadow;
}