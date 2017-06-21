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

#include "Heightmap.h"
#include "Platform.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

using namespace MaliSDK;
using namespace std;

Heightmap::Heightmap(unsigned int size, unsigned int levels)
    : size(size), levels(levels)
{
    //! [Initializing texture array]
    GL_CHECK(glGenTextures(1, &texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, texture));

    // Use half-float as we don't need full float precision.
    // GL_RG16UI would work as well as we don't need texture filtering.
    // 8-bit does not give sufficient precision except for low-detail heightmaps.
    // Use two components to allow storing current level's height as well as the height of the next level.
    GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG16F, size, size, levels));

    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

    // The repeat is crucial here. This allows us to update small sections of the texture when moving the camera.
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));

    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
    //! [Initializing texture array]

    // Upload to alternating PBOs for better pipelining.
    GL_CHECK(glGenBuffers(2, pixel_buffer));
    pixel_buffer_index = 0;
    for (unsigned int i = 0; i < 2; i++)
    {
        GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixel_buffer[i]));
        pixel_buffer_size = levels * size * size * sizeof(vec2);
        pixel_buffer_size *= 2; // Double because in worst case we update same region twice.
        GL_CHECK(glBufferData(GL_PIXEL_UNPACK_BUFFER, pixel_buffer_size, NULL, GL_STREAM_DRAW));
    }
    GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

    reset();
}

void Heightmap::reset()
{
    init_heightmap();
    level_info.resize(levels);
    for (unsigned int i = 0; i < levels; i++)
        level_info[i].cleared = true;
}

Heightmap::~Heightmap()
{
    GL_CHECK(glDeleteTextures(1, &texture));
    GL_CHECK(glDeleteBuffers(2, pixel_buffer));
}

// Divides, but always rounds down.
// Most compilers round towards 0 for negative numbers (implementation specific).
static inline int idiv(int x, int m)
{
    if (x >= 0)
        return x / m;
    else
        return -((-x + m - 1) / m);
}

// Modulo with wrapping behavior equal to that of GL_REPEAT.
static inline int imod(int x, int m)
{
    if (x >= 0)
        return x % m;
    else
    {
        int ret = m - (-x % m);
        return ret == m ? 0 : ret;
    }
}

static inline double sinc(double v)
{
    if (fabs(v) < 0.0001)
        return 1.0;
    else
        return sin(PI * v) / (PI * v);
}

// Can really do anything we want, but keep it simple here,
// so just generate a bandpass-filtered 2D grid and repeat it infinitely.
// This causes a second or two of startup time depending on optimization level and platform.
void Heightmap::init_heightmap()
{
    heightmap_size = 1024;
    heightmap.resize(heightmap_size * heightmap_size);

    vector<float> orig(heightmap_size * heightmap_size);
    vector<float> horiz(heightmap_size * heightmap_size);

    // Create some simple bandpass filters. Modulate up lanczos-windowed sinc low-pass filters.
#define FILTER_LEN 65
#define FILTER_CENTER ((FILTER_LEN - 1) / 2)
    float filter[FILTER_LEN] = {0.0f};

    struct Filter
    {
        double amp;
        double bw;
        double center;
    };

    static const Filter bands[] = {
        { 8.0, 0.0075, 0.0 },
        { 0.01, 0.1, 0.1 },
        { 0.005, 0.2, 0.2 },
        { 0.0025, 0.4, 0.4 },
    };

    for (unsigned int f = 0; f < sizeof(bands) / sizeof(bands[0]); f++)
        for (int x = 0; x < FILTER_LEN; x++)
            filter[x] += bands[f].amp * bands[f].bw * sinc(bands[f].bw * (x - FILTER_CENTER)) * sinc((x - FILTER_CENTER) / FILTER_CENTER) * cos(PI * x * bands[f].center);

    // White noise
    srand(0);
    for (unsigned int y = 0; y < heightmap_size; y++)
        for (unsigned int x = 0; x < heightmap_size; x++)
            orig[y * heightmap_size + x] = 50.0f * (float(rand()) / RAND_MAX - 0.5f);

    // Bandpass horizontally
    for (unsigned int y = 0; y < heightmap_size; y++)
    {
        const float *src = &orig[y * heightmap_size];
        float *dst = &horiz[y * heightmap_size];
        for (unsigned int x = 0; x < heightmap_size; x++)
        {
            float sum = 0.0f;
            for (unsigned int i = 0; i < FILTER_LEN; i++)
                sum += src[(x - i) & (heightmap_size - 1)] * filter[i];
            dst[x] = sum;
        }
    }

    // Bandpass vertically
    for (unsigned int x = 0; x < heightmap_size; x++)
    {
        const float *src = &horiz[x];
        float *dst = &heightmap[x];
        for (unsigned int y = 0; y < heightmap_size; y++)
        {
            float sum = 0.0f;
            for (unsigned int i = 0; i < FILTER_LEN; i++)
                sum += src[((y - i) & (heightmap_size - 1)) * heightmap_size] * filter[i];
            dst[y * heightmap_size] = sum;
        }
    }
}

