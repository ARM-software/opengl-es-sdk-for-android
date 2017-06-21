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

/**
 * \file tutorials/Bloom/jni/Native.cpp
 * \brief The application shows a bloom effect implementation. It draws cubes arranged in a two-dimensional
 *        5x5 array, from which only the diagonal ones are bloomed.
 *        The intensity of the bloom effect changes from very weak, where each cube affected by the effect looks
 *        exactly as the normal ones, to very strong, when bloomed cubes make up an X shape.
 *
 *        The bloom effect is implemented as follows:
 *        1. A scene (5x5 array of cubes: cubes on diagonals are white, others are blue)
 *           is drawn to a render target.
 *        2. Elements that should be bloomed (the brighter ones, in this case cubes placed on diagonals)
 *           are drawn into downscaled texture object (where the rest of the scene is black).
 *        3. The result texture from step 2 is horizontally blurred - the outcome is stored
 *           in a texture which is then used for the vertical blur.
 *           This step can be repeated as described later (*).
 *        4. Both the texture in which the vertically & horizontally blurred image has been stored
 *           (result of step 3), and the texture from step 1 are blended (horizontally & vertically) and drawn into the back buffer.
 *
 *        (*) The blend effect is not constant during the rendering process: it changes from very weak to very strong.
 *            This is achieved by repeating step 3 a varying amount of times
 *            (depending on the required intensity of the effect) - the only difference
 *            is that for the n-th iteration the generated result of (n-th - 1) is taken as a source for the horizontal blur.
 *            To make the bloom effect more smooth, we also use continuous sampling of the textures.
 *            The results of the last two iterations from step 3 are used for the final combination pass.
 *            The colours of those two textures are mixed together with an appropriate factor value.
 *            (for more details please see the mix() function description in the OpenGL ES Shading Language documentation).
 *
 *        Besides the bloom effect, the application also shows:
 *        - matrix calculations (e.g. used for perspective view),
 *        - instanced drawing (each cube drawn on a screen is an instance of the same object),
 *        - lighting (the model is lit by a directional light),
 *        - rendering into a texture.
 */


#include <jni.h>
#include <android/log.h>

#include <GLES3/gl3.h>

#include "CubeModel.h"
#include "Matrix.h"
#include "Shader.h"

using namespace MaliSDK;

/** Window resolution divisor will be used for downscaling the texture used for blurring. */
#define WINDOW_RESOLUTION_DIVISOR (2)

/* [Number of cubes] */
/** Number of cubes drawn on screen. */
#define NUMBER_OF_CUBES (25)
/* [Number of cubes] */

/* [Cube scalar] */
/** Cube size scalar. */
#define CUBE_SCALAR (0.8f)
/* [Cube scalar] */

/** Radius of the blur effect (in pixels). */
#define BLUR_RADIUS (3)

/** Number of vertex coordinates. Each vertex is described in 3D space with 3 values: xyz. */
#define NUMBER_OF_COMPONENTS_PER_VERTEX (3)

/** Maximum number of blur passes. */
#define MAX_NUMBER_OF_BLUR_PASSES (10)
/** Minimum number of blur passes. */
#define MIN_NUMBER_OF_BLUR_PASSES  (2)

/** Step which is used for changing mix factor values (used while mixing colour textures to get a continuous sampling effect). */
#define BLUR_MIX_FACTOR_STEP_VALUE (0.05f)
/** Indicates positive sign, used in blur calculations when the effect should be increased. */
#define BLUR_EFFECT_INCREASE       (1)
/** Indicates negative sign, used in blur calculations when the effect should be decreased. */
#define BLUR_EFFECT_DECREASE       (-1)
/** Indicates how much time should it take to switch between number of blur passes. */
#define TIME_INTERVAL              (1.0f)

/* [Color texture unit define] */
/** Texture unit which a color texture will be bound to. */
#define TEXTURE_UNIT_COLOR_TEXTURE           (0)
/* [Color texture unit define] */
/* [Bloom source texture unit define] */
/** Texture unit which a texture with bloomed elements will be bound to. */
#define TEXTURE_UNIT_BLOOM_SOURCE_TEXTURE    (1)
/* [Bloom source texture unit define] */
/** Texture unit which a texture with horizontally blurred elements will be bound to. */
#define TEXTURE_UNIT_HORIZONTAL_BLUR_TEXTURE (2)
/** Texture unit which a texture with weaker blur effect will be bound to. */
#define TEXTURE_UNIT_BLURRED_TEXTURE         (3)
/** Texture unit which a texture with stronger blur effect will be bound to. */
#define TEXTURE_UNIT_STRONGER_BLUR_TEXTURE   (4)

/** Camera depth location for horizontal position
 * (should be used when the window width is greater than window height).
 */
#define HORIZONTAL_POSITION_CAMERA_DEPTH_LOCATION (15.0f)
/** Camera depth location for vertical position
 * (should be used when the window height is greater than window width).
 */
#define VERTICAL_POSITION_CAMERA_DEPTH_LOCATION   (20.0f)

/** \brief Structure holding locations of uniforms
 *         used by a program object responsible for applying the blend effect.
 */
struct BlendingProgramLocations
{
    GLint uniformMixFactor;
    GLint uniformOriginalTexture;
    GLint uniformStrongerBlurTexture;
    GLint uniformWeakerBlurTexture;

    /* Default values constructor. */
    BlendingProgramLocations()
    {
        uniformMixFactor           = -1;
        uniformOriginalTexture     = -1;
        uniformStrongerBlurTexture = -1;
        uniformWeakerBlurTexture   = -1;
    }
};

/** \brief Structure holding ID of objects which were generated for blurring.
 */
struct BlurringObjects
{
    GLuint framebufferObjectId;
    GLuint textureObjectIdHorizontal;
    GLuint textureObjectIdVertical;

    /* Default values constructor. */
    BlurringObjects()
    {
        framebufferObjectId       = 0;
        textureObjectIdHorizontal = 0;
        textureObjectIdVertical   = 0;
    }
};

/** \brief Structure holding locations of uniforms
 *         used by a program object responsible for blurring.
 */
struct BlurringProgramLocations
{
    GLint uniformBlurRadius;
    GLint uniformTextureSampler;

    /* Default values constructor. */
    BlurringProgramLocations()
    {
        uniformBlurRadius     = -1;
        uniformTextureSampler = -1;
    }
};

/** \brief Structure holding IDs of objects which were generated
 *         for generating downscaled texture with luminance image.
 */
struct GetLuminanceImageBloomObjects
{
    GLuint framebufferObjectId;
    GLuint textureObjectId;

    /* Default values constructor. */
    GetLuminanceImageBloomObjects()
    {
        framebufferObjectId = 0;
        textureObjectId     = 0;
    }
};

/** \brief Structure holding program object ID and IDs of two shader objects (fragment and vertex).
 *         It is assumed that shader objects are/will be attached to program object.
 */
struct ProgramAndShadersIds
{
    GLuint fragmentShaderObjectId;
    GLuint programObjectId;
    GLuint vertexShaderObjectId;

    /* Default values constructor. */
    ProgramAndShadersIds()
    {
        fragmentShaderObjectId = 0;
        programObjectId        = 0;
        vertexShaderObjectId   = 0;
    }
};

/** \brief Structure holding ID of objects which were generated
 *         to support scene rendering.
 */
struct SceneRenderingObjects
{
    GLuint bufferObjectIdCubeCoords;
    GLuint bufferObjectIdCubeNormals;
    GLuint bufferObjectIdElementLocations;
    GLuint framebufferObjectId;
    GLuint textureObjectIdDepthImage;
    GLuint textureObjectIdOriginalImage;

    /* Default values constructor. */
    SceneRenderingObjects()
    {
        bufferObjectIdCubeCoords       = 0;
        bufferObjectIdCubeNormals      = 0;
        bufferObjectIdElementLocations = 0;
        framebufferObjectId            = 0;
        textureObjectIdDepthImage      = 0;
        textureObjectIdOriginalImage   = 0;
    }
};

/** \brief Structure holding locations of attributes and uniforms
 *         used by a program object responsible for scene rendering.
 */
struct SceneRenderingProgramLocations
{
    /* Attribute locations. */
    GLint attribCubeVertexCoordinates;
    GLint attribCubeVertexNormals;
    /* Uniform locations. */
    GLint uniformCameraPosition;
    GLint uniformBlockCubeProperties;
    GLint uniformLightPropertiesAmbient;
    GLint uniformLightPropertiesColor;
    GLint uniformLightPropertiesConstantAttenuation;
    GLint uniformLightPropertiesLinearAttenuation;
    GLint uniformLightPropertiesQuadraticAttenauation;
    GLint uniformLightPropertiesPosition;
    GLint uniformLightPropertiesShininess;
    GLint uniformLightPropertiesStrength;
    GLint uniformMvMatrix;
    GLint uniformMvpMatrix;

    /* Default values constructor. */
    SceneRenderingProgramLocations()
    {
        attribCubeVertexCoordinates                 = -1;
        attribCubeVertexNormals                     = -1;
        uniformCameraPosition                       = -1;
        uniformBlockCubeProperties                  = GL_INVALID_INDEX;
        uniformLightPropertiesAmbient               = -1;
        uniformLightPropertiesColor                 = -1;
        uniformLightPropertiesConstantAttenuation   = -1;
        uniformLightPropertiesLinearAttenuation     = -1;
        uniformLightPropertiesQuadraticAttenauation = -1;
        uniformLightPropertiesPosition              = -1;
        uniformLightPropertiesShininess             = -1;
        uniformLightPropertiesStrength              = -1;
        uniformMvMatrix                             = -1;
        uniformMvpMatrix                            = -1;
    }
};

/** \brief Structure holding IDs of objects which were generated
 *         for stronger texture blurring.
 */
struct StrongerBlurObjects
{
    GLuint framebufferObjectId;
    GLuint textureObjectId;

