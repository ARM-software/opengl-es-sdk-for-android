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

float noise1f(int x)
{
    x = (x<<13) ^ x;
    return ( 1.0 - float((x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

float snoise(float x)
{
    int xi = int(floor(x));
    float xf = x - float(xi);

    float h0 = noise1f(xi);
    float h1 = noise1f(xi + 1);

    // Smoothly nterpolate between noise values
    float t = smoothstep(0.0, 1.0, xf);
    return h0 + (h1 - h0) * t;
}

layout (local_size_x = 64) in;

layout (std140, binding = 0) buffer SpawnBuffer {
    vec4 SpawnInfo[];
};

uniform vec3 emitterPos;
uniform float particleLifetime;
uniform float time;

void main()
{
    uint index = gl_GlobalInvocationID.x;

    vec3 p;

    // Random offset
    float seed = float(index) * 100.0 * time;
    p.x = snoise(seed);
    p.z = snoise(seed + 13.0);
    p.y = snoise(seed + 127.0);

    // Normalize to get sphere distribution
    p = (0.06 + 0.04 * snoise(seed + 491.0)) * normalize(p);

    // Particle respawns at emitter
    p += emitterPos;

    // New lifetime with slight variation
    float newLifetime = (1.0 + 0.25 * snoise(seed)) * particleLifetime;

    SpawnInfo[index] = vec4(p, newLifetime);
}
