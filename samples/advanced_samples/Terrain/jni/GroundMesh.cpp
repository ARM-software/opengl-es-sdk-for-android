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

#include "GroundMesh.h"
#include <cstdio>
#include <cassert>
#include <cstdint>
#include "Platform.h"
#include "AABB.h"

using namespace MaliSDK;
using namespace std;

GroundMesh::GroundMesh(unsigned int size, unsigned int levels, float clip_scale)
    : size(size), level_size(4 * size - 1), levels(levels), clipmap_scale(clip_scale)
{
    setup_vertex_buffer(size);
    setup_index_buffer(size);
    setup_block_ranges(size);
    setup_uniform_buffer();

    setup_vertex_array();

    // UBOs must be bound with aligned length and offset, and it varies per vendor.
    GL_CHECK(glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_align));
}

GroundMesh::~GroundMesh()
{
    GL_CHECK(glDeleteBuffers(1, &vertex_buffer));
    GL_CHECK(glDeleteBuffers(1, &index_buffer));
    GL_CHECK(glDeleteBuffers(1, &uniform_buffer));
    GL_CHECK(glDeleteVertexArrays(1, &vertex_array));
}

//! [Snapping clipmap level to a grid]
// The clipmap levels only move in steps of texture coordinates.
// Computes top-left world position for the levels.
vec2 GroundMesh::get_offset_level(const vec2& camera_pos, unsigned int level)
{
    if (level == 0) // Must follow level 1 as trim region is fixed.
        return get_offset_level(camera_pos, 1) + vec2(size << 1);
    else
    {
        vec2 scaled_pos = camera_pos / vec2(clipmap_scale); // Snap to grid in the appropriate space.

        // Snap to grid of next level. I.e. we move the clipmap level in steps of two.
        vec2 snapped_pos = vec_floor(scaled_pos / vec2(1 << (level + 1))) * vec2(1 << (level + 1));

        // Apply offset so all levels align up neatly.
        // If snapped_pos is equal for all levels,
        // this causes top-left vertex of level N to always align up perfectly with top-left interior corner of level N + 1.
        // This gives us a bottom-right trim region.

        // Due to the flooring, snapped_pos might not always be equal for all levels.
        // The flooring has the property that snapped_pos for level N + 1 is less-or-equal snapped_pos for level N.
        // If less, the final position of level N + 1 will be offset by -2 ^ N, which can be compensated for with changing trim-region to top-left.
        vec2 pos = snapped_pos - vec2((2 * (size - 1)) << level);
        return pos;
    }
}
//! [Snapping clipmap level to a grid]

void GroundMesh::update_level_offsets(const vec2& camera_pos)
{
    level_offsets.resize(levels);
    for (unsigned int i = 0; i < levels; i++)
        level_offsets[i] = get_offset_level(camera_pos, i);
}

// Since we use instanced drawing, all the different instances of various block types
// can be grouped together to form one draw call per block type.
//
// For the get_draw_info* calls, we look through all possible places where blocks can be rendered
// and push this information to a draw list and a uniform buffer.
//
// The draw list struct (DrawInfo) contains information such as the number of instances for a block type,
// and from where in the uniform buffer to get per-instance data. The per-instance data contains information
// of offset and scale values required to render the blocks at correct positions and at correct scale.
//
// The get_draw_info_* calls are sort of repetitive so comments are only introduced when
// something different is done.
//
// It is important to note that instance.offset is a pre-scaled offset which denotes the
// world-space X/Z position of the top-left vertex in the block.
// instance.scale is used to scale vertex data in a block (which are just integers).
//
// World space X/Z coordinates are computed as instance.offset + vertex_coord * instance.scale.

GroundMesh::DrawInfo GroundMesh::get_draw_info_horiz_fixup(InstanceData *instances)
{
    DrawInfo info;
    InstanceData instance;

    // Horizontal
    info.index_buffer_offset = horizontal.offset;
    info.indices = horizontal.count;
    info.instances = 0;

    // We don't have any fixup regions for the lowest clipmap level.
    for (unsigned int i = 1; i < levels; i++)
    {
        // Left side horizontal fixup region.
        // Texel coordinates are derived by just dividing the world space offset with texture size.
        // The 0.5 texel offset required to sample exactly at the texel center is done in vertex shader.
        instance.texture_scale = 1.0f / level_size;
        instance.scale = clipmap_scale * float(1 << i);
        instance.level = i;

        instance.offset = level_offsets[i];
        instance.offset += vec2(0, 2 * (size - 1)) * vec2(1 << i);
        // Avoid texture coordinates which are very large as this can be difficult for the texture sampler
        // to handle (float precision). Since we use GL_REPEAT, fract() does not change the result.
        // Scale the offset down by 2^level first to get the appropriate texel.
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);

        // Only add the instance if it's visible.
        if (intersects_frustum(instance.offset, horizontal.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }

        // Right side horizontal fixup region.
        instance.offset = level_offsets[i];
        instance.offset += vec2(3 * (size - 1) + 2, 2 * (size - 1)) * vec2(1 << i);
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);

        // Only add the instance if it's visible.
        if (intersects_frustum(instance.offset, horizontal.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }
    }

    return info;
}

