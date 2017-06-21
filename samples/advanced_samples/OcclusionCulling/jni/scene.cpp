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

#include "scene.hpp"
#include "mesh.hpp"
#include <algorithm>
#include <stdlib.h>

using namespace std;

#define PHYSICS_GROUP_SIZE 128

// Spread our spheres out in three dimensions.
#define SPHERE_INSTANCES_X 24
#define SPHERE_INSTANCES_Y 24
#define SPHERE_INSTANCES_Z 24
#define SPHERE_INSTANCES (SPHERE_INSTANCES_X * SPHERE_INSTANCES_Y * SPHERE_INSTANCES_Z)

#define SPHERE_RADIUS 0.30f

// Defines how densely spheres should be tesselated (offline) at each LOD level.
#define SPHERE_VERT_PER_CIRC_LOD0 24
#define SPHERE_VERT_PER_CIRC_LOD1 20
#define SPHERE_VERT_PER_CIRC_LOD2 16
#define SPHERE_VERT_PER_CIRC_LOD3 12

// We use fixed uniform locations in the shaders (GLES 3.1 feature).
#define UNIFORM_MVP_LOCATION 0
#define UNIFORM_COLOR_LOCATION 1
#define UNIFORM_LIGHT_DIR_LOCATION 2

static const unsigned verts_per_circ[] = {
    SPHERE_VERT_PER_CIRC_LOD0,
    SPHERE_VERT_PER_CIRC_LOD1,
    SPHERE_VERT_PER_CIRC_LOD2,
    SPHERE_VERT_PER_CIRC_LOD3,
};

Scene::Scene()
{
    // Compile shaders.
    occluder_program = common_compile_shader_from_file("scene.vs", "scene.fs");
    sphere_program = common_compile_shader_from_file("scene_sphere.vs", "scene_sphere.fs");
    quad_program = common_compile_shader_from_file("quad.vs", "quad.fs");
    physics_program = common_compile_compute_shader_from_file("physics.cs");

    // Instantiate our various culling methods.
    culling_implementations.push_back(new HiZCulling);
    culling_implementations.push_back(new HiZCullingNoLOD);
    culling_implementation_index = CullHiZ;
    enable_culling = true;

    // Set up buffers, etc.
    init_instances();

    camera_rotation_y = 0.0f;
    camera_rotation_x = 0.0f;

    num_render_sphere_instances = SPHERE_INSTANCES;
    physics_speed = 1.0f;

    show_redundant = false;
}

// Move camera around. The view-projection matrix is recomputed elsewhere.
void Scene::move_camera(float delta_x, float delta_y)
{
    // Angles are mapped from [0, 1] => [0, 2pi] radians.
    camera_rotation_y -= delta_x * 0.25f;
    camera_rotation_x += delta_y * 0.15f;
    camera_rotation_x = clamp(camera_rotation_x, -0.20f, 0.20f);
    camera_rotation_y -= floor(camera_rotation_y);
}

// Bake our instanced occluder geometry into a single vertex buffer and index buffer.
// Makes the implementation somewhat more generic and simple.
void Scene::bake_occluder_geometry(vector<vec4> &occluder_positions, vector<uint32_t> &occluder_indices,
        const Mesh &box_mesh, const vec4 *instances, unsigned num_instances)
{
    unsigned total_vertices = num_instances * box_mesh.vbo.size();
    occluder_positions.reserve(total_vertices);

    unsigned total_indices = num_instances * box_mesh.ibo.size();
    occluder_indices.reserve(total_indices);

    // Bake the index buffer.
    for (unsigned i = 0; i < total_indices; i++)
    {
        unsigned instance = i / box_mesh.ibo.size();
        unsigned index = i % box_mesh.ibo.size();

        unsigned baked_index = box_mesh.ibo[index] + instance * box_mesh.vbo.size();
        occluder_indices.push_back(baked_index);
    }

    // Bake the vertex buffer.
    for (unsigned i = 0; i < total_vertices; i++)
    {
        unsigned instance = i / box_mesh.vbo.size();
        unsigned index = i % box_mesh.vbo.size();

        vec4 pos = instances[instance] + vec4(box_mesh.vbo[index].position, 1.0f);
        occluder_positions.push_back(pos);
    }
}

struct OccluderSorter
{
    bool operator()(const vec4 &a, const vec4 &b)
    {
        return vec_dot(a, a) < vec_dot(b, b);
    }
};

struct SphereInstance
{
    vec4 position;
    vec4 velocity;
};

