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

precision highp int;

layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform vec4 uScale;
layout(location = 3) uniform mediump vec2 uInvScale;
layout(location = 5) uniform highp vec2 uLodScaleOffset;
layout(location = 6) uniform mediump vec2 uInvHeightmapSize;

layout(binding = 0) uniform mediump sampler2D uHeightmapDisplacement;
layout(binding = 4) uniform mediump sampler2D uLod;

layout(location = 0) in uvec4 aPosition;
layout(location = 1) in vec4 aLODWeights; 

out highp vec3 vWorld;
out highp vec4 vGradNormalTex;

struct PatchData
{
    vec4 Offsets;
    vec4 LODs;
    vec4 InnerLOD;
    vec4 Padding;
};

#define MAX_INSTANCES 256
layout(std140, binding = 0) uniform Offsets
{
    PatchData data[MAX_INSTANCES];
} patches;

mediump vec2 lod_factor(vec2 position)
{
    vec2 uv = (position + patches.data[gl_InstanceID].Offsets.zw) * uLodScaleOffset;
    mediump float level = textureLod(uLod, uv, 0.0).x * (255.0 / 32.0);
    mediump float floor_level = floor(level);
    mediump float fract_level = level - floor_level;
    return vec2(floor_level, fract_level);
}

vec2 warp_position()
{
    // aLODWeights is a vertex attribute that contains either (1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1) or (0, 0, 0, 0).
    // It is all zero when our vertices is inside the patch, and contains a 1 if our vertex lies on an edge.
    // For corners, we don't really care which edge we belong to since the corner vertices
    // will never be snapped anywhere.
    // Using a dot product, this lets us "select" an appropriate LOD factor.
    // For the inner lod, we can conditionally select this efficiently using the boolean mix() operator.

    float vlod = dot(aLODWeights, patches.data[gl_InstanceID].LODs);
    vlod = mix(vlod, patches.data[gl_InstanceID].InnerLOD.x, all(equal(aLODWeights, vec4(0.0))));

    // aPosition.xy holds integer positions locally in the patch with range [0, patch_size].
    // aPosition.zw are either 0 or 1. These select in which direction we will snap our vertices when warping to the lower LOD.
    // It is important that we always round towards the center of the patch, since snapping to one of the edges can lead to popping artifacts.

    float floor_lod = floor(vlod);
    float fract_lod = vlod - floor_lod;
    uint ufloor_lod = uint(floor_lod);

    // Snap to grid corresponding to floor(lod) and ceil(lod).
    uvec2 mask = (uvec2(1u) << uvec2(ufloor_lod, ufloor_lod + 1u)) - 1u;
    uvec4 rounding = aPosition.zwzw * mask.xxyy;
    vec4 lower_upper_snapped = vec4((aPosition.xyxy + rounding) & ~mask.xxyy);

    // Then lerp between them to create a smoothly morphing mesh.
    return mix(lower_upper_snapped.xy, lower_upper_snapped.zw, fract_lod);
}

mediump vec3 sample_height_displacement(vec2 uv, vec2 off, mediump vec2 lod)
{
    return mix(
            textureLod(uHeightmapDisplacement, uv + 0.5 * off, lod.x).xyz,
            textureLod(uHeightmapDisplacement, uv + 1.0 * off, lod.x + 1.0).xyz,
            lod.y);
}

void main()
{
    vec2 pos = warp_position();
    mediump vec2 lod = lod_factor(pos);
    pos += patches.data[gl_InstanceID].Offsets.xy;

    vec2 tex = pos * uInvHeightmapSize;
    pos *= uScale.xy;

    mediump float delta_mod = exp2(lod.x);
    vec2 off = uInvHeightmapSize.xy * delta_mod;

    vGradNormalTex = vec4(tex + 0.5 * uInvHeightmapSize, tex * uScale.zw);
    vec3 height_displacement = sample_height_displacement(tex, off, lod);

    pos += height_displacement.yz;
    vWorld = vec3(pos.x, height_displacement.x, pos.y);
    gl_Position = uMVP * vec4(vWorld, 1.0);
}