// LUT-based approach. In a real application this would likely be way more complicated.
// Two common applications are pre-computed terrains and procedural generation.
// Sampling like this without appropriate low-pass filtering adds aliasing
// which can cause the heightmap to "pop" in as LOD levels decrease.
float Heightmap::sample_heightmap(int x, int y)
{
    x &= heightmap_size - 1;
    y &= heightmap_size - 1;
    return heightmap[y * heightmap_size + x];
}

//! [Compute heightmap]
// Compute the height at texel (x, y) for cliplevel.
// Also compute the sample for the lower resolution (with simple bilinear).
// This avoids an extra texture lookup in vertex shader, avoids complex offsetting and having to use GL_LINEAR.
vec2 Heightmap::compute_heightmap(int x, int y, int level)
{
    float height = sample_heightmap(x << level, y << level);
    float heights[2][2];
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++)
            heights[j][i] = sample_heightmap(((x + i) & ~1) << level, ((y + j) & ~1) << level);

    return vec2(
        height,
        (heights[0][0] + heights[0][1] + heights[1][0] + heights[1][1]) * 0.25f);
}
//! [Compute heightmap]

//! [Update region]
void Heightmap::update_region(vec2 *buffer, unsigned int& pixel_offset, int tex_x, int tex_y,
                              int width, int height,
                              int start_x, int start_y,
                              int level)
{
    if (width == 0 || height == 0)
        return;

    // Here we could either stream a "real" heightmap, or generate it procedurally on the GPU by rendering to these regions.

    buffer += pixel_offset;
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            buffer[y * width + x] = compute_heightmap(start_x + x, start_y + y, level);

    UploadInfo info;
    info.x = tex_x;
    info.y = tex_y;
    info.width = width;
    info.height = height;
    info.level = level;
    info.offset = pixel_offset * sizeof(vec2);
    upload_info.push_back(info);

    pixel_offset += width * height;
}
//! [Update region]

