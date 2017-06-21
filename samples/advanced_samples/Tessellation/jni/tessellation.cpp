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

#include "tessellation.h"
#define QUAD_RES_X 16
#define QUAD_RES_Y 16
#define SIDES_IN_A_CUBE 6
#define NUM_PATCHES (QUAD_RES_X * QUAD_RES_Y * SIDES_IN_A_CUBE)
#define VERTICES_PER_PATCH 4

struct Vertex
{
    float x, y, z;
};

enum CubeSide
{
    Top,
    Bottom,
    Left,
    Right,
    Front,
    Back
};

int fill_cube_side(Vertex *v, CubeSide side)
{
    #define CUBE_SIDE_BIAS 0.0f
    int i = 0;
    for (int y = 0; y < QUAD_RES_Y; y++)
    {
        for (int x = 0; x < QUAD_RES_X; x++)
        {
            float x0 = -1.0f - CUBE_SIDE_BIAS + (2.0f + 2.0f * CUBE_SIDE_BIAS) * ((float)x / (float)QUAD_RES_X);
            float x1 = -1.0f - CUBE_SIDE_BIAS + (2.0f + 2.0f * CUBE_SIDE_BIAS) * ((float)(x + 1) / (float)QUAD_RES_X);
            float y0 = -1.0f - CUBE_SIDE_BIAS + (2.0f + 2.0f * CUBE_SIDE_BIAS) * ((float)y / (float)QUAD_RES_Y);
            float y1 = -1.0f - CUBE_SIDE_BIAS + (2.0f + 2.0f * CUBE_SIDE_BIAS) * ((float)(y + 1) / (float)QUAD_RES_Y);
            Vertex v0 = { };
            Vertex v1 = { };
            Vertex v2 = { };
            Vertex v3 = { };
            switch (side)
            {
                case Top:
                    v0.x = x0; v0.y = 1.0f; v0.z = y0;
                    v1.x = x0; v1.y = 1.0f; v1.z = y1;
                    v2.x = x1; v2.y = 1.0f; v2.z = y1;
                    v3.x = x1; v3.y = 1.0f; v3.z = y0;
                    break;
                case Bottom:
                    v0.x = x0; v0.y = -1.0f; v0.z = y0;
                    v1.x = x1; v1.y = -1.0f; v1.z = y0;
                    v2.x = x1; v2.y = -1.0f; v2.z = y1;
                    v3.x = x0; v3.y = -1.0f; v3.z = y1;
                    break;
                case Left:
                    v0.x = -1.0f; v0.y = x0; v0.z = y0;
                    v1.x = -1.0f; v1.y = x0; v1.z = y1;
                    v2.x = -1.0f; v2.y = x1; v2.z = y1;
                    v3.x = -1.0f; v3.y = x1; v3.z = y0;
                    break;
                case Right:
                    v0.x = +1.0f; v0.y = x0; v0.z = y0;
                    v1.x = +1.0f; v1.y = x1; v1.z = y0;
                    v2.x = +1.0f; v2.y = x1; v2.z = y1;
                    v3.x = +1.0f; v3.y = x0; v3.z = y1;
                    break;
                case Front:
                    v0.x = x0; v0.y = y0; v0.z = +1.0f;
                    v1.x = x1; v1.y = y0; v1.z = +1.0f;
                    v2.x = x1; v2.y = y1; v2.z = +1.0f;
                    v3.x = x0; v3.y = y1; v3.z = +1.0f;
                    break;
                case Back:
                    v0.x = x0; v0.y = y0; v0.z = -1.0f;
                    v1.x = x0; v1.y = y1; v1.z = -1.0f;
                    v2.x = x1; v2.y = y1; v2.z = -1.0f;
                    v3.x = x1; v3.y = y0; v3.z = -1.0f;
                    break;
            }
            v[i++] = v0;
            v[i++] = v1;
            v[i++] = v2;
            v[i++] = v3;
        }
    }
    return i;
}

GLuint make_cube_mesh()
{
    Vertex v[NUM_PATCHES * VERTICES_PER_PATCH];
    Vertex *vptr = &v[0];
    vptr += fill_cube_side(vptr, Top);
    vptr += fill_cube_side(vptr, Bottom);
    vptr += fill_cube_side(vptr, Left);
    vptr += fill_cube_side(vptr, Right);
    vptr += fill_cube_side(vptr, Front);
    vptr += fill_cube_side(vptr, Back);
    GLuint result = 0;
    glGenBuffers(1, &result);
    glBindBuffer(GL_ARRAY_BUFFER, result);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    return result;
}

