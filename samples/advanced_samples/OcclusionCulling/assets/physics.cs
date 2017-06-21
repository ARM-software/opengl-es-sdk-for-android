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

// Not a very interesting shader.
// It's purpose is to move the spheres around a bit to see how Hi-Z works on more dynamic objects.

precision highp float;
precision highp int;

layout(local_size_x = 128) in;
layout(location = 0) uniform uint uNumBoundingBoxes;
layout(location = 1) uniform float uDeltaTime;

struct SphereInstance
{
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer SphereInstances
{
    SphereInstance instance[];
} spheres;

#define RANGE 20.0
#define RANGE_Y 10.0
// Super basic collision against some arbitrary walls.
void compute_collision(inout vec3 pos, float radius, inout vec3 velocity)
{
    vec3 dist = pos - vec3(0.0, 2.0, 0.0);
    float dist_sqr = dot(dist, dist);
    float minimum_distance = 2.0 + radius;
    if (dist_sqr < minimum_distance * minimum_distance)
    {
        if (dot(dist, velocity) < 0.0) // Sphere is heading towards us, "reflect" it away.
            velocity = reflect(velocity, normalize(dist));
    }
    else
    {
        // If we collide against our invisible walls, reflect the velocity.
        if (pos.x - radius < -RANGE)
            velocity.x = abs(velocity.x);
        else if (pos.x + radius > RANGE)
            velocity.x = -abs(velocity.x);

        if (pos.y - radius < 0.0)
            velocity.y = abs(velocity.y);
        else if (pos.y + radius > RANGE_Y)
            velocity.y = -abs(velocity.y);

        if (pos.z - radius < -RANGE)
            velocity.z = abs(velocity.z);
        else if (pos.z + radius > RANGE)
            velocity.z = -abs(velocity.z);
    }
}

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    if (ident >= uNumBoundingBoxes)
        return;

    // Load instance data.
    // position.w is sphere radius.
    SphereInstance sphere = spheres.instance[ident];

    // Move the sphere.
    sphere.position.xyz += sphere.velocity.xyz * uDeltaTime;

    // Test collision.
    compute_collision(sphere.position.xyz, sphere.position.w, sphere.velocity.xyz);

    // Write back result.
    spheres.instance[ident] = sphere;
}

