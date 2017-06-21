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

#extension GL_EXT_geometry_shader : require
precision highp float;
precision highp sampler3D;

layout(points) in;
layout(triangle_strip, max_vertices = 12) out;

flat in ivec3 v_texel[];
out vec3 gs_position;
out vec2 gs_quadcoord;
out vec3 gs_normal;
uniform sampler3D inSurface;
uniform sampler3D inCentroid;
uniform mat4 projection;
uniform mat4 view;

void EmitQuad(mat4 pv, vec3 v0, vec3 v1, vec3 v2, vec3 v3)
{
    gl_Position = pv * vec4(v0, 1.0);
    gs_position = v0;
    gs_quadcoord = vec2(0.0, 0.0);
    EmitVertex();
    gs_position = v1;
    gl_Position = pv * vec4(v1, 1.0);
    gs_quadcoord = vec2(0.0, 1.0);
    EmitVertex();
    gs_position = v3;
    gl_Position = pv * vec4(v3, 1.0);
    gs_quadcoord = vec2(1.0, 0.0);
    EmitVertex();
    gs_position = v2;
    gl_Position = pv * vec4(v2, 1.0);
    gs_quadcoord = vec2(1.0, 1.0);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    // We construct faces by considering the three edges of the
    // corner of the cell. When all four cells that share an edge
    // are surface nodes, we make a quad from their centroids.
    //    y
    //    |   z
    //    |  /
    //    | /
    //    |/
    //    +---------x
    // (texel)
    ivec3 texel = v_texel[0];

    // Sample the centroid offsets and the the surface boolean
    // The offset is stored in the .xyz components, the surface
    // boolean in the .w component.
    vec4 c_mid          = texelFetch(inCentroid, texel, 0);
    vec4 c_left         = texelFetch(inCentroid, texel - ivec3(1, 0, 0), 0);
    vec4 c_bottom       = texelFetch(inCentroid, texel - ivec3(0, 1, 0), 0);
    vec4 c_back         = texelFetch(inCentroid, texel - ivec3(0, 0, 1), 0);
    vec4 c_bottom_left  = texelFetch(inCentroid, texel - ivec3(1, 1, 0), 0);
    vec4 c_back_left    = texelFetch(inCentroid, texel - ivec3(1, 0, 1), 0);
    vec4 c_bottom_back  = texelFetch(inCentroid, texel - ivec3(0, 1, 1), 0);

    // Next we compute the resulting vertex positions in world-space
    // The centroid offset is a triple in the range [0, 1]
    // on each axis. When an offset is 0, we push the vertex
    // half a tile's width to the left. When it is 1, we push
    // it the same amount to the right. When it is 0.5, the
    // vertex stays in the cell center.
    float one_over_n = 1.0 / float(textureSize(inCentroid, 0).x);
    vec3 one            = vec3(1.0);
    vec3 v_mid          = -one + 2.0 * one_over_n * (vec3(texel) + c_mid.xyz);
    vec3 v_left         = -one + 2.0 * one_over_n * (vec3(texel - ivec3(1, 0, 0)) + c_left.xyz);
    vec3 v_bottom       = -one + 2.0 * one_over_n * (vec3(texel - ivec3(0, 1, 0)) + c_bottom.xyz);
    vec3 v_back         = -one + 2.0 * one_over_n * (vec3(texel - ivec3(0, 0, 1)) + c_back.xyz);
    vec3 v_bottom_left  = -one + 2.0 * one_over_n * (vec3(texel - ivec3(1, 1, 0)) + c_bottom_left.xyz);
    vec3 v_back_left    = -one + 2.0 * one_over_n * (vec3(texel - ivec3(1, 0, 1)) + c_back_left.xyz);
    vec3 v_bottom_back  = -one + 2.0 * one_over_n * (vec3(texel - ivec3(0, 1, 1)) + c_bottom_back.xyz);

    // Sample the volume texture in the lower octant
    // (We use these samples to compute the normal
    // and determine triangle orientation below).
    float volume000 = texelFetch(inSurface, texel, 0).r;
    float volume100 = texelFetch(inSurface, texel + ivec3(1, 0, 0), 0).r;
    float volume010 = texelFetch(inSurface, texel + ivec3(0, 1, 0), 0).r;
    float volume001 = texelFetch(inSurface, texel + ivec3(0, 0, 1), 0).r;

    // Before constructing the triangles, we need to find the
    // correct orientation. We define CCW to be frontfacing,
    // with the surface normal pointing in the direction of
    // increasing isovalue.

    bool ccw = volume000 < 0.0f;

    // If we know that the corner (texel) was negative,
    // then one of two cases apply:
    //   i) One of its adjacent corners must be positive
    //  ii) All of its adjacent corners are negative,
    //      but the diagonally opposite corner is positive.
    // In both cases, this defines a direction of increasing
    // isovalue, and thus gives us a facet normal.

    // We compute a facet normal by approximating the surface
    // gradient with forward difference.
    gs_normal = normalize(vec3(volume100 - volume000,
                               volume010 - volume000,
                               volume001 - volume000));

    // We generate a facet (two triangles) if all centroids
    // that make it up are on the surface. The bools below
    // are true for nodes that are on the surface.
    bool smid           = c_mid.w         > 0.5;
    bool sleft          = c_left.w        > 0.5;
    bool sbottom        = c_bottom.w      > 0.5;
    bool sback          = c_back.w        > 0.5;
    bool sbottom_left   = c_bottom_left.w > 0.5;
    bool sback_left     = c_back_left.w   > 0.5;
    bool sbottom_back   = c_bottom_back.w > 0.5;
    mat4 pv = projection * view;
    if (sback && sleft && sback_left)
    {
        if (ccw) { EmitQuad(pv, v_mid, v_back, v_back_left, v_left); }
        else     { EmitQuad(pv, v_mid, v_left, v_back_left, v_back); }
    }

    if (sbottom && sback && sbottom_back)
    {
        if (ccw) { EmitQuad(pv, v_mid, v_bottom, v_bottom_back, v_back); }
        else     { EmitQuad(pv, v_mid, v_back, v_bottom_back, v_bottom); }
    }

    if (sleft && sbottom && sbottom_left)
    {
        if (ccw) { EmitQuad(pv, v_mid, v_left, v_bottom_left, v_bottom); }
        else     { EmitQuad(pv, v_mid, v_bottom, v_bottom_left, v_left); }
    }
}
