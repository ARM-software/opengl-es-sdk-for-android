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

#include "app.h"
#include "timer.h"
#include "shader.h"
#include "primitives.h"
#include "noise.h"
#include "sort.h"
#include <math.h>
const float TIMESTEP = 0.005f;
const uint32 NUM_PARTICLES = NUM_KEYS;

Shader
    shader_plane,
    shader_sphere,
    shader_update,
    shader_spawn,
    shader_draw_particle,
    shader_shadow_map;

Mesh
    quad,
    plane,
    sphere;

mat4
    mat_projection,
    mat_projection_light,
    mat_view,
    mat_view_light;

vec2
    camera_angle,
    camera_angle_vel,
    last_tap;

vec3
    light_pos,
    light_color,
    light_ambient,
    smoke_color,
    smoke_shadow,
    emitter_pos,
    sphere_pos,
    sphere_pos_target,
    sort_axis;

float
    particle_lifetime;

bool
    front_to_back,
    dragging;

GLuint
    buffer_position,
    buffer_spawn,
    shadow_map_tex,
    shadow_map_fbo;

int
    window_width,
    window_height,
    shadow_map_width,
    shadow_map_height;

Shader
    shader_count;

bool load_app()
{
    string res = "/data/data/com.arm.malideveloper.openglessdk.computeparticles/files/";
    if (!shader_update.load_compute_from_file(res + "update.cs") ||
        !shader_spawn.load_compute_from_file(res + "spawn.cs") ||
        !shader_plane.load_from_file(res + "plane.vs", res + "plane.fs") ||
        !shader_sphere.load_from_file(res + "sphere.vs", res + "sphere.fs") ||
        !shader_shadow_map.load_from_file(res + "shadowmap.vs", res + "shadowmap.fs") ||
        !shader_draw_particle.load_from_file(res + "particle.vs", res + "particle.fs"))
        return false;

    if (!shader_update.link() ||
        !shader_spawn.link() ||
        !shader_plane.link() ||
        !shader_sphere.link() ||
        !shader_shadow_map.link() ||
        !shader_draw_particle.link())
        return false;

    if (!sort_init())
        return false;

    return true;
}

void free_app()
{
    shader_plane.dispose();
    shader_sphere.dispose();
    shader_update.dispose();
    shader_spawn.dispose();
    shader_shadow_map.dispose();
    shader_draw_particle.dispose();

    del_buffer(buffer_position);
    del_buffer(buffer_spawn);

    quad.dispose();
    plane.dispose();
    sphere.dispose();

    glDeleteTextures(1, &shadow_map_tex);
    glDeleteFramebuffers(1, &shadow_map_fbo);

    sort_free();
}

