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

#ifndef GROUND_MESH_H__
#define GROUND_MESH_H__

#include <vector>
#include <GLES3/gl3.h>
#include <stddef.h>
#include "vector_math.h"
#include "Frustum.h"

class GroundMesh
{
public:
    GroundMesh(unsigned int size, unsigned int levels, float clip_scale);
    ~GroundMesh();

    void set_frustum(const Frustum& frustum) { view_proj_frustum = frustum; }
    void update_level_offsets(const vec2& camera_pos);
    const std::vector<vec2>& get_level_offsets() const { return level_offsets; }

    void render();

private:
    GLuint vertex_buffer, index_buffer, vertex_array, uniform_buffer;
    unsigned int size;
    unsigned int level_size;
    unsigned int levels;
    size_t uniform_buffer_size;
    size_t num_indices;

    float clipmap_scale;

    struct Block
    {
        size_t offset;
        size_t count;
        vec2 range;
    };

    struct InstanceData
    {
        vec2 offset; // Offset of the block in XZ plane (world space). This is prescaled.
        vec2 texture_scale; // Scale factor of local offsets (vertex coordinates) translated into texture coordinates.
        vec2 texture_offset; // Offset for texture coordinates, similar to offset. Also prescaled.
        float scale; // Scale factor of local offsets (vertex coordinates).
        float level; // Clipmap LOD level of block.
    };

    Block block;
    Block vertical;
    Block horizontal;
    Block trim_full;
    Block trim_top_right;
    Block trim_bottom_right;
    Block trim_bottom_left;
    Block trim_top_left;
    Block degenerate_left;
    Block degenerate_top;
    Block degenerate_right;
    Block degenerate_bottom;

    void setup_vertex_buffer(unsigned int size);
    void setup_index_buffer(unsigned int size);
    void setup_block_ranges(unsigned int size);
    void setup_uniform_buffer();
    void setup_vertex_array();

    void update_draw_list();
    void render_draw_list();
    struct DrawInfo
    {
        size_t index_buffer_offset;
        size_t uniform_buffer_offset;
        unsigned int indices;
        unsigned int instances;
    };
    std::vector<DrawInfo> draw_list;
    GLint uniform_buffer_align;

    std::vector<vec2> level_offsets;

    typedef bool (*TrimConditional)(const vec2& offset);

    vec2 get_offset_level(const vec2& camera_pos, unsigned int level);
    void update_draw_list(DrawInfo& info, size_t& ubo_offset);
    DrawInfo get_draw_info_blocks(InstanceData *instance_data);
    DrawInfo get_draw_info_vert_fixup(InstanceData *instance_data);
    DrawInfo get_draw_info_horiz_fixup(InstanceData *instance_data);
    DrawInfo get_draw_info_degenerate(InstanceData *instance_data, const Block& block, const vec2& offset, const vec2& ring_offset);
    DrawInfo get_draw_info_degenerate_left(InstanceData *instance_data);
    DrawInfo get_draw_info_degenerate_right(InstanceData *instance_data);
    DrawInfo get_draw_info_degenerate_top(InstanceData *instance_data);
    DrawInfo get_draw_info_degenerate_bottom(InstanceData *instance_data);
    DrawInfo get_draw_info_trim_full(InstanceData *instance_data);
    DrawInfo get_draw_info_trim(InstanceData *instance_data, const Block& block, TrimConditional cond);
    DrawInfo get_draw_info_trim_top_right(InstanceData *instance_data);
    DrawInfo get_draw_info_trim_top_left(InstanceData *instance_data);
    DrawInfo get_draw_info_trim_bottom_right(InstanceData *instance_data);
    DrawInfo get_draw_info_trim_bottom_left(InstanceData *instance_data);

    bool intersects_frustum(const vec2& offset, const vec2& range, unsigned int level);

    Frustum view_proj_frustum;
};

#endif