    /* Default values constructor. */
    StrongerBlurObjects()
    {
        framebufferObjectId = 0;
        textureObjectId     = 0;
    }
};

/* Shader sources. */
/* [Blend fragment shader source] */
static const char blendFragmentShaderSource[] =         "#version 300 es\n"
                                                        "precision mediump float;\n"
                                                        "/* UNIFORMS */\n"
                                                        "/** Factor which will be used for mixing higher and lower blur effect texture colours. */\n"
                                                        "uniform float     mix_factor;\n"
                                                        "/** Texture storing colour data (with all the cubes). */\n"
                                                        "uniform sampler2D original_texture;\n"
                                                        "/** Texture in which (n+1) blur operations have been applied to the input texture. */\n"
                                                        "uniform sampler2D stronger_blur_texture;\n"
                                                        "/** Texture in which (n)   blur operations have been applied to the input texture. */\n"
                                                        "uniform sampler2D weaker_blur_texture;\n"
                                                        "/* INPUTS */\n"
                                                        "/** Texture coordinates. */\n"
                                                        "in vec2 texture_coordinates;\n"
                                                        "/* OUTPUTS */\n"
                                                        "/** Fragment colour to be returned. */\n"
                                                        "out vec4 color;\n"
                                                        "void main()\n"
                                                        "{\n"
                                                        "    vec4 stronger_blur_texture_color = texture(stronger_blur_texture, texture_coordinates);\n"
                                                        "    vec4 weaker_blur_texture_color   = texture(weaker_blur_texture,   texture_coordinates);\n"
                                                        "    vec4 mixed_blur_color            = mix(weaker_blur_texture_color, stronger_blur_texture_color, mix_factor);\n"
                                                        "    vec4 original_color              = texture(original_texture, texture_coordinates);\n"
                                                        "    /* Return blended colour. */\n"
                                                        "    color = original_color + mixed_blur_color;\n"
                                                        "}\n";
/* [Blend fragment shader source] */
/* [Blur fragment shader source for horizontal blurring] */
static const char blurHorizontalFragmentShaderSource[] = "#version 300 es\n"
                                                         "precision mediump float;\n"
                                                         "/** Defines gaussian weights. */\n"
                                                         "const float gaussian_weights[] = float[] (0.2270270270,\n"
                                                         "                                          0.3162162162,\n"
                                                         "                                          0.0702702703);\n"
                                                         "/* UNIFORMS */\n"
                                                         "/** Radius of a blur effect to be applied. */\n"
                                                         "uniform float     blur_radius;\n"
                                                         "/** Texture sampler on which the effect will be applied. */\n"
                                                         "uniform sampler2D texture_sampler;\n"
                                                         "/* INPUTS */\n"
                                                         "/** Texture coordinates. */\n"
                                                         "in vec2 texture_coordinates;\n"
                                                         "/* OUTPUTS */\n"
                                                         "/** Fragment colour that will be returned. */\n"
                                                         "out vec4 output_color;\n"
                                                         "void main()\n"
                                                         "{\n"
                                                         "    vec4  total_color      = vec4(0.0);\n"
                                                         "    float image_resolution = float((textureSize(texture_sampler, 0)).x);\n"
                                                         "    float blur_step        = blur_radius / image_resolution;\n"
                                                         "    /* Calculate blurred colour. */\n"
                                                         "    /* Blur a texel on the right. */\n"
                                                         "    total_color = texture(texture_sampler, vec2(texture_coordinates.x + 1.0 * blur_step, texture_coordinates.y)) * gaussian_weights[0] +\n"
                                                         "                  texture(texture_sampler, vec2(texture_coordinates.x + 2.0 * blur_step, texture_coordinates.y)) * gaussian_weights[1] +\n"
                                                         "                  texture(texture_sampler, vec2(texture_coordinates.x + 3.0 * blur_step, texture_coordinates.y)) * gaussian_weights[2];\n"
                                                         "    /* Blur a texel on the left. */\n"
                                                         "    total_color += texture(texture_sampler, vec2(texture_coordinates.x - 1.0 * blur_step, texture_coordinates.y)) * gaussian_weights[0] +\n"
                                                         "                   texture(texture_sampler, vec2(texture_coordinates.x - 2.0 * blur_step, texture_coordinates.y)) * gaussian_weights[1] +\n"
                                                         "                   texture(texture_sampler, vec2(texture_coordinates.x - 3.0 * blur_step, texture_coordinates.y)) * gaussian_weights[2];\n"
                                                         "    /* Set the output colour. */\n"
                                                         "    output_color = vec4(total_color.xyz, 1.0);\n"
                                                         "}";
/* [Blur fragment shader source for horizontal blurring] */
/* [Blur fragment shader source for vertical blurring] */
static const char blurVerticalFragmentShaderSource[] =  "#version 300 es\n"
                                                        "precision mediump float;\n"
                                                        "/** Defines gaussian weights. */\n"
                                                        "const float gaussian_weights[] = float[] (0.2270270270,\n"
                                                        "                                          0.3162162162,\n"
                                                        "                                          0.0702702703);\n"
                                                        "/* UNIFORMS */\n"
                                                        "/** Radius of a blur effect to be applied. */\n"
                                                        "uniform float     blur_radius;\n"
                                                        "/** Texture sampler on which the effect will be applied. */\n"
                                                        "uniform sampler2D texture_sampler;\n"
                                                        "/* INPUTS */\n"
                                                        "/** Texture coordinates. */\n"
                                                        "in vec2 texture_coordinates;\n"
                                                        "/* OUTPUTS */\n"
                                                        "/** Fragment colour that will be returned. */\n"
                                                        "out vec4 output_color;\n"
                                                        "void main()\n"
                                                        "{\n"
                                                        "    vec4  total_color      = vec4(0.0);\n"
                                                        "    float image_resolution = float((textureSize(texture_sampler, 0)).y);\n"
                                                        "    float blur_step        = blur_radius / image_resolution;\n"
                                                        "    /* Calculate blurred colour. */\n"
                                                        "    /* Blur a texel to the top. */\n"
                                                        "    total_color = texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y + 1.0 * blur_step)) * gaussian_weights[0] +\n"
                                                        "                  texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y + 2.0 * blur_step)) * gaussian_weights[1] +\n"
                                                        "                  texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y + 3.0 * blur_step)) * gaussian_weights[2];\n"
                                                        "    /* Blur a texel to the bottom. */\n"
                                                        "    total_color += texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y - 1.0 * blur_step)) * gaussian_weights[0] +\n"
                                                        "                   texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y - 2.0 * blur_step)) * gaussian_weights[1] +\n"
                                                        "                   texture(texture_sampler, vec2(texture_coordinates.x, texture_coordinates.y - 3.0 * blur_step)) * gaussian_weights[2];\n"
                                                        "    /* Set the output colour. */\n"
                                                        "    output_color = vec4(total_color.xyz, 1.0);\n"
                                                        "}";
/* [Blur fragment shader source for vertical blurring] */
/* [Get luminance image fragment shader source] */
static const char getLuminanceImageFragmentShaderSource[] = "#version 300 es\n"
                                                            "precision highp float;\n"
                                                            "/* UNIFORMS */\n"
                                                            "uniform sampler2D texture_sampler;\n"
                                                            "/* INPUTS */\n"
                                                            "in vec2 texture_coordinates;\n"
                                                            "/* OUTPUTS */\n"
                                                            "out vec4 scene_color;\n"
                                                            "#define MIN_LUMINANCE (0.9)\n"
                                                            "void main()\n"
                                                            "{\n"
                                                            "    vec4  sample_color = texture(texture_sampler, texture_coordinates);\n"
                                                            "    float luminance    = 0.2125 * sample_color.x +\n"
                                                            "                         0.7154 * sample_color.y +\n"
                                                            "                         0.0721 * sample_color.z;\n"
                                                            "    if (luminance > MIN_LUMINANCE)\n"
                                                            "    {\n"
                                                            "        scene_color = sample_color;\n"
                                                            "    }\n"
                                                            "    else\n"
                                                            "    {\n"
                                                            "        scene_color =  vec4(0.0);\n"
                                                            "    }\n"
                                                            "}";