void init_shadowmap(int width, int height)
{
    glGenTextures(1, &shadow_map_tex);
    glBindTexture(GL_TEXTURE_2D, shadow_map_tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &shadow_map_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_map_tex, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT(status == GL_FRAMEBUFFER_COMPLETE, "Framebuffer not complete\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_particles()
{
    // Store particle position (x, y, z) and lifetime (w)
    vec4 *data = new vec4[NUM_PARTICLES];
    for (int i = 0; i < NUM_PARTICLES; ++i)
    {
        // Distribute initial position inside a sphere of radius 0.3
        vec3 p = vec3(0.0f, 0.0f, 0.0f);
        p.x = 0.3f * (-1.0f + 2.0f * frand());
        p.y = 0.3f * (-1.0f + 2.0f * frand());
        p.z = 0.3f * (-1.0f + 2.0f * frand());

        // Each particle has a slightly randomized lifetime, around a constant value
        float lifetime = (1.0 + 0.25 * frand()) * particle_lifetime;
        data[i] = vec4(p, lifetime);
    }
    buffer_position = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW, NUM_PARTICLES * sizeof(vec4), data);
    buffer_spawn = gen_buffer(GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW, NUM_PARTICLES * sizeof(vec4), NULL);
    delete[] data;
}

void init_app(int width, int height)
{
    window_width = width;
    window_height = height;

    camera_angle = vec2(-0.7f, 0.0f);
    camera_angle_vel = vec2(0.0f, 0.0f);
    mat_view = translate(0.0f, 0.0f, -2.0f) * rotateX(camera_angle.x) * rotateY(camera_angle.y);
    mat_view_light = translate(0.0f, 0.0f, -2.0f) * rotateX(-PI / 2.0f) * rotateY(PI / 2.0f);
    mat_projection = perspective(PI / 4.0f, float(width) / height, 0.1f, 10.0f);
    mat_projection_light = orthographic(-2.0f, 2.0f, -2.0f, 2.0f, 1.5f, 2.5f);

    sphere = gen_unit_sphere(24, 24);
    plane = gen_normal_plane();
    quad = gen_tex_quad();

    emitter_pos = vec3(0.0, 0.0, 0.0);
    sphere_pos = vec3(0.0, -0.5, 0.0);
    last_tap = vec2(0.0f, 0.0f);
    particle_lifetime = 0.7f;
    dragging = false;

    light_ambient = vec3(0.00137f, 0.0029f, 0.0063f);
    light_color = vec3(1.0f, 1.0f, 1.0f);
    smoke_color = vec3(0.93f, 0.79f, 0.72f);
    smoke_shadow = vec3(0.1f, 0.12f, 0.18f);

    shadow_map_width = 512;
    shadow_map_height = 512;

    init_particles();
    init_shadowmap(shadow_map_width, shadow_map_height);
}

/*
 * Sort the particles back-to-front relative to the point of view.
 * The sorting key is the distance along the sort axis, converted to
 * a 16-bit integer. To convert to an integer we need to map the distance
 * from a valid range (here -2 to 2) to [0, 65535].
*/
int pass = 0;
void sort_particles()
{
    // Calculate vector towards eye (in world space)
    vec4 v = vec4(0.0, 0.0, 0.0, 1.0);
    v = inverse(mat_view) * v;
    vec3 view_axis = normalize(v.xyz());
    radix_sort(buffer_position, view_axis, -2.0f, 2.0f);
}

/*
 * Simulates the particles according to a turbulent curl-noise fluid field,
 * superposed with a repulsion field around the sphere.
 * Particles run out of life after a while and respawn, using the information
 * stored in the spawn buffer.
*/
void update_particles()
{
    const uint32 WORK_GROUP_SIZE = 64;

    // Generate respawn info
    use_shader(shader_spawn);
    uniform("time", get_elapsed_time());
    uniform("emitterPos", emitter_pos);
    uniform("particleLifetime", particle_lifetime);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer_spawn);
    glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Advect through velocity field
    use_shader(shader_update);
    uniform("dt", TIMESTEP);
    uniform("time", get_elapsed_time());
    uniform("seed", vec3(13.0f, 127.0f, 449.0f));
    uniform("spherePos", sphere_pos);
    uniform("particleLifetime", particle_lifetime);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffer_position);
    glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
}

void update_shadow_map()
{
    // Additive blending
    blend_mode(true, GL_ONE, GL_ONE);

    // Clear shadowmap (all components 0!)
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo);
    glViewport(0, 0, shadow_map_width, shadow_map_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render shadow info
    use_shader(shader_shadow_map);
    uniform("projection", mat_projection_light);
    uniform("view", mat_view_light);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_position);
    attribfv("position", 4, 0, 0);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

    blend_mode(false);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
}

void update_app(float dt)
{
    float t = get_elapsed_time() * 0.7f;
    camera_angle += camera_angle_vel * dt;
    camera_angle.x = clamp(camera_angle.x, -PI - 0.3f, 0.3f);
    camera_angle_vel *= 0.95f;

    mat_view = translate(0.0f, 0.0f, -3.0f) * rotateX(camera_angle.x) * rotateY(camera_angle.y);
    mat_view_light = translate(0.0f, 0.0f, -2.0f) * rotateX(-PI / 2.0f);

    light_pos = (inverse(mat_view_light) * vec4(0.0, 0.0, 0.0, 1.0)).xyz();

    emitter_pos.x = 0.8f * sin(t * 1.2f);
    emitter_pos.z = 0.8f * cos(t * 0.7f);
    emitter_pos.y = 0.8f * sin(t * 2.0f) * 0.2f;

    sphere_pos += (sphere_pos_target - sphere_pos) * 3.5f * dt;

    update_particles();
    sort_particles();

    update_shadow_map();
}

