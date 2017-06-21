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

#include "mesh.hpp"
#include <utility>
using namespace std;

GLDrawable::GLDrawable()
{
    aabb.minpos = vec4(0.0f);
    aabb.maxpos = vec4(0.0f);

    GL_CHECK(glGenVertexArrays(1, &vertex_array));
    GL_CHECK(glGenBuffers(1, &vertex_buffer));
    GL_CHECK(glGenBuffers(1, &index_buffer));

    GL_CHECK(glBindVertexArray(vertex_array));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));

    static const float vertices[] = {
        -1, -1,
         1, -1,
        -1,  1,
         1,  1,
    };
    static const uint16_t indices[] = { 0, 1, 2, 3, 2, 1, };

    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    num_elements = 6;
}

GLDrawable::GLDrawable(const Mesh &mesh)
{
    aabb = mesh.aabb;
    num_elements = mesh.ibo.size();

    GL_CHECK(glGenVertexArrays(1, &vertex_array));
    GL_CHECK(glGenBuffers(1, &vertex_buffer));
    GL_CHECK(glGenBuffers(1, &index_buffer));

    GL_CHECK(glBindVertexArray(vertex_array));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, mesh.vbo.size() * sizeof(Vertex), &mesh.vbo[0],
                GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.ibo.size() * sizeof(uint16_t),
                &mesh.ibo[0], GL_STATIC_DRAW));

    // Vertex position
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                reinterpret_cast<const GLvoid*>(offsetof(Vertex, position))));

    // Normal
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                reinterpret_cast<const GLvoid*>(offsetof(Vertex, normal))));

    // Tex coord
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                reinterpret_cast<const GLvoid*>(offsetof(Vertex, tex))));

    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

GLDrawable::~GLDrawable()
{
    glDeleteVertexArrays(1, &vertex_array);
    glDeleteBuffers(1, &vertex_buffer);
    glDeleteBuffers(1, &index_buffer);
}

const AABB& GLDrawable::get_aabb() const
{
    return aabb;
}

unsigned GLDrawable::get_num_elements() const
{
    return num_elements;
}

GLuint GLDrawable::get_vertex_array() const
{
    return vertex_array;
}

Mesh create_sphere_mesh(float radius, vec3 center, unsigned vertices_per_circumference)
{
    Mesh mesh;

    mesh.vbo.resize(2 + vertices_per_circumference * vertices_per_circumference);

    // No idea if the tangents here are correct.

    // Bottom
    mesh.vbo[0] = Vertex(vec3(0, -1, 0) * vec3(radius) + center, vec3(0, -1, 0), vec2(0.5f, 0.0f));
    // Top
    mesh.vbo[vertices_per_circumference * vertices_per_circumference + 1] = Vertex(vec3(0, +1, 0) * vec3(radius) + center, vec3(0, +1, 0), vec2(0.5f, 1.0f));

    for (unsigned y = 0; y < vertices_per_circumference; y++)
    {
        for (unsigned x = 0; x < vertices_per_circumference; x++)
        {
            float pos_y = sin(PI * (float(y + 1) / (vertices_per_circumference + 1) - 0.5f));
            float xz_mod = sqrt(1.0f - pos_y * pos_y);
            float radians_rot_y = 2.0f * PI * x / vertices_per_circumference;
            float pos_x = xz_mod * cos(radians_rot_y);
            float pos_z = xz_mod * -sin(radians_rot_y);

            vec3 normal(pos_x, pos_y, pos_z);

            mesh.vbo[y * vertices_per_circumference + x + 1] = Vertex(
                vec3(radius) * normal + center,
                normal,
                vec2(float(x) / vertices_per_circumference, (y + 1.0f) / (vertices_per_circumference + 1))
            );
        }
    }

    // Bottom
    for (unsigned x = 0; x < vertices_per_circumference - 1; x++)
    {
        mesh.ibo.push_back(x + 1);
        mesh.ibo.push_back(0);
        mesh.ibo.push_back(x + 2);
    }
    mesh.ibo.push_back(vertices_per_circumference);
    mesh.ibo.push_back(0);
    mesh.ibo.push_back(1);

    // Quads
    for (unsigned y = 0; y < vertices_per_circumference - 1; y++)
    {
        for (unsigned x = 0; x < vertices_per_circumference - 1; x++)
        {
            unsigned base = 1 + y * vertices_per_circumference + x;
            mesh.ibo.push_back(base);
            mesh.ibo.push_back(base + 1);
            mesh.ibo.push_back(base + vertices_per_circumference);
            mesh.ibo.push_back(base + vertices_per_circumference + 1);
            mesh.ibo.push_back(base + vertices_per_circumference);
            mesh.ibo.push_back(base + 1);
        }
        unsigned base = 1 + y * vertices_per_circumference + (vertices_per_circumference - 1);
        mesh.ibo.push_back(base);
        mesh.ibo.push_back(base - (vertices_per_circumference - 1));
        mesh.ibo.push_back(base + vertices_per_circumference);
        mesh.ibo.push_back(base + 1);
        mesh.ibo.push_back(base + vertices_per_circumference);
        mesh.ibo.push_back(base - (vertices_per_circumference - 1));
    }

    // Top
    for (unsigned x = 0; x < vertices_per_circumference - 1; x++)
    {
        unsigned base = 1 + (vertices_per_circumference - 1) * vertices_per_circumference + x;
        mesh.ibo.push_back(base);
        mesh.ibo.push_back(base + 1);
        mesh.ibo.push_back(vertices_per_circumference * vertices_per_circumference + 1);
    }
    mesh.ibo.push_back(1 + (vertices_per_circumference - 1) * vertices_per_circumference + vertices_per_circumference - 1);
    mesh.ibo.push_back(1 + (vertices_per_circumference - 1) * vertices_per_circumference);
    mesh.ibo.push_back(vertices_per_circumference * vertices_per_circumference + 1);

    mesh.aabb.minpos = vec4(center - vec3(radius), 0.0f);
    mesh.aabb.maxpos = vec4(center + vec3(radius), 0.0f);

    return mesh;
}

