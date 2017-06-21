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

#include "common.hpp"
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;

static GLuint common_compile(GLenum type, const char *source)
{
    GL_CHECK(GLuint shader = glCreateShader(type));
    GL_CHECK(glShaderSource(shader, 1, &source, NULL));
    GL_CHECK(glCompileShader(shader));

    GLint status;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len));
        vector<char> buf(len);
        GL_CHECK(glGetShaderInfoLog(shader, len, &out_len, &buf[0]));
        LOGI("Shader log:\n%s", &buf[0]);

        GL_CHECK(glDeleteShader(shader));
        return 0;
    }

    return shader;
}

static bool check_program(GLuint prog)
{
    GLint status;
    GL_CHECK(glGetProgramiv(prog, GL_LINK_STATUS, &status));
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        GL_CHECK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len));
        vector<char> buf(len);
        GL_CHECK(glGetProgramInfoLog(prog, len, &out_len, &buf[0]));
        LOGI("Program log:\n%s", &buf[0]);

        GL_CHECK(glDeleteProgram(prog));
        return false;
    }

    return true;
}

GLuint common_compile_shader(const char *vs_source, const char *fs_source)
{
    GL_CHECK(GLuint prog = glCreateProgram());
    GLuint vs = common_compile(GL_VERTEX_SHADER, vs_source);
    if (!vs)
    {
        glDeleteProgram(prog);
        return 0;
    }

    GLuint fs = common_compile(GL_FRAGMENT_SHADER, fs_source);
    if (!fs)
    {
        GL_CHECK(glDeleteShader(vs));
        GL_CHECK(glDeleteProgram(prog));
        return 0;
    }

    GL_CHECK(glAttachShader(prog, vs));
    GL_CHECK(glAttachShader(prog, fs));
    GL_CHECK(glLinkProgram(prog));

    GL_CHECK(glDeleteShader(vs));
    GL_CHECK(glDeleteShader(fs));

    if (!check_program(prog))
    {
        LOGE("Failed to link program.");
        return 0;
    }

    return prog;
}

GLuint common_compile_shader(const char *vs_source,
        const char *tc_source, const char *te_source, const char *geom_source,
        const char *fs_source)
{
    GLuint prog = glCreateProgram();
    GLuint shaders[5] = {0};

    const char *sources[] = {
        vs_source,
        tc_source,
        te_source,
        geom_source,
        fs_source,
    };

    const GLenum stages[] = {
        GL_VERTEX_SHADER,
        GL_TESS_CONTROL_SHADER_EXT,
        GL_TESS_EVALUATION_SHADER_EXT,
        GL_GEOMETRY_SHADER_EXT,
        GL_FRAGMENT_SHADER,
    };

    for (unsigned i = 0; i < 5; i++)
    {
        if (sources[i])
        {
            shaders[i] = common_compile(stages[i], sources[i]);
            if (shaders[i] == 0)
            {
                goto error;
            }
        }
    }

    for (auto shader : shaders)
    {
        if (shader != 0)
        {
            GL_CHECK(glAttachShader(prog, shader));
        }
    }
    GL_CHECK(glLinkProgram(prog));

    for (auto shader : shaders)
    {
        if (shader != 0)
        {
            GL_CHECK(glDeleteShader(shader));
        }
    }

    if (!check_program(prog))
    {
        LOGE("Failed to link program.");
        return 0;
    }

    return prog;

error:
    for (auto shader : shaders)
    {
        if (shader != 0)
        {
            GL_CHECK(glDeleteShader(shader));
        }
    }
    GL_CHECK(glDeleteProgram(prog));
    return 0;
}

GLuint common_compile_compute_shader(const char *cs_source)
{
    GL_CHECK(GLuint prog = glCreateProgram());
    GLuint cs = common_compile(GL_COMPUTE_SHADER, cs_source);
    if (!cs)
    {
        GL_CHECK(glDeleteProgram(prog));
        return 0;
    }

    GL_CHECK(glAttachShader(prog, cs));
    GL_CHECK(glLinkProgram(prog));

    GL_CHECK(glDeleteShader(cs));

    if (!check_program(prog))
    {
        LOGE("Failed to link program.");
        return 0;
    }

    return prog;
}

bool common_read_file_string(const char *path, char **out_buf)
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
    if (!common_read_file_string(vs_source, &vs_buf))
    {
        return 0;
    }

    if (!common_read_file_string(fs_source, &fs_buf))
    {
        free(vs_buf);
        return 0;
    }

    GLuint prog = common_compile_shader(vs_buf, fs_buf);
    free(vs_buf);
    free(fs_buf);
    return prog;
}

GLuint common_compile_shader_from_file(const char *vs_source,
        const char *tc_source, const char *te_source, const char *geom_source,
        const char *fs_source)
{
    LOGI("Compiling shader: %s, %s, %s, %s, %s.",
            vs_source,
            tc_source ? tc_source : "none",
            te_source ? te_source : "none",
            geom_source ? geom_source : "none",
            fs_source);

    GLuint prog = 0;
    char *vs_buf = NULL;
    char *tc_buf = NULL;
    char *te_buf = NULL;
    char *geom_buf = NULL;
    char *fs_buf = NULL;

    if (vs_source && !common_read_file_string(vs_source, &vs_buf))
    {
        goto end;
    }

    if (tc_source && !common_read_file_string(tc_source, &tc_buf))
    {
        goto end;
    }

    if (te_source && !common_read_file_string(te_source, &te_buf))
    {
        goto end;
    }

    if (geom_source && !common_read_file_string(geom_source, &geom_buf))
    {
        goto end;
    }

    if (fs_source && !common_read_file_string(fs_source, &fs_buf))
    {
        goto end;
    }

    prog = common_compile_shader(vs_buf, tc_buf, te_buf, geom_buf, fs_buf);

end:
    free(vs_buf);
    free(tc_buf);
    free(te_buf);
    free(geom_buf);
    free(fs_buf);
    return prog;
}

GLuint common_compile_compute_shader_from_file(const char *cs_source)
{
    LOGI("Compiling compute shader from %s.", cs_source);
    char *cs_buf = NULL;
    if (!common_read_file_string(cs_source, &cs_buf))
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