/* [Get luminance image fragment shader source] */
/* [Render scene fragment shader source] */
static const char renderSceneFragmentShaderSource[] =   "#version 300 es\n"
                                                        "precision lowp float;\n"
                                                        "/** Defines epsilon used for float values comparison. */\n"
                                                        "#define EPSILON (0.00001)\n"
                                                        "/** Structure holding light properties. */\n"
                                                        "struct _light_properties\n"
                                                        "{\n"
                                                        "    vec3  ambient;\n"
                                                        "    vec3  color;\n"
                                                        "    float constant_attenuation;\n"
                                                        "    float linear_attenuation;\n"
                                                        "    vec3  position;\n"
                                                        "    float quadratic_attenauation;\n"
                                                        "    float shininess;\n"
                                                        "    float strength;\n"
                                                        "};\n"
                                                        "/* UNIFORMS */\n"
                                                        "/** Camera position in a space. */\n"
                                                        "uniform vec3              camera_position;\n"
                                                        "/** Directional light properties. */\n"
                                                        "uniform _light_properties light_properties;\n"
                                                        "/* INPUTS */\n"
                                                        "/** Vertex normal. */\n"
                                                        "     in vec3 normal;\n"
                                                        "/** Vertex coordinates. */\n"
                                                        "     in vec4 vertex;\n"
                                                        "/** Indicates whether a cube is placed on diagonal. */\n"
                                                        "flat in int  is_cube_placed_on_diagonal;\n"
                                                        "/* OUTPUTS */\n"
                                                        "/* Stores scene colour.*/\n"
                                                        "out vec4 scene_color;\n"
                                                        "void main()\n"
                                                        "{\n"
                                                        "    vec4  dark_cube_colour   = vec4(0.2, 0.4, 0.8, 1.0);\n"
		                                                "    vec4  light_cube_colour  = vec4(1.0);\n"
                                                        "    vec3  normalized_normals = normalize(normal);\n"
                                                        "    vec3  light_direction    = normalize(vec3(light_properties.position - vertex.xyz));\n"
                                                        "    float attenuation        = 1.0 / (light_properties.constant_attenuation + (light_properties.linear_attenuation + light_properties.quadratic_attenauation));\n"
                                                        "    vec3  camera_direction   = camera_position - vec3(vertex);\n"
                                                        "    float diffuse            = max(0.0, dot(normalized_normals, light_direction));\n"
                                                        "    vec3  half_vector        = normalize(light_direction + camera_direction);\n"
                                                        "    float specular           = 0.0;\n"
                                                        "    if (abs(diffuse - 0.0) > EPSILON)\n"
                                                        "    {\n"
                                                        "        specular = max(0.0, dot(half_vector, normal));\n"
                                                        "        specular = pow(specular, light_properties.shininess) * light_properties.strength;\n"
                                                        "    }\n"
                                                        "    vec3 scattered_light                = light_properties.ambient * attenuation + diffuse * attenuation * light_properties.color;\n"
                                                        "    vec3 reflected_light                = light_properties.color   * specular              * attenuation;\n"
                                                        "    vec3 dark_cube_colour_with_lighting = min(dark_cube_colour.xyz     * scattered_light       + reflected_light, vec3(1.0) );\n"
                                                        "    /* If we are dealing with a cube placed on a diagonal, use white colour.\n"
                                                        "     * Otherwise, we want to output a regular cube (which means the previously\n"
                                                        "     * calculated cube colour with lighting applied). */\n"
                                                        "    if (is_cube_placed_on_diagonal == 1)\n"
                                                        "    {\n"
                                                        "        scene_color = light_cube_colour;\n"
                                                        "    }\n"
                                                        "    else\n"
                                                        "    {\n"
                                                        "        scene_color = vec4(dark_cube_colour_with_lighting, 1.0);\n"
                                                        "    }\n"
                                                        "}\n";
/* [Render scene fragment shader source] */
/* [Render scene vertex shader source] */
static const char renderSceneVertexShaderSource[] =     "#version 300 es\n"
                                                        "precision mediump float;\n"
                                                        "/** Defines number of cubes that will be rendered. */\n"
                                                        "#define NUMBER_OF_CUBES (25)\n"
                                                        "/** Array holding information whether a cube is placed on diagonal (1) or not (0). */\n"
                                                        "const int is_diagonal_cube[NUMBER_OF_CUBES] = int[NUMBER_OF_CUBES](1, 0, 0, 0, 1,\n"
                                                        "                                                                   0, 1, 0, 1, 0,\n"
                                                        "                                                                   0, 0, 1, 0, 0,\n"
                                                        "                                                                   0, 1, 0, 1, 0,\n"
                                                        "                                                                   1, 0, 0, 0, 1);\n"
                                                        "/* UNIFORMS */\n"
                                                        "/** Model * View matrix. */\n"
                                                        "uniform mat4 mv_matrix;\n"
                                                        "/** Model * View * Projection matrix. */\n"
                                                        "uniform mat4 mvp_matrix;\n"
                                                        "/** Cubes' properties. */\n"
                                                        "uniform      cube_properties\n"
                                                        "{\n"
                                                        "    /** Cubes' locations in a space. */\n"
                                                        "    vec2 locations[NUMBER_OF_CUBES];\n"
                                                        "};\n"
                                                        "/* ATTRIBUTES */\n"
                                                        "/** Cube vertex coordinates. */\n"
                                                        "in vec3 cube_vertex_coordinates;\n"
                                                        "/** Cube vertex normals. */\n"
                                                        "in vec3 cube_vertex_normals;\n"
                                                        "/* OUTPUTS */\n"
                                                        "/** Cube vertex normals in eye space. */\n"
                                                        "     out vec3 normal;\n"
                                                        "/** Cube vertex coordinates in eye space. */\n"
                                                        "     out vec4 vertex;\n"
                                                        "/** 1, if cube is placed on diagonal, 0 otherwise. */\n"
                                                        "flat out int  is_cube_placed_on_diagonal;\n"
                                                        "void main()\n"
                                                        "{\n"
                                                        "    /* Prepare translation matrix. */\n"
                                                        "    mat4 cube_location_matrix = mat4(1.0,                        0.0,                        0.0, 0.0,\n"
                                                        "                                     0.0,                        1.0,                        0.0, 0.0,\n"
                                                        "                                     0.0,                        0.0,                        1.0, 0.0,\n"
                                                        "                                     locations[gl_InstanceID].x, locations[gl_InstanceID].y, 0.0, 1.0);\n"
                                                        "    /* Calculate matrices. */\n"
                                                        "    mat4 model_view_matrix            = mv_matrix  * cube_location_matrix;\n"
                                                        "    mat4 model_view_projection_matrix = mvp_matrix * cube_location_matrix;\n"
                                                        "    /* Set output values. */\n"
                                                        "    is_cube_placed_on_diagonal = is_diagonal_cube[gl_InstanceID];\n"
                                                        "    normal                     = vec3(model_view_matrix * vec4(cube_vertex_normals, 0.0)).xyz;\n"
                                                        "    vertex                     = model_view_matrix * vec4(cube_vertex_coordinates, 1.0);\n"
                                                        "    /* Set vertex position in NDC space. */\n"
                                                        "    gl_Position = model_view_projection_matrix * vec4(cube_vertex_coordinates, 1.0);\n"
                                                        "}\n";
/* [Render scene vertex shader source] */
/* [Texture rendering vertex shader] */
static const char renderTextureVertexShaderSource[] =   "#version 300 es\n"
                                                        "precision mediump float;\n"
                                                        "/** GL_TRIANGLE_FAN-type quad vertex data. */\n"
                                                        "const vec4 vertex_positions[4] = vec4[4](vec4( 1.0, -1.0, 0.0, 1.0),\n"
                                                        "                                         vec4(-1.0, -1.0, 0.0, 1.0),\n"
                                                        "                                         vec4(-1.0,  1.0, 0.0, 1.0),\n"
                                                        "                                         vec4( 1.0,  1.0, 0.0, 1.0) );\n"
                                                        "/** Texture UVs. */\n"
                                                        "const vec2 texture_uv[4]       = vec2[4](vec2(1.0, 0.0),\n"
                                                        "                                         vec2(0.0, 0.0),\n"
                                                        "                                         vec2(0.0, 1.0),\n"
                                                        "                                         vec2(1.0, 1.0) );\n"
                                                        "/* OUTPUTS */\n"
                                                        "/** Texture coordinates. */\n"
                                                        "out vec2 texture_coordinates;\n"
                                                        "void main()\n"
                                                        "{\n"
                                                        "    /* Return vertex coordinates. */\n"
                                                        "    gl_Position         = vertex_positions[gl_VertexID];\n"
                                                        "    /* Pass texture coordinated to fragment shader. */\n"
                                                        "    texture_coordinates = texture_uv[gl_VertexID];\n"
                                                        "}\n";
/* [Texture rendering vertex shader] */

/* Variables used for scene view configurations. */
Matrix      cameraLookAtMatrix;
Vec3f       cameraPosition;
Matrix      cameraProjectionMatrix;
Matrix      cameraViewMatrix;
Matrix      cameraViewProjectionMatrix;
const float farPlane                    = 100.f;
Vec3f       lightPosition               = {0.0f, 0.0f, 10.0f};
const Vec3f lookAtPoint                 = {0.0f, 0.0f, 0.0f};
int         max_window_dimension        = 0;
int         min_window_dimension        = 0;
const float nearPlane                   = 0.01f;
const Vec3f upVector                    = {0.0f, 1.0f, 0.0f};
int         windowHeight                = 0;
int         windowWidth                 = 0;

/* Number of blur loop iterations. */
int lastNumberOfIterations = 0;

/* Variables used for rendering a geometry. */
GLfloat* cubeCoordinates    = NULL;
GLfloat* cubeLocations      = NULL;
GLfloat* cubeNormals        = NULL;
int      nOfCubeCoordinates = 0;
int      nOfCubeLocations   = 0;
int      nOfCubeNormals     = 0;

/* Variables used for program object configurations. */
BlendingProgramLocations       blendingProgramLocations;
ProgramAndShadersIds           blendingProgramShaderObjects;
BlurringProgramLocations       blurringHorizontalProgramLocations;
ProgramAndShadersIds           blurringHorizontalProgramShaderObjects;
BlurringProgramLocations       blurringVerticalProgramLocations;
ProgramAndShadersIds           blurringVerticalProgramShaderObjects;
ProgramAndShadersIds           getLuminanceImageProgramShaderObjects;
SceneRenderingProgramLocations sceneRenderingProgramLocations;
ProgramAndShadersIds           sceneRenderingProgramShaderObjects;

/* Variables used to store generated objects IDs. */
BlurringObjects               blurringObjects;
GetLuminanceImageBloomObjects getLuminanceImageBloomObjects;
SceneRenderingObjects         sceneRenderingObjects;
StrongerBlurObjects           strongerBlurObjects;

/** \brief Delete objects which were generated for blurring purposes.
 *         According to the OpenGL ES specification, objects will not be deleted if bound.
 *         It is the user's responsibility to call glBindBuffer(), glBindFramebuffer()
 *         and glBindTexture() with default object ids (id = 0) at some point.
 *
 * \param objectIdsStoragePtr Objects described by the structure will be deleted by the function.
 *                            Cannot be NULL.
 */
static void deleteBlurringObjects(BlurringObjects* objectIdsStoragePtr)
{
    ASSERT(objectIdsStoragePtr != NULL);

    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectIdHorizontal) );
    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectIdVertical) );
    GL_CHECK(glDeleteFramebuffers(1, &objectIdsStoragePtr->framebufferObjectId) );

    objectIdsStoragePtr->textureObjectIdHorizontal = 0;
    objectIdsStoragePtr->textureObjectIdVertical   = 0;
    objectIdsStoragePtr->framebufferObjectId       = 0;
}

