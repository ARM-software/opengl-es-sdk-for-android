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

#include "Image.h"
#include "Quaternions.h"
#include "Text.h"
#include "Skybox.h"

#include <jni.h>
#include <GLES3/gl3.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>

/* Window resolution. */
unsigned int window_width  = 0;
unsigned int window_height = 0;

using namespace Skybox;

/* Location of a 'viewMat' uniform variable. */
GLint location_viewMat = 0;

/* ID of a program object. */
GLuint program_id = 0;

/* Quaternions representing rotations around X, Y and Z axes. */
Quaternion Q_X = { 0.0f, 0.0f, 0.0f, 0.0f };
Quaternion Q_Y = { 0.0f, 0.0f, 0.0f, 0.0f };
Quaternion Q_Z = { 0.0f, 0.0f, 0.0f, 0.0f };

/* Quaternions to store resultant products. */
Quaternion Q_XY  = { 0.0f, 0.0f, 0.0f, 0.0f };
Quaternion Q_XYZ = { 0.0f, 0.0f, 0.0f, 0.0f };

/* Used to hold cube-map texture face data when initializing skybox cube-map texture. */
ImageFile cubemap_image = { 0, 0, NULL };

/* Instance of text renderer. */
Text* text = NULL;

/* Texture cubemap name. */
GLuint cubemap_texture = 0;

/* Number of degrees to rotate counterclockwise around X, Y and Z axes respectively. */
float roll = 0.0f, pitch = 0.0f, yaw = 0.0f;

/* 4x4 matrix that transforms the skybox's vertices from model space to world space. */
float model_view_matrix[16] = {0.0f};

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

void setup_graphics(int width, int height)
{
    window_width = width;
    window_height = height;

    /* Path to resource directory. */
    const char resource_directory[] = "/data/data/com.arm.malideveloper.openglessdk.skybox/files/";

    /* Path to cubemap texture. */
    char file_name[] = "/data/data/com.arm.malideveloper.openglessdk.skybox/files/greenhouse_skybox-0.ppm";

    /* Texture cubemap targets. */
    GLenum cubemap_faces[] =
    {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    /* Generate texture name and bind it to the texture cubemap target. */
    GL_CHECK(glGenTextures(1, &cubemap_texture));
    GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture));

    /* Set up texture parameters. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));

    /* Load cubemap texture. */
    cubemap_image = load_ppm_file(file_name);

    /* Specify storage for all levels of a cubemap texture. */
    GL_CHECK(glTexStorage2D(GL_TEXTURE_CUBE_MAP,    /* Texture target */
                            1,                      /* Number of texture levels */
                            GL_RGB8,                /* Internal format for texture storage */
                            cubemap_image.width,    /* Width of the texture image */
                            cubemap_image.height)); /* Height of the texture image */

    for (int n_face = 0; n_face < sizeof(cubemap_faces) / sizeof(cubemap_faces[0]); n_face++)
    {
        if (n_face != 0)
        {
            sprintf(file_name, "/data/data/com.arm.malideveloper.openglessdk.skybox/files/greenhouse_skybox-%d.ppm", n_face);

            cubemap_image = load_ppm_file(file_name);
        }

        GL_CHECK(glTexSubImage2D(cubemap_faces[n_face],                  /* Texture target. */
                                 0,                                      /* Level-of-detail number. */
                                 0,                                      /* Texel offset in the x direction. */
                                 0,                                      /* Texel offset in the y direction. */
                                 cubemap_image.width,                    /* Width of the texture image. */
                                 cubemap_image.height,                   /* Height of the texture image. */
                                 GL_RGB,                                 /* Format of the pixel data. */
                                 GL_UNSIGNED_BYTE,                       /* Type of the pixel data. */
                                 (const GLvoid*) cubemap_image.pixels)); /* Pointer to the image data. */

        FREE_CHECK(cubemap_image.pixels);
    }

    /* Create a program object that we will attach the fragment and vertex shader to. */
    program_id = create_program(skybox_vertex_shader_source, skybox_fragment_shader_source);

    /* The program object has been successfully linked. Let's use it. */
    GL_CHECK(glUseProgram(program_id));

    /* Retrieve uniform location for "viewMat" uniform defined in vertex shader. */
    location_viewMat = GL_CHECK(glGetUniformLocation(program_id, "viewMat"));

    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    text = new Text(resource_directory, window_width, window_height);
    text->clear();
    text->addString(0, 0, "Skybox Sample", 255, 255, 0, 255);
}

/**
 * \brief Renders a single frame.
 */
void render_frame(void)
{
    /* Construct quaternions for X, Y and Z axes. */
    Q_X = construct_quaternion(1.0f, 0.0f, 0.0f, roll);
    Q_Y = construct_quaternion(0.0f, 1.0f, 0.0f, pitch);
    Q_Z = construct_quaternion(0.0f, 0.0f, 1.0f, yaw);

    /* Obtain the resultant quaternion. */
    Q_XY  = multiply_quaternions(Q_X, Q_Y);
    Q_XYZ = multiply_quaternions(Q_XY, Q_Z);

    /* Compute a modelview matrix. Model matrix is a unit matrix. */
    construct_modelview_matrix(Q_XYZ, model_view_matrix);

    /* In this demo, we do not need to provide the vertex shader with any mesh data, because a predefined set
       of 4 vertices is embedded within the shader. These vertices, expressed in Normalized Device Coordinates,
       correspond to four corners of the visible screen space. By using this vertices to form a triangle strip,
       we end up with a full-screen quad that is later used for rasterization stage. */

    /* Restore the cubemap program object, because it has been changed by text rendering call. */
    GL_CHECK(glUseProgram(program_id));

    /* Upload the matrix to view matrix uniform so that it can be used for vertex shader stage. */
    GL_CHECK(glUniformMatrix4fv(location_viewMat, 1, GL_FALSE, (const GLfloat*) model_view_matrix));

    /* The angles can be decremented too to reverse the direction of rotation. */
    roll  += 0.2f;
    pitch += 0.4f;
    yaw   += 0.2f;

    /* Rotating the skybox by more than 360 or less than 0 degrees is not permitted, in order to avoid problems. */
    if (fabs(roll) >= 360.0f)
    {
        if (roll < 0.0f)
        {
            roll += 360.0f;
        }
        else
        {
            roll -= 360.0f;
        }
    }

    if (fabs(pitch) >= 360.0f)
    {
        if (pitch < 0.0f)
        {
            pitch += 360.0f;
        }
        else
        {
            pitch -= 360.0f;
        }
    }

    if (fabs(yaw) >= 360.0f)
    {
        if (yaw < 0.0f)
        {
            yaw += 360.0f;
        }
        else
        {
            yaw -= 360.0f;
        }
    }

    /* Render a full-screen quad, as described above.
       Note that the actual content of the quad is drawn within the fragment shader. */
    GL_CHECK(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    text->draw();
}

/**
 * \brief Perform graphics clean-up actions.
 */
void cleanup_graphics(void)
{
    /* Delete the cube map texture. */
    GL_CHECK(glDeleteTextures(1, &cubemap_texture));

    /* Release shaders. */
    GL_CHECK(glUseProgram(0));
    GL_CHECK(glDeleteProgram(program_id));
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_step(JNIEnv*, jobject);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_uninit(JNIEnv*, jobject);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_init(JNIEnv*, jobject, jint width, jint height)
{
    setup_graphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_step(JNIEnv*, jobject)
{
    render_frame();
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_skybox_NativeLibrary_uninit(JNIEnv*, jobject)
{
    cleanup_graphics();
}
