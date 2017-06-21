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

#include <jni.h>
#include <GLES3/gl3.h>

#include <cstdlib>
#include <cmath>
#include <string>

#include "Text.h"
#include "AstcTextures.h"
#include "Timer.h"
#include "SolidSphere.h"

using namespace AstcTextures;
using namespace std;

/* Instance of Timer used to measure texture switch time. */
Timer timer;

/* Instance of Timer used to determine current time. */
Timer fps_timer;

/* Instance of text renderer used to display name of the compressed texture internalformat,
   in which textures used for rendering are stored. */
Text* text_displayer = NULL;

/* Instance of SolidSphere which provides mesh data for the globe. */
SolidSphere* solid_sphere = NULL;

/* Place where all asset files are located. */
const string resource_directory("/data/data/com.arm.malideveloper.openglessdk.astctextures/files/");

/* Window resolution. */
int window_width  = 0;
int window_height = 0;

/* Field of view in y-direction set up to 60 deg., expressed in radians. */
const float field_of_view = M_PI * 60.0f / 180.0f;

/* Aspect ratio of perspective frustrum. */
float x_to_y_ratio = 0;

/* Distances between camera and near/far plane of clipping frustrum. */
const float z_near = 0.01f;
const float z_far  = 100.0f;

/* Sampler locations. */
GLint cloud_texture_location     = 0;
GLint daytime_texture_location   = 0;
GLint nighttime_texture_location = 0;

/* Uniform locations. */
GLint mv_location  = 0;
GLint mvp_location = 0;

/* Attribute locations. */
GLint normal_location         = 0;
GLint position_location       = 0;
GLint texture_coords_location = 0;

/* Buffer object ID. */
GLuint bo_id = 0;

/* Vertex and fragment shader IDs. */
GLuint vert_shader_id = 0;
GLuint frag_shader_id = 0;

/* Program object ID. */
GLuint program_id = 0;

/* Vertex array object ID. */
GLuint vao_id = 0;

/* Initial angles around x, y and z axes. */
float angle_x = 0;
float angle_y = 0;
float angle_z = 0;

/* Time value for rotation and translation calculations. */
float current_time = 0;

/* Model-view transform matrix. */
Matrix model_view_matrix;

/* Model-view-perspective transform matrix. */
Matrix mvp_matrix;

/* Perspective projection matrix. */
Matrix perspective_matrix;

/* Rotation matrix. */
Matrix rotate_matrix;

/* Indicates which texture set is to be bound to texture units. */
unsigned int current_texture_set_id = 0;

int sphere_indices_size = 0;
unsigned short* sphere_indices = NULL;