/** \brief Delete objects which were generated for getting downscaled luminance image.
 *         According to the OpenGL ES specification, objects will not be deleted if bound.
 *         It is the user's responsibility to call glBindFramebuffer() and glBindTexture()
 *         with default object ids (id = 0) at some point.
 *
 * \param objectIdsStoragePtr Objects described by the structure will be deleted by the function.
 *                            Cannot be NULL.
 */
static void deleteGetLuminanceImageBloomObjects(GetLuminanceImageBloomObjects* objectIdsStoragePtr)
{
    ASSERT(objectIdsStoragePtr != NULL);

    GL_CHECK(glDeleteFramebuffers(1, &objectIdsStoragePtr->framebufferObjectId) );
    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectId) );

    objectIdsStoragePtr->framebufferObjectId = 0;
    objectIdsStoragePtr->textureObjectId     = 0;
}

/**  \brief Delete program and shader objects.
 *          According to the OpenGL ES specification, program object will not be deleted if it is active.
 *          It is the user's responsibility to call glUseProgram(0) at some point.
 *
 * \param objectsToBeDeletedPtr Objects described by the structure will be deleted by the function.
 *                              Cannot be NULL.
 */
static void deleteProgramShaderObjects(ProgramAndShadersIds* objectsToBeDeletedPtr)
{
    ASSERT(objectsToBeDeletedPtr != NULL);

    GL_CHECK(glDeleteShader (objectsToBeDeletedPtr->fragmentShaderObjectId) );
    GL_CHECK(glDeleteShader (objectsToBeDeletedPtr->vertexShaderObjectId) );
    GL_CHECK(glDeleteProgram(objectsToBeDeletedPtr->programObjectId) );

    objectsToBeDeletedPtr->fragmentShaderObjectId = 0;
    objectsToBeDeletedPtr->vertexShaderObjectId   = 0;
    objectsToBeDeletedPtr->programObjectId        = 0;
}

/** \brief Delete objects which were generated for scene rendering purposes.
 *         According to the OpenGL ES specification, objects will not be deleted if bound.
 *         It is the user's responsibility to call glBindBuffer(), glBindFramebuffer()
 *         and glBindTexture() with default object ids (id = 0) at some point.
 *
 * \param objectIdsStoragePtr Objects described by the structure will be deleted by the function.
 *                            Cannot be NULL.
 */
static void deleteSceneRenderingObjects(SceneRenderingObjects* objectIdsStoragePtr)
{
    ASSERT(objectIdsStoragePtr != NULL);

    GL_CHECK(glDeleteBuffers     (1, &objectIdsStoragePtr->bufferObjectIdCubeCoords) );
    GL_CHECK(glDeleteBuffers     (1, &objectIdsStoragePtr->bufferObjectIdCubeNormals) );
    GL_CHECK(glDeleteBuffers     (1, &objectIdsStoragePtr->bufferObjectIdElementLocations) );
    GL_CHECK(glDeleteFramebuffers(1, &objectIdsStoragePtr->framebufferObjectId) );
    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectIdDepthImage) );
    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectIdOriginalImage) );

    objectIdsStoragePtr->bufferObjectIdCubeCoords       = 0;
    objectIdsStoragePtr->bufferObjectIdCubeNormals      = 0;
    objectIdsStoragePtr->bufferObjectIdElementLocations = 0;
    objectIdsStoragePtr->framebufferObjectId            = 0;
    objectIdsStoragePtr->textureObjectIdDepthImage      = 0;
    objectIdsStoragePtr->textureObjectIdOriginalImage   = 0;
}

/** \brief Delete objects which were generated for performing the stronger blur effect on downscaled textures.
 *         According to the OpenGL ES specification, objects will not be deleted if bound.
 *         It is the user's responsibility to call glBindFramebuffer() and glBindTexture()
 *         with default object ids (id = 0) at some point.
 *
 * \param objectIdsStoragePtr Objects described by the structure will be deleted by the function.
 *                            Cannot be NULL.
 */
static void deleteStrongerBlurObjects(StrongerBlurObjects* objectIdsStoragePtr)
{
    ASSERT(objectIdsStoragePtr != NULL);

    GL_CHECK(glDeleteFramebuffers(1, &objectIdsStoragePtr->framebufferObjectId) );
    GL_CHECK(glDeleteTextures    (1, &objectIdsStoragePtr->textureObjectId) );

    objectIdsStoragePtr->framebufferObjectId = 0;
    objectIdsStoragePtr->textureObjectId     = 0;
}

/** \brief Generate texture and framebuffer objects, configure texture parameters.
 *         Finally, reset GL_TEXTURE_2D texture binding to 0 for active texture unit.
 *         Objects will be used for applying blur effect.
 *
 *  \param framebufferObjectIdPtr       Deref will be used to store generated framebuffer object ID.
 *                                      Cannot be NULL.
 *  \param horizontalTextureObjectIdPtr Deref will be used to store generated horizontal texture object ID.
 *                                      Cannot be NULL.
 *  \param verticalTextureObjectIdPtr   Deref will be used to store generated vertical texture object ID.
 *                                      Cannot be NULL.
 */
static void generateAndPrepareObjectsUsedForBlurring(GLuint* framebufferObjectIdPtr,
                                                     GLuint* horizontalTextureObjectIdPtr,
                                                     GLuint* verticalTextureObjectIdPtr)
{
    ASSERT(framebufferObjectIdPtr       != NULL);
    ASSERT(horizontalTextureObjectIdPtr != NULL);
    ASSERT(verticalTextureObjectIdPtr   != NULL);

    /* Generate objects. */
    GL_CHECK(glGenFramebuffers(1,
                               framebufferObjectIdPtr) );
    GL_CHECK(glGenTextures    (1,
                               horizontalTextureObjectIdPtr) );
    GL_CHECK(glGenTextures    (1,
                               verticalTextureObjectIdPtr) );

    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                            *horizontalTextureObjectIdPtr) );
    GL_CHECK(glTexImage2D   (GL_TEXTURE_2D,
                             0,
                             GL_RGBA8,
                             windowWidth  / WINDOW_RESOLUTION_DIVISOR,
                             windowHeight / WINDOW_RESOLUTION_DIVISOR,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             NULL) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR) );

    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                            *verticalTextureObjectIdPtr) );
    GL_CHECK(glTexImage2D   (GL_TEXTURE_2D,
                             0,
                             GL_RGBA8,
                             windowWidth  / WINDOW_RESOLUTION_DIVISOR,
                             windowHeight / WINDOW_RESOLUTION_DIVISOR,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             NULL) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR) );

    /* At the end, restore default environment settings (bind default TO). */
    GL_CHECK(glBindTexture(GL_TEXTURE_2D,
                           0) );
}

/** \brief Generate texture and framebuffer objects and configure texture parameters accordingly.
 *         Finally, reset GL_FRAEMBUFFER and GL_TEXTURE_2D bindings to 0.
 *         Objects will be used for rendering the scene into texture.
 *
 *  \param framebufferObjectIdPtr     Deref will be used to store generated framebuffer object ID.
 *                                    Cannot be NULL.
 *  \param originalTextureObjectIdPtr Deref will be used to store generated original texture object ID.
 *                                    Cannot be NULL.
 *  \param depthToIdPtr               Deref will be used to store generated depth texture object ID.
 *                                    Cannot be NULL.
 */
static void generateAndPrepareObjectsUsedForSceneRendering(GLuint* framebufferObjectIdPtr,
                                                           GLuint* originalTextureObjectIdPtr,
                                                           GLuint* depthToIdPtr)
{
    ASSERT(framebufferObjectIdPtr    != NULL);
    ASSERT(originalTextureObjectIdPtr!= NULL);
    ASSERT(depthToIdPtr              != NULL);

    /* [Generate Objects For Scene Rendering] */
    /* Generate objects. */
    GL_CHECK(glGenFramebuffers(1,
                               framebufferObjectIdPtr) );
    GL_CHECK(glGenTextures    (1,
                               originalTextureObjectIdPtr) );
    GL_CHECK(glGenTextures    (1,
                               depthToIdPtr) );

    /* Bind generated framebuffer and texture objects to specific binding points. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,
                              *framebufferObjectIdPtr) );

    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                            *originalTextureObjectIdPtr) );
    GL_CHECK(glTexImage2D   (GL_TEXTURE_2D,
                             0,
                             GL_RGBA8,
                             windowWidth,
                             windowHeight,
                             0,
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             NULL) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR) );

    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                            *depthToIdPtr) );
    GL_CHECK(glTexImage2D   (GL_TEXTURE_2D,
                             0,
                             GL_DEPTH_COMPONENT32F,
                             windowWidth,
                             windowHeight,
                             0,
                             GL_DEPTH_COMPONENT,
                             GL_FLOAT,
                             NULL) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_NEAREST) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_NEAREST) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE) );
    /* [Generate Objects For Scene Rendering] */

   /* [Bind Textures to Framebuffer] */
    /* Bind colour and depth textures to framebuffer object. */
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D,
                                   *originalTextureObjectIdPtr,
                                    0) );
    GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER,
                                    GL_DEPTH_ATTACHMENT,
                                    GL_TEXTURE_2D,
                                   *depthToIdPtr,
                                    0) );
    /* [Bind Textures to Framebuffer] */

    /* At the end, restore default environment settings (bind default FBO and TO). */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,
                               0) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
}

/* [Generate downscaled objects] */
/** \brief Generate texture and framebuffer objects and configure texture parameters accordingly.
 *         Finally, reset GL_FRAEMBUFFER and GL_TEXTURE_2D bindings to 0.
 *         Texture size is equal to window resolution / WINDOW_RESOLUTION_DIVISOR.
 *
 *  \param fboIdPtr Deref will be used to store generated framebuffer object ID.
 *                  Cannot be NULL.
 *  \param toIdPtr  Deref will be used to store generated texture object ID.
 *                  Cannot be NULL.
 */
