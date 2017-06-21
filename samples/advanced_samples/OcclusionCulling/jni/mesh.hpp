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

#ifndef MESH_HPP__
#define MESH_HPP__

#include <vector>
#include <stdint.h>
#include "common.hpp"

struct Vertex
{
    Vertex() {}
    Vertex(vec3 position, vec3 normal, vec2 tex)
        : position(position), normal(normal), tex(tex) {}

    vec3 position;
    vec3 normal;
    vec2 tex;
};

struct AABB
{
    vec4 minpos;
    vec4 maxpos;
};

struct Mesh
{
    std::vector<Vertex> vbo;
    std::vector<uint16_t> ibo;
    AABB aabb;
};

Mesh create_box_mesh(const AABB &aabb);
Mesh create_sphere_mesh(float radius, vec3 center, unsigned vertices_per_circumference);

class GLDrawable
{
    public:
        GLDrawable(const Mesh &mesh);
        GLDrawable();
        ~GLDrawable();

        const AABB& get_aabb() const;

        GLuint get_vertex_array() const;
        unsigned get_num_elements() const;

    private:
        GLuint vertex_array;
        GLuint vertex_buffer;
        GLuint index_buffer;

        unsigned num_elements;
        AABB aabb;
};

#endif
