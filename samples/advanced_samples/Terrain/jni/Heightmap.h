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

#ifndef HEIGHTMAP_H__
#define HEIGHTMAP_H__

#include <GLES3/gl3.h>
#include "vector_math.h"
#include <vector>

class Heightmap
{
public:
    Heightmap(unsigned int size, unsigned int levels);
    ~Heightmap();

    void update_heightmap(const std::vector<vec2>& level_offsets);
    void reset();
    GLuint get_texture() const { return texture; }

private:
    GLuint texture;
    GLuint pixel_buffer[2];
    unsigned int pixel_buffer_index;
    unsigned int pixel_buffer_size;
    unsigned int size;
    unsigned int levels;

    struct LevelInfo
    {
        int x; // top-left coord of texture in texels.
        int y;
        bool cleared;
    };
    std::vector<LevelInfo> level_info;

    struct UploadInfo
    {
        int x;
        int y;
        int width;
        int height;
        int level;
        uintptr_t offset;
    };
    std::vector<UploadInfo> upload_info;

    void update_level(vec2 *buffer, unsigned int& pixel_offset, const vec2& level_offset, unsigned level);
    vec2 compute_heightmap(int x, int y, int level);
    float sample_heightmap(int x, int y);
    void update_region(vec2 *buffer, unsigned int& pixel_offset, int x, int y,
        int width, int height,
        int start_x, int start_y,
        int level);

    std::vector<float> heightmap;
    unsigned int heightmap_size;
    void init_heightmap();
};

#endif