static void generateDownscaledObjects(GLuint* fboIdPtr,
                                      GLuint* toIdPtr)
{
    ASSERT(fboIdPtr != NULL);
    ASSERT(toIdPtr  != NULL);

    /* Generate objects. */
    GL_CHECK(glGenFramebuffers(1,
                               fboIdPtr) );
    GL_CHECK(glGenTextures    (1,
                               toIdPtr) );

    /* Set texture parameters. */
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                            *toIdPtr) );
    GL_CHECK(glTexStorage2D (GL_TEXTURE_2D,
                             1,
                             GL_RGBA8,
                             windowWidth   / WINDOW_RESOLUTION_DIVISOR,
                             windowHeight / WINDOW_RESOLUTION_DIVISOR));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_S,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_WRAP_T,
                             GL_CLAMP_TO_EDGE) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MAG_FILTER,
                             GL_LINEAR) );
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D,
                             GL_TEXTURE_MIN_FILTER,
                             GL_LINEAR) );

    /* Make framebuffer object active and bind texture object to it. */
    GL_CHECK(glBindFramebuffer     (GL_FRAMEBUFFER,
                                   *fboIdPtr) );
    GL_CHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                    GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_2D,
                                   *toIdPtr,
                                    0) );

    /* Restore default bindings. */
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,
                               0) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
}
/* [Generate downscaled objects] */

/* [Calculate cube locations] */
/** \brief Calculate the world space locations of all the cubes that we will be rendering.
 *         The cubes are arranged in a 2D array consisting of \p numberOfColumns columns
 *         and \p numberOfRows rows with the requested space between cubes.
 *         It is the user's responsibility to free allocated memory.
 *
 * \param numberOfColumns                        Number of columns that cubes will be placed in.
 * \param numberOfRows                           Number of rows that cubes will be placed in.
 * \param cubeScalar                             Cube size scalar.
 * \param distanceBetweenCubes                   Distance between cubes.
 * \param numberOfCubeLocationCoordinatesPtr     Deref will be used to store number of generated cubes.
 *                                               Cannot be NULL.
 *
 * \return Pointer to calculated cube positions if successful, NULL otherwise.
 */
static GLfloat* getCubeLocations(GLint   numberOfColumns,
                                 GLint   numberOfRows,
                                 GLfloat cubeScalar,
                                 GLfloat distanceBetweenCubes,
                                 GLint*  numberOfCubeLocationCoordinatesPtr)
{
    ASSERT(numberOfCubeLocationCoordinatesPtr != NULL);

    const float distance                 = distanceBetweenCubes + 2 * cubeScalar; /* A single cube spreads out from
                                                                                   * <-cubeScalar, -cubeScalar, -cubeScalar> to
                                                                                   * <cubeScalar,   cubeScalar,  cubeScalar>,
                                                                                   * with <0, 0, 0> representing the center of the cube.
                                                                                   * We have to enlarge the requested distance between cubes
                                                                                   * (2 * cubeScalar) times. */
    int         index                           = 0;
    int         numberOfCubeLocationCoordinates = 0;
    const int   numberOfPointCoordinates        = 2;
    GLfloat*    result                          = NULL;
    const float xStart                          = -( float(numberOfColumns - 1) / 2.0f * distance);
    const float yStart                          = -( float(numberOfRows    - 1) / 2.0f * distance);

    numberOfCubeLocationCoordinates = numberOfPointCoordinates * numberOfColumns * numberOfRows;
    result                          = (GLfloat*) malloc(numberOfCubeLocationCoordinates * sizeof(GLfloat) );

    /* Make sure memory allocation succeeded. */
    ASSERT(result != NULL);

    for (int rowIndex = 0; rowIndex < numberOfRows; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < numberOfColumns; columnIndex++)
        {
            result[index++] = xStart + (rowIndex    * distance);
            result[index++] = yStart + (columnIndex * distance);
        }
    }

    *numberOfCubeLocationCoordinatesPtr = numberOfCubeLocationCoordinates;

    return result;
}
/* [Calculate cube locations] */

/** \brief Retrieve the locations of uniforms for the program object responsible for applying the blend effect.
 *         Can be called only if \p programObjectId is currently active.
 *
 * \param programObjectId     A valid program object ID. Indicates the program object for which locations are queried.
 * \param locationsStoragePtr Deref will be used to store retrieved info.
 *                            Cannot be NULL.
 */
static void getLocationsForBlendingProgram(GLuint                    programObjectId,
                                           BlendingProgramLocations* locationsStoragePtr)
{
    ASSERT(locationsStoragePtr != NULL);
    ASSERT(programObjectId     != 0);

    locationsStoragePtr->uniformMixFactor           = GL_CHECK(glGetUniformLocation(programObjectId, "mix_factor") );
    locationsStoragePtr->uniformOriginalTexture     = GL_CHECK(glGetUniformLocation(programObjectId, "original_texture") );
    locationsStoragePtr->uniformStrongerBlurTexture = GL_CHECK(glGetUniformLocation(programObjectId, "stronger_blur_texture") );
    locationsStoragePtr->uniformWeakerBlurTexture   = GL_CHECK(glGetUniformLocation(programObjectId, "weaker_blur_texture") );

    ASSERT(locationsStoragePtr->uniformMixFactor           != -1);
    ASSERT(locationsStoragePtr->uniformOriginalTexture     != -1);
    ASSERT(locationsStoragePtr->uniformStrongerBlurTexture != -1);
    ASSERT(locationsStoragePtr->uniformWeakerBlurTexture   != -1);
}

/** \brief Retrieve the locations of uniforms for the program object responsible for applying the blur effect.
 *         Can be called only if \p programObjectId is currently active.
 *
 * \param programObjectId     A valid program object ID. Indicates the program object for which locations are queried.
 * \param locationsStoragePtr Deref will be used to store retrieved info.
 *                            Cannot be NULL.
 */
static void getLocationsForBlurringProgram(GLuint                    programObjectId,
                                           BlurringProgramLocations* locationsStoragePtr)
{
    ASSERT(locationsStoragePtr != NULL);
    ASSERT(programObjectId     != 0);

    locationsStoragePtr->uniformBlurRadius       = GL_CHECK(glGetUniformLocation(programObjectId, "blur_radius") );
    locationsStoragePtr->uniformTextureSampler   = GL_CHECK(glGetUniformLocation(programObjectId, "texture_sampler") );

    ASSERT(locationsStoragePtr->uniformBlurRadius       != -1);
    ASSERT(locationsStoragePtr->uniformTextureSampler   != -1);
}

/** \brief Retrieve the locations of attributes and uniforms for the program object responsible for scene rendering.
 *         Can be called only if \p programObjectId is currently active.
 *
 * \param programObjectId     A valid program object ID. Indicates the program object for which locations are queried.
 * \param locationsStoragePtr Deref will be used to store retrieved info.
 *                            Cannot be NULL.
 */
static void getLocationsForSceneRenderingProgram(GLuint                          programObjectId,
                                                 SceneRenderingProgramLocations* locationsStoragePtr)
{
    ASSERT(locationsStoragePtr != NULL);
    ASSERT(programObjectId     != 0);

    /* [Get cube vertex coordinates attrib location] */
    locationsStoragePtr->attribCubeVertexCoordinates                 = GL_CHECK(glGetAttribLocation   (programObjectId,
                                                                                                       "cube_vertex_coordinates") );
    /* [Get cube vertex coordinates attrib location] */
    locationsStoragePtr->attribCubeVertexNormals                     = GL_CHECK(glGetAttribLocation   (programObjectId,
                                                                                                       "cube_vertex_normals") );
    /* [Get cube locations uniform block location] */
    locationsStoragePtr->uniformBlockCubeProperties                  = GL_CHECK(glGetUniformBlockIndex(programObjectId,
                                                                                                       "cube_properties") );
    /* [Get cube locations uniform block location] */
    locationsStoragePtr->uniformCameraPosition                       = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "camera_position") );
    locationsStoragePtr->uniformLightPropertiesAmbient               = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.ambient") );
    locationsStoragePtr->uniformLightPropertiesColor                 = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.color") );
    locationsStoragePtr->uniformLightPropertiesConstantAttenuation   = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.constant_attenuation") );
    locationsStoragePtr->uniformLightPropertiesLinearAttenuation     = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.linear_attenuation") );
    locationsStoragePtr->uniformLightPropertiesPosition              = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.position") );
    locationsStoragePtr->uniformLightPropertiesQuadraticAttenauation = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.quadratic_attenauation") );
    locationsStoragePtr->uniformLightPropertiesShininess             = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.shininess") );
    locationsStoragePtr->uniformLightPropertiesStrength              = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "light_properties.strength") );
    locationsStoragePtr->uniformMvMatrix                             = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "mv_matrix") );
    locationsStoragePtr->uniformMvpMatrix                            = GL_CHECK(glGetUniformLocation  (programObjectId,
                                                                                                       "mvp_matrix") );

    ASSERT(locationsStoragePtr->attribCubeVertexCoordinates                 != -1);
    ASSERT(locationsStoragePtr->attribCubeVertexNormals                     != -1);
    ASSERT(locationsStoragePtr->uniformBlockCubeProperties                  != GL_INVALID_INDEX);
    ASSERT(locationsStoragePtr->uniformCameraPosition                       != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesAmbient               != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesColor                 != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesConstantAttenuation   != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesLinearAttenuation     != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesPosition              != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesQuadraticAttenauation != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesShininess             != -1);
    ASSERT(locationsStoragePtr->uniformLightPropertiesStrength              != -1);
    ASSERT(locationsStoragePtr->uniformMvMatrix                             != -1);
    ASSERT(locationsStoragePtr->uniformMvpMatrix                            != -1);
}

