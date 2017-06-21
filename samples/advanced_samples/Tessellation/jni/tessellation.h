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

#ifndef _tessellation_h_
#define _tessellation_h_
#include <stdio.h>
#include "matrix.h"

#define NUM_SCENES 5
struct Scene
{
    GLuint heightmap;
    GLuint diffusemap;
    vec3 sun_dir;
    bool use_mip;
    float max_lod_coverage;
    float height_scale;
    float fov;
    float z_near;
    float z_far;
};

struct App
{
    int window_width;
    int window_height;
    float elapsed_time;

    int current_scene;
    Scene scenes[5];

    // Displacement mapping shader
    GLuint program_mapping;
    GLuint a_mapping_position;
    GLuint u_mapping_height_scale;
    GLuint u_mapping_use_mip;
    GLuint u_mapping_max_lod_coverage;
    GLuint u_mapping_screen_size;
    GLuint u_mapping_diffusemap;
    GLuint u_mapping_heightmap;
    GLuint u_mapping_model;
    GLuint u_mapping_view;
    GLuint u_mapping_projection;

    // Backdrop shader
    GLuint program_backdrop;
    GLuint a_backdrop_position;
    GLuint u_backdrop_view;
    GLuint u_backdrop_sun_dir;
    GLuint u_backdrop_screen_size;
    GLuint u_backdrop_inv_tan_fov;

    // Geometry
    GLuint vao;
    GLuint vbo_cube;
    GLuint vbo_quad;
};

void app_initialize(App *app);
void app_update_and_render(App *app);

/////////////////////////////////////
// Convenience macros

#define get_attrib_location(prog, name) \
    app->a_##prog##_##name = glGetAttribLocation(app->program_##prog, #name); \
    if (app->a_##prog##_##name < 0) { \
        printf("Invalid or unused attribute %s\n", #name); \
    }

#define get_uniform_location(prog, name) \
    app->u_##prog##_##name = glGetUniformLocation(app->program_##prog, #name); \
    if (app->u_##prog##_##name < 0) { \
        printf("Invalid or unused uniform %s\n", #name); \
    }

#define attribfv(prog, name, n, offset) \
    glEnableVertexAttribArray(app->a_##prog##_##name); \
    glVertexAttribPointer(app->a_##prog##_##name, n, GL_FLOAT, GL_FALSE, \
                          n * sizeof(float), (void*)offset);

#define uniform1f(prog, name, value)  glUniform1f(app->u_##prog##_##name, value);
#define uniform2f(prog, name, x, y)   glUniform2f(app->u_##prog##_##name, x, y);
#define uniform3fv(prog, name, value) glUniform3fv(app->u_##prog##_##name, 1, &value[0]);
#define uniform1i(prog, name, value)  glUniform1i(app->u_##prog##_##name, value);
#define uniformm4(prog, name, value)  glUniformMatrix4fv(app->u_##prog##_##name, 1, GL_FALSE, value.value_ptr());

#endif
