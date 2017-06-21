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

// Exactly same algorithm as hiz_cull.cs, but with only one LOD.
// See hiz_cull.cs for more information.

precision highp float;
precision highp int;
precision highp sampler2DShadow;

layout(local_size_x = 64) in;

layout(binding = 0, std140) uniform UBO
{
    mat4 uVP;
    mat4 uView;
    vec4 uProj[4];
    vec4 uFrustum[6];
    vec2 zNearFar;
};

layout(location = 0) uniform uint uNumBoundingBoxes;
layout(binding = 0) uniform sampler2DShadow uDepth;

layout(binding = 0, offset = 0) uniform atomic_uint instanceCountLOD0;

struct SphereInstance
{
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 0) buffer PerInstanceInput
{
    readonly SphereInstance data[];
} input_instance;

layout(std430, binding = 1) buffer PerInstanceOutputLOD0
{
    writeonly vec4 data[];
} output_instance_lod0;

layout(std430, binding = 2) buffer PerInstanceOutputLOD1
{
    writeonly vec4 data[];
} output_instance_lod1;

layout(std430, binding = 3) buffer PerInstanceOutputLOD2
{
    writeonly vec4 data[];
} output_instance_lod2;

layout(std430, binding = 4) buffer PerInstanceOutputLOD3
{
    writeonly vec4 data[];
} output_instance_lod3;

void append_instance()
{
    uint count = atomicCounterIncrement(instanceCountLOD0);
    output_instance_lod0.data[count] = input_instance.data[gl_GlobalInvocationID.x].position;
}

bool frustum_test(vec3 center, float radius)
{
    for (int f = 0; f < 6; f++)
    {
        float plane_distance = dot(uFrustum[f], vec4(center, 1.0));
        // Bounding sphere not inside frustum. Can safely cull.
        if (plane_distance < -radius)
            return false;
    }
    return true;
}

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    if (ident >= uNumBoundingBoxes)
        return;

    vec4 instance_data = input_instance.data[ident].position;
    vec3 center = instance_data.xyz;
    float radius = instance_data.w;

    if (!frustum_test(center, radius))
        return;

    vec3 view_center = (uView * vec4(center, 1.0)).xyz;
    float nearest_z = view_center.z + radius;

    // Sphere clips against near plane, just assume visibility.
    if (nearest_z >= -zNearFar.x)
    {
        append_instance();
        return;
    }

    float az_plane_horiz_length = length(view_center.xz);
    float az_plane_vert_length = length(view_center.yz);
    vec2 az_plane_horiz_norm = view_center.xz / az_plane_horiz_length;
    vec2 az_plane_vert_norm = view_center.yz / az_plane_vert_length;

    vec2 t = sqrt(vec2(az_plane_horiz_length, az_plane_vert_length) * vec2(az_plane_horiz_length, az_plane_vert_length) - radius * radius);
    vec4 w = vec4(t, radius, radius) / vec2(az_plane_horiz_length, az_plane_vert_length).xyxy;

    vec4 horiz_cos_sin = az_plane_horiz_norm.xyyx * t.x * vec4(w.xx, -w.z, w.z);
    vec4 vert_cos_sin  = az_plane_vert_norm.xyyx * t.y * vec4(w.yy, -w.w, w.w);

    vec2 horiz0 = horiz_cos_sin.xy + horiz_cos_sin.zw;
    vec2 horiz1 = horiz_cos_sin.xy - horiz_cos_sin.zw;
    vec2 vert0  = vert_cos_sin.xy + vert_cos_sin.zw;
    vec2 vert1  = vert_cos_sin.xy - vert_cos_sin.zw;

    vec4 projected = -0.5 * vec4(uProj[0][0], uProj[0][0], uProj[1][1], uProj[1][1]) *
        vec4(horiz0.x, horiz1.x, vert0.x, vert1.x) /
        vec4(horiz0.y, horiz1.y, vert0.y, vert1.y) + 0.5;

    vec2 min_xy = projected.yw;
    vec2 max_xy = projected.xz;

    vec2 zw = mat2(uProj[2].zw, uProj[3].zw) * vec2(nearest_z, 1.0);
    nearest_z = 0.5 * zw.x / zw.y + 0.5;

    vec2 diff_pix = (max_xy - min_xy) * vec2(textureSize(uDepth, 0));
    float max_diff = max(max(diff_pix.x, diff_pix.y), 1.0);
    float lod = ceil(log2(max_diff));
    vec2 mid_pix = 0.5 * (max_xy + min_xy);

    if (textureLod(uDepth, vec3(mid_pix, nearest_z), lod) > 0.0)
        append_instance();
}

