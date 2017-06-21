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

#ifndef SHADERS_H__
#define SHADERS_H__

// Put shader sources here to avoid having to deal with asset loading.

// A heightmap like this would also have a corresponding normal map.
// For simplicitly, this is ignored here.
// Normals could be computed on-the-fly by sampling neighboring vertices as well in the vertex shader.

static const char vertex_shader_source[] =
    "#version 300 es\n"
    "layout(std140) uniform;\n"

    "uniform mediump sampler2DArray sHeightmap;\n"

    "uniform mat4 uViewProjection;\n"
    "uniform vec3 uCameraPos;\n"
    "uniform float uInvLevelSize[10]; // GL doesn't allow unsized array when accessed from non-constant.\n"

    "struct PerInstanceData\n"
    "{\n"
    "  vec2 offset; // World-space offset in XZ plane.\n"
    "  vec2 texture_scale;\n"
    "  vec2 texture_offset; // Same as for world-space offset/scale, just for texture coordinates\n"
    "  float scale; // Scaling factor for vertex offsets (per-instance)\n"
    "  float level; // LOD-level to use when sampling heightmap\n"
    "};\n"

    "uniform InstanceData\n"
    "{\n"
    "  PerInstanceData instance[256];\n"
    "};\n"

    "#define LOCATION_VERTEX 0\n"
    "#define HEIGHTMAP_MIN -20.0 // Depends on the heightmap.\n"
    "#define HEIGHTMAP_MAX 20.0\n"

    "layout(location = LOCATION_VERTEX) in vec2 aVertex;\n"
    "out float vHeight;\n"
    "out vec2 vLod;\n"
    "out float vFog;\n"

    "void main()\n"
    "{\n"
    "  vec2 local_offset = aVertex * instance[gl_InstanceID].scale;\n"
    "  vec2 pos = instance[gl_InstanceID].offset + local_offset;\n"

    "  float level = instance[gl_InstanceID].level;\n"
    "  vec2 tex_offset = (aVertex + 0.5) * instance[gl_InstanceID].texture_scale; // 0.5 offset to sample mid-texel.\n"
    "  vec2 texcoord = instance[gl_InstanceID].texture_offset + tex_offset;\n"
  
    "  vec2 heights = texture(sHeightmap, vec3(texcoord, level)).rg;\n"

    "  // Find blending factors for heightmap. The detail level must not have any discontinuities or it shows as 'artifacts'.\n"
    "  vec2 dist = abs(pos - uCameraPos.xz) * uInvLevelSize[int(level)];\n"
    "  vec2 a = clamp((dist - 0.325) * 8.0, 0.0, 1.0);\n"
    "  float lod_factor = max(a.x, a.y);\n"
    "  float height = mix(heights.x, heights.y, lod_factor);\n"

    "  height = clamp(height, HEIGHTMAP_MIN, HEIGHTMAP_MAX); // To ensure frustum culling assumptions are met.\n"

    "  vec4 vert = vec4(pos.x, height, pos.y, 1.0);\n"

    "  gl_Position = uViewProjection * vert;\n"
    "  vHeight = height;\n"
    "  vLod = vec2(level, lod_factor);\n"

    "  vec3 dist_camera = uCameraPos - vert.xyz;\n"
    "  vFog = clamp(dot(dist_camera, dist_camera) / 250000.0, 0.0, 1.0); // Simple per-vertex fog.\n"
    "}";

static const char fragment_shader_source[] =
    "#version 300 es\n"
    "layout(std140) uniform;\n"
    "precision highp float;\n"

    "out vec4 FragColor;\n"
    "in float vHeight;\n"
    "in vec2 vLod;\n"
    "in float vFog;\n"

    "// Compress (-inf, +inf) to (0, 1).\n"
    "float map_height(float h)\n"
    "{\n"
    "  return 1.0 / (1.0 + exp(-h / 20.0));\n"
    "}\n"

    "// Make the heightmap look somewhat cloudy and fluffy.\n"

    "void main()\n"
    "{\n"
    "  vec3 color = vec3(1.2, 1.2, 1.0) * vec3(map_height(vHeight) + (vLod.x + vLod.y) * 0.1);\n"
    "  vec3 final_color = mix(color, vec3(0.5), vFog);\n"
    "  FragColor = vec4(final_color, 1.0);\n"
    "}\n";

#endif

