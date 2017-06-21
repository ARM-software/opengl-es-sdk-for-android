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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint load_texture(const char *filename)
{
    int width, height, channels;
    unsigned char *pixels = stbi_load(filename, &width, &height, &channels, 4);

    if (!pixels)
    {
        LOGE("Failed to load texture %s\n", filename);
        exit(1);
    }

    GLuint result = 0;
    glGenTextures(1, &result);
    glBindTexture(GL_TEXTURE_2D, result);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
    return result;
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        LOGE("Failed to open file %s\n", filename);
        exit (1);
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = (char *)calloc(length + 1, sizeof(char));
    if (!data)
    {
        LOGE("Failed to allocate memory for file data %s\n", filename);
        exit(1);
    }
    size_t read = fread(data, sizeof(char), length, file);
    if (read != length)
    {
        LOGE("Failed to read whole file %s\n", filename);
        exit(1);
    }
    data[length] = '\0';
    fclose(file);
    return data;
}

GLuint compile_shader(const char *source, GLenum type)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, (const GLchar**)&source, NULL);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = new GLchar[length];
        glGetShaderInfoLog(result, length, NULL, info);
        LOGE("[COMPILE] %s\n%s\n", source, info);
        delete[] info;
        exit(1);
    }
    return result;
}

GLuint link_program(GLuint *shaders, int count)
{
    GLuint program = glCreateProgram();
    for (int i = 0; i < count; ++i)
        glAttachShader(program, shaders[i]);

    glLinkProgram(program);

    for (int i = 0; i < count; ++i)
        glDetachShader(program, shaders[i]);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        GLchar *info = new GLchar[length];
        glGetProgramInfoLog(program, length, NULL, info);
        LOGE("[LINK] %s\n", info);
        delete[] info;
        exit(1);
    }
    return program;
}

void load_backdrop_shader(App *app)
{
    char *vs_src = read_file(SHADER_PATH("backdrop.vs"));
    char *fs_src = read_file(SHADER_PATH("backdrop.fs"));

    GLuint shaders[2];
    shaders[0] = compile_shader(vs_src, GL_VERTEX_SHADER);
    shaders[1] = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    app->program_backdrop = link_program(shaders, 2);

    LOGD("%s\n", vs_src);
    LOGD("%s\n", fs_src);

    free(vs_src);
    free(fs_src);
}

void load_geometry_shader(App *app)
{
    char *vs_src = read_file(SHADER_PATH("geometry.vs"));
    char *fs_src = read_file(SHADER_PATH("geometry.fs"));
    char *gs_src = read_file(SHADER_PATH("geometry.gs"));

    GLuint shaders[] = {
        compile_shader(vs_src, GL_VERTEX_SHADER),
        compile_shader(fs_src, GL_FRAGMENT_SHADER),
        compile_shader(gs_src, GL_GEOMETRY_SHADER)
    };
    app->program_geometry = link_program(shaders, 3);

    free(vs_src);
    free(fs_src);
    free(gs_src);
}

void load_centroid_shader(App *app)
{
    char *cs_src = read_file(SHADER_PATH("centroid.cs"));

    GLuint shaders[] = { compile_shader(cs_src, GL_COMPUTE_SHADER) };
    app->program_centroid = link_program(shaders, 1);

    free(cs_src);
}

void load_generate_shader(App *app)
{
    char *cs_src = read_file(SHADER_PATH("generate.cs"));

    GLuint shaders[] = { compile_shader(cs_src, GL_COMPUTE_SHADER) };
    app->program_generate = link_program(shaders, 1);

    free(cs_src);
}

void load_assets(App *app)
{
    load_geometry_shader(app);
    load_centroid_shader(app);
    load_generate_shader(app);
    load_backdrop_shader(app);

    app->tex_material = load_texture(TEXTURE_PATH("texture11.jpg"));
}