/** \brief Create and compile shader objects.
 *         If successful, they are attached to the program object, which is then linked.
 *
 * \param objectIdsPtr         Deref where generated program and shader objects IDs will be stored.
 *                             Cannot be NULL.
 * \param fragmentShaderSource Fragment shader source code.
 *                             Cannot be NULL.
 * \param  vertexShaderSource  Vertex shader source code.
 *                             Cannot be NULL.
 */
static void initializeProgramObject(ProgramAndShadersIds* objectIdsPtr,
                                    const char*           fragmentShaderSource,
                                    const char*           vertexShaderSource)
{
    ASSERT(objectIdsPtr != NULL);

    GLint linkStatus = 0;

    objectIdsPtr->programObjectId = GL_CHECK(glCreateProgram() );

    Shader::processShader(&objectIdsPtr->fragmentShaderObjectId,
                           fragmentShaderSource,
                           GL_FRAGMENT_SHADER);
    Shader::processShader(&objectIdsPtr->vertexShaderObjectId,
                           vertexShaderSource,
                           GL_VERTEX_SHADER);

    GL_CHECK(glAttachShader(objectIdsPtr->programObjectId, objectIdsPtr->fragmentShaderObjectId) );
    GL_CHECK(glAttachShader(objectIdsPtr->programObjectId, objectIdsPtr->vertexShaderObjectId) );

    GL_CHECK(glLinkProgram(objectIdsPtr->programObjectId) );

    GL_CHECK(glGetProgramiv(objectIdsPtr->programObjectId, GL_LINK_STATUS, &linkStatus) );

    ASSERT(linkStatus == GL_TRUE);
}

/* \brief Render the luminance image (which then can be bloomed) and store the result in corresponding texture object.
 */
static void renderDowscaledLuminanceTexture()
{
    /* [Render luminance image into downscaled texture] */
    /* Get the luminance image, store it in the downscaled texture. */
    GL_CHECK(glUseProgram(getLuminanceImageProgramShaderObjects.programObjectId));
    {
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                   getLuminanceImageBloomObjects.framebufferObjectId));
        /* Set the viewport for the whole screen size. */
        GL_CHECK(glViewport(0,
                            0,
                            windowWidth  / WINDOW_RESOLUTION_DIVISOR,
                            windowHeight / WINDOW_RESOLUTION_DIVISOR) );
        /* Clear the framebuffer's content. */
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
        /* Draw texture. */
        GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4) );
    }
    /* [Render luminance image into downscaled texture] */
}

/* \brief Render the scene and store the result in corresponding texture object.
 */
static void renderSceneColourTexture()
{
    /* [Render scene into texture objects] */
    /* Render scene.
    *  The scene is rendered to two render targets:
    *  - 1. First texture will store color data;
    *  - 2. Second texture will store color data, but only for the cubes that should be
    *       affected by the bloom operation (remaining objects will not be rendered).
    */
    GL_CHECK(glUseProgram(sceneRenderingProgramShaderObjects.programObjectId) );
    {
        /* Bind a framebuffer object to the GL_DRAW_FRAMEBUFFER framebuffer binding point,
        * so that everything we render will end up in the FBO's attachments. */
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                   sceneRenderingObjects.framebufferObjectId) );
        /* Set the viewport for the whole screen size. */
        GL_CHECK(glViewport(0,
                            0,
                            windowWidth,
                            windowHeight) );
        /* Clear the framebuffer's content. */
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
        /* [Instanced drawing] */
        /* Draw scene. */
        GL_CHECK(glDrawArraysInstanced(GL_TRIANGLES,
                                       0,
                                       nOfCubeCoordinates,
                                       NUMBER_OF_CUBES) );
        /* [Instanced drawing] */
    }
    /* [Render scene into texture objects] */
}

/** \brief Configure the scene rendering program's uniforms.
 *
 * \param locationsPtr   Pointer to structure object where uniform locations are stored.
 *                       Cannot be NULL.
 * \param mvMatrix       Model-View matrix.
 * \param mvpMatrix      Model-View-Projection matrix.
 * \param cameraPosition Camera (eye) position in space.
 * \param lightPosition  Light position in space.
 */
static void setUniformValuesForSceneRenderingProgram(const SceneRenderingProgramLocations* locationsPtr,
                                                           Matrix                          mvMatrix,
                                                           Matrix                          mvpMatrix,
                                                           Vec3f                           cameraPosition,
                                                           Vec3f                           lightPosition)
{
    ASSERT(locationsPtr != NULL);

    const Vec3f lightAmbient              = {0.3f, 0.3f, 0.3f};
    const Vec3f lightColor                = {1.0f, 1.0f, 1.0f};
    const float lightConstantAttenuation  = 0.9f;
    const float lightLinearAttenuation    = 0.0f;
    const float lightQuadraticAttenuation = 0.05f;
    const float lightShininess            = 0.1f;
    const float lightStrength             = 0.01f;
    bool        result                    = true;

    /* Set model-view-(projection) matrices. */
    GL_CHECK(glUniformMatrix4fv(locationsPtr->uniformMvMatrix,
                                1,
                                GL_FALSE,
                                mvMatrix.getAsArray() ) );
    GL_CHECK(glUniformMatrix4fv(locationsPtr->uniformMvpMatrix,
                                1,
                                GL_FALSE,
                                mvpMatrix.getAsArray() ) );

    /* Set light properties uniforms. */
    GL_CHECK(glUniform3f(locationsPtr->uniformCameraPosition,                       cameraPosition.x,
                                                                                    cameraPosition.y,
                                                                                    cameraPosition.z) );
    GL_CHECK(glUniform3f(locationsPtr->uniformLightPropertiesAmbient,               lightAmbient.x,
                                                                                    lightAmbient.y,
                                                                                    lightAmbient.z) );
    GL_CHECK(glUniform3f(locationsPtr->uniformLightPropertiesColor,                 lightColor.x,
                                                                                    lightColor.y,
                                                                                    lightColor.z) );
    GL_CHECK(glUniform1f(locationsPtr->uniformLightPropertiesConstantAttenuation,   lightConstantAttenuation) );
    GL_CHECK(glUniform1f(locationsPtr->uniformLightPropertiesLinearAttenuation,     lightLinearAttenuation) );
    GL_CHECK(glUniform3f(locationsPtr->uniformLightPropertiesPosition,              lightPosition.x,
                                                                                    lightPosition.y,
                                                                                    lightPosition.z) );
    GL_CHECK(glUniform1f(locationsPtr->uniformLightPropertiesQuadraticAttenauation, lightQuadraticAttenuation) );
    GL_CHECK(glUniform1f(locationsPtr->uniformLightPropertiesShininess,             lightShininess) );
    GL_CHECK(glUniform1f(locationsPtr->uniformLightPropertiesStrength,              lightStrength) );
}

/** \brief Setup the environment: create and prepare objects for rendering purposes.
 *
 *  \param width  Window resolution: width.
 *  \param height Window resolution: height.
 *
 *  \return True if successful, false otherwise.
 */