Mesh create_box_mesh(const AABB &aabb)
{
    static const Vertex vertex_data[] = {
        Vertex( vec3(-1, -1,  1), vec3( 0, 0,  1 ), vec2(0, 0) ), // Front
        Vertex( vec3( 1, -1,  1), vec3( 0, 0,  1 ), vec2(1, 0) ),
        Vertex( vec3(-1,  1,  1), vec3( 0, 0,  1 ), vec2(0, 1) ),
        Vertex( vec3( 1,  1,  1), vec3( 0, 0,  1 ), vec2(1, 1) ),

        Vertex( vec3( 1, -1, -1), vec3( 0, 0, -1 ), vec2(0, 0) ), // Back
        Vertex( vec3(-1, -1, -1), vec3( 0, 0, -1 ), vec2(1, 0) ),
        Vertex( vec3( 1,  1, -1), vec3( 0, 0, -1 ), vec2(0, 1) ),
        Vertex( vec3(-1,  1, -1), vec3( 0, 0, -1 ), vec2(1, 1) ),

        Vertex( vec3(-1, -1, -1), vec3(-1, 0,  0 ), vec2(0, 0) ), // Left
        Vertex( vec3(-1, -1,  1), vec3(-1, 0,  0 ), vec2(1, 0) ),
        Vertex( vec3(-1,  1, -1), vec3(-1, 0,  0 ), vec2(0, 1) ),
        Vertex( vec3(-1,  1,  1), vec3(-1, 0,  0 ), vec2(1, 1) ),

        Vertex( vec3( 1, -1,  1), vec3( 1, 0,  0 ), vec2(0, 0) ), // Right
        Vertex( vec3( 1, -1, -1), vec3( 1, 0,  0 ), vec2(1, 0) ),
        Vertex( vec3( 1,  1,  1), vec3( 1, 0,  0 ), vec2(0, 1) ),
        Vertex( vec3( 1,  1, -1), vec3( 1, 0,  0 ), vec2(1, 1) ),

        Vertex( vec3(-1,  1,  1), vec3( 0, 1,  0 ), vec2(0, 0) ), // Top
        Vertex( vec3( 1,  1,  1), vec3( 0, 1,  0 ), vec2(1, 0) ),
        Vertex( vec3(-1,  1, -1), vec3( 0, 1,  0 ), vec2(0, 1) ),
        Vertex( vec3( 1,  1, -1), vec3( 0, 1,  0 ), vec2(1, 1) ),

        Vertex( vec3(-1, -1, -1), vec3( 0, -1, 0), vec2(0, 0) ), // Bottom
        Vertex( vec3( 1, -1, -1), vec3( 0, -1, 0), vec2(1, 0) ),
        Vertex( vec3(-1, -1,  1), vec3( 0, -1, 0), vec2(0, 1) ),
        Vertex( vec3( 1, -1,  1), vec3( 0, -1, 0), vec2(1, 1) ),
    };

    static const GLushort indices[] = {
        0, 1, 2, // Front
        3, 2, 1,

        4, 5, 6, // Back
        7, 6, 5,

        8, 9, 10, // Left
        11, 10, 9,

        12, 13, 14, // Right
        15, 14, 13,

        16, 17, 18, // Top
        19, 18, 17,

        20, 21, 22, // Bottom
        23, 22, 21,
    };

    Mesh mesh;
    mesh.aabb = aabb;

    vec3 half_position = vec3(0.5f) * (vec3(aabb.minpos) + vec3(aabb.maxpos));
    vec3 half_distance = vec3(0.5f) * (vec3(aabb.maxpos) - vec3(aabb.minpos));

    for (unsigned i = 0; i < sizeof(vertex_data) / sizeof(Vertex); i++)
    {
        const Vertex &vert = vertex_data[i];
        vec3 pos = half_position + half_distance * vert.position;
        mesh.vbo.push_back(Vertex(pos, vert.normal, vert.tex));
    }

    mesh.ibo.insert(mesh.ibo.begin(), indices, indices + sizeof(indices) / sizeof(indices[0]));
    return mesh;
}