/* Array containing information about all texture sets. */
texture_set_info texture_sets_info[] =
{
    /* Compression type.                        Path to cloud texture.       Path to day texture.       Path to night texture.     Name of compression algorithm. */
    GL_COMPRESSED_RGBA_ASTC_4x4_KHR,           "CloudAndGloss4x4.astc",     "Earth-Color4x4.astc",     "Earth-Night4x4.astc",     "4x4 ASTC",
    GL_COMPRESSED_RGBA_ASTC_5x4_KHR,           "CloudAndGloss5x4.astc",     "Earth-Color5x4.astc",     "Earth-Night5x4.astc",     "5x4 ASTC",
    GL_COMPRESSED_RGBA_ASTC_5x5_KHR,           "CloudAndGloss5x5.astc",     "Earth-Color5x5.astc",     "Earth-Night5x5.astc",     "5x5 ASTC",
    GL_COMPRESSED_RGBA_ASTC_6x5_KHR,           "CloudAndGloss6x5.astc",     "Earth-Color6x5.astc",     "Earth-Night6x5.astc",     "6x5 ASTC",
    GL_COMPRESSED_RGBA_ASTC_6x6_KHR,           "CloudAndGloss6x6.astc",     "Earth-Color6x6.astc",     "Earth-Night6x6.astc",     "6x6 ASTC",
    GL_COMPRESSED_RGBA_ASTC_8x5_KHR,           "CloudAndGloss8x5.astc",     "Earth-Color8x5.astc",     "Earth-Night8x5.astc",     "8x5 ASTC",
    GL_COMPRESSED_RGBA_ASTC_8x6_KHR,           "CloudAndGloss8x6.astc",     "Earth-Color8x6.astc",     "Earth-Night8x6.astc",     "8x6 ASTC",
    GL_COMPRESSED_RGBA_ASTC_8x8_KHR,           "CloudAndGloss8x8.astc",     "Earth-Color8x8.astc",     "Earth-Night8x8.astc",     "8x8 ASTC",
    GL_COMPRESSED_RGBA_ASTC_10x5_KHR,          "CloudAndGloss10x5.astc",    "Earth-Color10x5.astc",    "Earth-Night10x5.astc",    "10x5 ASTC",
    GL_COMPRESSED_RGBA_ASTC_10x6_KHR,          "CloudAndGloss10x6.astc",    "Earth-Color10x6.astc",    "Earth-Night10x6.astc",    "10x6 ASTC",
    GL_COMPRESSED_RGBA_ASTC_10x8_KHR,          "CloudAndGloss10x8.astc",    "Earth-Color10x8.astc",    "Earth-Night10x8.astc",    "10x8 ASTC",
    GL_COMPRESSED_RGBA_ASTC_10x10_KHR,         "CloudAndGloss10x10.astc",   "Earth-Color10x10.astc",   "Earth-Night10x10.astc",   "10x10 ASTC",
    GL_COMPRESSED_RGBA_ASTC_12x10_KHR,         "CloudAndGloss12x10.astc",   "Earth-Color12x10.astc",   "Earth-Night12x10.astc",   "12x10 ASTC",
    GL_COMPRESSED_RGBA_ASTC_12x12_KHR,         "CloudAndGloss12x12.astc",   "Earth-Color12x12.astc",   "Earth-Night12x12.astc",   "12x12 ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR,   "CloudAndGloss4x4.astc",     "Earth-Color4x4.astc",     "Earth-Night4x4.astc",     "4x4 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR,   "CloudAndGloss5x4.astc",     "Earth-Color5x4.astc",     "Earth-Night5x4.astc",     "5x4 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR,   "CloudAndGloss5x5.astc",     "Earth-Color5x5.astc",     "Earth-Night5x5.astc",     "5x5 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR,   "CloudAndGloss6x5.astc",     "Earth-Color6x5.astc",     "Earth-Night6x5.astc",     "6x5 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR,   "CloudAndGloss6x6.astc",     "Earth-Color6x6.astc",     "Earth-Night6x6.astc",     "6x6 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR,   "CloudAndGloss8x5.astc",     "Earth-Color8x5.astc",     "Earth-Night8x5.astc",     "8x5 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR,   "CloudAndGloss8x6.astc",     "Earth-Color8x6.astc",     "Earth-Night8x6.astc",     "8x6 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR,   "CloudAndGloss8x8.astc",     "Earth-Color8x8.astc",     "Earth-Night8x8.astc",     "8x8 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR,  "CloudAndGloss10x5.astc",    "Earth-Color10x5.astc",    "Earth-Night10x5.astc",    "10x5 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR,  "CloudAndGloss10x6.astc",    "Earth-Color10x6.astc",    "Earth-Night10x6.astc",    "10x6 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR,  "CloudAndGloss10x8.astc",    "Earth-Color10x8.astc",    "Earth-Night10x8.astc",    "10x8 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR, "CloudAndGloss10x10.astc",   "Earth-Color10x10.astc",   "Earth-Night10x10.astc",   "10x10 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR, "CloudAndGloss12x10.astc",   "Earth-Color12x10.astc",   "Earth-Night12x10.astc",   "12x10 SRGB ASTC",
    GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR, "CloudAndGloss12x12.astc",   "Earth-Color12x12.astc",   "Earth-Night12x12.astc",   "12x12 SRGB ASTC"
};

