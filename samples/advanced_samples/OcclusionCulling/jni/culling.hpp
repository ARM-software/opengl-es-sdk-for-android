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

#ifndef CULLING_HPP__
#define CULLING_HPP__

#include "common.hpp"
#include "mesh.hpp"
#include <vector>
#include <stdint.h>
#include <stddef.h>
#define SPHERE_LODS 4

// Layout is defined by OpenGL ES 3.1.
// We don't care about the three last elements in this case.
struct IndirectCommand
{
    GLuint count;
    GLuint instanceCount;
    GLuint zero[3];
};

class CullingInterface
{
    public:
        virtual ~CullingInterface() {}

        // Sets up occlusion geometry. This is mostly static and should be done at startup of a scene.
        virtual void setup_occluder_geometry(const std::vector<vec4> &positions, const std::vector<uint32_t> &indices) = 0;

        // Sets current view and projection matrices.
        virtual void set_view_projection(const mat4 &projection, const mat4& view, const vec2 &zNearFar) = 0;

        // Rasterize occluders to depth map.
        virtual void rasterize_occluders() = 0;

        // Test bounding boxes in our scene.
        virtual void test_bounding_boxes(GLuint counter_buffer, const unsigned *counter_offsets, unsigned num_offsets,
                const GLuint *culled_instance_buffer, GLuint instance_data_buffer,
                unsigned num_instances) = 0;

        // Debugging functionality. Verify that the depth map is being rasterized correctly.
        virtual GLuint get_depth_texture() const { return 0; }

        virtual unsigned get_num_lods() const { return SPHERE_LODS; }

    protected:
        // Common functionality for various occlusion culling implementations.
        void compute_frustum_from_view_projection(vec4 *planes, const mat4 &view_projection);
};

#define DEPTH_SIZE 256
#define DEPTH_SIZE_LOG2 8
class HiZCulling : public CullingInterface
{
    public:
        HiZCulling();
        HiZCulling(const char *program);
        ~HiZCulling();

        void setup_occluder_geometry(const std::vector<vec4> &positions, const std::vector<uint32_t> &indices);
        void set_view_projection(const mat4 &projection, const mat4 &view, const vec2 &zNearFar);

        void rasterize_occluders();
        void test_bounding_boxes(GLuint counter_buffer, const unsigned *counter_offsets, unsigned num_offsets,
                const GLuint *culled_instance_buffer, GLuint instance_data_buffer,
                unsigned num_instances);

        GLuint get_depth_texture() const { return depth_texture; }

    private:
        GLuint depth_render_program;
        GLuint depth_mip_program;
        GLuint culling_program;

        GLDrawable quad;

        struct
        {
            GLuint vertex;
            GLuint index;
            GLuint vao;
            unsigned elements;
        } occluder;

        GLuint depth_texture;
        GLuint shadow_sampler;
        unsigned lod_levels;
        std::vector<GLuint> framebuffers;

        GLuint uniform_buffer;
        struct Uniforms
        {
            mat4 uVP;
            mat4 uView;
            mat4 uProj;
            vec4 planes[6];
            vec2 zNearFar;
        };
        Uniforms uniforms;

        void init();
};

// Variant of HiZRasterizer which only uses a single LOD.
class HiZCullingNoLOD : public HiZCulling
{
    public:
        HiZCullingNoLOD()
            : HiZCulling("hiz_cull_no_lod.cs")
        {}
        unsigned get_num_lods() const { return 1; }
};

#endif

