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

#extension GL_EXT_tessellation_shader : require

layout(vertices = 1) out;
in vec2 vPatchPosBase[];

layout(location = 1) uniform vec4 uScale;
layout(location = 4) uniform highp vec3 uCamPos;
layout(location = 5) uniform vec2 uPatchSize;
layout(location = 6) uniform vec2 uMaxTessLevel;
layout(location = 7) uniform float uDistanceMod;
layout(location = 9) uniform vec4 uFrustum[6];

patch out vec2 vOutPatchPosBase;
patch out vec4 vPatchLods;

float lod_factor(vec2 pos)
{
    pos *= uScale.xy;
    vec3 dist_to_cam = uCamPos - vec3(pos.x, 0.0, pos.y);
    float level = log2((length(dist_to_cam) + 0.0001) * uDistanceMod);
    return clamp(level, 0.0, uMaxTessLevel.x);
}

float tess_level(float lod)
{
    return uMaxTessLevel.y * exp2(-lod);
}

vec4 tess_level(vec4 lod)
{
    return uMaxTessLevel.y * exp2(-lod);
}

// Guard band for vertex displacement.
#define GUARD_BAND 10.0
bool frustum_cull(vec2 p0)
{
    vec2 min_xz = (p0 - GUARD_BAND) * uScale.xy;
    vec2 max_xz = (p0 + uPatchSize + GUARD_BAND) * uScale.xy;

    vec3 bb_min = vec3(min_xz.x, -GUARD_BAND, min_xz.y);
    vec3 bb_max = vec3(max_xz.x, +GUARD_BAND, max_xz.y);
    vec3 center = 0.5 * (bb_min + bb_max);
    float radius = 0.5 * length(bb_max - bb_min);

    vec3 f0 = vec3(
        dot(uFrustum[0], vec4(center, 1.0)),
        dot(uFrustum[1], vec4(center, 1.0)),
        dot(uFrustum[2], vec4(center, 1.0)));

    vec3 f1 = vec3(
        dot(uFrustum[3], vec4(center, 1.0)),
        dot(uFrustum[4], vec4(center, 1.0)),
        dot(uFrustum[5], vec4(center, 1.0)));

    return !(any(lessThanEqual(f0, vec3(-radius))) || any(lessThanEqual(f1, vec3(-radius))));
}

void compute_tess_levels(vec2 p0)
{
    vOutPatchPosBase = p0;

    float l0 = lod_factor(p0 + vec2(0.0, 1.0) * uPatchSize);
    float l1 = lod_factor(p0 + vec2(0.0, 0.0) * uPatchSize);
    float l2 = lod_factor(p0 + vec2(1.0, 0.0) * uPatchSize);
    float l3 = lod_factor(p0 + vec2(1.0, 1.0) * uPatchSize);

    vec4 lods = vec4(l0, l1, l2, l3);
    vPatchLods = lods;

    vec4 outer_lods = min(lods.xyzw, lods.yzwx);
    vec4 tess_levels = tess_level(outer_lods);
    float inner_level = max(max(tess_levels.x, tess_levels.y), max(tess_levels.z, tess_levels.w));

    gl_TessLevelInner[0] = inner_level;
    gl_TessLevelInner[1] = inner_level;

    gl_TessLevelOuter[0] = tess_levels.x;
    gl_TessLevelOuter[1] = tess_levels.y;
    gl_TessLevelOuter[2] = tess_levels.z;
    gl_TessLevelOuter[3] = tess_levels.w;
}

void main()
{
    vec2 p0 = vPatchPosBase[0];
    if (!frustum_cull(p0))
    {
        gl_TessLevelOuter[0] = -1.0;
        gl_TessLevelOuter[1] = -1.0;
        gl_TessLevelOuter[2] = -1.0;
        gl_TessLevelOuter[3] = -1.0;
        gl_TessLevelInner[0] = -1.0;
        gl_TessLevelInner[1] = -1.0;
    }
    else
    {
        compute_tess_levels(p0);
    }
}

