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
precision highp image3D;
layout (local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
layout (binding = 0, r32f) uniform readonly image3D inSurface;
layout (binding = 1, rgba8) uniform writeonly image3D outCentroid;

layout (binding = 2) uniform atomic_uint outCount;
layout (binding = 3, std430) buffer IndexBuffer {
    uint outIndices[];
};

uniform float voxel_mode;

// return: offset (x, y, z) in the local cell
//    and: if the cell was on the surface (w)
vec4 ComputeCentroid(ivec3 texel)
{
    // Load the isovalue at each corner of the cube
    float val[8] = float[8](
        imageLoad(inSurface, texel + ivec3(0, 0, 0)).r,
        imageLoad(inSurface, texel + ivec3(1, 0, 0)).r,
        imageLoad(inSurface, texel + ivec3(1, 0, 1)).r,
        imageLoad(inSurface, texel + ivec3(0, 0, 1)).r,
        imageLoad(inSurface, texel + ivec3(0, 1, 0)).r,
        imageLoad(inSurface, texel + ivec3(1, 1, 0)).r,
        imageLoad(inSurface, texel + ivec3(1, 1, 1)).r,
        imageLoad(inSurface, texel + ivec3(0, 1, 1)).r
    );

    // Construct a mask where a bit is set if the
    // corner is below the isovalue
    int mask = 0;
    if (val[0] < 0.0) mask |= 1;
    if (val[1] < 0.0) mask |= 2;
    if (val[2] < 0.0) mask |= 4;
    if (val[3] < 0.0) mask |= 8;
    if (val[4] < 0.0) mask |= 16;
    if (val[5] < 0.0) mask |= 32;
    if (val[6] < 0.0) mask |= 64;
    if (val[7] < 0.0) mask |= 128;

    // Early termination if we are either fully inside or
    // fully outside the isosurface.
    if (mask == 0 || mask == 0xff)
        return vec4(0.0);

    vec3 cube_vertices[8] = vec3[8](
        vec3(-1.0, -1.0, -1.0),
        vec3(+1.0, -1.0, -1.0),
        vec3(+1.0, -1.0, +1.0),
        vec3(-1.0, -1.0, +1.0),
        vec3(-1.0, +1.0, -1.0),
        vec3(+1.0, +1.0, -1.0),
        vec3(+1.0, +1.0, +1.0),
        vec3(-1.0, +1.0, +1.0)
    );

    // To make it easier to compute intersection points
    // we store the indices of the edge's vertices. I.e.
    // edge number 0 has endpoint number 0 and 1.
    int cube_edges[24] = int[24](
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        4, 5,
        5, 6,
        6, 7,
        7, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7
    );

    // Like Paul Bourke, we construct an edge table which
    // maps the above mask to which edges that cross the
    // isoline level.
    int edge_table[256] = int[256](
    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0  );

    int edge_mask = edge_table[mask];

    // For each edge in the cube
    int edge_crossings = 0;
    vec3 offset = vec3(0.0);
    for (int i = 0; i < 12; i++)
    {
        // Skip if edge does not cross the zero level
        if ((edge_mask & (1 << i)) == 0)
            continue;

        // Unpack vertices of the edge on the cube
        int ei0 = cube_edges[i * 2];
        int ei1 = cube_edges[i * 2 + 1];
        vec3 e0 = cube_vertices[ei0];
        vec3 e1 = cube_vertices[ei1];

        // Unpack potential function values
        float v0 = val[ei0];
        float v1 = val[ei1];

        // Compute intersection point by linear interpolation
        float t = clamp((0.0 - v0) / (v1 - v0), 0.0, 1.0);
        offset += e0 + t * (e1 - e0); // And accumulate

        edge_crossings++;
    }

    // Compute the average
    offset /= float(edge_crossings);

    // If we don't offset the vertices we get a voxel-type mesh
    offset *= 1.0 - voxel_mode;

    // Finally, since we were on the surface, we write out this
    // cell's index to the index buffer.
    ivec3 size = imageSize(outCentroid);
    uint unique = atomicCounterIncrement(outCount);
    int index = texel.z * size.x * size.y + texel.y * size.x + texel.x;
    outIndices[unique] = uint(index);

    return vec4(offset, 1.0);
}

void main()
{
    ivec3 texel = ivec3(gl_GlobalInvocationID.xyz);
    vec4 v = ComputeCentroid(texel);

    // Remap to fit into 8-bit
    vec3 offset = vec3(0.5) + 0.5 * v.xyz;

    imageStore(outCentroid, texel, vec4(offset, v.w));
}