void setupGraphics(int width, int height)
{
    float cameraDepthPosition = 0.0f;

    /* Store window resolution. */
    windowHeight = height;
    windowWidth  = width;

    if (windowHeight > windowWidth)
    {
        /* We are dealing with vertical position of a screen used for rendering. */
        cameraDepthPosition  = VERTICAL_POSITION_CAMERA_DEPTH_LOCATION;
        max_window_dimension = windowHeight;
        min_window_dimension = windowWidth;
    }
    else
    {
        /* We are dealing with horizontal position of a screen used for rendering. */
        cameraDepthPosition  = HORIZONTAL_POSITION_CAMERA_DEPTH_LOCATION;
        max_window_dimension = windowWidth;
        min_window_dimension = windowHeight;
    }

    /* Get geometry needed to render a scene. We will be drawing a cube in multiple instances. */
    /* [Get cube triangle representation] */
    CubeModel::getTriangleRepresentation(CUBE_SCALAR, &nOfCubeCoordinates, &cubeCoordinates);
    /* [Get cube triangle representation] */
    /* [Get cube normals] */
    CubeModel::getNormals(&nOfCubeNormals, &cubeNormals);
    /* [Get cube normals] */

    cubeLocations = getCubeLocations(5, 5, CUBE_SCALAR, CUBE_SCALAR / 2.0f, &nOfCubeLocations);

    /* Configure camera view on a scene. */
    cameraPosition.x           = 0.0f;
    cameraPosition.y           = 0.0f;
    cameraPosition.z           = cameraDepthPosition;
    cameraLookAtMatrix         = Matrix::matrixCameraLookAt(cameraPosition, lookAtPoint, upVector);
    cameraProjectionMatrix     = Matrix::matrixPerspective(degreesToRadians(45.0f),
                                                   (float) windowWidth / windowHeight,
                                                           nearPlane,
                                                           farPlane);
    cameraViewMatrix           = cameraLookAtMatrix;
    cameraViewProjectionMatrix = cameraProjectionMatrix * cameraViewMatrix;

    /* Create program object responsible for scene rendering. */
    initializeProgramObject(&sceneRenderingProgramShaderObjects,
                             renderSceneFragmentShaderSource,
                             renderSceneVertexShaderSource);
    /* Create program object responsible for blending. */
    initializeProgramObject(&blendingProgramShaderObjects,
                             blendFragmentShaderSource,
                             renderTextureVertexShaderSource);
    /* Create program object responsible for blurring (horizontal blur). */
    initializeProgramObject(&blurringHorizontalProgramShaderObjects,
                             blurHorizontalFragmentShaderSource,
                             renderTextureVertexShaderSource );
    /* Create program object responsible for blurring (vertical blur). */
    initializeProgramObject(&blurringVerticalProgramShaderObjects,
                             blurVerticalFragmentShaderSource,
                             renderTextureVertexShaderSource );
    /* Create program object responsible for generating luminance image that will be then bloomed. */
    initializeProgramObject(&getLuminanceImageProgramShaderObjects,
                             getLuminanceImageFragmentShaderSource,
                             renderTextureVertexShaderSource);

    /* [Create cube vertices buffer object] */
    /* Generate buffer object and fill it with cube vertex coordinates data. */
    GL_CHECK(glGenBuffers(1,
                         &sceneRenderingObjects.bufferObjectIdCubeCoords) );
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          sceneRenderingObjects.bufferObjectIdCubeCoords) );
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          nOfCubeCoordinates * sizeof(GLfloat),
                (GLfloat*)cubeCoordinates,
                          GL_STATIC_DRAW) );
    /* [Create cube vertices buffer object] */

    /* Generate buffer object and fill it with cube normals data. */
    GL_CHECK(glGenBuffers(1,
                         &sceneRenderingObjects.bufferObjectIdCubeNormals) );
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER,
                          sceneRenderingObjects.bufferObjectIdCubeNormals) );
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
                          nOfCubeNormals * sizeof(GLfloat),
                (GLfloat*)cubeNormals,
                          GL_STATIC_DRAW) );

    /* [Uniform buffer with cube locations] */
    /* Generate uniform buffer object and fill it with cube positions data. */
    GL_CHECK(glGenBuffers(1,
                         &sceneRenderingObjects.bufferObjectIdElementLocations) );
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER,
                          sceneRenderingObjects.bufferObjectIdElementLocations) );
    GL_CHECK(glBufferData(GL_UNIFORM_BUFFER,
                          nOfCubeLocations * sizeof (GLfloat),
                          cubeLocations,
                          GL_STATIC_DRAW) );
    /* [Uniform buffer with cube locations] */

    /* Generate objects which will be used for scene rendering. */
    generateAndPrepareObjectsUsedForSceneRendering(&sceneRenderingObjects.framebufferObjectId,
                                                   &sceneRenderingObjects.textureObjectIdOriginalImage,
                                                   &sceneRenderingObjects.textureObjectIdDepthImage);

    /* Generate objects which will be used for applying blur effect. */
    generateAndPrepareObjectsUsedForBlurring(&blurringObjects.framebufferObjectId,
                                             &blurringObjects.textureObjectIdHorizontal,
                                             &blurringObjects.textureObjectIdVertical);
    /* Generate objects that will be used for rendering into downscaled texture. */
    generateDownscaledObjects(&getLuminanceImageBloomObjects.framebufferObjectId,
                              &getLuminanceImageBloomObjects.textureObjectId);
    generateDownscaledObjects(&strongerBlurObjects.framebufferObjectId,
                              &strongerBlurObjects.textureObjectId);

    /* Set attribute/uniform values for program object responsible for scene rendering. */
    GL_CHECK(glUseProgram(sceneRenderingProgramShaderObjects.programObjectId) );
    {
        /* Restore uniform locations for program object responsible for scene rendering. */
        getLocationsForSceneRenderingProgram(sceneRenderingProgramShaderObjects.programObjectId,
                                            &sceneRenderingProgramLocations);
        /* Set values for uniforms, which are constant during rendering process. */
        setUniformValuesForSceneRenderingProgram(&sceneRenderingProgramLocations,
                                                  cameraViewMatrix,
                                                  cameraViewProjectionMatrix,
                                                  cameraPosition,
                                                  lightPosition);
        /* [Uniform block settings] */
        /* Cube locations are constant during rendering process. Set them now. */
        GL_CHECK(glUniformBlockBinding(sceneRenderingProgramShaderObjects.programObjectId,
                                       sceneRenderingProgramLocations.uniformBlockCubeProperties,
                                       0 ) );
        GL_CHECK(glBindBufferBase     (GL_UNIFORM_BUFFER,
                                       0,
                                       sceneRenderingObjects.bufferObjectIdElementLocations) );
        /* [Uniform block settings] */
    }

    /* [Define vertex attrib data array] */
    /* Cube coordinates are constant during rendering process. Set them now. */
    GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                   sceneRenderingObjects.bufferObjectIdCubeCoords) );
    GL_CHECK(glVertexAttribPointer(sceneRenderingProgramLocations.attribCubeVertexCoordinates,
                                   NUMBER_OF_COMPONENTS_PER_VERTEX,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   0,
                                   NULL) );
    /* [Define vertex attrib data array] */
    GL_CHECK(glBindBuffer         (GL_ARRAY_BUFFER,
                                   sceneRenderingObjects.bufferObjectIdCubeNormals) );
    GL_CHECK(glVertexAttribPointer(sceneRenderingProgramLocations.attribCubeVertexNormals,
                                   NUMBER_OF_COMPONENTS_PER_VERTEX,
                                   GL_FLOAT,
                                   GL_FALSE,
                                   0,
                                   NULL) );

    /* Enable VAAs. */
    /* [Enable cube vertex coordinates attrib array] */
    GL_CHECK(glEnableVertexAttribArray(sceneRenderingProgramLocations.attribCubeVertexCoordinates) );
    /* [Enable cube vertex coordinates attrib array] */
    GL_CHECK(glEnableVertexAttribArray(sceneRenderingProgramLocations.attribCubeVertexNormals) );

    /* Retrieve uniform and attribute locations for program object responsible for applying blur effect. */
    GL_CHECK(glUseProgram(blurringHorizontalProgramShaderObjects.programObjectId) );
    {
        getLocationsForBlurringProgram(blurringHorizontalProgramShaderObjects.programObjectId,
                                      &blurringHorizontalProgramLocations);

        /* Set values for uniforms which are constant during rendering process. */
        GL_CHECK(glUniform1f(blurringHorizontalProgramLocations.uniformBlurRadius, BLUR_RADIUS) );
    }

    /* Retrieve uniform and attribute locations for program object responsible for applying blur effect. */
    GL_CHECK(glUseProgram(blurringVerticalProgramShaderObjects.programObjectId) );
    {
        getLocationsForBlurringProgram(blurringVerticalProgramShaderObjects.programObjectId,
                                      &blurringVerticalProgramLocations);

        /* Set values for uniforms which are constant during rendering process. */
        GL_CHECK(glUniform1f(blurringVerticalProgramLocations.uniformBlurRadius, BLUR_RADIUS) );
    }

    /* Retrieve uniform and attribute locations for program object responsible for applying blend effect. */
    GL_CHECK(glUseProgram(blendingProgramShaderObjects.programObjectId) );
    {
        getLocationsForBlendingProgram(blendingProgramShaderObjects.programObjectId,
                                      &blendingProgramLocations);

        /* Set values for uniforms which are constant during rendering process. */
        /* [Set original texture uniform value] */
        GL_CHECK(glUniform1i(blendingProgramLocations.uniformOriginalTexture,     TEXTURE_UNIT_COLOR_TEXTURE) );
        /* [Set original texture uniform value] */
        GL_CHECK(glUniform1i(blendingProgramLocations.uniformStrongerBlurTexture, TEXTURE_UNIT_STRONGER_BLUR_TEXTURE) );
        GL_CHECK(glUniform1i(blendingProgramLocations.uniformWeakerBlurTexture,   TEXTURE_UNIT_BLURRED_TEXTURE) );
    }

    /* Set up texture unit bindings. */
    /* [Bind colour texture object to specific binding point] */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_COLOR_TEXTURE) );
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             sceneRenderingObjects.textureObjectIdOriginalImage) );
    /* [Bind colour texture object to specific binding point] */
    /* [Bind bloom source texture object to specific binding point] */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_BLOOM_SOURCE_TEXTURE) );
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             getLuminanceImageBloomObjects.textureObjectId) );
    /* [Bind bloom source texture object to specific binding point] */
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_HORIZONTAL_BLUR_TEXTURE) );
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             blurringObjects.textureObjectIdHorizontal) );
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_BLURRED_TEXTURE) );
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             blurringObjects.textureObjectIdVertical) );
    GL_CHECK(glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_STRONGER_BLUR_TEXTURE) );
    GL_CHECK(glBindTexture  (GL_TEXTURE_2D,
                             strongerBlurObjects.textureObjectId) );

    /* Prepare for drawing. */
    GL_CHECK(glClearColor(0.0f,
                          0.0f,
                          0.0f,
                          1.0f) );
    GL_CHECK(glEnable    (GL_DEPTH_TEST) );

    GLint   x                = 0;
    GLint   y                = 0;
    GLsizei scissorBoxWidth  = min_window_dimension / WINDOW_RESOLUTION_DIVISOR;
    GLsizei scissorBoxHeight = min_window_dimension / WINDOW_RESOLUTION_DIVISOR;

    if (windowWidth > windowHeight)
    {
        x = (max_window_dimension - min_window_dimension) / 2 / WINDOW_RESOLUTION_DIVISOR;
        y = 0;
    }
    else
    {
        x = 0;
        y = (max_window_dimension - min_window_dimension) / 2 / WINDOW_RESOLUTION_DIVISOR;
    }

    GL_CHECK(glScissor(x, y, scissorBoxWidth, scissorBoxHeight));

    /* The model is not changing during the rendering process (the only thing that changes is the strength of the bloom effect).
     * That is why it is enough to render the scene and the luminance image only once and then use them as an input for blooming
     * and blurring functions in the next steps. */
    renderSceneColourTexture();
    renderDowscaledLuminanceTexture();
}

/** \brief Render one frame.
 *
 *  \param time Time passed from the beginning of the rendering to the current frame render request (in seconds).
 */