/* Number of texture sets. */
const int n_texture_ids = sizeof(texture_sets_info) / sizeof(texture_sets_info[0]);

/* Array storing texture bindings. */
texture_set texture_ids[n_texture_ids] = { 0 };

/* Please see header for specification. */
GLint get_and_check_attrib_location(GLuint program, const GLchar* attrib_name)
{
    GLint attrib_location = GL_CHECK(glGetAttribLocation(program, attrib_name));

    if (attrib_location == -1)
    {
        LOGE("Cannot retrieve location of %s attribute.\n", attrib_name);
        exit(EXIT_FAILURE);
    }

    return attrib_location;
}

/* Please see header for specification. */
GLint get_and_check_uniform_location(GLuint program, const GLchar* uniform_name)
{
    GLint uniform_location = GL_CHECK(glGetUniformLocation(program, uniform_name));

    if (uniform_location == -1)
    {
        LOGE("Cannot retrieve location of %s uniform.\n", uniform_name);
        exit(EXIT_FAILURE);
    }

    return uniform_location;
}

/* Please see header for specification. */
GLuint load_shader(GLenum shader_type, const char* shader_source)
{
    GLuint shader = GL_CHECK(glCreateShader(shader_type));

    if (shader != 0)
    {
        GL_CHECK(glShaderSource(shader, 1, &shader_source, NULL));
        GL_CHECK(glCompileShader(shader));

        GLint compiled = 0;

        GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled));

        if (compiled != GL_TRUE)
        {
            GLint info_len = 0;

            GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len));

            if (info_len > 0)
            {
                char* log_buffer = NULL;

                MALLOC_CHECK(char*, log_buffer, info_len);

                GL_CHECK(glGetShaderInfoLog(shader, info_len, NULL, log_buffer));
                LOGE("Could not compile shader 0x%x:\n%s\n", shader_type, log_buffer);
                FREE_CHECK(log_buffer);

                GL_CHECK(glDeleteShader(shader));
                shader = 0;

                exit(EXIT_FAILURE);
            }
        }
    }

    return shader;
}

/* Please see header for specification. */
GLuint create_program(const char* vertex_source, const char* fragment_source)
{
    GLuint vertexShader = load_shader(GL_VERTEX_SHADER, vertex_source);
    GLuint fragmentShader = load_shader(GL_FRAGMENT_SHADER, fragment_source);
    GLuint program = GL_CHECK(glCreateProgram());

    if (program != 0)
    {
        GL_CHECK(glAttachShader(program, vertexShader));
        GL_CHECK(glAttachShader(program, fragmentShader));
        GL_CHECK(glLinkProgram(program));

        GLint linkStatus = GL_FALSE;

        GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &linkStatus));

        if (linkStatus != GL_TRUE)
        {
            GLint buf_length = 0;

            GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &buf_length));

            if (buf_length > 0)
            {
                char* log_buffer = NULL;

                MALLOC_CHECK(char*, log_buffer, buf_length);
                GL_CHECK(glGetProgramInfoLog(program, buf_length, NULL, log_buffer));
                LOGE("Could not link program:\n%s\n", log_buffer);
                FREE_CHECK(log_buffer);
            }

            GL_CHECK(glDeleteProgram(program));
            program = 0;

            exit(EXIT_FAILURE);
        }
    }
    else
    {
        LOGE("Error creating program object.");
        exit(EXIT_FAILURE);
    }

    return program;
}

/**
 * \brief Update texture bindings and text presented by text renderer.
 *
 * \param[in] force_switch_texture If true texture is immediately switched,
 *                                 if false texture is switched after interval passes.
 */
