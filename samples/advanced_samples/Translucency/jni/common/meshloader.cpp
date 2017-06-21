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

#include "meshloader.h"
#include "glutil.h"
#include "common.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
using std::stringstream;

/*
The .bin format is simply a memory dump of the model data
as it resides in the GPU. For instance, the teapot mesh has
3xPosition, 2xTexel, 3xNormal and has N vertices.
This gives N * 8 attributes, which is the first part of
the .bin file. The remaining data is the element index data.
*/
bool load_mesh_binary(Mesh &mesh, string path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;
    std::stringstream bindata;
    bindata << file.rdbuf();
    file.close();
    int attrib_count; bindata >> attrib_count;
    float *vertex_data = new float[attrib_count];
    for (int i = 0; i < attrib_count; i++)
        bindata >> vertex_data[i];
    int index_count; bindata >> index_count;
    uint32 *index_data = new uint32[index_count];
    for (int i = 0; i < index_count; i++)
        bindata >> index_data[i];

    mesh.vertex_buffer = gen_buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, attrib_count * sizeof(float), vertex_data);
    mesh.index_buffer = gen_buffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, index_count * sizeof(uint32), index_data);
    delete[] vertex_data;
    delete[] index_data;
    mesh.num_indices = index_count;
    return true;
}