void renderFrame (float time)
{
    /* True:  if scene should be updated (number of blur iterations changes),
     * False: if there is no need to update the scene, but blend operations
     *        will use updated mix factor value.
     */
    bool shouldSceneBeUpdated = false;

    /* Variables used for blur effect calculations. */
    int       blurEffectDirection       = BLUR_EFFECT_INCREASE;
    int       currentNumberOfIterations = 0;
    float     mixFactor                 = 0.0f;
    const int numberOfBlurPasses        = (MAX_NUMBER_OF_BLUR_PASSES - MIN_NUMBER_OF_BLUR_PASSES + 1) * 2;
    int       nOfIterations             =  0;
    int       timeIntervalIndex         = 0;

    /* [Mix factor calculations] */
    /* Mix factor value is calculated for a specific frame (for a specific time).
     * - The number of blur passes varies from MIN_NUMBER_OF_BLUR_PASSES to MAX_NUMBER_OF_BLUR_PASSES
     *   and to MIN_NUMBER_OF_BLUR_PASSES again which indicates the constant animation of increasing
     *   and decreasing blur effect strength.
     * - For each frame (time) there is a check done to verify the current number of blur passes.
     * - Once we get the current number of blur passes, we have to calculate the mix factor:
     *   It is changing from 0 to 1 (if the blur effect is increasing) or from 1 to 0 (if the blur effect is decreasing).
     *   This value is set based on a time which passed from the beginning of current number of blur passes rendering in
     *   compare to the total time requested for changing this number.
     *
     *   The 'rendering frame for a specific time' approach is used to avoid a situation of a different effect for slower and faster devices.
     */
    /* Increase or decrease mixFactor value (depends on blurEffectDirection). */
    timeIntervalIndex = (int)(time / TIME_INTERVAL);
    nOfIterations     = (int) timeIntervalIndex % numberOfBlurPasses;

    if (nOfIterations >= (numberOfBlurPasses / 2))
    {
        nOfIterations       = numberOfBlurPasses - (nOfIterations % numberOfBlurPasses) - 1;
        blurEffectDirection = BLUR_EFFECT_DECREASE;
    }

    mixFactor                 = (time - ((int)(time / TIME_INTERVAL) * TIME_INTERVAL)) / TIME_INTERVAL;
    currentNumberOfIterations = MIN_NUMBER_OF_BLUR_PASSES + nOfIterations;

    if (blurEffectDirection == BLUR_EFFECT_DECREASE)
    {
        mixFactor = 1.0f - mixFactor;
    }

    if (currentNumberOfIterations != lastNumberOfIterations)
    {
        shouldSceneBeUpdated = true;
    }

    /* Store current number of iterations for future use. */
    lastNumberOfIterations = currentNumberOfIterations;
    /* [Mix factor calculations] */

    /* Update the scene only if needed. */
    if (shouldSceneBeUpdated)
    {
        /* [Blur loop] */
        /* Apply the blur effect.
        * The blur effect is applied in two basic steps (note that lower resolution textures are used).
        *   a. First, we blur the downscaled bloom texture horizontally.
        *   b. The result of horizontal blurring is then used for vertical blurring.
        *      The result texture contains image blrured in both directions.
        *   c. To amplify the blur effect, steps (a) and (b) are applied multiple times
        *      (with an exception that we now use the resulting blurred texture from the previous pass
        *       as an input to the horizontal blurring pass).
        *   d. The result of last iteration of applying the total blur effect (which is the result after the vertical blur is applied)
        *      is stored in a separate texture. Thanks to that, we have the last and previous blur result textures,
        *      both of which will be then used for continuous sampling (for the blending pass).
        *
        */
        /* Bind a framebuffer object to the GL_DRAW_FRAMEBUFFER framebuffer binding point,
        * so that everything we render will end up in the FBO's attachments. */
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                   blurringObjects.framebufferObjectId) );
        /* Set the lower viewport resolution. It corresponds to size of the texture we will be rendering to. */
        GL_CHECK(glViewport(0,
                            0,
                            windowWidth  / WINDOW_RESOLUTION_DIVISOR,
                            windowHeight / WINDOW_RESOLUTION_DIVISOR) );
        GL_CHECK(glEnable  (GL_SCISSOR_TEST) );

        /* Apply the blur effect multiple times. */
        for (int blurIterationIndex = 0;
                 blurIterationIndex < currentNumberOfIterations;
                 blurIterationIndex++)
        {
            /* FIRST PASS - HORIZONTAL BLUR
             * Take the texture showing cubes which should be bloomed and apply a horizontal blur operation.
             */
            GL_CHECK(glUseProgram(blurringHorizontalProgramShaderObjects.programObjectId) );
            {
                /* Attach the texture we want the color data to be rendered to the current draw framebuffer.*/
                GL_CHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                                GL_COLOR_ATTACHMENT0,
                                                GL_TEXTURE_2D,
                                                blurringObjects.textureObjectIdHorizontal,
                                                0) );

                /* In first iteration we have to take the texture which shows the cubes we want blurred.
                * Later, we have to take the same texture that has already been blurred vertically. */
                if (blurIterationIndex == 0)
                {
                    GL_CHECK(glUniform1i(blurringHorizontalProgramLocations.uniformTextureSampler,
                                        TEXTURE_UNIT_BLOOM_SOURCE_TEXTURE) );
                }
                else
                {
                    GL_CHECK(glUniform1i(blurringHorizontalProgramLocations.uniformTextureSampler,
                                        TEXTURE_UNIT_BLURRED_TEXTURE) );
                }

                /* Draw texture. */
                GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4) );
            } /* FIRST PASS - HORIZONTAL BLUR */

            /* SECOND PASS - VERTICAL BLUR
            * Take the result of the previous pass (horizontal blur) and apply a vertical blur to this texture.
            */
            GL_CHECK(glUseProgram(blurringVerticalProgramShaderObjects.programObjectId) );
            {
                if (blurIterationIndex == currentNumberOfIterations - 1)
                {
                    /* In case of the last iteration, use a different framebuffer object.
                     * The rendering results will be written to the only color attachment of the fbo. */
                    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                                               strongerBlurObjects.framebufferObjectId) );
                }
                else
                {
                    /* Bind a texture object we want the result data to be stored in.*/
                    GL_CHECK(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
                                                    GL_COLOR_ATTACHMENT0,
                                                    GL_TEXTURE_2D,
                                                    blurringObjects.textureObjectIdVertical,
                                                    0) );
                }

                /* Set uniform values. */
                GL_CHECK(glUniform1i(blurringVerticalProgramLocations.uniformTextureSampler,
                                     TEXTURE_UNIT_HORIZONTAL_BLUR_TEXTURE) ); /* Indicates which texture object content should be blurred. */

                /* Draw texture. */
                GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4) );
            } /* SECOND PASS - VERTICAL BLUR */
        } /* for (int blur_iteration_index = 0; i < numberOfIterations; blur_iteration_index++) */

        GL_CHECK(glDisable(GL_SCISSOR_TEST));
        /* [Blur loop] */
    } /* if (shouldSceneBeUpdated) */

    /* [Blending] */
    /* Apply blend effect.
     * Take the original scene texture and blend it with texture that contains the total blurring effect.
     */
    GL_CHECK(glUseProgram(blendingProgramShaderObjects.programObjectId) );
    {
        /* Bind the default framebuffer object. That indicates that the result is to be drawn to the back buffer. */
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );
        /* Set viewport values so that the rendering will take whole screen space. */
        GL_CHECK(glViewport(0, 0, windowWidth, windowHeight) );
        /* Set uniform value. */
        GL_CHECK(glUniform1f(blendingProgramLocations.uniformMixFactor, mixFactor) ); /* Current mixFactor will be used for mixing two textures color values
                                                                                       * (texture with higher and lower blur effect value). */
        /* Clear framebuffer content. */
        GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
        /* Draw texture. */
        GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, 4) );
    }
    /* [Blending] */
}

/** \brief Delete created objects and free allocated memory.
 *
 */
void uninit()
{
    /* Destroy created objects. */
    GL_CHECK(glUseProgram     (0) );
    GL_CHECK(glBindBuffer     (GL_ARRAY_BUFFER,
                               0) );
    GL_CHECK(glBindBuffer     (GL_UNIFORM_BUFFER,
                               0) );
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER,
                               0) );
    GL_CHECK(glActiveTexture  (GL_TEXTURE0 + TEXTURE_UNIT_COLOR_TEXTURE) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
    GL_CHECK(glActiveTexture  (GL_TEXTURE0 + TEXTURE_UNIT_BLOOM_SOURCE_TEXTURE) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
    GL_CHECK(glActiveTexture  (GL_TEXTURE0 + TEXTURE_UNIT_HORIZONTAL_BLUR_TEXTURE) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
    GL_CHECK(glActiveTexture  (GL_TEXTURE0 + TEXTURE_UNIT_BLURRED_TEXTURE) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );
    GL_CHECK(glActiveTexture  (GL_TEXTURE0 + TEXTURE_UNIT_STRONGER_BLUR_TEXTURE) );
    GL_CHECK(glBindTexture    (GL_TEXTURE_2D,
                               0) );

    deleteBlurringObjects              (&blurringObjects);
    deleteGetLuminanceImageBloomObjects(&getLuminanceImageBloomObjects);
    deleteProgramShaderObjects         (&blendingProgramShaderObjects);
    deleteProgramShaderObjects         (&blurringHorizontalProgramShaderObjects);
    deleteProgramShaderObjects         (&blurringVerticalProgramShaderObjects);
    deleteProgramShaderObjects         (&getLuminanceImageProgramShaderObjects);
    deleteProgramShaderObjects         (&sceneRenderingProgramShaderObjects);
    deleteSceneRenderingObjects        (&sceneRenderingObjects);
    deleteStrongerBlurObjects          (&strongerBlurObjects);

    /* Free allocated memory. */
    if (cubeCoordinates != NULL)
    {
        free(cubeCoordinates);

        cubeCoordinates = NULL;
    }

    if (cubeNormals != NULL)
    {
        free(cubeNormals);

        cubeNormals = NULL;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_init  (JNIEnv * env, jobject obj, jint width, jint height);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_step  (JNIEnv * env, jobject obj, jfloat time);
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_uninit(JNIEnv * env, jobject obj);
};

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_init(
        JNIEnv * env, jobject obj, jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_uninit(
        JNIEnv * env, jobject obj)
{
    uninit();
}


JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_bloom_NativeLibrary_step(
        JNIEnv * env, jobject obj, jfloat time)
{
    renderFrame(time);
}
