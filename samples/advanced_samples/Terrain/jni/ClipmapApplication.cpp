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

#include "ClipmapApplication.h"
#include "shaders.h"
#include "Platform.h"
#include <cstdio>

using namespace MaliSDK;
using namespace std;

ClipmapApplication::ClipmapApplication(unsigned int size, unsigned int levels, float clip_scale)
    : mesh(size, levels, clip_scale), heightmap(size * 4 - 1, levels), frame(0)
{
    // Compile shaders and grab uniform locations for later use.
    program = compile_program(vertex_shader_source, fragment_shader_source);
    GL_CHECK(glUseProgram(program));
    GL_CHECK(glUniformBlockBinding(program, glGetUniformBlockIndex(program, "InstanceData"), 0));

    GL_CHECK(mvp_loc = glGetUniformLocation(program, "uViewProjection"));
    GL_CHECK(camera_pos_loc = glGetUniformLocation(program, "uCameraPos"));

    GL_CHECK(GLint heightmap_loc = glGetUniformLocation(program, "sHeightmap"));
    GL_CHECK(glUniform1i(heightmap_loc, 0));

    vector<GLfloat> inv_level_size;
    float inv_size = 1.0f / (clip_scale * (size * 4 - 1));
    for (unsigned int i = 0; i < levels; i++)
    {
        inv_level_size.push_back(inv_size);
        inv_size *= 0.5f;
    }

    GL_CHECK(GLint inv_level_size_loc = glGetUniformLocation(program, "uInvLevelSize"));
    GL_CHECK(glUniform1fv(inv_level_size_loc, inv_level_size.size(), &inv_level_size[0]));
    GL_CHECK(glUseProgram(0));
}

ClipmapApplication::~ClipmapApplication()
{
    GL_CHECK(glDeleteProgram(program));
}

GLuint ClipmapApplication::compile_program(const char *vertex_shader, const char *fragment_shader)
{
    GL_CHECK(GLuint prog = glCreateProgram());
    GL_CHECK(GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_shader));
    GL_CHECK(GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_shader));

    GL_CHECK(glAttachShader(prog, vertex));
    GL_CHECK(glAttachShader(prog, fragment));
    GL_CHECK(glLinkProgram(prog));

    GLint status = 0;
    GL_CHECK(glGetProgramiv(prog, GL_LINK_STATUS, &status));
    if (!status)
    {
        GLint info_len = 0;
        GL_CHECK(glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &info_len));
        if (info_len)
        {
            char *buffer = new char[info_len];
            GLint actual_len;
            GL_CHECK(glGetProgramInfoLog(prog, info_len, &actual_len, buffer));
            LOGE("Program failed to link: %s.\n", buffer);
            delete[] buffer;
        }
    }

    // Don't need these anymore.
    GL_CHECK(glDeleteShader(vertex));
    GL_CHECK(glDeleteShader(fragment));
    return prog;
}

GLuint ClipmapApplication::compile_shader(GLenum type, const char *source)
{
    GL_CHECK(GLuint shader = glCreateShader(type));
    GL_CHECK(glShaderSource(shader, 1, &source, NULL));
    GL_CHECK(glCompileShader(shader));

    GLint status = 0;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));

    if (!status)
    {
        GLint info_len = 0;
        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len));
        if (info_len)
        {
            char *buffer = new char[info_len];
            GLint actual_len;
            GL_CHECK(glGetShaderInfoLog(shader, info_len, &actual_len, buffer));

            LOGE("Shader error: %s.\n", buffer);
            delete[] buffer;
        }
    }

    return shader;
}

void ClipmapApplication::render(unsigned int width, unsigned int height)
{
    GL_CHECK(glClearColor(0.5f, 0.5f, 0.5f, 1.0f));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glViewport(0, 0, width, height));

    // Rebind program every frame for clarity.
    GL_CHECK(glUseProgram(program));

    // Non-interactive camera that just moves in one direction.
    frame++;
    vec2 camera_pos = vec2(frame) * vec2(0.5f, 1.0f);
    vec3 world_camera_pos = vec3(camera_pos.c.x, 20, camera_pos.c.y);

    mat4 view = mat_look_at(world_camera_pos, world_camera_pos + vec3(1.0f, -0.2f, 2.0f), vec3(0, 1, 0));
    mat4 proj = mat_perspective_fov(45.0f, float(width) / float(height), 1.0f, 1000.0f);
    mat4 vp = proj * view;

    GL_CHECK(glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, vp.data));

    // Used for frustum culling.
    mesh.set_frustum(Frustum(vp));

    // The clipmap moves along with the camera.
    mesh.update_level_offsets(camera_pos);

    GL_CHECK(glUniform3fv(camera_pos_loc, 1, world_camera_pos.data));

    // As we move around, the heightmap textures are updated incrementally, allowing for an "endless" terrain.
    heightmap.update_heightmap(mesh.get_level_offsets());

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, heightmap.get_texture()));
    mesh.render();

    GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
}
