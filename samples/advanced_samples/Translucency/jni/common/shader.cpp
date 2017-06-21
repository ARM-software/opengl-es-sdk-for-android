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

#include "shader.h"
#include "glutil.h"

Shader::Shader() : m_id(0), m_attributes(), m_uniforms(), m_shaders() 
{
    m_shaders.clear(); 
}

bool compile_shader(GLuint shader, GLenum type, const char *source)
{
    const char *src = source;
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = new GLchar[length];
        glGetShaderInfoLog(shader, length, NULL, info);
        LOGE("Error compiling shader: %s\n", info);
        delete[] info;
        return false;
    }
    return true;
}

bool link_program(GLuint program, std::vector<GLuint> shaders)
{
    for (unsigned int i = 0; i < shaders.size(); ++i)
        glAttachShader(program, shaders[i]);

    glLinkProgram(program);

    for (unsigned int i = 0; i < shaders.size(); ++i)
        glDetachShader(program, shaders[i]);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = new GLchar[length];
        glGetProgramInfoLog(program, length, NULL, info);
        LOGE("Error linking program: %s\n", info);
        delete[] info;
        return false;
    }
    return true;
}

bool Shader::load_from_src(const string *sources, GLenum *types, int count)
{
    m_id = glCreateProgram();
    m_shaders.clear();
    for (int i = 0; i < count; ++i)
    {
        GLuint shader = glCreateShader(types[i]);
        if (!compile_shader(shader, types[i], sources[i].c_str()))
            return false;
        m_shaders.push_back(shader);
    }
    return true;
}

bool Shader::load_from_src(string vs_src, string fs_src)
{
    string sources[] = { vs_src, fs_src };
    GLenum types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    return load_from_src(sources, types, 2);
}

bool Shader::load_from_file(const string *paths, GLenum *types, int count)
{
    m_id = glCreateProgram();
    m_shaders.clear();
    for (int i = 0; i < count; ++i)
    {
        GLuint shader = glCreateShader(types[i]);
        string src;
        if (!read_file(paths[i], src))
            return false;
        if (!compile_shader(shader, types[i], src.c_str()))
            return false;
        m_shaders.push_back(shader);
    }
    return true;
}

bool Shader::load_from_file(string vs_path, string fs_path)
{
    string paths[] = { vs_path, fs_path };
    GLenum types[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    return load_from_file(paths, types, 2);
}

bool Shader::link()
{
    return link_program(m_id, m_shaders);
}

void Shader::dispose()
{
    m_attributes.clear();
    m_uniforms.clear();
    for (int i = 0; i < m_shaders.size(); ++i)
        glDeleteShader(m_shaders[i]);
    glDeleteProgram(m_id);
}

void Shader::use()
{
    glUseProgram(m_id);
}

void Shader::unuse()
{
    glUseProgram(0);
}

GLint Shader::get_uniform_location(string name)
{
    std::unordered_map<std::string, GLint>::iterator it = m_uniforms.find(name);
    if(it != m_uniforms.end())
    {
        return it->second;
    }
    else
    {
        GLint location = glGetUniformLocation(m_id, name.c_str());
        string msg = "Invalid shader uniform [" + name + "]";
        ASSERT(location >= 0, msg.c_str());
        m_uniforms[name] = location;
        return location;
    }
}

GLint Shader::get_attribute_location(string name)
{
    std::unordered_map<std::string, GLint>::iterator it = m_attributes.find(name);
    if(it != m_attributes.end())
    {
        return it->second;
    }
    else
    {
        GLint location = glGetAttribLocation(m_id, name.c_str());
        string msg = "Invalid shader attribute [" + name + "]";
        ASSERT(location >= 0, msg.c_str());
        m_attributes[name] = location;
        return location;
    }
}

void Shader::set_attribfv(string name, GLsizei num_components, 
                          GLsizei stride, GLsizei offset)
{
    GLint loc = get_attribute_location(name);
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(
        loc, 
        num_components, // in the attribute,
        GL_FLOAT, // each component is a float,
        GL_FALSE, // not normalized,
        stride * sizeof(GLfloat), // the attribs are seperated by this much,
        (void*)(offset * sizeof(GLfloat)) // beginning at this offset in the array.
        );
}

void Shader::unset_attrib(string name)
{
    glDisableVertexAttribArray(get_attribute_location(name));
}

void Shader::set_uniform(string name, const mat4 &v) { glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, v.value_ptr()); }
void Shader::set_uniform(string name, const vec4 &v) { glUniform4f(get_uniform_location(name), v.x, v.y, v.z, v.w); }
void Shader::set_uniform(string name, const vec3 &v) { glUniform3f(get_uniform_location(name), v.x, v.y, v.z); }
void Shader::set_uniform(string name, const vec2 &v) { glUniform2f(get_uniform_location(name), v.x, v.y); }
void Shader::set_uniform(string name, double v) { glUniform1f(get_uniform_location(name), v); }
void Shader::set_uniform(string name, float v) { glUniform1f(get_uniform_location(name), v); }
void Shader::set_uniform(string name, int v) { glUniform1i(get_uniform_location(name), v); }
void Shader::set_uniform(string name, unsigned int v) { glUniform1ui(get_uniform_location(name), v); }