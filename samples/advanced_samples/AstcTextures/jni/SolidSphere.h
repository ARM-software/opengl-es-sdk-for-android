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

#ifndef SOLID_SPHERE_H
#define SOLID_SPHERE_H

namespace AstcTextures
{
    class SolidSphere
    {
        public:
            /** \brief Solid sphere constructor. It generates vertex position, normals
             *         and texture coordinates data based on user-provided arguments.
             *
             *  \param[in] radius  radius of sphere.
             *  \param[in] rings   number of parallels sphere consists of.
             *  \param[in] sectors number of meridians sphere consists of.
             */
            SolidSphere(const float radius, const unsigned int rings, const unsigned int sectors);

            /** \brief Returns sphere vertices.
             *
             *  \param[out] vertex_data_size Size of vertex data.
             *  \return     Pointer to sphere vertex coordinates.
             */
            float* getSphereVertexData(int* vertex_data_size);

            /** \brief Returns normal coordinates.
             *
             *  \param[out] normal_data_size Size of normal data.
             *  \return     Pointer to sphere normal coordinates.
             */
            float* getSphereNormalData(int* normal_data_size);

            /** \brief Returns texture coordinates.
             *
             *  \param[out] texcoords_size Size of texture coordinates.
             *  \return     Pointer to sphere texture coordinates.
             */
            float* getSphereTexcoords (int* texcoords_size);

            /** \brief Returns sphere indices.
             *
             *  \param[out] n_indices Number of indices.
             *  \return     Pointer to indices.
             */
            unsigned short* getSphereIndices(int* n_indices);

        private:

            /* Pointers to mesh data. */
            float* sphere_vertices;
            float* sphere_normals;
            float* sphere_texcoords;

            /* Indices that determine how to construct a sequence of primitives for glDrawElements call. */
            unsigned short* sphere_indices;

            /* Sizes of mesh data. */
            int sphere_vertex_data_size;
            int sphere_normal_data_size;
            int sphere_texcoords_size;

            /* Number of indices. */
            int sphere_n_indices;
    };
}

#endif /* SOLID_SPHERE_H */