void update_texture_bindings(bool force_switch_texture)
{
    if (timer.getTime() >= ASTC_TEXTURE_SWITCH_INTERVAL || force_switch_texture)
    {
        /* If the current texture set is to be changed, reset timer to start counting time again. */
        timer.reset();

        /* Clear current text. */
        text_displayer->clear();

        if (!force_switch_texture)
        {
            if (current_texture_set_id < n_texture_ids - 1)
            {
                current_texture_set_id++;
            }
            else
            {
                current_texture_set_id = 0;
            }
        }

        /* Change displayed text. */
        text_displayer->addString((window_width - (Text::textureCharacterWidth * strlen(texture_ids[current_texture_set_id].name))) >> 1,
                                   window_height - Text::textureCharacterHeight,
                                   texture_ids[current_texture_set_id].name,
                                   255,  /* Red channel. */
                                   255,  /* Green channel. */
                                   0,    /* Blue channel. */
                                   255); /* Alpha channel. */
    }

    /* Update texture units with new bindings. */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_ids[current_texture_set_id].cloud_and_gloss_texture_id));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_ids[current_texture_set_id].earth_color_texture_id));
    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_ids[current_texture_set_id].earth_night_texture_id));
}

/**
 * \brief Define and retrieve compressed texture image.
 *
 * \param[in] file_name                       Texture file name.
 * \param[in] compressed_data_internal_format ASTC compression internal format.
 */
GLuint load_texture(const char* file_name, GLenum compressed_data_internal_format)
{
    unsigned char* compressed_data = NULL;
    unsigned char* input_data      = NULL;

    long         file_size       = 0;
    unsigned int n_bytes_to_read = 0;
    size_t       result          = 0;
    GLuint       to_id           = 0;

    /* Number of blocks in the x, y and z direction. */
    int xblocks = 0;
    int yblocks = 0;
    int zblocks = 0;

    /* Number of bytes for each dimension. */
    int xsize = 0;
    int ysize = 0;
    int zsize = 0;

    FILE* compressed_data_file = fopen(file_name, "rb");

    if (compressed_data_file == NULL)
    {
        LOGE("Could not open a file.\n");
        exit(EXIT_FAILURE);
    }

    LOGI("Loading texture [%s]\n", file_name);

    /* Obtain file size. */
    fseek(compressed_data_file, 0, SEEK_END);
    file_size = ftell(compressed_data_file);
    rewind(compressed_data_file);

    /* Allocate memory to contain the whole file. */
    MALLOC_CHECK(unsigned char*, input_data, sizeof(unsigned char) * file_size);

    /* Copy the file into the buffer. */
    result = fread(input_data, 1, file_size, compressed_data_file);

    if (result != file_size)
    {
        LOGE("Reading error [%s] ... FILE: %s LINE: %i\n", file_name, __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    /* Traverse the file structure. */
    astc_header* astc_data_ptr = (astc_header*) input_data;

    /* Merge x,y,z-sizes from 3 chars into one integer value. */
    xsize = astc_data_ptr->xsize[0] + (astc_data_ptr->xsize[1] << 8) + (astc_data_ptr->xsize[2] << 16);
    ysize = astc_data_ptr->ysize[0] + (astc_data_ptr->ysize[1] << 8) + (astc_data_ptr->ysize[2] << 16);
    zsize = astc_data_ptr->zsize[0] + (astc_data_ptr->zsize[1] << 8) + (astc_data_ptr->zsize[2] << 16);

    /* Compute number of blocks in each direction. */
    xblocks = (xsize + astc_data_ptr->blockdim_x - 1) / astc_data_ptr->blockdim_x;
    yblocks = (ysize + astc_data_ptr->blockdim_y - 1) / astc_data_ptr->blockdim_y;
    zblocks = (zsize + astc_data_ptr->blockdim_z - 1) / astc_data_ptr->blockdim_z;

    /* Each block is encoded on 16 bytes, so calculate total compressed image data size. */
    n_bytes_to_read = xblocks * yblocks * zblocks << 4;

    /* We now have file contents in memory so let's fill a texture object with the data. */
    GL_CHECK(glGenTextures(1, &to_id));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, to_id));

    /* Upload texture data to ES. */
    GL_CHECK(glCompressedTexImage2D(GL_TEXTURE_2D,
                                    0,
                                    compressed_data_internal_format,
                                    xsize,
                                    ysize,
                                    0,
                                    n_bytes_to_read,
                                    (const GLvoid*)&astc_data_ptr[1]));

    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT));

    /* Unbind texture from target. */
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

    /* Terminate file operations. */
    fclose(compressed_data_file);
    FREE_CHECK(input_data);

    return to_id;
}

