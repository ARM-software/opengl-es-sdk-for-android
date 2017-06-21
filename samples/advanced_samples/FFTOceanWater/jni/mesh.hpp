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

#ifndef MESH_HPP__
#define MESH_HPP__

#include "common.hpp"
#include "vector_math.h"
#include <vector>

class Mesh
{
    public:
        struct RenderInfo
        {
            // MVP for rendering.
            mat4 mvp;

            // Frustum planes for culling.
            vec4 frustum[6];

            // Texture size of heightmap.
            uvec2 fft_size;

            // The world space size for heightmap tiles.
            vec2 tile_extent;

            // The frequency scale of high-frequency normal-map.
            vec2 normal_scale;

            // Camera position.
            vec3 cam_pos;

            // Height and displacement texture.
            GLuint height_displacement;

            // Gradient and jacobian texture.
            GLuint gradient_jacobian;

            // Normalmap texture.
            GLuint normal;

            // Skydome for reflection and fog.
            GLuint skydome;

            // Viewport
            unsigned vp_width;
            unsigned vp_height;

            // Downsampling factor displacement map.
            unsigned displacement_downsample;
        };

        virtual ~Mesh();
        virtual void render(const RenderInfo &info) = 0;

    protected:
        Mesh(const char *vs_shader, const char *fs_shader);
        Mesh(const char *vs_shader, const char *tc_shader, const char *te_shader,
                const char *geom_shader, const char *fs_shader);
        void bind_textures(const RenderInfo &info);
        GLuint prog = 0;
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ibo = 0;
};

class MorphedGeoMipMapMesh : public Mesh
{
    public:
        MorphedGeoMipMapMesh();
        ~MorphedGeoMipMapMesh();

        MorphedGeoMipMapMesh(MorphedGeoMipMapMesh&&) = delete;
        MorphedGeoMipMapMesh& operator=(MorphedGeoMipMapMesh&&) = delete;

        void render(const RenderInfo &info) override;

    private:
        void build_lod(unsigned lod);
        void build_patches();
        void init_lod_tex();
        void init();

        void calculate_lods(const RenderInfo &info);

        // Per-instance data.
        struct PatchData
        {
            // .xy = World space offset for patch. .zw = Local offset in heightmap grid for purposes of sampling lod texture.
            vec4 Offsets;
            // LOD factors for left, top, right and bottom edges.
            vec4 LODs;
            // .x = Inner LOD.
            vec4 InnerLOD;
            // Padding to align on 64-byte cacheline.
            vec4 Padding;
        };

        struct LODMesh
        {
            LODMesh() = default;
            LODMesh(unsigned offset, unsigned elems)
                : offset(offset), elems(elems)
            {}

            // Offset in index buffer.
            unsigned offset;
            // Number of indices.
            unsigned elems;
            // Number of instances to draw this mesh.
            unsigned instances;
            void draw(GLuint ubo, unsigned ubo_offset);
        };

        struct LOD
        {
            unsigned full_vbo;
            LODMesh full;
        };

        struct Patch
        {
            vec2 pos;
            float lod;
            bool visible;
        };

        std::vector<LOD> lod_meshes;
        std::vector<Patch> patches;
        GLuint ubo;
        GLuint pbo;

        static constexpr unsigned patch_size = 64;
        // Do not use lowest "quad" LOD since it forces popping when switching between lod 5 and 6.
        static constexpr unsigned lods = 6;
        static constexpr float lod0_distance = 50.0f;
        static constexpr unsigned blocks_x = 32;
        static constexpr unsigned blocks_z = 32;

        // 16KiB UBO limit. If we have more instances, split a LOD in more draw calls.
        static constexpr unsigned max_instances = (16 * 1024) / sizeof(PatchData);

        GLuint lod_tex;
        std::vector<float> lod_buffer;

        struct Vertex
        {
            Vertex(GLubyte x, GLubyte y) : x(x), y(y) {}
            // Local offset in the patch.
            GLubyte x, y;
            // Rounding factors. If less than patch_size / 2, this is 1, otherwise 0.
            GLubyte rounding_x, rounding_y;
            // Lod weights to select correct LOD in vertex shader.
            GLubyte lod_weight[4];
        };

        std::vector<Vertex> vertices;
        std::vector<GLushort> indices;
};

class TessellatedMesh : public Mesh
{
    public:
        TessellatedMesh();
        void render(const RenderInfo &info) override;

    private:
        void init_vao();
        unsigned num_vertices;

        static constexpr float patch_size = 32.0f;
        static constexpr float lod0_distance = 50.0f;

        static constexpr unsigned blocks_x = 64;
        static constexpr unsigned blocks_z = 64;
};

// Simple bounding sphere implementation for MorphedGeoMipMapMesh.
struct BoundingSphere
{
    BoundingSphere(vec3 center, vec3 radius)
    {
        this->center = vec4(center, 1.0f);
        this->radius = vec_length(radius);
    }

    bool test_frustum(const vec4 *frustum) const;

    vec4 center;
    float radius;
};

#endif

