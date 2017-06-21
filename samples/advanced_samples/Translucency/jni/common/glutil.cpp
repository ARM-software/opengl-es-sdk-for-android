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

#include "glutil.h"
#include <fstream>
#include <iostream>

Shader current;

void cull(bool enabled, GLenum front, GLenum mode)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glFrontFace(front);
        glCullFace(mode);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void depth_test(bool enabled, GLenum func)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(func);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void depth_write(bool enabled)
{
    if (enabled)
    {
        glDepthMask(GL_TRUE);
        glDepthRangef(0.0f, 1.0f);
    }
    else
    {
        glDepthMask(GL_FALSE);
    }
}

void blend_mode(bool enabled, GLenum src, GLenum dest, GLenum func)
{
    if (enabled)
    {
        glEnable(GL_BLEND);
        glBlendFunc(src, dest);
        glBlendEquation(func);
    }
    else
    {
        glDisable(GL_BLEND);
    }
}

void use_shader(Shader shader)
{
    current = shader;
    current.use();
}

void attribfv(string name, GLsizei num_components, GLsizei stride, GLsizei offset)
{ 
    current.set_attribfv(name, num_components, stride, offset); 
}

void unset_attrib(string name)
{
    current.unset_attrib(name);
}

void uniform(string name, const mat4 &v) { current.set_uniform(name, v); }
void uniform(string name, const vec4 &v) { current.set_uniform(name, v); }
void uniform(string name, const vec3 &v) { current.set_uniform(name, v); }
void uniform(string name, const vec2 &v) { current.set_uniform(name, v); }
void uniform(string name, double v) { current.set_uniform(name, v); }
void uniform(string name, float v) { current.set_uniform(name, v); }
void uniform(string name, int v) { current.set_uniform(name, v); }
void uniform(string name, unsigned int v) { current.set_uniform(name, v); }

bool read_file(const std::string &path, std::string &dest)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);

    std::string msg = "Failed to open file " + path;
    ASSERT(in.is_open() && in.good(), msg.c_str());
    
    in.seekg(0, std::ios::end);         // Set get position to end
    dest.resize(in.tellg());            // Resize string to support enough bytes
    in.seekg(0, std::ios::beg);         // Set get position to beginning
    in.read(&dest[0], dest.size());     // Read file to string
    in.close();

    return true;
}

GLuint gen_buffer(GLenum target, GLenum usage, GLsizei size, const void *data)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, size, data, usage);
    glBindBuffer(target, 0);
    return buffer;
}

GLuint gen_buffer(GLenum target, GLsizei size, const void *data)
{
    return gen_buffer(target, GL_STATIC_DRAW, size, data);
}

void del_buffer(GLuint buffer)
{
    glDeleteBuffers(1, &buffer);
}