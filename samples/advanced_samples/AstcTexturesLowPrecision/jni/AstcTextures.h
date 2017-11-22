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

#ifndef ASTC_TEXTURES_H
#define ASTC_TEXTURES_H

#include <android/log.h>
#include <GLES3/gl3.h>

#define LOG_TAG "libNative"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)

#define MALLOC_CHECK(ptr_type, ptr, size)                                        \
{                                                                                \
    ptr = (ptr_type) malloc(size);                                               \
    if (ptr == NULL)                                                             \
    {                                                                            \
        LOGF("Memory allocation error FILE: %s LINE: %i\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE);                                                      \
    }                                                                            \
}

#define REALLOC_CHECK(ptr_type, ptr, size)                                       \
{                                                                                \
    ptr = (ptr_type) realloc(ptr, size);                                         \
    if (ptr == NULL)                                                             \
    {                                                                            \
        LOGF("Memory allocation error FILE: %s LINE: %i\n", __FILE__, __LINE__); \
        exit(EXIT_FAILURE);                                                      \
    }                                                                            \
}

#define FREE_CHECK(ptr) \
{                       \
    free((void*) ptr);  \
    ptr = NULL;         \
}

#define GL_CHECK(x)                                                                             \
    x;                                                                                          \
    {                                                                                           \
        GLenum glError = glGetError();                                                          \
        if(glError != GL_NO_ERROR)                                                              \
        {                                                                                       \
            LOGE("glGetError() = %i (%#.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                                                 \
        }                                                                                       \
    }

/* ASTC texture compression internal formats. */
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR            (0x93B0)
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR            (0x93B1)
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR            (0x93B2)
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR            (0x93B3)
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR            (0x93B4)
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR            (0x93B5)
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR            (0x93B6)
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR            (0x93B7)
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR           (0x93B8)
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR           (0x93B9)
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR           (0x93BA)
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR          (0x93BB)
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR          (0x93BC)
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR          (0x93BD)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR    (0x93D0)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR    (0x93D1)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR    (0x93D2)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR    (0x93D3)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR    (0x93D4)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR    (0x93D5)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR    (0x93D6)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR    (0x93D7)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR   (0x93D8)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR   (0x93D9)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR   (0x93DA)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR  (0x93DB)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR  (0x93DC)
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR  (0x93DD)

/* EXT_texture_compression_astc_decode_mode */
#define GL_TEXTURE_ASTC_DECODE_PRECISION_EXT       (0x8F69)

/* Time period for each texture set to be displayed. */
#define ASTC_TEXTURE_SWITCH_INTERVAL               (5) /* sec */

/* Angular rates around several axes. */
#define X_ROTATION_SPEED                           (5)
#define Y_ROTATION_SPEED                           (4)
#define Z_ROTATION_SPEED                           (3)

/* Vertex shader source code. */
const char earth_vertex_shader_source[] =
{
    "#version 300 es\n"
    "in vec4 av4position;\n"
    "in vec3 vv3normal;\n"
    "in vec2 vv3tex2dcoord;\n"
    "uniform mat4 mv;\n"
    "uniform mat4 mvp;\n"
    "out vec2 tex2dcoord;\n"
    "out vec3 normal;\n"
    "out vec3 light;\n"
    "out vec3 view;\n"
    "void main() {\n"
    "    vec3 light_position = vec3(15.0, 0.0, 0.0);\n"
    "    vec4 P = mv * av4position;\n"
    "    normal = mat3(mv) * vv3normal;\n"
    "    light = light_position - P.xyz;\n"
    "    view  = -P.xyz;\n"
    "    tex2dcoord = vv3tex2dcoord;\n"
    "    gl_Position = mvp * av4position;\n"
    "}\n"
};

/* Fragment shader source code. */
const char earth_fragment_shader_source[] =
{
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D cloud_texture;\n"
    "uniform sampler2D daytime_texture;\n"
    "uniform sampler2D nighttime_texture;\n"
    "in vec2 tex2dcoord;\n"
    "in vec3 normal;\n"
    "in vec3 light;\n"
    "in vec3 view;\n"
    "out vec4 color;\n"
    "void main() {\n"
    "    vec3 diffuse_albedo  = vec3(2.0, 2.0, 3.0);\n"
    "    vec3 specular_albedo = vec3(0.1);\n"
    "    float specular_power = 16.0;\n"
    "    vec3 Normal = normalize(normal);\n"
    "    vec3 Light  = normalize(light);\n"
    "    vec3 View   = normalize(view);\n"
    "    vec3 reflected_light = reflect(-Light, Normal);\n"
    "    vec3 diffuse = max(dot(Normal, Light), 0.0) * diffuse_albedo;\n"
    "    vec3 specular = pow(max(dot(reflected_light, View), 0.0), specular_power) * specular_albedo;\n"
    "    vec2 clouds    = texture(cloud_texture,     tex2dcoord).rg;\n"
    "    vec3 daytime   = (texture(daytime_texture,  tex2dcoord).rgb * diffuse + specular * clouds.g) * (1.0 - clouds.r) + clouds.r * diffuse;\n"
    "    vec3 nighttime = texture(nighttime_texture, tex2dcoord).rgb * (1.0 - clouds.r) * 2.0;\n"
    "    color = vec4(mix(nighttime, daytime, 0.5), 1.0);\n"
    "}\n"
};

/**
 * \brief Create shader object and compile its source code.
 *
 * \param[in] shader_type   Vertex or fragment shader.
 * \param[in] shader_source Shader source code.
 * \return    Shader object ID.
 */
GLuint load_shader(GLenum shader_type, const char* shader_source);

/**
 * \brief Create program object, attach vertex and fragment shader to it.
 *        Link program object and check whether it has succeeded.
 *
 * \param[in] vertex_source   Vertex or fragment shader.
 * \param[in] fragment_source Shader source code.
 * \return    Program object ID.
 */
GLuint create_program(const char* vertex_source, const char* fragment_source);

/* ASTC header declaration. */
typedef struct
{
    unsigned char  magic[4];
    unsigned char  blockdim_x;
    unsigned char  blockdim_y;
    unsigned char  blockdim_z;
    unsigned char  xsize[3];   /* x-size = xsize[0] + xsize[1] + xsize[2] */
    unsigned char  ysize[3];   /* x-size, y-size and z-size are given in texels */
    unsigned char  zsize[3];   /* block count is inferred */
} astc_header;

/* Contains information about texture set bindings. */
typedef struct texture_set
{
    /* Bindings for each texture unit. */
    GLuint cloud_and_gloss_texture_id;
    GLuint earth_color_texture_id;
    GLuint earth_night_texture_id;

    /* Name of compression algorithm. */
    const char* name;
} texture_set;


/* Contains information about texture set files. */
typedef struct texture_set_info
{
    /* Texture internal format. */
    const GLenum compressed_data_internal_format;

    /* Paths to texture images for one texture set. */
    /* Also assign decode format for each texture. */
    const GLenum cloud_and_gloss_decode_format;
    const char* cloud_and_gloss_texture_file_path;

    const GLenum earth_color_decode_format;
    const char* earth_color_texture_file_path;

    const GLenum earth_night_decode_format;
    const char* earth_night_texture_file_path;

    /* Name of compression algorithm. */
    const char* compressed_texture_format_name;
} texture_set_info;

/**
 * \brief Invoke glGetAttribLocation(), if it has returned a positive value.
 *        Otherwise, print a message and exit. Function used for clarity reasons.
 *
 * \param[in] program     OpenGL ES specific.
 * \param[in] attrib_name OpenGL ES specific.
 */
GLint get_and_check_attrib_location(GLuint program, const GLchar* attrib_name);

/**
 * \brief Invoke glGetUniformLocation, if it has returned a positive value.
 *        Otherwise, print a message and exit. Function used for clarity reasons.
 *
 * \param[in] program      OpenGL ES specific.
 * \param[in] uniform_name OpenGL ES specific.
 */
GLint get_and_check_uniform_location(GLuint program, const GLchar* uniform_name);

#endif /* ASTC_TEXTURES_H */
