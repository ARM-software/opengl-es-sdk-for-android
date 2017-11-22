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

#include "SolidSphere.h"
#include "AstcTextures.h"

#include <cstdlib>
#include <cmath>

namespace AstcTextures
{
    /* Please see header for specification. */
    SolidSphere::SolidSphere(const float radius, const unsigned int rings, const unsigned int sectors)
    {
        unsigned int r = 0;
        unsigned int s = 0;

        /* Number of coordinates for vertex, texture and normal coordinates. */
        const unsigned int n_vertex_coordinates  = 3;
        const unsigned int n_texture_coordinates = 2;
        const unsigned int n_normal_coordinates  = 3;
        const unsigned int n_indices_per_vertex  = 6;

        const float R = 1.0f / (float)(rings - 1);
        const float S = 1.0f / (float)(sectors - 1);

        sphere_vertex_data_size = rings * sectors * n_vertex_coordinates  * sizeof(float);
        sphere_normal_data_size = rings * sectors * n_normal_coordinates  * sizeof(float);
        sphere_texcoords_size   = rings * sectors * n_texture_coordinates * sizeof(float);
        sphere_n_indices        = rings * sectors * n_indices_per_vertex;

        MALLOC_CHECK(float*,          sphere_vertices, sphere_vertex_data_size);
        MALLOC_CHECK(float*,          sphere_normals, sphere_normal_data_size);
        MALLOC_CHECK(float*,          sphere_texcoords, sphere_texcoords_size);
        MALLOC_CHECK(unsigned short*, sphere_indices, sphere_n_indices * sizeof(unsigned short));

        float*          vertices  = sphere_vertices;
        float*          normals   = sphere_normals;
        float*          texcoords = sphere_texcoords;
        unsigned short* indices   = sphere_indices;

        for (r = 0; r < rings; r++)
        {
            for (s = 0; s < sectors; s++)
            {
                const float x = sinf(M_PI * r * R) * cosf(2 * M_PI * s * S);
                const float y = sinf(-M_PI_2 + M_PI * r * R);
                const float z = sinf(2.0f * M_PI * s * S) * sinf(M_PI * r * R);

                *texcoords++ = s * S;
                *texcoords++ = r * R;

                *vertices++ = x * radius;
                *vertices++ = y * radius;
                *vertices++ = z * radius;

                *normals++ = x;
                *normals++ = y;
                *normals++ = z;
            }
        }

        for (r = 0; r < rings; r++)
        {
            for (s = 0; s < sectors; s++)
            {
                /* First triangle. */
                *indices++ = r       * sectors + s;
                *indices++ = r       * sectors + (s + 1);
                *indices++ = (r + 1) * sectors + s;

                /* Second triangle. */
                *indices++ = r       * sectors + (s + 1);
                *indices++ = (r + 1) * sectors + (s + 1);
                *indices++ = (r + 1) * sectors + s;
            }
        }
    }

    /* Please see header for specification. */
    float* SolidSphere::getSphereVertexData(int* vertex_data_size)
    {
        if (vertex_data_size == NULL)
        {
            LOGF("Memory error: vertex_data_size = NULL");
            exit(EXIT_FAILURE);
        }

        *vertex_data_size = sphere_vertex_data_size;

        return sphere_vertices;
    }

    /* Please see header for specification. */
    float* SolidSphere::getSphereNormalData(int* normal_data_size)
    {
        if (normal_data_size == NULL)
        {
            LOGF("Memory error: normal_data_size = NULL");
            exit(EXIT_FAILURE);
        }

        *normal_data_size = sphere_normal_data_size;

        return sphere_normals;
    }

    /* Please see header for specification. */
    float* SolidSphere::getSphereTexcoords(int* texcoords_size)
    {
        if (texcoords_size == NULL)
        {
            LOGF("Memory error: texcoords_size = NULL");
            exit(EXIT_FAILURE);
        }

        *texcoords_size = sphere_texcoords_size;

        return sphere_texcoords;
    }

    /* Please see header for specification. */
    unsigned short* SolidSphere::getSphereIndices(int* n_indices)
    {
        if (n_indices == NULL)
        {
            LOGF("Memory error: n_indices = NULL");
            exit(EXIT_FAILURE);
        }

        *n_indices = sphere_n_indices;

        return sphere_indices;
    }
}