void Scene::init_instances()
{
    // Create occluder boxes.
    AABB aabb = { vec4(-1.0f, 0.0f, -1.0f, 0.0f), vec4(1.0f, 8.0f, 1.0f, 0.0f) };
    Mesh box_mesh = create_box_mesh(aabb);
    box = new GLDrawable(box_mesh);

    // Create meshes for spheres at various LOD levels.
    for (unsigned i = 0; i < SPHERE_LODS; i++)
    {
        sphere[i] = new GLDrawable(create_sphere_mesh(1.0f, vec3(0, 0, 0), verts_per_circ[i]));
    }

    // Spread occluder geometry out on a grid on the XZ plane.
    // Skip the center, because we put our camera there.
    vector<vec4> occluder_instances;
    for (int z = 0; z < 13; z++)
    {
        for (int x = 0; x < 13; x++)
        {
            if (z >= 5 && z <= 7 && x >= 5 && x <= 7)
            {
                continue;
            }
            occluder_instances.push_back(vec4(3.0f) * vec4(x - 6, 0, z - 6, 0));
        }
    }
    // Rough front-to-back ordering to get more ideal rendering order.
    sort(occluder_instances.begin(), occluder_instances.end(), OccluderSorter());

    num_occluder_instances = occluder_instances.size();

    // Upload instance buffer.
    GL_CHECK(glGenBuffers(1, &occluder_instances_buffer));
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, occluder_instances_buffer));
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, occluder_instances.size() * sizeof(vec4), &occluder_instances[0], GL_STATIC_DRAW));

    // Place out spheres with different positions and velocities.
    // The W component contains the sphere radius, which is random.
    std::vector<SphereInstance> sphere_instances;
    for (int x = 0; x < SPHERE_INSTANCES_X; x++)
    {
        for (int y = 0; y < SPHERE_INSTANCES_Y; y++)
        {
            for (int z = 0; z < SPHERE_INSTANCES_Z; z++)
            {
                SphereInstance instance;
                instance.position = vec4(1.0f) * vec4(x - 11.35f, y * 0.10f + 0.5f, z - 11.45f, 0);
                instance.position.c.w = SPHERE_RADIUS * (1.0f - 0.5f * rand() / RAND_MAX);
                instance.velocity = vec4(vec3(4.0) * vec_normalize(vec3(x - 11.35f, 0.5f * y - 11.55f, z - 11.25f)), 0.0f);

                sphere_instances.push_back(instance);
            }
        }
    }

    // Upload sphere instance buffer.
    GL_CHECK(glGenBuffers(1, &sphere_instances_buffer));
    GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphere_instances_buffer));
    GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sphere_instances.size() * sizeof(SphereInstance), &sphere_instances[0], GL_STATIC_DRAW));

    // Initialize storage for our post-culled instance buffer.
    // The buffers must be at least as large as the sphere instance buffer (in case we have 100% visibility).
    GL_CHECK(glGenBuffers(SPHERE_LODS, indirect.instance_buffer));
    for (unsigned i = 0; i < SPHERE_LODS; i++)
    {
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, indirect.instance_buffer[i]));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sphere_instances.size() * sizeof(vec4), NULL, GL_DYNAMIC_COPY));
    }

    // Setup occluder geometry for each implementation.
    vector<vec4> occluder_positions;
    vector<uint32_t> occluder_indices;
    bake_occluder_geometry(occluder_positions, occluder_indices, box_mesh, &occluder_instances[0], occluder_instances.size());

    for (unsigned i = 0; i < culling_implementations.size(); i++)
    {
        culling_implementations[i]->setup_occluder_geometry(occluder_positions, occluder_indices);
    }

    // Initialize our indirect draw buffers.
    // Use a ring buffer of them, since we might want to read back old results to monitor our culling performance without stalling the pipeline.
    GL_CHECK(glGenBuffers(INDIRECT_BUFFERS, indirect.buffer));
    for (unsigned i = 0; i < SPHERE_LODS; i++)
    {
        GL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.buffer[i]));
        GL_CHECK(glBufferData(GL_DRAW_INDIRECT_BUFFER, SPHERE_LODS * sizeof(IndirectCommand), NULL, GL_DYNAMIC_COPY));
    }
    indirect.buffer_index = 0;
}

#define Z_NEAR 1.0f
#define Z_FAR 500.0f
void Scene::update_camera(float rotation_y, float rotation_x, unsigned viewport_width, unsigned viewport_height)
{
    // Compute view and projection matrices.
    float radians_y = 2.0f * PI * rotation_y;
    float radians_x = 2.0f * PI * rotation_x;

    mat4 rotation_matrix_y = mat_rotate_y(radians_y);
    mat4 rotation_matrix_x = mat_rotate_x(radians_x);
    vec3 camera_dir = vec3(rotation_matrix_y * rotation_matrix_x * vec4(0, 0, -1, 1));

    vec3 camera_position = vec3(0, 2, 0);

    view = mat_look_at(camera_position, camera_position + camera_dir, vec3(0, 1, 0));
    projection = mat_perspective_fov(60.0f, float(viewport_width) / viewport_height, Z_NEAR, Z_FAR);
    mat4 view_projection = projection * view;

    GL_CHECK(glProgramUniformMatrix4fv(occluder_program, UNIFORM_MVP_LOCATION, 1, GL_FALSE, value_ptr(view_projection)));
    GL_CHECK(glProgramUniformMatrix4fv(sphere_program, UNIFORM_MVP_LOCATION, 1, GL_FALSE, value_ptr(view_projection)));
}