/**
 * \brief Define 32 texture sets that the demo will switch between every 5 seconds.
 */
void load_textures(void)
{
    /* Paths to texture images. */
    string cloud_and_gloss_texture_file_path;
    string earth_color_texture_file_path;
    string earth_night_texture_file_path;

    for (int i = 0; i < n_texture_ids; i++)
    {
        cloud_and_gloss_texture_file_path = resource_directory + texture_sets_info[i].cloud_and_gloss_texture_file_path;
        earth_color_texture_file_path     = resource_directory + texture_sets_info[i].earth_color_texture_file_path;
        earth_night_texture_file_path     = resource_directory + texture_sets_info[i].earth_night_texture_file_path;

        texture_ids[i].cloud_and_gloss_texture_id = load_texture(cloud_and_gloss_texture_file_path.c_str(), texture_sets_info[i].compressed_data_internal_format);
        texture_ids[i].earth_color_texture_id     = load_texture(earth_color_texture_file_path.c_str(),     texture_sets_info[i].compressed_data_internal_format);
        texture_ids[i].earth_night_texture_id     = load_texture(earth_night_texture_file_path.c_str(),     texture_sets_info[i].compressed_data_internal_format);
        texture_ids[i].name                       = texture_sets_info[i].compressed_texture_format_name;
    }

    /* Configure texture set. */
    update_texture_bindings(true);
}

/**
 * \brief This function sets up a program object that will be used for rendering,
 *        as well as retrieves attribute & uniform locations.
 */
void setup_program(void)
{
    /* Create program object and initialize it. */
    program_id = create_program(earth_vertex_shader_source, earth_fragment_shader_source);

    /* Get attribute locations of attributes vertex position,cd normal and texture coordinates. */
    position_location       = get_and_check_attrib_location(program_id, "av4position");
    normal_location         = get_and_check_attrib_location(program_id, "vv3normal");
    texture_coords_location = get_and_check_attrib_location(program_id, "vv3tex2dcoord");

    /* Get uniform locations. */
    mv_location                = get_and_check_uniform_location(program_id, "mv");
    mvp_location               = get_and_check_uniform_location(program_id, "mvp");
    cloud_texture_location     = get_and_check_uniform_location(program_id, "cloud_texture");
    daytime_texture_location   = get_and_check_uniform_location(program_id, "daytime_texture");
    nighttime_texture_location = get_and_check_uniform_location(program_id, "nighttime_texture");

    /* Activate program object. */
    GL_CHECK(glUseProgram(program_id));
}

/**
 * \brief Sets up a buffer object that will hold mesh data (vertex positions, normal vectors, textures UV coordinates).
 */
