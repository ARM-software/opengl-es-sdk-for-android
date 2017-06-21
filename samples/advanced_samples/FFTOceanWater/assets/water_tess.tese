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
precision highp int;

layout(cw, quads, fractional_even_spacing) in;

patch in vec2 vOutPatchPosBase;
patch in vec4 vPatchLods;

layout(location = 0) uniform mat4 uMVP;
layout(location = 1) uniform vec4 uScale;
layout(location = 3) uniform mediump vec2 uInvScale;
layout(location = 4) uniform highp vec3 uCamPos;
layout(location = 5) uniform vec2 uPatchSize;
layout(location = 8) uniform mediump vec2 uInvHeightmapSize;

layout(binding = 0) uniform mediump sampler2D uHeightmapDisplacement;

out highp vec3 vWorld;
out highp vec4 vGradNormalTex;

vec2 lerp_vertex(vec2 tess_coord)
{
    return vOutPatchPosBase + tess_coord * uPatchSize;
}

mediump vec2 lod_factor(vec2 tess_coord)
{
    // Bilinear interpolation from patch corners.
    mediump vec2 x = mix(vPatchLods.yx, vPatchLods.zw, tess_coord.x);
    mediump float level = mix(x.x, x.y, tess_coord.y);

    mediump float floor_level = floor(level);
    mediump float fract_level = level - floor_level;
    return vec2(floor_level, fract_level);
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
    vec2 tess_coord = gl_TessCoord.xy;
    vec2 pos = lerp_vertex(tess_coord);
    mediump vec2 lod = lod_factor(tess_coord);

    vec2 tex = pos * uInvHeightmapSize.xy;
    pos *= uScale.xy;

    mediump float delta_mod = exp2(lod.x);
    vec2 off = uInvHeightmapSize.xy * delta_mod;

    vGradNormalTex = vec4(tex + 0.5 * uInvHeightmapSize.xy, tex * uScale.zw);
    vec3 height_displacement = sample_height_displacement(tex, off, lod);

    pos += height_displacement.yz;
    vWorld = vec3(pos.x, height_displacement.x, pos.y);
    gl_Position = uMVP * vec4(vWorld, 1.0);
}