void Scene::set_culling_method(CullingMethod method)
{
    if (method == CullNone)
    {
        enable_culling = false;
        culling_implementation_index = 0;
    }
    else
    {
        enable_culling = true;
        culling_implementation_index = static_cast<unsigned>(method);
    }
}

void Scene::apply_physics(float delta_time)
{
    if (physics_speed <= 0.0f)
    {
        return;
    }

    // Do physics on the spheres, in a compute shader.
    GL_CHECK(glUseProgram(physics_program));
    GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphere_instances_buffer));
    GL_CHECK(glProgramUniform1ui(physics_program, 0, SPHERE_INSTANCES));
    GL_CHECK(glProgramUniform1f(physics_program, 1, physics_speed * delta_time));
    GL_CHECK(glDispatchCompute((SPHERE_INSTANCES + PHYSICS_GROUP_SIZE - 1) / PHYSICS_GROUP_SIZE, 1, 1));

    // We don't need data here until bounding box check, so we can let rasterizer and physics run in parallel, avoiding memory barrier here.
}

void Scene::update(float delta_time, unsigned width, unsigned height)
{
    // Update scene rendering parameters.
    update_camera(camera_rotation_y, camera_rotation_x, width, height);

    // Update light direction, here it's static.
    vec3 light_dir = vec_normalize(vec3(2, 4, 1));
    GL_CHECK(glProgramUniform3fv(occluder_program, UNIFORM_LIGHT_DIR_LOCATION, 1, value_ptr(light_dir)));
    GL_CHECK(glProgramUniform3fv(sphere_program, UNIFORM_LIGHT_DIR_LOCATION, 1, value_ptr(light_dir)));

    // Move spheres around in a compute shader to make it more exciting.
    apply_physics(delta_time);

    if (enable_culling)
    {
        CullingInterface *culler = culling_implementations[culling_implementation_index];
        num_sphere_render_lods = culler->get_num_lods();

        // Rasterize occluders to depth map and mipmap it.
        culler->set_view_projection(projection, view, vec2(Z_NEAR, Z_FAR));
        culler->rasterize_occluders();

        // We need physics results after this.
        GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));

        IndirectCommand indirect_command[SPHERE_LODS];
        unsigned offsets[SPHERE_LODS];

        memset(indirect_command, 0, sizeof(indirect_command));

        for (unsigned i = 0; i < SPHERE_LODS; i++)
        {
            indirect_command[i].count = sphere[i]->get_num_elements();
            offsets[i] = 4 + sizeof(IndirectCommand) * i;
        }

        // Clear out our indirect draw buffer.
        GL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.buffer[indirect.buffer_index]));
        GL_CHECK(glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirect_command), indirect_command, GL_STREAM_DRAW));

        // Test occluders and build indirect commands as well as per-instance buffers for every LOD.
        culler->test_bounding_boxes(indirect.buffer[indirect.buffer_index], offsets, SPHERE_LODS,
                indirect.instance_buffer, sphere_instances_buffer,
                num_render_sphere_instances);
    }
    else
    {
        // If we don't do culling, we need to make sure we call the memory barrier for physics.
        GL_CHECK(glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT));
        num_sphere_render_lods = 1;
    }
}

void Scene::render_spheres(vec3 color_mod)
{
    if (enable_culling)
    {
        GL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect.buffer[indirect.buffer_index]));

        for (unsigned i = 0; i < num_sphere_render_lods; i++)
        {
            // Use different colors for different LOD levels to easily spot them.
            GL_CHECK(glProgramUniform3fv(sphere_program, UNIFORM_COLOR_LOCATION, 1,
                        value_ptr(color_mod * vec3(0.8f - 0.2f * i, 1.2f - 0.2f * i, 0.8f + 0.2f * i))));

            // Use different meshes for different LOD levels.
            GL_CHECK(glBindVertexArray(sphere[i]->get_vertex_array()));

            GL_CHECK(glEnableVertexAttribArray(3));
            GL_CHECK(glVertexAttribDivisor(3, 1));
            GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, indirect.instance_buffer[i]));
            GL_CHECK(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), 0));

            GL_CHECK(glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT,
                        reinterpret_cast<const void*>(i * sizeof(IndirectCommand))));
        }
    }
    else
    {
        // Unconditionally draw every instance of LOD0.
        GL_CHECK(glProgramUniform3fv(sphere_program, UNIFORM_COLOR_LOCATION, 1, value_ptr(color_mod * vec3(0.8f, 1.2f, 0.8f))));
        GL_CHECK(glBindVertexArray(sphere[0]->get_vertex_array()));
        GL_CHECK(glEnableVertexAttribArray(3));
        GL_CHECK(glVertexAttribDivisor(3, 1));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, sphere_instances_buffer));
        GL_CHECK(glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(vec4), 0));
        GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, sphere[0]->get_num_elements(),
                    GL_UNSIGNED_SHORT, NULL, num_render_sphere_instances));
    }
}