// Same as horizontal, just different vertex data and offsets.
GroundMesh::DrawInfo GroundMesh::get_draw_info_vert_fixup(InstanceData *instances)
{
    DrawInfo info;
    InstanceData instance;

    // Vertical
    info.index_buffer_offset = vertical.offset;
    info.indices = vertical.count;
    info.instances = 0;

    for (unsigned int i = 1; i < levels; i++)
    {
        // Top region
        instance.texture_scale = 1.0f / level_size;
        instance.scale = clipmap_scale * float(1 << i);
        instance.level = i;

        instance.offset = level_offsets[i];
        instance.offset += vec2(2 * (size - 1), 0) * vec2(1 << i);
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);

        if (intersects_frustum(instance.offset, vertical.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }

        // Bottom region
        instance.offset = level_offsets[i];
        instance.offset += vec2(2 * (size - 1), 3 * (size - 1) + 2) * vec2(1 << i);
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);

        if (intersects_frustum(instance.offset, vertical.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }
    }

    return info;
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_degenerate(InstanceData *instances, const Block& block, const vec2& offset, const vec2& ring_offset)
{
    DrawInfo info;
    info.instances = 0;
    info.index_buffer_offset = block.offset;
    info.indices = block.count;
    InstanceData instance;
    instance.texture_scale = 1.0f / level_size;

    // No need to connect the last clipmap level to next level (there is none).
    for (unsigned int i = 0; i < levels - 1; i++)
    {
        instance.level = i;
        instance.offset = level_offsets[i];
        instance.offset += offset * vec2(1 << i);

        // This is required to differentiate between level 0 and the other levels.
        // In clipmap level 0, we only have tightly packed N-by-N blocks.
        // In other levels however, there are horizontal and vertical fixup regions, therefore a different
        // offset (2 extra texels) is required.
        if (i > 0)
            instance.offset += ring_offset * vec2(1 << i);
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);
        instance.scale = clipmap_scale * float(1 << i);

        if (intersects_frustum(instance.offset, block.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }
    }

    return info;
}

// Use the generalized get_draw_info_degenerate().
GroundMesh::DrawInfo GroundMesh::get_draw_info_degenerate_left(InstanceData *instances)
{
    return get_draw_info_degenerate(instances, degenerate_left, vec2(0.0f), vec2(0.0f));
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_degenerate_right(InstanceData *instances)
{
    return get_draw_info_degenerate(instances, degenerate_right, vec2(4 * (size - 1), 0.0f), vec2(2.0f, 0.0f));
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_degenerate_top(InstanceData *instances)
{
    return get_draw_info_degenerate(instances, degenerate_top, vec2(0.0f), vec2(0.0f));
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_degenerate_bottom(InstanceData *instances)
{
    return get_draw_info_degenerate(instances, degenerate_bottom, vec2(0.0f, 4 * (size - 1) + 2), vec2(0.0f, 2.0f));
}

// Only used for cliplevel 1 to encapsulate cliplevel 0.
GroundMesh::DrawInfo GroundMesh::get_draw_info_trim_full(InstanceData *instances)
{
    DrawInfo info;
    info.index_buffer_offset = trim_full.offset;
    info.indices = trim_full.count;
    info.instances = 0;

    InstanceData instance;
    instance.texture_scale = 1.0f / level_size;
    instance.level = 1;
    instance.offset = level_offsets[1];
    instance.offset += vec2((size - 1) << 1);
    instance.texture_offset = vec_fract((instance.offset / vec2(1 << 1)) * instance.texture_scale);
    instance.offset *= vec2(clipmap_scale);
    instance.scale = clipmap_scale * float(1 << 1);

    if (intersects_frustum(instance.offset, trim_full.range, 1))
    {
        *instances = instance;
        info.instances++;
    }

    return info;
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_trim(InstanceData *instances, const Block& block, TrimConditional cond)
{
    DrawInfo info;
    info.index_buffer_offset = block.offset;
    info.indices = block.count;
    info.instances = 0;

    // Level 1 always fills in the gap to level 0 using get_draw_info_trim_full().
    // From level 2 and out, we only need a single L-shaped trim region as levels 1 and up
    // use horizontal/vertical trim regions as well, which increases the size slightly (get_draw_info_blocks()).
    for (unsigned int i = 2; i < levels; i++)
    {
        vec2 offset_prev_level = level_offsets[i - 1];
        vec2 offset_current_level = level_offsets[i] + vec2((size - 1) << i);

        // There are four different ways (top-right, bottom-right, top-left, bottom-left)
        // to apply a trim region depending on how camera snapping is done in get_offset_level().
        // A function pointer is introduced here so we can check if a particular trim type
        // should be used for this level. Only one conditional will return true for a given level.
        if (!cond(offset_prev_level - offset_current_level))
            continue;

        InstanceData instance;
        instance.texture_scale = 1.0f / level_size;
        instance.level = i;
        instance.offset = level_offsets[i];
        instance.offset += vec2((size - 1) << i);
        instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
        instance.offset *= vec2(clipmap_scale);
        instance.scale = clipmap_scale * float(1 << i);

        if (intersects_frustum(instance.offset, block.range, i))
        {
            *instances++ = instance;
            info.instances++;
        }
    }

    return info;
}

// offset.x and offset.y are either 0 or at least 1.
// Using 0.5f as threshold is a safe way to check for this difference.
static inline bool trim_top_right_cond(const vec2& offset)
{
    return offset.c.x < 0.5f && offset.c.y > 0.5f;
}

static inline bool trim_top_left_cond(const vec2& offset)
{
    return offset.c.x > 0.5f && offset.c.y > 0.5f;
}

static inline bool trim_bottom_right_cond(const vec2& offset)
{
    return offset.c.x < 0.5f && offset.c.y < 0.5f;
}

static inline bool trim_bottom_left_cond(const vec2& offset)
{
    return offset.c.x > 0.5f && offset.c.y < 0.5f;
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_trim_top_right(InstanceData *instances)
{
    return get_draw_info_trim(instances, trim_top_right, trim_top_right_cond);
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_trim_top_left(InstanceData *instances)
{
    return get_draw_info_trim(instances, trim_top_left, trim_top_left_cond);
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_trim_bottom_right(InstanceData *instances)
{
    return get_draw_info_trim(instances, trim_bottom_right, trim_bottom_right_cond);
}

GroundMesh::DrawInfo GroundMesh::get_draw_info_trim_bottom_left(InstanceData *instances)
{
    return get_draw_info_trim(instances, trim_bottom_left, trim_bottom_left_cond);
}

// These are the basic N-by-N tesselated quads.
GroundMesh::DrawInfo GroundMesh::get_draw_info_blocks(InstanceData *instances)
{
    // Special case for level 0, here we draw the base quad in a tight 4x4 grid. This needs to be padded with a full trim (get_draw_info_trim_full()).
    DrawInfo info;
    info.instances = 0;
    info.index_buffer_offset = block.offset;
    info.indices = block.count;

    InstanceData instance;
    instance.scale = clipmap_scale;
    instance.texture_scale = 1.0f / level_size;

    for (unsigned int z = 0; z < 4; z++)
    {
        for (unsigned int x = 0; x < 4; x++)
        {
            instance.level = 0;
            instance.offset = level_offsets[0];
            instance.offset += vec2(x, z) * vec2(size - 1);
            instance.texture_offset = vec_fract(instance.offset * instance.texture_scale);
            instance.offset *= vec2(clipmap_scale);

            if (intersects_frustum(instance.offset, block.range, 0))
            {
                *instances++ = instance;
                info.instances++;
            }
        }
    }

    // From level 1 and out, the four center blocks are already filled with the lower clipmap level, so
    // skip these.
    for (unsigned int i = 1; i < levels; i++)
    {
        instance.texture_scale = 1.0f / level_size;

        for (unsigned int z = 0; z < 4; z++)
        {
            for (unsigned int x = 0; x < 4; x++)
            {
                if (z != 0 && z != 3 && x != 0 && x != 3)
                {
                    // Already occupied, skip.
                    continue;
                }

                instance.scale = clipmap_scale * float(1 << i);
                instance.level = i;
                instance.offset = level_offsets[i];
                instance.offset += vec2(x, z) * vec2((size - 1) << i);

                // Skip 2 texels horizontally and vertically at the middle to get a symmetric structure.
                // These regions are filled with horizontal and vertical fixup regions.
                if (x >= 2)
                    instance.offset.c.x += 2 << i;
                if (z >= 2)
                    instance.offset.c.y += 2 << i;

                instance.texture_offset = vec_fract((instance.offset / vec2(1 << i)) * instance.texture_scale);
                instance.offset *= vec2(clipmap_scale);

                if (intersects_frustum(instance.offset, block.range, i))
                {
                    *instances++ = instance;
                    info.instances++;
                }
            }
        }
    }

    return info;
}

bool GroundMesh::intersects_frustum(const vec2& offset, const vec2& range, unsigned int level)
{
    // These depend on the heightmap itself. These should be as small as possible to be able to cull more blocks.
    // We know the range of the block in the XZ-plane, but not in Y as it depends on the heightmap texture.
    // In the vertex shader, we enforce a min/max height, so it is safe to assume a range for Y.
    float y_min = -20.0f;
    float y_max =  20.0f;

    // Create an axis-aligned bounding box.
    // Add a twiddle factor to account for potential precision issues.
    AABB aabb(vec3(offset.c.x, y_min, offset.c.y) + vec3(-0.01f),
        vec3(range.c.x, 0.0f, range.c.y) * vec3(float(1 << level)) * vec3(clipmap_scale) + vec3(0, y_max - y_min, 0) + vec3(0.02f));

    return view_proj_frustum.intersects_aabb(aabb);
}

// Helper template to keep the ugly pointer casting to one place.
template<typename T>
static inline T *buffer_offset(T *buffer, size_t offset)
{
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(buffer) + offset);
}

// Round up to nearest aligned offset.
static inline unsigned int realign_offset(size_t offset, size_t align)
{
    return (offset + align - 1) & ~(align - 1);
}

void GroundMesh::update_draw_list(DrawInfo& info, size_t& uniform_buffer_offset)
{
    info.uniform_buffer_offset = uniform_buffer_offset;
    draw_list.push_back(info);

    // Have to ensure that the uniform buffer is always bound at aligned offsets.
    uniform_buffer_offset = realign_offset(uniform_buffer_offset + info.instances * sizeof(InstanceData), uniform_buffer_align);
}

void GroundMesh::update_draw_list()
{
    draw_list.clear();

    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer));

    // Map the uniform buffer.
    GL_CHECK(InstanceData *data = static_cast<InstanceData*>(glMapBufferRange(GL_UNIFORM_BUFFER,
        0, uniform_buffer_size, GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT)));

    if (!data)
    {
        LOGE("Failed to map uniform buffer.\n");
        return;
    }

    DrawInfo info;
    size_t uniform_buffer_offset = 0;

    // Create a draw list. The number of draw calls is equal to the different types
    // of blocks. The blocks are instanced as necessary in the get_draw_info* calls.

    // Main blocks
    info = get_draw_info_blocks(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Vertical ring fixups
    info = get_draw_info_vert_fixup(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Horizontal ring fixups
    info = get_draw_info_horiz_fixup(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Left-side degenerates
    info = get_draw_info_degenerate_left(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Right-side degenerates
    info = get_draw_info_degenerate_right(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Top-side degenerates
    info = get_draw_info_degenerate_top(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Bottom-side degenerates
    info = get_draw_info_degenerate_bottom(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Full trim
    info = get_draw_info_trim_full(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Top-right trim
    info = get_draw_info_trim_top_right(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Top-left trim
    info = get_draw_info_trim_top_left(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Bottom-right trim
    info = get_draw_info_trim_bottom_right(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    // Bottom-left trim
    info = get_draw_info_trim_bottom_left(buffer_offset(data, uniform_buffer_offset));
    update_draw_list(info, uniform_buffer_offset);

    GL_CHECK(glUnmapBuffer(GL_UNIFORM_BUFFER));
}

//! [Rendering the entire terrain]
void GroundMesh::render_draw_list()
{
    for (std::vector<DrawInfo>::const_iterator itr = draw_list.begin(); itr != draw_list.end(); ++itr)
    {
        if (!itr->instances)
            continue;

        // Bind uniform buffer at correct offset.
        GL_CHECK(glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniform_buffer,
                    itr->uniform_buffer_offset, realign_offset(itr->instances * sizeof(InstanceData), uniform_buffer_align)));

        // Draw all instances.
        GL_CHECK(glDrawElementsInstanced(GL_TRIANGLE_STRIP, itr->indices, GL_UNSIGNED_SHORT,
            reinterpret_cast<const GLvoid*>(itr->index_buffer_offset * sizeof(GLushort)), itr->instances));
    }
}
//! [Rendering the entire terrain]

//! [Render scene]
void GroundMesh::render()
{
    // Create a draw-list.
    update_draw_list();

    // Explicitly bind and unbind GL state to ensure clarity.
    GL_CHECK(glBindVertexArray(vertex_array));
    render_draw_list();
    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0));
}
//! [Render scene]
