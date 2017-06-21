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

#ifndef _geometry_h_
#define _geometry_h_
#include <stdio.h>
#include "matrix.h"
#include <GLES3/gl31.h>

struct Volume
{
    GLuint tex;
    int x, y, z;
};

struct App
{
    int window_width;
    int window_height;
    float frame_time;
    float elapsed_time;
    float fov;
    float z_near;
    float z_far;

    // Scene interaction
    float pointer_x;
    float pointer_y;
    bool pointer_down;
    bool pointer_released;
    float rotate_x;
    float rotate_y;
    float translate_z;
    vec3 sphere_pos;
    float sphere_radius;
    float voxel_mode;

    // Geometry construction shader
    GLuint program_geometry;
    GLuint a_geometry_texel;
    GLuint u_geometry_inCentroid;
    GLuint u_geometry_inSurface;
    GLuint u_geometry_inMaterial;
    GLuint u_geometry_view;
    GLuint u_geometry_projection;

    // Backdrop shader
    GLuint program_backdrop;
    GLuint a_backdrop_position;

    // Centroid placement shader
    GLuint program_centroid;
    GLuint u_centroid_voxel_mode;

    // Isosurface generation shader
    GLuint program_generate;
    GLuint u_generate_sphere_radius;
    GLuint u_generate_sphere_pos;
    GLuint u_generate_dimension;
    GLuint u_generate_time;

    // Geometry
    GLuint vao;
    GLuint vbo_point;
    GLuint vbo_points;
    GLuint ibo_points;
    GLuint vbo_quad;

    // Indirect draw call buffer and
    // associated indices
    GLuint indirect_buffer;
    GLuint index_buffer;

    // 2D textures
    GLuint tex_material;

    // 3D textures
    Volume tex_centroid;
    Volume tex_surface;
};

void gl_check(const char *msg);
void app_initialize(App *app);
void app_update_and_render(App *app);

/////////////////////////////////////
// Convenience macros

#define get_attrib_location(prog, name) \
    app->a_##prog##_##name = glGetAttribLocation(app->program_##prog, #name); \
    if (app->a_##prog##_##name < 0) { \
        LOGE("Invalid or unused attribute %s\n", #name); \
    }

#define get_uniform_location(prog, name) \
    app->u_##prog##_##name = glGetUniformLocation(app->program_##prog, #name); \
    if (app->u_##prog##_##name < 0) { \
        LOGE("Invalid or unused uniform %s\n", #name); \
    }

#define attribfv(prog, name, n, offset) \
    glEnableVertexAttribArray(app->a_##prog##_##name); \
    glVertexAttribPointer(app->a_##prog##_##name, n, GL_FLOAT, GL_FALSE, \
                          n * sizeof(float), (void*)offset);

#define attribiv(prog, name, n, offset) \
    glEnableVertexAttribArray(app->a_##prog##_##name); \
    glVertexAttribPointer(app->a_##prog##_##name, n, GL_INT, GL_FALSE, \
                          n * sizeof(GLint), (void*)offset);

#define uniform1f(prog, name, value)  glUniform1f(app->u_##prog##_##name, value);
#define uniform2f(prog, name, x, y)   glUniform2f(app->u_##prog##_##name, x, y);
#define uniform3fv(prog, name, value) glUniform3fv(app->u_##prog##_##name, 1, &value[0]);
#define uniform1i(prog, name, value)  glUniform1i(app->u_##prog##_##name, value);
#define uniformm4(prog, name, value)  glUniformMatrix4fv(app->u_##prog##_##name, 1, GL_FALSE, value.value_ptr());

#endif
