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

// This file holds functions for loading textures from images,
// as well as loading and compiling shader programs from file.
// You must define HEIGHTMAP_PATH, DIFFUSEMAP_PATH and
// SHADER_PATH that take in asset names and return the
// correct full asset path.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint load_packed_cubemap(const char *filename)
{
    int width, height, channels;
    unsigned char *pixels = stbi_load(filename, &width, &height, &channels, 4);
    if (!pixels)
    {
        LOGE("Failed to load texture %s\n", filename);
        exit(1);
    }

    if (width != height)
    {
        LOGE("Cubemap dimensions must match %s\n", filename);
        exit(1);
    }

    // The cubemap is layed out in the following format
    // Each face has dimensions (width / 4) x (height / 4)
    //  .  .  .  .
    //  . +Y  .  .
    // -X +Z +X -Z
    //  . -Y  .  .

    int s = width / 4;
    GLuint texture = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*2);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*2);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*2);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*3);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*1);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*3);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*2);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_SKIP_PIXELS, s*1);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, s*2);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, s, s, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    stbi_image_free(pixels);
    return texture;
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
        LOGE("[COMPILE] %s\n", info);
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

void load_mapping_shader(App *app)
{
    char *vs_src = read_file(SHADER_PATH("shader.vs"));
    char *fs_src = read_file(SHADER_PATH("shader.fs"));
    char *tc_src = read_file(SHADER_PATH("shader.tcs"));
    char *te_src = read_file(SHADER_PATH("shader.tes"));

    GLuint shaders[4];
    shaders[0] = compile_shader(vs_src, GL_VERTEX_SHADER);
    shaders[1] = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    shaders[2] = compile_shader(tc_src, GL_TESS_CONTROL_SHADER);
    shaders[3] = compile_shader(te_src, GL_TESS_EVALUATION_SHADER);
    app->program_mapping = link_program(shaders, 4);

    free(vs_src);
    free(fs_src);
    free(tc_src);
    free(te_src);
}

void load_backdrop_shader(App *app)
{
    char *vs_src = read_file(SHADER_PATH("backdrop.vs"));
    char *fs_src = read_file(SHADER_PATH("backdrop.fs"));

    GLuint shaders[2];
    shaders[0] = compile_shader(vs_src, GL_VERTEX_SHADER);
    shaders[1] = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    app->program_backdrop = link_program(shaders, 2);

    free(vs_src);
    free(fs_src);
}

void load_assets(App *app)
{
    load_mapping_shader(app);
    load_backdrop_shader(app);

    app->scenes[0].heightmap = load_packed_cubemap(HEIGHTMAP_PATH("magicmoon"));
    app->scenes[0].diffusemap = load_packed_cubemap(DIFFUSEMAP_PATH("magicmoon"));
    app->scenes[0].sun_dir = normalize(vec3(1.0f, 1.0f, -0.5f));
    app->scenes[0].use_mip = true;
    app->scenes[0].max_lod_coverage = 150.0f;
    app->scenes[0].height_scale = 0.2f;
    app->scenes[0].fov = PI / 4.0f;
    app->scenes[0].z_near = 0.1f;
    app->scenes[0].z_far = 16.0f;

    app->scenes[1].heightmap = load_packed_cubemap(HEIGHTMAP_PATH("swirly"));
    app->scenes[1].diffusemap = load_packed_cubemap(DIFFUSEMAP_PATH("swirly"));
    app->scenes[1].sun_dir = normalize(vec3(0.5f, 0.2f, -0.2f));
    app->scenes[1].use_mip = true;
    app->scenes[1].max_lod_coverage = 150.0f;
    app->scenes[1].height_scale = 0.2f;
    app->scenes[1].fov = PI / 4.0f;
    app->scenes[1].z_near = 0.1f;
    app->scenes[1].z_far = 16.0f;

    app->scenes[2].heightmap = load_packed_cubemap(HEIGHTMAP_PATH("voronoi_env"));
    app->scenes[2].diffusemap = load_packed_cubemap(DIFFUSEMAP_PATH("voronoi_env"));
    app->scenes[2].sun_dir = normalize(vec3(0.8f, 0.2f, -0.2f));
    app->scenes[2].use_mip = true;
    app->scenes[2].max_lod_coverage = 350.0f;
    app->scenes[2].height_scale = 0.2f;
    app->scenes[2].fov = PI / 4.0f;
    app->scenes[2].z_near = 0.1f;
    app->scenes[2].z_far = 16.0f;

    app->scenes[3].heightmap = load_packed_cubemap(HEIGHTMAP_PATH("voronoi_sharp"));
    app->scenes[3].diffusemap = load_packed_cubemap(DIFFUSEMAP_PATH("voronoi_sharp"));
    app->scenes[3].sun_dir = normalize(vec3(0.3f, 1.0f, 0.3f));
    app->scenes[3].use_mip = true;
    app->scenes[3].max_lod_coverage = 250.0f;
    app->scenes[3].height_scale = 0.2f;
    app->scenes[3].fov = PI / 4.0f;
    app->scenes[3].z_near = 0.1f;
    app->scenes[3].z_far = 16.0f;

    app->scenes[4].heightmap = load_packed_cubemap(HEIGHTMAP_PATH("wavey"));
    app->scenes[4].diffusemap = load_packed_cubemap(DIFFUSEMAP_PATH("wavey"));
    app->scenes[4].sun_dir = normalize(vec3(0.8f, 0.2f, -0.2f));
    app->scenes[4].use_mip = true;
    app->scenes[4].max_lod_coverage = 115.0f;
    app->scenes[4].height_scale = 0.2f;
    app->scenes[4].fov = PI / 4.0f;
    app->scenes[4].z_near = 0.1f;
    app->scenes[4].z_far = 16.0f;
}