void load_buffer_data(void)
{
    const float sphere_radius = 1.0f;

    /* Number of pararells and meridians the sphere should consists of. */
    const unsigned int n_sectors = 64;
    const unsigned int n_rings   = 64;

    /* New instance of sphere. */
    solid_sphere = new SolidSphere(sphere_radius, n_rings, n_sectors);

    /* Obtain sizes of the several subbuffers. */
    GLsizei sphere_vertices_size  = 0;
    GLsizei sphere_normals_size   = 0;
    GLsizei sphere_texcoords_size = 0;
    GLsizei buffer_offset         = 0;

    /* Load generated mesh data from SolidSphere object. */
    float* sphere_vertices  = solid_sphere->getSphereVertexData(&sphere_vertices_size);
    float* sphere_normals   = solid_sphere->getSphereNormalData(&sphere_normals_size);
    float* sphere_texcoords = solid_sphere->getSphereTexcoords(&sphere_texcoords_size);

    /* Size of the entire buffer. */
    GLsizei buffer_total_size = sphere_vertices_size + sphere_normals_size + sphere_texcoords_size;

    /* Create buffer object to hold all mesh data. */
    GL_CHECK(glGenBuffers(1, &bo_id));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, bo_id));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, buffer_total_size, NULL, GL_STATIC_DRAW));

    /* Upload subsets of mesh data to buffer object. */
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, sphere_vertices_size, sphere_vertices));

    buffer_offset += sphere_vertices_size;

    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, sphere_normals_size, sphere_normals));

    buffer_offset += sphere_normals_size;

    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, sphere_texcoords_size, sphere_texcoords));

    /* Release resources. */
    FREE_CHECK(sphere_vertices);
    FREE_CHECK(sphere_normals);
    FREE_CHECK(sphere_texcoords);

    /* Configure vertex attribute arrays, so that position/normals/texture coordinate data is available to the vertex shader. */
    GL_CHECK(glGenVertexArrays(1, &vao_id));
    GL_CHECK(glBindVertexArray(vao_id));
    GL_CHECK(glEnableVertexAttribArray(position_location));
    GL_CHECK(glEnableVertexAttribArray(normal_location));
    GL_CHECK(glEnableVertexAttribArray(texture_coords_location));

    buffer_offset = 0;

    /* Populate attribute for position. */
    GL_CHECK(glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) buffer_offset));

    buffer_offset += sphere_vertices_size;

    /* Populate attribute for normals. */
    GL_CHECK(glVertexAttribPointer(normal_location, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) buffer_offset));

    buffer_offset += sphere_normals_size;

    /* Populate attribute for texture coordinates. */
    GL_CHECK(glVertexAttribPointer(texture_coords_location, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) buffer_offset));

    /* Bind texture units to texture samplers. */
    GL_CHECK(glUniform1i(cloud_texture_location,     0));
    GL_CHECK(glUniform1i(daytime_texture_location,   1));
    GL_CHECK(glUniform1i(nighttime_texture_location, 2));

    /* Load generated indices from SolidSphere object. */
    sphere_indices = solid_sphere->getSphereIndices(&sphere_indices_size);
}

void setup_graphics(int width, int height)
{
    window_width  = width;
    window_height = height;

    x_to_y_ratio = (float) window_width / (float) window_height;

    perspective_matrix = Matrix::matrixPerspective(field_of_view, x_to_y_ratio, z_near, z_far);

    /* Make sure the required ASTC extension is present. */
    const GLubyte* extensions = GL_CHECK(glGetString(GL_EXTENSIONS));

    if (strstr((const char*) extensions, "GL_KHR_texture_compression_astc_ldr") == NULL)
    {
        LOGI("OpenGL ES 3.0 implementation does not support GL_KHR_texture_compression_astc_ldr extension.\n");
        exit(EXIT_SUCCESS);
    }

    /* Enable culling and depth testing. */
    GL_CHECK(glEnable(GL_CULL_FACE));
    GL_CHECK(glEnable(GL_DEPTH_TEST));

    /* Enable blending and specify pixel arihmetic.
       Transparency is implemented using blend function with primitives sorted from the farthest to the nearest. */
    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    text_displayer = new Text(resource_directory.c_str(), window_width, window_height);

    /* Create texture objects and fill them with texture data. */
    load_textures();

    /* Set up shader objects.
       Rerieve uniform and attribute locations. */
    setup_program();

    /* Prepare buffer objects that will hold mesh data. */
    load_buffer_data();

    /* Start counting time. */
    timer.reset();
    fps_timer.reset();
}

