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

#include "geometry.h"
#include <vector>

// The size of one grid side length
#define N 64

Volume make_surface_volume()
{
    // This should be one more than the dimensions of
    // the centroid texture, since each centroid should
    // have two neighbor noise values.
    int width  = N + 1;
    int height = N + 1;
    int depth  = N + 1;

    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_3D, handle);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, width, height, depth);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    Volume result;
    result.tex = handle;
    result.x = width;
    result.y = height;
    result.z = depth;
    return result;
}

Volume make_centroid_volume()
{
    int width  = N;
    int height = N;
    int depth  = N;

    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_3D, handle);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_RGBA8, width, height, depth);
    Volume result;
    result.tex = handle;
    result.x = width;
    result.y = height;
    result.z = depth;
    return result;
}

GLuint make_quad()
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

void make_points(GLuint *vbo, GLuint *ibo)
{
    std::vector<int> v(N*N*N*3);
    int *vptr = &v[0];
    unsigned int i = 0;
    std::vector<unsigned int> I(N*N*N);
    for (int z = 0; z < N; z++)
    for (int y = 0; y < N; y++)
    for (int x = 0; x < N; x++)
    {
        vptr[0] = x;
        vptr[1] = y;
        vptr[2] = z;
        vptr += 3;
        I[i] = i;
        i++;
    }
    glGenBuffers(1, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v[0]) * v.size(), v.data(), GL_STATIC_DRAW);
    glGenBuffers(1, ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(I[0]) * I.size(), I.data(), GL_STATIC_DRAW);
}

void clear_indirect_buffer(App *app)
{
    typedef  struct {
        GLuint  count;
        GLuint  instanceCount;
        GLuint  firstIndex;
        GLint   baseVertex;
        GLuint  reservedMustBeZero;
    } DrawElementsIndirectCommand;

    DrawElementsIndirectCommand cmd = {};
    cmd.instanceCount = 1;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, app->indirect_buffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(cmd), &cmd, GL_STREAM_DRAW);
}