void Heightmap::update_level(vec2 *buffer, unsigned int& pixel_offset, const vec2& offset, unsigned int level)
{
    LevelInfo& info = level_info[level];
    int start_x = int(offset.c.x) >> level;
    int start_y = int(offset.c.y) >> level;

    // Nothing to do for this level.
    if (start_x == info.x && start_y == info.y && !info.cleared)
        return;

    int delta_x = start_x - info.x;
    int delta_y = start_y - info.y;

    int old_base_x = idiv(info.x, size) * size;
    int old_base_y = idiv(info.y, size) * size;
    int base_x = idiv(start_x, size) * size;
    int base_y = idiv(start_y, size) * size;

    // We have suddenly moved to a completely different place in the heightmap, or we need to recompute everything.
    if (abs(delta_x) >= int(size) || abs(delta_y) >= int(size) || info.cleared)
    {
        int wrapped_x = start_x - base_x;
        int wrapped_y = start_y - base_y;

        update_region(buffer, pixel_offset,
            0, 0, wrapped_x, wrapped_y,
            base_x + size, base_y + size, level);

        update_region(buffer, pixel_offset,
            wrapped_x, 0, size - wrapped_x, wrapped_y,
            start_x, base_y + size, level);

        update_region(buffer, pixel_offset,
            0, wrapped_y, wrapped_x, size - wrapped_y,
            base_x + size, start_y, level);

        update_region(buffer, pixel_offset,
            wrapped_x, wrapped_y, size - wrapped_x, size - wrapped_y,
            start_x, start_y, level);

        info.cleared = false;
    }
    else // Incremental update. Upload what we need.
    {
        int old_wrapped_x = info.x - old_base_x;
        int old_wrapped_y = info.y - old_base_y;
        int wrapped_x = start_x - base_x;
        int wrapped_y = start_y - base_y;

        int wrap_delta_x = wrapped_x - old_wrapped_x;
        int wrap_delta_y = wrapped_y - old_wrapped_y;

        // There can be significant overlap between X-delta and Y-delta uploads if deltas are large.
        // Avoiding this would add even more complexities and is therefore ignored here.

        // Do this in two steps. First update as we're moving in X, then  move in Y.
        if (wrap_delta_x >= 0 && delta_x >= 0) // One update region for X, simple case. Have to update both Y regions however.
        {
            update_region(buffer, pixel_offset,
                old_wrapped_x, 0, wrap_delta_x, old_wrapped_y,
                info.x + size, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                old_wrapped_x, old_wrapped_y, wrap_delta_x, size - old_wrapped_y,
                info.x + size, info.y, level);
        }
        else if (wrap_delta_x < 0 && delta_x < 0) // One update region for X, simple case. Have to update both Y regions however.
        {
            update_region(buffer, pixel_offset,
                wrapped_x, 0, -wrap_delta_x, old_wrapped_y,
                start_x, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                wrapped_x, old_wrapped_y, -wrap_delta_x, size - old_wrapped_y,
                start_x, info.y, level);
        }
        else if (wrap_delta_x < 0 && delta_x >= 0) // Two update regions in X, and also have to update both Y regions.
        {
            update_region(buffer, pixel_offset,
                0, 0, wrapped_x, old_wrapped_y,
                base_x + size, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                old_wrapped_x, 0, size - old_wrapped_x, old_wrapped_y,
                base_x + old_wrapped_x, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                0, old_wrapped_y, wrapped_x, size - old_wrapped_y,
                base_x + size, info.y, level);

            update_region(buffer, pixel_offset,
                old_wrapped_x, old_wrapped_y, size - old_wrapped_x, size - old_wrapped_y,
                base_x + old_wrapped_x, info.y, level);
        }
        else if (wrap_delta_x >= 0 && delta_x < 0) // Two update regions in X, and also have to update both Y regions.
        {
            update_region(buffer, pixel_offset,
                0, 0, old_wrapped_x, old_wrapped_y,
                base_x + size, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                wrapped_x, 0, size - wrapped_x, old_wrapped_y,
                start_x, old_base_y + size, level);

            update_region(buffer, pixel_offset,
                0, old_wrapped_y, old_wrapped_x, size - old_wrapped_y,
                base_x + size, info.y, level);

            update_region(buffer, pixel_offset,
                wrapped_x, old_wrapped_y, size - wrapped_x, size - old_wrapped_y,
                start_x, info.y, level);
        }

        if (wrap_delta_y >= 0 && delta_y >= 0)
        {
            update_region(buffer, pixel_offset,
                0, old_wrapped_y, wrapped_x, wrap_delta_y,
                base_x + size, info.y + size, level);

            update_region(buffer, pixel_offset,
                wrapped_x, old_wrapped_y, size - wrapped_x, wrap_delta_y,
                start_x, info.y + size, level);
        }
        else if (wrap_delta_y < 0 && delta_y < 0)
        {
            update_region(buffer, pixel_offset,
                0, wrapped_y, wrapped_x, -wrap_delta_y,
                base_x + size, start_y, level);

            update_region(buffer, pixel_offset,
                wrapped_x, wrapped_y, size - wrapped_x, -wrap_delta_y,
                start_x, start_y, level);
        }
        else if (wrap_delta_y < 0 && delta_y >= 0)
        {
            update_region(buffer, pixel_offset,
                0, 0, wrapped_x, wrapped_y,
                base_x + size, base_y + size, level);

            update_region(buffer, pixel_offset,
                0, old_wrapped_y, wrapped_x, size - old_wrapped_y,
                base_x + size, base_y + old_wrapped_y, level);

            update_region(buffer, pixel_offset,
                wrapped_x, 0, size - wrapped_x, wrapped_y,
                start_x, base_y + size, level);

            update_region(buffer, pixel_offset,
                wrapped_x, old_wrapped_y, size - wrapped_x, size - old_wrapped_y,
                start_x, base_y + old_wrapped_y, level);
        }
        else if (wrap_delta_y >= 0 && delta_y < 0)
        {
            update_region(buffer, pixel_offset,
                0, 0, wrapped_x, old_wrapped_y,
                base_x + size, base_y + size, level);

            update_region(buffer, pixel_offset,
                0, wrapped_y, wrapped_x, size - wrapped_y,
                base_x + size, start_y, level);

            update_region(buffer, pixel_offset,
                wrapped_x, 0, size - wrapped_x, old_wrapped_y,
                start_x, base_y + size, level);

            update_region(buffer, pixel_offset,
                wrapped_x, wrapped_y, size - wrapped_x, size - wrapped_y,
                start_x, start_y, level);
        }
    }

    info.x = start_x;
    info.y = start_y;
}

void Heightmap::update_heightmap(const vector<vec2>& level_offsets)
{
    upload_info.clear();

    GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixel_buffer[pixel_buffer_index]));
    pixel_buffer_index ^= 1; // Alternate which PBO we upload to.
    GL_CHECK(vec2 *buffer = reinterpret_cast<vec2*>(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0,
                    pixel_buffer_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)));
    if (!buffer)
    {
        LOGE("Failed to map heightmap PBO.\n");
        return;
    }

    unsigned int pixel_offset = 0;
    for (unsigned int i = 0; i < levels; i++)
        update_level(buffer, pixel_offset, level_offsets[i], i);

    GL_CHECK(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));

    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, texture));
    for (vector<UploadInfo>::const_iterator itr = upload_info.begin(); itr != upload_info.end(); ++itr)
    {
        GL_CHECK(glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
            itr->x, itr->y, itr->level,
            itr->width, itr->height, 1,
            GL_RG, GL_FLOAT, reinterpret_cast<const GLvoid*>(itr->offset))); // GLES can convert float to half-float here.
    }
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
    GL_CHECK(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
}
