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

#include "common.hpp"
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;

static GLuint common_compile(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        vector<char> buf(len);
        glGetShaderInfoLog(shader, len, &out_len, &buf[0]);
        LOGI("Shader log:\n%s", &buf[0]);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static bool check_program(GLuint prog)
{
    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        vector<char> buf(len);
        glGetProgramInfoLog(prog, len, &out_len, &buf[0]);
        LOGI("Program log:\n%s", &buf[0]);

        glDeleteProgram(prog);
        return false;
    }

    return true;
}

GLuint common_compile_shader(const char *vs_source, const char *fs_source)
{
    GLuint prog = glCreateProgram();
    GLuint vs = common_compile(GL_VERTEX_SHADER, vs_source);
    if (!vs)
    {
        glDeleteProgram(prog);
        return 0;
    }

    GLuint fs = common_compile(GL_FRAGMENT_SHADER, fs_source);
    if (!fs)
    {
        glDeleteShader(vs);
        glDeleteProgram(prog);
        return 0;
    }

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!check_program(prog))
    {
        LOGE("Failed to link program.");
        return 0;
    }

    return prog;
}

GLuint common_compile_compute_shader(const char *cs_source)
{
    GLuint prog = glCreateProgram();
    GLuint cs = common_compile(GL_COMPUTE_SHADER, cs_source);
    if (!cs)
    {
        glDeleteProgram(prog);
        return 0;
    }

    glAttachShader(prog, cs);
    glLinkProgram(prog);

    glDeleteShader(cs);

    if (!check_program(prog))
    {
        LOGE("Failed to link program.");
        return 0;
    }

    return prog;
}

static bool read_file_string(const char *path, char **out_buf)
{
    FILE *file = common_fopen(path, "rb");
    if (!file)
    {
        LOGE("Failed to open file: %s.", path);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);

    char *buf = (char*)malloc(len + 1);
    if (!buf)
    {
        fclose(file);
        return false;
    }

    long ret = fread(buf, 1, len, file);
    fclose(file);

    if (ret == len)
    {
        buf[len] = '\0';
        *out_buf = buf;
        return true;
    }
    else
    {
        free(buf);
        *out_buf = NULL;
        return false;
    }
}

GLuint common_compile_shader_from_file(const char *vs_source, const char *fs_source)
{
    LOGI("Compiling vertex/fragment shader: %s, %s.", vs_source, fs_source);
    char *vs_buf = NULL;
    char *fs_buf = NULL;
    if (!read_file_string(vs_source, &vs_buf))
    {
        return 0;
    }

    if (!read_file_string(fs_source, &fs_buf))
    {
        free(vs_buf);
        return 0;
    }

    GLuint prog = common_compile_shader(vs_buf, fs_buf);
    free(vs_buf);
    free(fs_buf);
    return prog;
}

GLuint common_compile_compute_shader_from_file(const char *cs_source)
{
    LOGI("Compiling compute shader from %s.", cs_source);
    char *cs_buf = NULL;
    if (!read_file_string(cs_source, &cs_buf))
    {
        return 0;
    }

    GLuint prog = common_compile_compute_shader(cs_buf);
    free(cs_buf);
    return prog;
}

static string common_basedir;
void common_set_basedir(const char *basedir)
{
    common_basedir = basedir;
}

string common_get_path(const char *basepath)
{
    if (!common_basedir.empty())
    {
        return common_basedir + "/" + basepath;
    }
    else
    {
        return basepath;
    }
}

FILE *common_fopen(const char *path, const char *mode)
{
    string join_path = common_get_path(path);
    FILE *ret = fopen(join_path.c_str(), mode);
    LOGI("Opening: %s (%s).", join_path.c_str(), ret ? "success" : "failure");
    return ret;
}