void update_surface(App *app)
{
    int local_size_x = 4;
    int local_size_y = 4;
    int local_size_z = 4;

    // Since the surface volume texture has a resolution
    // one higher than the centroid texture, we cannot divide
    // it evenly to work groups of size larger than one. To
    // accommodate this, we add one work group for the edge.
    int work_groups_x = 1 + (app->tex_surface.x - 1) / local_size_x;
    int work_groups_y = 1 + (app->tex_surface.y - 1) / local_size_y;
    int work_groups_z = 1 + (app->tex_surface.z - 1) / local_size_z;

    glUseProgram(app->program_generate);
    uniform1f(generate,  time,          app->elapsed_time);
    uniform1i(generate,  dimension,     N);
    uniform3fv(generate, sphere_pos,    app->sphere_pos);
    uniform1f(generate,  sphere_radius, app->sphere_radius);
    glBindImageTexture(0, app->tex_surface.tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
    glDispatchCompute(work_groups_x, work_groups_y, work_groups_z);

    // Ensure that the surface texture is properly updated
    // before it is sampled in the centroid shader (using imageLoad)
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void update_centroid(App *app)
{
    int local_size_x = 4;
    int local_size_y = 4;
    int local_size_z = 4;

    int work_groups_x = app->tex_centroid.x / local_size_x;
    int work_groups_y = app->tex_centroid.y / local_size_y;
    int work_groups_z = app->tex_centroid.z / local_size_z;

    clear_indirect_buffer(app);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, app->indirect_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, app->index_buffer);

    glUseProgram(app->program_centroid);
    uniform1f(centroid, voxel_mode, app->voxel_mode);
    glBindImageTexture(0, app->tex_surface.tex, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(1, app->tex_centroid.tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(work_groups_x, work_groups_y, work_groups_z);

    // Ensure that the centroid offsets are properly written
    // before we attempt to read them in the geometry shader.
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    // Ensure that the indirect draw buffer and the index buffer
    // is properly written before we attempt to use it for drawing.
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
    glMemoryBarrier(GL_ELEMENT_ARRAY_BARRIER_BIT);
}

void update_sphere(App *app, mat4 mat_view)
{
    // Raycast mouse point onto the floor
    float u = -1.0f + 2.0f * app->pointer_x / app->window_width;
    float v = +1.0f - 2.0f * app->pointer_y / app->window_height;
    u *= (float)app->window_width / (float)app->window_height;

    // Compute the camera basis from the view-transformation matrix
    mat4 R = transpose(mat_view);
    vec3 up = normalize(R.y.xyz());
    vec3 right = normalize(R.x.xyz());
    vec3 forward = normalize(R.z.xyz());

    // Compute the ray origin and direction
    vec4 camera_pos = -vec4(mat_view.w.xyz(), 0.0f);
    vec3 ro = (R * camera_pos).xyz();
    vec3 rd = normalize(-forward * (1.0f / tan(app->fov / 2.0f)) + right * u + up * v);

    // Raytrace floor
    float t = -ro.y / rd.y;
    vec3 target = ro + rd * t;

    // Animate sphere position and size
    app->sphere_pos = target;
    if (app->pointer_down && app->sphere_radius < 0.15f)
        app->sphere_radius += 10.0f * app->frame_time * (0.15f - app->sphere_radius);
    else if (!app->pointer_down && app->sphere_radius > 0.0f)
        app->sphere_radius += 10.0f * app->frame_time * (0.0f - app->sphere_radius);
}

// Make the camera bob from side to side as the user drags the cursor
mat4 animate_view(App *app)
{
    float center_tz = -4.0f;
    if (app->window_height > app->window_width)
        center_tz = -6.0f;

    float center_rx = -0.50f;
    float center_ry = -0.25f;

    float pan = -1.0f + 2.0f * app->pointer_x / app->window_width;
    float target_ry = 0.1f * pan;
    float target_rx = -0.1f * pan * pan;
    float target_tz = 1.2f * pan * pan;
    float translate_x = -0.5f * app->rotate_y;

    app->rotate_y    += 2.5f * app->frame_time * (target_ry - app->rotate_y);
    app->rotate_x    += 1.2f * app->frame_time * (target_rx - app->rotate_x);
    app->translate_z += 0.8f * app->frame_time * (target_tz - app->translate_z);
    mat4 result = translate(translate_x, 0.0f, center_tz + app->translate_z) *
                  rotateX(center_rx + app->rotate_x) *
                  rotateY(center_ry + app->rotate_y);

    return result;
}

void app_initialize(App *app)
{
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glViewport(0, 0, app->window_width, app->window_height);

    app->tex_centroid = make_centroid_volume();
    app->tex_surface = make_surface_volume();
    app->vbo_quad = make_quad();
    make_points(&app->vbo_points, &app->ibo_points);

    app->fov = PI / 7.0f;
    app->z_near = 1.0f;
    app->z_far = 15.0f;
    app->pointer_x = 0.0f;
    app->pointer_y = 0.0f;
    app->voxel_mode = 0.0f;

    app->rotate_x = 0.0f;
    app->rotate_y = 0.0f;
    app->translate_z = 0.0f;
    app->sphere_pos = vec3(0.0f, 0.0f, 0.0f);
    app->sphere_radius = 0.0f;

    get_attrib_location (backdrop, position);

    get_attrib_location (geometry, texel);
    get_uniform_location(geometry, inCentroid);
    get_uniform_location(geometry, inSurface);
    get_uniform_location(geometry, inMaterial);
    get_uniform_location(geometry, view);
    get_uniform_location(geometry, projection);

    get_uniform_location(generate, sphere_radius);
    get_uniform_location(generate, sphere_pos);
    get_uniform_location(generate, dimension);
    get_uniform_location(generate, time);

    get_uniform_location(centroid, voxel_mode);

    glGenBuffers(1, &app->indirect_buffer);
    glGenBuffers(1, &app->index_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, app->index_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N*N*N*sizeof(GLuint), 0, GL_STREAM_DRAW);

    update_surface(app);
    update_centroid(app);
}

void app_update_and_render(App *app)
{
    update_surface(app);
    update_centroid(app);

    float aspect_ratio = app->window_width / (float)app->window_height;
    mat4 mat_projection = perspective(app->fov, aspect_ratio, app->z_near, app->z_far);
    mat4 mat_view = animate_view(app);
    update_sphere(app, mat_view);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRangef(0.0, 1.0);

    glClearDepthf(1.0f);
    glClearColor(0.16f, 0.16f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ////////////////////////////
    // Backdrop shader

    glDepthMask(GL_FALSE);
    glUseProgram(app->program_backdrop);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo_quad);
    attribfv(backdrop, position, 2, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    ////////////////////////////
    // Geometry shader

    glDepthMask(GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, app->tex_surface.tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, app->tex_centroid.tex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, app->tex_material);

    glUseProgram(app->program_geometry);
    uniform1i(geometry, inSurface,  0);
    uniform1i(geometry, inCentroid, 1);
    uniform1i(geometry, inMaterial, 2);
    uniformm4(geometry, projection, mat_projection);
    uniformm4(geometry, view,       mat_view);

    glBindBuffer(GL_ARRAY_BUFFER, app->vbo_points);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->index_buffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, app->indirect_buffer);
    attribiv(geometry, texel, 3, 0);
    glDrawElementsIndirect(GL_POINTS, GL_UNSIGNED_INT, 0);
}