void Scene::render(unsigned width, unsigned height)
{
    if (enable_culling)
    {
        GL_CHECK(glClearColor(0.02f, 0.02f, 0.35f, 0.05f));
    }
    else
    {
        GL_CHECK(glClearColor(0.35f, 0.02f, 0.02f, 0.05f));
    }

    // Enable depth testing and culling.
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    GL_CHECK(glViewport(0, 0, width, height));

    // Render occluder boxes.
    GL_CHECK(glUseProgram(occluder_program));
    GL_CHECK(glProgramUniform3f(occluder_program, UNIFORM_COLOR_LOCATION, 1.2f, 0.6f, 0.6f));
    GL_CHECK(glBindVertexArray(box->get_vertex_array()));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, occluder_instances_buffer));
    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vec4), 0));
    GL_CHECK(glVertexAttribDivisor(3, 1));
    GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, box->get_num_elements(), GL_UNSIGNED_SHORT, 0, num_occluder_instances));

    GL_CHECK(glUseProgram(sphere_program));
    if (show_redundant)
    {
        // Draw false-positive meshes in a dark color.
        // False-positives will fail the depth test (pass with GL_GREATER).
        // We don't want to update the depth buffer, so the false-positives will be rendered in a "glitchy"
        // way due to the random ordering that occlusion culling introduces.
        GL_CHECK(glDepthFunc(GL_GREATER));
        GL_CHECK(glDepthMask(GL_FALSE));
        render_spheres(vec3(0.25f));
        GL_CHECK(glDepthMask(GL_TRUE));
        GL_CHECK(glDepthFunc(GL_LESS));
    }
    render_spheres(vec3(1.0f));

    if (enable_culling)
    {
        render_depth_map();
    }

    // Restore viewport (for text rendering).
    GL_CHECK(glViewport(0, 0, width, height));

    // Jump to next indirect draw buffer (ring buffer).
    indirect.buffer_index = (indirect.buffer_index + 1) % INDIRECT_BUFFERS;
}

void Scene::render_depth_map()
{
    GL_CHECK(glDisable(GL_DEPTH_TEST));
    GL_CHECK(glUseProgram(quad_program));

    // Debug
    GL_CHECK(glBindVertexArray(quad.get_vertex_array()));

    unsigned offset_x = 0;

    GL_CHECK(glBindTexture(GL_TEXTURE_2D, culling_implementations[culling_implementation_index]->get_depth_texture()));
    for (unsigned lod = 0; lod <= DEPTH_SIZE_LOG2; lod++)
    {
        GL_CHECK(glViewport(offset_x, 0, DEPTH_SIZE >> lod, DEPTH_SIZE >> lod));

        // Mipmapped filtering mode will ensure we draw correct miplevel.
        GL_CHECK(glDrawElements(GL_TRIANGLES, quad.get_num_elements(), GL_UNSIGNED_SHORT, 0));

        offset_x += DEPTH_SIZE >> lod;
    }
}

Scene::~Scene()
{
    delete box;

    for (unsigned i = 0; i < SPHERE_LODS; i++)
    {
        delete sphere[i];
    }

    for (unsigned i = 0; i < culling_implementations.size(); i++)
    {
        delete culling_implementations[i];
    }

    GL_CHECK(glDeleteBuffers(1, &occluder_instances_buffer));
    GL_CHECK(glDeleteBuffers(1, &sphere_instances_buffer));
    GL_CHECK(glDeleteProgram(occluder_program));
    GL_CHECK(glDeleteProgram(quad_program));
    GL_CHECK(glDeleteProgram(physics_program));
    GL_CHECK(glDeleteProgram(sphere_program));

    GL_CHECK(glDeleteBuffers(INDIRECT_BUFFERS, indirect.buffer));
    GL_CHECK(glDeleteBuffers(SPHERE_LODS, indirect.instance_buffer));
}