void render_geometry()
{
    // Sphere
    glBindTexture(GL_TEXTURE_2D, shadow_map_tex);
    cull(true, GL_CW, GL_BACK);
    use_shader(shader_sphere);
    uniform("projection", mat_projection);
    uniform("projectionViewLight", mat_projection_light * mat_view_light);
    uniform("view", mat_view);
    uniform("lightDir", normalize(light_pos));
    uniform("shadowMap0", 0);
    uniform("model", translate(sphere_pos) * scale(0.1f));
    uniform("color", vec3(0.20f, 0.34f, 0.09f));
    sphere.bind();
    attribfv("position", 3, 0, 0);
    glDrawElements(GL_TRIANGLES, sphere.num_indices, GL_UNSIGNED_INT, 0);

    // Floor
    use_shader(shader_plane);
    uniform("projection", mat_projection);
    uniform("projectionViewLight", mat_projection_light * mat_view_light);
    uniform("view", mat_view);
    uniform("shadowMap0", 0);
    uniform("model", translate(0.0f, -1.0f, 0.0f) * scale(8.0f));
    uniform("color", vec3(0.20f, 0.05f, 0.022f));
    plane.bind();
    attribfv("position", 3, 6, 0);
    glDrawElements(GL_TRIANGLES, plane.num_indices, GL_UNSIGNED_INT, 0);
}

void render_particles()
{
    // Alphablending with premultiplied alpha
    blend_mode(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_FUNC_ADD);

    use_shader(shader_draw_particle);
    uniform("projection", mat_projection);
    uniform("view", mat_view);
    uniform("particleLifetime", particle_lifetime);
    uniform("projectionViewLight", mat_projection_light * mat_view_light);
    uniform("smokeColor", smoke_color);
    uniform("smokeShadow", smoke_shadow);
    uniform("shadowMap0", 0);
    glBindTexture(GL_TEXTURE_2D, shadow_map_tex);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_position);
    attribfv("position", 4, 0, 0);
    glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void render_app(float dt)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthRangef(0.0f, 1.0f);
    glDepthFunc(GL_LEQUAL);
    glClearDepthf(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render_geometry();

    // The particles are rendered without depth writes, but with depth testing
    // If they write to the depth buffer you'll likely get some artifacts here and there.
    glDepthMask(GL_FALSE);
    render_particles();
}

void on_pointer_down(float x, float y)
{
    // Raycast mouse point onto the floor
    float xndc = -1.0f + 2.0f * x / window_width;
    float yndc = 1.0f - 2.0f * y / window_height;

    // Project onto near clipping plane
    vec4 view = inverse(mat_projection) * vec4(xndc, yndc, 1.0f, 1.0f);

    // Solve ray intersection equation
    vec3 origin = (inverse(mat_view) * vec4(0.0f, 0.0f, 0.0f, 1.0f)).xyz();
    vec3 dir = normalize((inverse(mat_view) * view).xyz());
    float t = -origin.y / dir.y;
    vec3 p = origin + dir * t;

    if (p.x < -2.0f || p.x > 2.0f || p.z < -2.0f || p.z > 2.0f || dragging)
    {
        if (!dragging)
        {
            dragging = true;
            last_tap = vec2(x, y);
        }

        float dx = x - last_tap.x;
        float dy = y - last_tap.y;
        camera_angle_vel -= vec2(dy, dx) * 0.0025f;
        last_tap = vec2(x, y);
    }
    else if (!dragging)
    {
        p.x = clamp(p.x, -2.0f, 2.0f);
        p.z = clamp(p.z, -2.0f, 2.0f);
        sphere_pos_target = p;
    }
}

void on_pointer_up(float x, float y)
{
    dragging = false;
}
