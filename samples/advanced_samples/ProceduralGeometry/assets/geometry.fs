#version 310 es

/* Copyright (c) 2015-2017, ARM Limited and Contributors
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

precision highp float;

in vec3 gs_position;
in vec2 gs_quadcoord;
in vec3 gs_normal;
uniform sampler2D inMaterial;
out vec4 f_color;

float Wireframe(float s, float t, float wx, float wy)
{
    float h = 1.0 - (smoothstep(0.0, wx, s) - smoothstep(1.0 - wx, 1.0, s));
    float v = 1.0 - (smoothstep(0.0, wy, t) - smoothstep(1.0 - wy, 1.0, t));
    return clamp(h + v, 0.0, 1.0);
}

vec3 light(vec3 n, vec3 l, vec3 c)
{
    float ndotl = max(dot(n, l), 0.0);
    return c * ndotl;
}

void main()
{
    // Edges might look scruffy, so we reject them here
    float r2 = dot(gs_position, gs_position);
    vec3 a = abs(gs_position);
    if (any(bvec3(a.x > 0.95, a.y > 0.95, a.z > 0.95)))
        discard;

    float wx = fwidth(gs_quadcoord.x);
    float wy = fwidth(gs_quadcoord.y);
    float wireframe = Wireframe(gs_quadcoord.x, gs_quadcoord.y, wx, wy);

    // Altitude-based color lookup
    float u = smoothstep(-0.15, 0.12, gs_position.y);
    float v = 0.5 + 0.5 * sin(dot(gs_position.xz, gs_position.xz) * 0.2);
    vec3 albedo = texture(inMaterial, vec2(u, v)).rgb;

    // Clouds have their own color
    if (gs_position.y > 0.5)
        albedo = vec3(1.1, 1.0, 0.85);

    // Lighting
    vec3 n = normalize(gs_normal);
    vec3 color = vec3(0.0);
    color += light(n, normalize(vec3(-0.3, 1.0, -0.2)), vec3(1.3, 1.0, 0.6) * 0.2);
    color += light(n, normalize(vec3(1.0)), vec3(1.3, 1.2, 0.8) * 1.2);
    color += light(n, normalize(vec3(-1.0, -1.0, 0.2)), vec3(0.72, 0.85, 0.93) * 0.4);
    color *= albedo;

    // Wireframe blending
    float wireframe_amount = 0.1;
    color = mix(color, vec3(1.7, 1.6, 2.0) * color, wireframe_amount * wireframe);

    f_color = vec4(color, 1.0);
}
