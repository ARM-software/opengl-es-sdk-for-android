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

#ifndef SCENE_HPP__
#define SCENE_HPP__

#include "mesh.hpp"
#include "culling.hpp"
#include <vector>
#include <stdint.h>

class Scene
{
    public:
        Scene();
        ~Scene();

        void update(float delta_time, unsigned width, unsigned height);
        void render(unsigned width, unsigned height);
        void move_camera(float delta_x, float delta_y);

        enum CullingMethod {
            CullHiZ = 0,
            CullHiZNoLOD = 1,
            CullNone = -1
        };
        void set_culling_method(CullingMethod method);

        void set_physics_speed(float speed) { physics_speed = speed; }
        float get_physics_speed() const { return physics_speed; }
        void set_show_redundant(bool enable) { show_redundant = enable; }
        bool get_show_redundant() const { return show_redundant; }

    private:
        GLDrawable *box;
        GLDrawable *sphere[SPHERE_LODS];
        std::vector<CullingInterface*> culling_implementations;

        unsigned culling_implementation_index;

        bool show_redundant;
        bool enable_culling;

        void render_spheres(vec3 color_mod);

        void bake_occluder_geometry(std::vector<vec4> &occluder_positions,
                std::vector<uint32_t> &occluder_indices,
                const Mesh &box_mesh, const vec4 *instances, unsigned num_instances);

        GLuint occluder_program;
        GLuint sphere_program;

        GLDrawable quad;
        GLuint quad_program;

        // Allow for readbacks of atomic counter without stalling GPU pipeline.
#define INDIRECT_BUFFERS 4
        struct
        {
            GLuint buffer[INDIRECT_BUFFERS];
            unsigned buffer_index;
            GLuint instance_buffer[SPHERE_LODS];
        } indirect;

        void init_instances();
        GLuint physics_program;
        GLuint occluder_instances_buffer;
        GLuint sphere_instances_buffer;
        unsigned num_occluder_instances;
        unsigned num_sphere_render_lods;
        unsigned num_render_sphere_instances;

        void apply_physics(float delta_time);
        float physics_speed;

        void render_depth_map();

        mat4 projection;
        mat4 view;

        float camera_rotation_y;
        float camera_rotation_x;
        void update_camera(float rotation_y, float rotation_x, unsigned viewport_width, unsigned viewport_height);
};

#endif

