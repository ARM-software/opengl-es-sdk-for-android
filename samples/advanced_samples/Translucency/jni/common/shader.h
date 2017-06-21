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

#ifndef SHADER_H
#define SHADER_H
#include "matrix.h"
#include "common.h"
#include <unordered_map>

class Shader
{
public:
    Shader();

    bool load_from_src(const string *sources, GLenum *types, int count);
    bool load_from_src(string vs_src, string fs_src);
    bool load_from_file(const string *paths, GLenum *types, int count);
    bool load_from_file(string vs_path, string fs_path);
    bool link();
    void dispose();

    void use();
    void unuse();

    GLint get_uniform_location(string name);
    GLint get_attribute_location(string name);

    void set_attribfv(string name, GLsizei num_components, GLsizei stride, GLsizei offset);
    void unset_attrib(string name);

    void set_uniform(string name, const mat4 &v);
    void set_uniform(string name, const vec4 &v);
    void set_uniform(string name, const vec3 &v);
    void set_uniform(string name, const vec2 &v);
    void set_uniform(string name, double v);
    void set_uniform(string name, float v);
    void set_uniform(string name, int v);
    void set_uniform(string name, unsigned int v);
private:
    std::unordered_map<string, GLint> m_attributes;
    std::unordered_map<string, GLint> m_uniforms;
    GLuint m_id;
    std::vector<GLuint> m_shaders;
};

#endif