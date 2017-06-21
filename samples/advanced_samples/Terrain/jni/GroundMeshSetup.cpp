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
#include "Platform.h"
#include <assert.h>

using namespace MaliSDK;

void GroundMesh::setup_vertex_buffer(unsigned int size)
{
    // Assume that size <= 64. Saves some vertex space since we can use 8-bit vertex coordinates.
    assert(size <= 64);

    // The ground consists of many smaller tesselated quads.
    // These smaller quads can be instanced to stamp out a big area (clipmap) where quads further away from camera
    // can be larger, and hence, less detail.
    // The grid is completely flat (XZ-plane), but they are offset in Y direction with a heightmap in vertex shader.
    // We also need padding/fixup regions to fill the missing space which
    // shows up when the clipmap is put together.

    // See Doxygen for an illustration on how these blocks are laid out to form the terrain.

    unsigned int num_vertices = size * size; // Regular block

    // Ring fixups (vertical and horiz). The regions are 3-by-N vertices.
    unsigned int ring_vertices = size * 3;
    num_vertices += 2 * ring_vertices;

    // Trim regions are thin stripes which surround blocks from the lower LOD level.
    // Need (2 * size + 1)-by-2 vertices. One stripe for each four sides are needed.
    unsigned int trim_vertices = (2 * size + 1) * 2;
    num_vertices += trim_vertices * 4;

    // Degenerate triangles. These are run on the edge between clipmap levels.
    // These are needed to avoid occasional "missing pixels" between clipmap levels as
    // imperfections in precision can cause the terrain to not perfectly overlap at the clipmap level boundary.
    //
    // 5 vertices are used per vertex to create a suitable triangle strip.
    // (This is somewhat redundant, but it simplifies the implementation).
    // Two different strips are needed for left/right and top/bottom.
    unsigned int degenerate_vertices = 2 * (size - 1) * 5;
    num_vertices += degenerate_vertices * 2;

    GLubyte *vertices = new GLubyte[2 * num_vertices];
    //! [Generating vertex buffer]
    GLubyte *pv = vertices;

    // Block
    for (unsigned int z = 0; z < size; z++)
    {
        for (unsigned int x = 0; x < size; x++)
        {
            pv[0] = x;
            pv[1] = z;
            pv += 2;
        }
    }
    //! [Generating vertex buffer]

    // Vertical ring fixup
    for (unsigned int z = 0; z < size; z++)
    {
        for (unsigned int x = 0; x < 3; x++)
        {
            pv[0] = x;
            pv[1] = z;
            pv += 2;
        }
    }

    // Horizontal ring fixup
    for (unsigned int z = 0; z < 3; z++)
    {
        for (unsigned int x = 0; x < size; x++)
        {
            pv[0] = x;
            pv[1] = z;
            pv += 2;
        }
    }

    // Full interior trim
    // Top
    for (unsigned int z = 0; z < 2; z++)
    {
        for (unsigned int x = 0; x < 2 * size + 1; x++)
        {
            pv[0] = x;
            pv[1] = z;
            pv += 2;
        }
    }

    // Right
    for (int x = 1; x >= 0; x--)
    {
        for (unsigned int z = 0; z < 2 * size + 1; z++)  
        {
            pv[0] = x + 2 * size - 1;
            pv[1] = z;
            pv += 2;
        }
    }

    // Bottom
    for (int z = 1; z >= 0; z--)
    {
        for (unsigned int x = 0; x < 2 * size + 1; x++)
        {
            pv[0] = 2 * size - x;
            pv[1] = z + 2 * size - 1;
            pv += 2;
        }
    }

    // Left
    for (unsigned int x = 0; x < 2; x++)
    {
        for (unsigned int z = 0; z < 2 * size + 1; z++)  
        {
            pv[0] = x;
            pv[1] = 2 * size - z;
            pv += 2;
        }
    }

    // Degenerate triangles.
    // Left, right
    for (unsigned int y = 0; y < (size - 1) * 2; y++)
    {
        pv[0] = 0;
        pv[1] = y * 2;
        pv[2] = 0;
        pv[3] = y * 2;
        pv[4] = 0;
        pv[5] = y * 2 + 1;
        pv[6] = 0;
        pv[7] = y * 2 + 2;
        pv[8] = 0;
        pv[9] = y * 2 + 2;
        pv += 10;
    }

    // Top, bottom
    for (unsigned int x = 0; x < (size - 1) * 2; x++)
    {
        pv[0] = x * 2;
        pv[1] = 0;
        pv[2] = x * 2;
        pv[3] = 0;
        pv[4] = x * 2 + 1;
        pv[5] = 0;
        pv[6] = x * 2 + 2;
        pv[7] = 0;
        pv[8] = x * 2 + 2;
        pv[9] = 0;
        pv += 10;
    }

    // Right and bottom share vertices with left and top respectively.

    GL_CHECK(glGenBuffers(1, &vertex_buffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, 2 * num_vertices * sizeof(GLubyte), vertices, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

    delete[] vertices;
}

// Returns number of indices needed to create a triangle stripped mesh using generate_block_indices() below.
static unsigned int block_index_count(unsigned int width, unsigned int height)
{
    unsigned int strips = height - 1;
    return strips * (2 * width - 1) + 1;
}

//! [Generating index buffer]
static GLushort *generate_block_indices(GLushort *pi, unsigned int vertex_buffer_offset,
                                               unsigned int width, unsigned int height)
{
    // Stamp out triangle strips back and forth.
    int pos = vertex_buffer_offset;
    unsigned int strips = height - 1;

    // After even indices in a strip, always step to next strip.
    // After odd indices in a strip, step back again and one to the right or left.
    // Which direction we take depends on which strip we're generating.
    // This creates a zig-zag pattern.
    for (unsigned int z = 0; z < strips; z++)
    {
        int step_even = width;
        int step_odd = ((z & 1) ? -1 : 1) - step_even;

        // We don't need the last odd index.
        // The first index of the next strip will complete this strip.
        for (unsigned int x = 0; x < 2 * width - 1; x++)
        {
            *pi++ = pos;
            pos += (x & 1) ? step_odd : step_even;
        }
    }
    // There is no new strip, so complete the block here.
    *pi++ = pos;

    // Return updated index buffer pointer.
    // More explicit than taking reference to pointer.
    return pi;
}
//! [Generating index buffer]

void GroundMesh::setup_block_ranges(unsigned int size)
{
    // Used for frustum culling.
    // The range is the number of vertices covered minus 1.
    block.range = vec2(size - 1);

    vertical.range = vec2(2, size - 1);
    horizontal.range = vec2(size - 1, 2);

    trim_full.range = vec2(2 * size);
    trim_top_left.range = vec2(2 * size);
    trim_bottom_right.range = vec2(2 * size);
    trim_top_right.range = vec2(2 * size);
    trim_bottom_left.range = vec2(2 * size);

    degenerate_left.range = vec2(0, 4 * size - 2);
    degenerate_right.range = vec2(0, 4 * size - 2);
    degenerate_top.range = vec2(4 * size - 2, 0);
    degenerate_bottom.range = vec2(4 * size - 2, 0);
}

void GroundMesh::setup_index_buffer(unsigned int size)
{
    unsigned int vertex_buffer_offset = 0;

    block.count = block_index_count(size, size);

    vertical.count = block_index_count(3, size);
    horizontal.count = block_index_count(size, 3);

    unsigned int trim_region_indices = block_index_count(2 * size + 1, 2);
    trim_full.count = 4 * trim_region_indices;
    trim_top_left.count = 2 * trim_region_indices;
    trim_bottom_right = trim_bottom_left = trim_top_right = trim_top_left;

    // 6 indices are used here per vertex.
    // Need to repeat one vertex to get correct winding when
    // connecting the triangle strips.
    degenerate_left.count = (size - 1) * 2 * 6;
    degenerate_right = degenerate_bottom = degenerate_top = degenerate_left;

    num_indices = block.count + vertical.count + horizontal.count + trim_full.count +
        4 * trim_top_left.count +
        4 * degenerate_left.count;

    GLushort *indices = new GLushort[num_indices];
    GLushort *pi = indices;

    // Main block
    block.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, size, size);
    vertex_buffer_offset += size * size;

    // Vertical fixup
    vertical.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, 3, size);
    vertex_buffer_offset += 3 * size;

    // Horizontal fixup
    horizontal.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, size, 3);
    vertex_buffer_offset += 3 * size;

    // Full interior trim
    // All trims can be run after each other.
    // The vertex buffer is generated such that this creates a "ring".
    // The full trim is only used to connect clipmap level 0 to level 1. See Doxygen for more detail.
    trim_full.offset = pi - indices;
    unsigned int full_trim_offset = vertex_buffer_offset;
    unsigned int trim_vertices = (2 * size + 1) * 2;
    pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Top
    full_trim_offset += trim_vertices;
    pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Right
    full_trim_offset += trim_vertices;
    pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Bottom
    full_trim_offset += trim_vertices;
    pi = generate_block_indices(pi, full_trim_offset, 2 * size + 1, 2); // Left
    full_trim_offset += trim_vertices;

    // Top-right interior trim
    // This is a half ring (L-shaped).
    trim_top_right.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Top
    pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Right
    vertex_buffer_offset += trim_vertices;

    // Right-bottom interior trim
    // This is a half ring (L-shaped).
    trim_bottom_right.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Right
    pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Bottom
    vertex_buffer_offset += trim_vertices;

    // Bottom-left interior trim
    // This is a half ring (L-shaped).
    trim_bottom_left.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Bottom
    pi = generate_block_indices(pi, vertex_buffer_offset + (2 * size + 1) * 2, 2 * size + 1, 2); // Left
    vertex_buffer_offset += trim_vertices;

    // Left-top interior trim
    // This is a half ring (L-shaped).
    trim_top_left.offset = pi - indices;
    pi = generate_block_indices(pi, vertex_buffer_offset, 2 * size + 1, 2); // Left
    pi = generate_block_indices(pi, vertex_buffer_offset - 6 * (2 * size + 1), 2 * size + 1, 2); // Top
    vertex_buffer_offset += trim_vertices;

    // One of the trim regions will be used to connect level N with level N + 1.

    // Degenerates. Left and right share vertices (with different offsets in vertex shader). Top and bottom share.
    // Left
    degenerate_left.offset = pi - indices;
    for (unsigned int z = 0; z < (size - 1) * 2; z++)
    {
        pi[0] = (5 * z) + 0 + vertex_buffer_offset;
        pi[1] = (5 * z) + 1 + vertex_buffer_offset;
        pi[2] = (5 * z) + 2 + vertex_buffer_offset;
        pi[3] = (5 * z) + 3 + vertex_buffer_offset;
        pi[4] = (5 * z) + 4 + vertex_buffer_offset;
        pi[5] = (5 * z) + 4 + vertex_buffer_offset;
        pi += 6;
    }

    // Right
    degenerate_right.offset = pi - indices;
    unsigned int start_z = (size - 1) * 2 - 1;
    for (unsigned int z = 0; z < (size - 1) * 2; z++)
    {
        // Windings are in reverse order on this side.
        pi[0] = (5 * (start_z - z)) + 4 + vertex_buffer_offset;
        pi[1] = (5 * (start_z - z)) + 3 + vertex_buffer_offset;
        pi[2] = (5 * (start_z - z)) + 2 + vertex_buffer_offset;
        pi[3] = (5 * (start_z - z)) + 1 + vertex_buffer_offset;
        pi[4] = (5 * (start_z - z)) + 0 + vertex_buffer_offset;
        pi[5] = (5 * (start_z - z)) + 0 + vertex_buffer_offset;
        pi += 6;
    }

    vertex_buffer_offset += (size - 1) * 2 * 5;

    // Top
    degenerate_top.offset = pi - indices;
    for (unsigned int x = 0; x < (size - 1) * 2; x++)
    {
        pi[0] = (5 * x) + 0 + vertex_buffer_offset;
        pi[1] = (5 * x) + 1 + vertex_buffer_offset;
        pi[2] = (5 * x) + 2 + vertex_buffer_offset;
        pi[3] = (5 * x) + 3 + vertex_buffer_offset;
        pi[4] = (5 * x) + 4 + vertex_buffer_offset;
        pi[5] = (5 * x) + 4 + vertex_buffer_offset;
        pi += 6;
    }

    // Bottom
    degenerate_bottom.offset = pi - indices;
    unsigned int start_x = (size - 1) * 2 - 1;
    for (unsigned int x = 0; x < (size - 1) * 2; x++)
    {
        // Windings are in reverse order on this side.
        pi[0] = (5 * (start_x - x)) + 4 + vertex_buffer_offset;
        pi[1] = (5 * (start_x - x)) + 3 + vertex_buffer_offset;
        pi[2] = (5 * (start_x - x)) + 2 + vertex_buffer_offset;
        pi[3] = (5 * (start_x - x)) + 1 + vertex_buffer_offset;
        pi[4] = (5 * (start_x - x)) + 0 + vertex_buffer_offset;
        pi[5] = (5 * (start_x - x)) + 0 + vertex_buffer_offset;
        pi += 6;
    }

    GL_CHECK(glGenBuffers(1, &index_buffer));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof(GLushort), indices, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    delete[] indices;
}

void GroundMesh::setup_uniform_buffer()
{
    GL_CHECK(glGenBuffers(1, &uniform_buffer));
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer));

    // Per level we can draw up to 12 regular blocks, 4 vert/horiz rings, one trim, and four degenerate strips.
    // Double the UBO size just in case we have very high levels for UBO buffer alignment.
    uniform_buffer_size = 2 * (12 + 4 + 1 + 4) * levels * sizeof(InstanceData);
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, uniform_buffer_size, NULL, GL_STREAM_DRAW));

    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

// Already defined in the shader.
#define LOCATION_VERTEX 0

void GroundMesh::setup_vertex_array()
{
    GL_CHECK(glGenVertexArrays(1, &vertex_array));
    GL_CHECK(glBindVertexArray(vertex_array));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));

    GL_CHECK(glVertexAttribPointer(LOCATION_VERTEX, 2, GL_UNSIGNED_BYTE, GL_FALSE, 0, 0));
    GL_CHECK(glEnableVertexAttribArray(LOCATION_VERTEX));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    // Element array buffer state is part of the vertex array object, have to unbind it after the vertex array.
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