GLuint make_quad_mesh()
{
    float v[] = {
        -1.0f, -1.0f,
        +1.0f, -1.0f,
        +1.0f, +1.0f,
        +1.0f, +1.0f,
        -1.0f, +1.0f,
        -1.0f, -1.0f
    };
    GLuint result = 0;
    glGenBuffers(1, &result);
    glBindBuffer(GL_ARRAY_BUFFER, result);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    return result;
}

void app_initialize(App *app)
{
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glViewport(0, 0, app->window_width, app->window_height);

    app->vbo_cube = make_cube_mesh();
    app->vbo_quad = make_quad_mesh();

    get_attrib_location (mapping, position);
    get_uniform_location(mapping, height_scale);
    get_uniform_location(mapping, use_mip);
    get_uniform_location(mapping, max_lod_coverage);
    get_uniform_location(mapping, screen_size);
    get_uniform_location(mapping, diffusemap);
    get_uniform_location(mapping, heightmap);
    get_uniform_location(mapping, model);
    get_uniform_location(mapping, view);
    get_uniform_location(mapping, projection);

    get_attrib_location (backdrop, position);
    get_uniform_location(backdrop, sun_dir);
    get_uniform_location(backdrop, screen_size);
    get_uniform_location(backdrop, inv_tan_fov);
    get_uniform_location(backdrop, view);

    app->current_scene = 0;
}

float animate_model_scale(float t)
{
    int n = (int)(t / 20.0f);
    float modt = t - n * 20.0f;
    if (modt >= 19.5f && modt <= 20.0f)
    {
        return cos((modt - 19.5f) * 3.1415926f);
    }
    else if (modt <= 0.5f)
    {
        return sin(modt * 3.1415926f);
    }
    return 1.0f;
}

mat4 animate_camera(float t)
{
    float rx = -0.3f + 0.25f * sin(t * 0.1f);
    float ry = t * 0.4f;
    float zoom = 0.5f + 0.5f * sin(t * 0.25f);
    float z = -5.0f + 3.0f * sin(t * 0.25f);
    return translate(0.6f, -0.4f, z) * rotateX(rx) * rotateY(0.1f * t);
}

void app_update_and_render(App *app)
{
    app->current_scene = (int)(app->elapsed_time / 20.0f) % NUM_SCENES;
    Scene scene = app->scenes[app->current_scene];

    float model_scale = animate_model_scale(app->elapsed_time);

    float aspect_ratio = app->window_width / (float)app->window_height;
    mat4 mat_projection = perspective(scene.fov, aspect_ratio, scene.z_near, scene.z_far);
    mat4 mat_cube_model = rotateX(PI / 2.0f) * scale(model_scale);
    mat4 mat_view = animate_camera(app->elapsed_time);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthRangef(0.0, 1.0);

    glClearDepthf(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    ////////////////
    // Draw backdrop
    glDepthMask(GL_FALSE);
    glUseProgram(app->program_backdrop);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo_quad);
    attribfv(backdrop, position, 2, 0);
    uniform3fv(backdrop,    sun_dir,        scene.sun_dir);
    uniform2f(backdrop,     screen_size,    app->window_width, app->window_height);
    uniform1f(backdrop,     inv_tan_fov,    1.0f / (scene.fov / 2.0f));
    uniformm4(backdrop,     view,           mat_view);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDepthMask(GL_TRUE);

    //////////////////////////
    // Draw tessellated sphere
    glUseProgram(app->program_mapping);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo_cube);
    attribfv(mapping, position, 3, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, scene.heightmap);
    uniform1i(mapping, heightmap, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, scene.diffusemap);
    uniform1i(mapping, diffusemap, 1);

    uniform1f(mapping, height_scale,        scene.height_scale);
    uniform1f(mapping, use_mip,             scene.use_mip ? 1.0f : 0.0f);
    uniform1f(mapping, max_lod_coverage,    scene.max_lod_coverage);
    uniform2f(mapping, screen_size,         app->window_width, app->window_height);
    uniformm4(mapping, model,               mat_cube_model);
    uniformm4(mapping, view,                mat_view);
    uniformm4(mapping, projection,          mat_projection);
    glPatchParameteri(GL_PATCH_VERTICES, VERTICES_PER_PATCH);
    glDrawArrays(GL_PATCHES, 0, NUM_PATCHES * VERTICES_PER_PATCH);
}
