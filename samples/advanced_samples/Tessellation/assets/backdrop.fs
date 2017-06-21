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

in vec2 v_texel;
out vec4 f_color;

uniform mat4 view;
uniform vec2 screen_size;
uniform vec3 sun_dir;
uniform float inv_tan_fov;

float TraceFloor(vec3 ro, vec3 rd)
{
    return (-1.2 - ro.y) / rd.y;
}

vec3 Sky(vec3 rd)
{
    float rdl = max(dot(sun_dir, rd), 0.0);

    vec3 color = vec3(0.18, 0.25, 0.35) * (1.0 - 0.8 * rd.y) * 0.85;
    color += 0.15 * vec3(1.0, 0.95, 0.7) * pow(rdl, 1.0);
    color += 0.25 * vec3(1.0, 0.95, 0.7) * pow(rdl, 16.0);
    color += 0.25 * vec3(1.0, 0.95, 0.8) * pow(rdl, 512.0);
    color += 0.15 * vec3(1.0, 0.95, 0.5) * pow(clamp(1.0 - rd.y, 0.0, 1.0), 4.0);
    return color;
}
float CheckerPattern(vec3 vPosition)
{
    // Procedural checkerpatterns! :D
    float d = 0.03;
    float size = 0.75;
    float fx = mod(vPosition.x / size, 2.0);
    fx = smoothstep(1.0 - d, 1.0, fx) - smoothstep(2.0 - d, 2.0, fx);
    float fz = mod(vPosition.z / size, 2.0);
    fz = smoothstep(1.0 - d, 1.0, fz) - smoothstep(2.0 - d, 2.0, fz);
    float pattern = fx * fz + (1.0 - fx) * (1.0 - fz);
    return pattern;
}

void main()
{
    float u = -1.0 + 2.0 * gl_FragCoord.x / screen_size.x;
    float v = -1.0 + 2.0 * gl_FragCoord.y / screen_size.y;
    u *= screen_size.x / screen_size.y;

    // Compute the camera basis from the view-transformation matrix
    mat3 R = mat3(transpose(view));
    vec3 right = normalize(R[0].xyz);
    vec3 up = normalize(R[1].xyz);
    vec3 forward = normalize(R[2].xyz);

    // Compute the ray origin and direction
    vec3 ro = -R * view[3].xyz;
    vec3 rd = normalize(-forward * inv_tan_fov + right * u + up * v);

    float t = TraceFloor(ro, rd);
    vec3 p = ro + t * rd;
    if (t >= 0.0)
    {
        float r2 = dot(p.xz, p.xz);
        float cp = CheckerPattern(p);
        float fog = clamp(length(p.xz) / 25.0, 0.0, 1.0);
        f_color.rgb = vec3(0.8, 0.79, 0.72);
        f_color.rgb -= vec3(0.22, 0.23, 0.25) * cp * (1.0 - fog);
        f_color.rgb = mix(f_color.rgb, Sky(rd), fog);

        // Sphere shadow
        f_color.rgb *= 1.0 - exp2(-3.0 * r2);
    }
    else
    {
        f_color.rgb = Sky(rd);
    }

    f_color.a = 1.0;
}
