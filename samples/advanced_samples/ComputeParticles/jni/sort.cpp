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

#include "sort.h"
#include "common/matrix.h"
#include "common/glutil.h"
#include "common/shader.h"
#include "common/common.h"
#include <string.h>

#define MAX_SCAN_LEVELS 4

Shader
    shader_scan,
    shader_scan_first,
    shader_resolve,
    shader_reorder;

GLuint
    buf_scan[MAX_SCAN_LEVELS],
    buf_sums[MAX_SCAN_LEVELS],
    buf_flags,
    buf_sorted;

unsigned scan_levels;

bool sort_init()
{
    string res = "/data/data/com.arm.malideveloper.openglessdk.computeparticles/files/";
    if (!shader_scan.load_compute_from_file(res + "scan.cs") ||
            !shader_scan_first.load_compute_from_file(res + "scan_first.cs") ||
            !shader_resolve.load_compute_from_file(res + "scan_resolve.cs") ||
            !shader_reorder.load_compute_from_file(res + "scan_reorder.cs"))
    {
        return false;
    }

    if (!shader_scan.link() ||
            !shader_scan_first.link() ||
            !shader_resolve.link() ||
            !shader_reorder.link())
    {
        return false;
    }

    // We do the scan recursively. We have to do scan until one the entire dispatch can be computed by a single work group.
    unsigned elems = NUM_KEYS;
    scan_levels = 0;
    while (elems > 1)
    {
       scan_levels++;
       elems = (elems + BLOCK_SIZE - 1) / BLOCK_SIZE;
    }

    buf_sorted = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, NUM_KEYS * sizeof(vec4), NULL);
    buf_flags  = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, NUM_KEYS * sizeof(GLuint), NULL);

    // Allocate memory for scan levels. Make sure to properly pad them to a workgroups worth of work.
    elems = NUM_BLOCKS;
    for (unsigned i = 0; i < scan_levels; i++)
    {
        buf_scan[i] = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, elems * BLOCK_SIZE * 4 * sizeof(GLuint), NULL);
        elems = (elems + BLOCK_SIZE - 1) / BLOCK_SIZE;
        buf_sums[i] = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_COPY, elems * BLOCK_SIZE * 4 * sizeof(GLuint), NULL);
    }

    return true;
}

void sort_free()
{
    del_buffer(buf_sorted);
    del_buffer(buf_flags);

    for (unsigned i = 0; i < scan_levels; i++)
    {
        del_buffer(buf_scan[i]);
        del_buffer(buf_sums[i]);
    }

    shader_scan.dispose();
    shader_scan_first.dispose();
    shader_resolve.dispose();
    shader_reorder.dispose();
}

void sort_bits(GLuint buf_input, int bit_offset, vec3 axis, float z_min, float z_max)
{
    // Keep track of which dispatch sizes we used to make the resolve steps simpler.
    unsigned dispatch_sizes[MAX_SCAN_LEVELS] = {0};

    unsigned blocks = NUM_BLOCKS;

    // First pass. Compute 16-bit unsigned depth and apply first pass of scan algorithm.
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_input);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_scan[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_sums[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_flags);
    use_shader(shader_scan_first);
    uniform("bitOffset", bit_offset);
    uniform("axis", axis);
    uniform("zMin", z_min);
    uniform("zMax", z_max);
    dispatch_sizes[0] = blocks;
    glDispatchCompute(blocks, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // If we processed more than one work group of data, we're not done,
    // so scan buf_sums[0] and keep scanning recursively like this until buf_sums[N] becomes
    // a single value.
    use_shader(shader_scan);
    for (unsigned i = 1; i < scan_levels; i++)
    {
        blocks = (blocks + BLOCK_SIZE - 1) / BLOCK_SIZE;
        dispatch_sizes[i] = blocks;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_sums[i - 1]);
        // If we only do one work group we don't need to resolve it later,
        // and we can update the scan buffer inplace.
        if (blocks <= 1)
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_sums[i - 1]);
        }
        else
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_scan[i]);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_sums[i]);

        glDispatchCompute(blocks, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Go backwards, we want to end up with a buf_sums[0] which has been properly scanned.
    // Once we have buf_scan[0] and buf_sums[0], we can do the reordering step.
    use_shader(shader_resolve);
    for (unsigned i = scan_levels - 1; i; i--)
    {
        if (dispatch_sizes[i] <= 1) // No need to resolve, buf_sums[i - 1] is already correct
        {
            continue;
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_scan[i]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_sums[i]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_sums[i - 1]);
        glDispatchCompute(dispatch_sizes[i], 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // We can now reorder our input properly.
    use_shader(shader_reorder);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_input);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buf_scan[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, buf_sums[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, buf_sorted);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, buf_flags);
    glDispatchCompute(NUM_BLOCKS, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Now we're done :)
}

void radix_sort(GLuint buf_input, vec3 axis, float z_min, float z_max)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        sort_bits(buf_input, i * 2, axis, z_min, z_max);

        // Swap for the next digit stage
        // The <buf_input> buffer will in the end hold the latest sorted data
        std::swap(buf_input, buf_sorted);
    }

    // We use the position data to draw the particles afterwards
    // Thus we need to ensure that the data is up to date
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
}