/**
 * \brief Renders a single frame.
 */
void render_frame(void)
{
    /* Prepare rotation matrices and use them to set up model+view matrix. */
    model_view_matrix = Matrix::createRotationX(angle_x);
    rotate_matrix     = Matrix::createRotationY(angle_y);
    model_view_matrix = rotate_matrix * model_view_matrix;
    rotate_matrix     = Matrix::createRotationZ(-angle_z);
    model_view_matrix = rotate_matrix * model_view_matrix;

    /* Pull the camera back from the cube - move back and forth as time goes by.
       To achieve it scale with time translational part of model matrix for z-direction. */
    model_view_matrix[14] -= 2.5f + sinf(current_time / 5.0f) * 0.5f;

    /* Upload view matrix. */
    GL_CHECK(glUniformMatrix4fv(mv_location, 1, GL_FALSE, &model_view_matrix[0]));

    /* Bring model from camera space into Normalized Device Coordinates (NDC). */
    mvp_matrix = perspective_matrix * model_view_matrix;

    /* Upload complete Model Space->World Space->Camera(Eye) Space->NDC transformation by updating relevant MVP uniform.
       Vertex shader will then use this matrix to transform all input vertices from model space to screen space. */
    GL_CHECK(glUniformMatrix4fv(mvp_location, 1, GL_FALSE, &mvp_matrix[0]));

    /* Get the current time. */
    current_time = fps_timer.getTime();

    /* Obtain angular rates around X, Y, Z axes. */
    angle_x = (float) (current_time * X_ROTATION_SPEED);
    angle_y = (float) (current_time * Y_ROTATION_SPEED);
    angle_z = (float) (current_time * Z_ROTATION_SPEED);

    /* Clear buffers. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* Draw the sphere from array data. */
    GL_CHECK(glDrawElements(GL_TRIANGLES, sphere_indices_size, GL_UNSIGNED_SHORT, sphere_indices));

    /* Also don't forget to show name of the compression algorithm in action. */
    text_displayer->draw();

    /* Switch the texture set if more than 5 seconds has passed since the last switch event. */
    update_texture_bindings(false);
}

/**
 * \brief Perform graphics clean-up actions.
 */
void cleanup_graphics(void)
{
    /* Delete all used textures. */
    for (int i = 0; i < n_texture_ids; i++)
    {
        GL_CHECK(glDeleteTextures(1, &texture_ids[i].cloud_and_gloss_texture_id));
        GL_CHECK(glDeleteTextures(1, &texture_ids[i].earth_color_texture_id));
        GL_CHECK(glDeleteTextures(1, &texture_ids[i].earth_night_texture_id));
    }

    /* Cleanup shaders. */
    GL_CHECK(glUseProgram(0));
    GL_CHECK(glDeleteShader(vert_shader_id));
    GL_CHECK(glDeleteShader(frag_shader_id));
    GL_CHECK(glDeleteProgram(program_id));

    /* Delete vertex array object. */
    GL_CHECK(glDisableVertexAttribArray(position_location));
    GL_CHECK(glDisableVertexAttribArray(normal_location));
    GL_CHECK(glDisableVertexAttribArray(texture_coords_location));
    GL_CHECK(glDeleteVertexArrays(1, &vao_id));

    /* Free buffer object memory. */
    GL_CHECK(glDeleteBuffers(1, &bo_id));

    /* Free indices buffer. */
    FREE_CHECK(sphere_indices);

    /* Release memory for Text and SolidSphere instances. */
    delete text_displayer;
    delete solid_sphere;
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_step(JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_uninit(JNIEnv*, jobject);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height)
{
    setup_graphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_step(JNIEnv*, jobject)
{
    render_frame();
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_astctextures_NativeLibrary_uninit(JNIEnv*, jobject)
{
    cleanup_graphics();
}
