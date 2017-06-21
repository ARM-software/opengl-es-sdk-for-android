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
 * \file Metaballs/jni/Native.cpp
 * \brief Using a GPU to create organic-looking 3-dimensional objects in OpenGL ES 3.0.
 *
 * This tutorial demonstrates how a GPU can be used to render organic-looking
 * 3D objects using OpenGL ES 3.0's transform feedback feature.
 * All calculations are implemented on the GPU's shader processors.
 * Surface triangulation is performed using the Marching Cubes algorithm.
 * The Phong model is used for lighting metaball objects.
 * 3D textures are used to provide access to three dimentional arrays in shaders.
 *
 * For more information please see documentation.
 */

#include <cstdio>
#include <cstdlib>

#include <jni.h>
#include <android/log.h>

#include "Shader.h"
#include "Timer.h"
#include "Matrix.h"

#include "GLES3/gl3.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include <string>
#include <cmath>

#define LOG_TAG "libNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using std::string;
using namespace MaliSDK;

/**
 * In this vertex shader we calculate the sphere positions according to the specified time moment.
 * The shader contains an initial sphere positions and data required to calculate the sphere positions.
 * Each shader instance updates one sphere position, which is returned in the sphere_position output variable.
 */
const char* spheres_updater_vert_shader          = "#version 300 es\n"
"\n"
"/** Structure that describes parameters of a single sphere moving across the scalar field. */\n"
"struct sphere_descriptor\n"
"{\n"
"    /* Coefficients for Lissajou equations. Current coordinates calculated by formula:\n"
"     * v(t) = start_center + lissajou_amplitude * sin(lissajou_frequency * t + lissajou_phase) */\n"
"    vec3  start_center;        /* Center in space around which sphere moves.  */\n"
"    vec3  lissajou_amplitude;  /* Lissajou equation amplitudes for all axes.  */\n"
"    vec3  lissajou_frequency;  /* Lissajou equation frequencies for all axes. */\n"
"    vec3  lissajou_phase;      /* Lissajou equation phases for all axes.      */\n"
"    /* Other sphere parameters. */\n"
"    float size;                /* Size of a sphere (weight or charge).        */\n"
"};\n"
"\n"
"/* [Stage 1 Uniforms] */\n"
"/** Current time moment. */\n"
"uniform float time;\n"
"/* [Stage 1 Uniforms] */\n"
"\n"
"/* [Stage 1 Output data] */\n"
"/** Calculated sphere positions. */\n"
"out vec4 sphere_position;\n"
"/* [Stage 1 Output data] */\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"    /* Stores information on spheres moving across the scalar field. Specified in model coordinates (range 0..1]) */\n"
"    sphere_descriptor spheres[] = sphere_descriptor[]\n"
"    (\n"
"        /*                      (---- center ----)      (--- amplitude --)      (--- frequency ---)      (----- phase -----) (weight)*/\n"
"        sphere_descriptor(  vec3(0.50, 0.50, 0.50), vec3(0.20, 0.25, 0.25), vec3( 11.0, 21.0, 31.0), vec3( 30.0, 45.0, 90.0),  0.100),\n"
"        sphere_descriptor(  vec3(0.50, 0.50, 0.50), vec3(0.25, 0.20, 0.25), vec3( 22.0, 32.0, 12.0), vec3( 45.0, 90.0,120.0),  0.050),\n"
"        sphere_descriptor(  vec3(0.50, 0.50, 0.50), vec3(0.25, 0.25, 0.20), vec3( 33.0, 13.0, 23.0), vec3( 90.0,120.0,150.0),  0.250)\n"
"    );\n"
"\n"
"    /* Calculate new xyz coordinates of the sphere. */\n"
"    vec3 sphere_position3 = spheres[gl_VertexID].start_center\n"
"                          + spheres[gl_VertexID].lissajou_amplitude\n"
"                          * sin(radians(spheres[gl_VertexID].lissajou_frequency) * time + radians(spheres[gl_VertexID].lissajou_phase));\n"
"\n"
"    /* Update sphere position coordinates. w-coordinte represents sphere weight. */\n"
"    sphere_position = vec4(sphere_position3, spheres[gl_VertexID].size);\n"
"}\n";

/**
 *  Dummy fragment shader for a program object to successfully link.
 *  Fragment shader is not used in this stage, but needed for a program object to successfully link.
 */
const char* spheres_updater_frag_shader          = "#version 300 es\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"}\n";

/**
 * This vertex shader calculates a scalar field and stores it in the scalar_field_value output variable.
 * As input data we use sphere positions calculated in a previous stage and passed into the shader as
 * a uniform block.
 */
const char* scalar_field_vert_shader             = "#version 300 es\n"
"\n"
"/** Precision to avoid division-by-zero errors. */\n"
"#define EPSILON 0.000001f\n"
"\n"
"/** Amount of spheres defining scalar field. This value should be synchronized between all files. */\n"
"#define N_SPHERES 3\n"
"\n"
"/* [Stage 2 Uniforms] */\n"
"/* Uniforms: */\n"
"/** Amount of samples taken for each axis of a scalar field; */\n"
"uniform int samples_per_axis;\n"
"\n"
"/** Uniform block encapsulating sphere locations. */\n"
"uniform spheres_uniform_block\n"
"{\n"
"    vec4 input_spheres[N_SPHERES];\n"
"};\n"
"/* [Stage 2 Uniforms] */\n"
"\n"
"/* [Stage 2 Output data] */\n"
"/* Output data: */\n"
"/** Calculated scalar field value. */\n"
"out float scalar_field_value;\n"
"/* [Stage 2 Output data] */\n"
"\n"
"/* [Stage 2 decode_space_position] */\n"
"/** Decode coordinates in space from vertex number.\n"
" *  Assume 3D space of samples_per_axis length for each axis and following encoding:\n"
" *  encoded_position = x + y * samples_per_axis + z * samples_per_axis * samples_per_axis\n"
" *\n"
" *  @param  vertex_index Encoded vertex position\n"
" *  @return              Coordinates of a vertex in space ranged [0 .. samples_per_axis-1]\n"
" */\n"
"ivec3 decode_space_position(in int vertex_index)\n"
"{\n"
"    int   encoded_position = vertex_index;\n"
"    ivec3 space_position;\n"
"\n"
"    /* Calculate coordinates from vertex number. */\n"
"    space_position.x = encoded_position % samples_per_axis;\n"
"    encoded_position = encoded_position / samples_per_axis;\n"
"\n"
"    space_position.y = encoded_position % samples_per_axis;\n"
"    encoded_position = encoded_position / samples_per_axis;\n"
"\n"
"    space_position.z = encoded_position;\n"
"\n"
"    return space_position;\n"
"}\n"
"/* [Stage 2 decode_space_position] */\n"
"\n"
"/** Normalizes each coordinate interpolating input coordinates\n"
" *  from range [0 .. samples_per_axis-1] to [0.0 .. 1.0] range.\n"
" *\n"
" *  @param  space_position Coordinates in range [0 .. samples_per_axis-1]\n"
" *  @return Coordinates in range [0.0 .. 1.0]\n"
" */\n"
"/* [Stage 2 normalize_space_position_coordinates] */\n"
"vec3 normalize_space_position_coordinates(in ivec3 space_position)\n"
"{\n"
"    vec3 normalized_space_position = vec3(space_position) / float(samples_per_axis - 1);\n"
"\n"
"    return normalized_space_position;\n"
"}\n"
"/* [Stage 2 normalize_space_position_coordinates] */\n"
"\n"
"/** Calculates scalar field at user-defined location.\n"
" *\n"
" *  @param position Space position for which scalar field value is calculated\n"
" *  @return         Scalar field value\n"
" */\n"
"/* [Stage 2 calculate_scalar_field_value] */\n"
"float calculate_scalar_field_value(in vec3 position)\n"
"{\n"
"    float field_value = 0.0f;\n"
"\n"
"    /* Field value in given space position influenced by all spheres. */\n"
"    for (int i = 0; i < N_SPHERES; i++)\n"
"    {\n"
"        vec3  sphere_position         = input_spheres[i].xyz;\n"
"        float vertex_sphere_distance  = length(distance(sphere_position, position));\n"
"\n"
"        /* Field value is a sum of all spheres fields in a given space position.\n"
"         * Sphere weight (or charge) is stored in w-coordinate.\n"
"         */\n"
"        field_value += input_spheres[i].w / pow(max(EPSILON, vertex_sphere_distance), 2.0);\n"
"    }\n"
"\n"
"    return field_value;\n"
"}\n"
"/* [Stage 2 calculate_scalar_field_value] */\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"    /* Decode point space position defined by gl_VertexID. */\n"
"    ivec3 space_position      = decode_space_position(gl_VertexID);\n"
"\n"
"    /* Normalize point space position. */\n"
"    vec3  normalized_position = normalize_space_position_coordinates(space_position);\n"
"\n"
"    /* Calculate field value and assign field value to output variable. */\n"
"    scalar_field_value = calculate_scalar_field_value(normalized_position);\n"
"}\n";

/**
 *  Dummy fragment shader for a program object to successfully link.
 *  Fragment shader is not used in this stage, but needed for a program object to successfully link.
 */
const char* scalar_field_frag_shader             = "#version 300 es\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"}\n";

/**
 * The Marching Cube algorithm cell splitting stage vertex shader.
 *
 * In this vertex shader we analyse the isosurface in each cell of space and
 * assign one of 256 possible types to each cell. The cell type data
 * for each cell is returned in cell_type_index output variable.
 */
const char* marching_cubes_cells_vert_shader     = "#version 300 es\n"
"\n"
"/** Specify low precision for sampler3D type. */\n"
"precision lowp sampler3D;\n"
"\n"
"/* Uniforms: */\n"
"/** Scalar field is stored in a 3D texture. */\n"
"uniform sampler3D scalar_field;\n"
"\n"
"/** Amount of samples taken for each axis of a scalar field. */\n"
"uniform int cells_per_axis;\n"
"\n"
"/** Isosurface level. */\n"
"uniform float iso_level;\n"
"\n"
"/* Output data: */\n"
"/** Cell type index. */\n"
"flat out int cell_type_index;\n"
"\n"
"/** Calculates cell type index for provided cell and isosurface level.\n"
" *\n"
" *  @param cell_corner_field_value Scalar field values in cell corners\n"
" *  @param isolevel                Scalar field value which defines isosurface level\n"
" */\n"
"/* [Stage 3 get_cell_type_index] */\n"
"int get_cell_type_index(in float cell_corner_field_value[8], in float isolevel)\n"
"{\n"
"    int cell_type_index = 0;\n"
"\n"
"    /* Iterate through all cell corners. */\n"
"    for (int i = 0; i < 8; i++)\n"
"    {\n"
"        /* If corner is inside isosurface then set bit in cell type index index. */\n"
"        if (cell_corner_field_value[i] < isolevel)\n"
"        {\n"
"            /* Set appropriate corner bit in cell type index. */\n"
"            cell_type_index |= (1<<i);\n"
"        }\n"
"    }\n"
"\n"
"    return cell_type_index;\n"
"}\n"
"/* [Stage 3 get_cell_type_index] */\n"
"\n"
"/** Decode coordinates in space from cell number.\n"
" *  Assume cubical space of cells_per_axis cells length by each axis and following encoding:\n"
" *  encoded_position = x + y * cells_per_axis + z * cells_per_axis * cells_per_axis\n"
" *\n"
" *  @param  cell_index Encoded cell position\n"
" *  @return            Coordinates of a cell in space ranged [0 .. cells_per_axis-1]\n"
" */\n"
"/* [Stage 3 decode_space_position] */\n"
"ivec3 decode_space_position(in int cell_index)\n"
"{\n"
"    ivec3 space_position;\n"
"    int   encoded_position = cell_index;\n"
"\n"
"    /* Calculate coordinates from encoded position */\n"
"    space_position.x       = encoded_position % cells_per_axis;\n"
"    encoded_position       = encoded_position / cells_per_axis;\n"
"\n"
"    space_position.y       = encoded_position % cells_per_axis;\n"
"    encoded_position       = encoded_position / cells_per_axis;\n"
"\n"
"    space_position.z       = encoded_position;\n"
"\n"
"    return space_position;\n"
"}\n"
"/* [Stage 3 decode_space_position] */\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"    /* Cubic cell has exactly 8 corners. */\n"
"    const int corners_in_cell = 8;\n"
"\n"
"    /* Cell corners in space relatively to cell's base point [0]. */\n"
"    const ivec3 cell_corners_offsets[corners_in_cell] = ivec3[]\n"
"    (\n"
"        ivec3(0, 0, 0),\n"
"        ivec3(1, 0, 0),\n"
"        ivec3(1, 0, 1),\n"
"        ivec3(0, 0, 1),\n"
"        ivec3(0, 1, 0),\n"
"        ivec3(1, 1, 0),\n"
"        ivec3(1, 1, 1),\n"
"        ivec3(0, 1, 1)\n"
"    );\n"
"\n"
"    /* Scalar field texture size, used for normalization purposes. */\n"
"    vec3 scalar_field_normalizers = vec3(textureSize(scalar_field, 0)) - vec3(1, 1, 1);\n"
"\n"
"    /* Scalar field value in corners. Corners numbered according to Marching Cubes algorithm. */\n"
"    float scalar_field_in_cell_corners[8];\n"
"\n"
"    /* Find cell position processed by this shader instance (defined by gl_VertexID). */\n"
"    ivec3 space_position = decode_space_position(gl_VertexID);\n"
"\n"
"    /* [Stage 3 Gather values for the current cell] */\n"
"    /* Find scalar field values in cell corners. */\n"
"    for (int i = 0; i < corners_in_cell; i++)\n"
"    {\n"
"        /* Calculate cell corner processed at this iteration. */\n"
"        ivec3 cell_corner = space_position + cell_corners_offsets[i];\n"
"\n"
"        /* Calculate cell corner's actual position ([0.0 .. 1.0] range.) */\n"
"        vec3 normalized_cell_corner  = vec3(cell_corner) / scalar_field_normalizers;\n"
"\n"
"        /* Get scalar field value in cell corner from scalar field texture. */\n"
"        scalar_field_in_cell_corners[i] = textureLod(scalar_field, normalized_cell_corner, 0.0).r;\n"
"    }\n"
"    /* [Stage 3 Gather values for the current cell] */\n"
"\n"
"    /* Get cube type index. */\n"
"    cell_type_index = get_cell_type_index(scalar_field_in_cell_corners, iso_level);\n"
"}\n";

/**
 *  Dummy fragment shader for a program object to successfully link.
 *  Fragment shader is not used in this stage, but needed for a program object to successfully link.
 */
const char* marching_cubes_cells_frag_shader     = "#version 300 es\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"}\n";

/**
 * The vertex shader generates a set of triangles for each cell appropriate for the cell type.
 *
 * In this shader we generate exactly (3 vertices * 5 triangles per cell *
 * amount of cells the scalar field is split to) triangle vertices.
 * A one shader instance processes only one triangle vertex.
 * Due to requirement for a vertex shader instance to issue a vertex,
 * it issues a vertex in any case, including a dummy triangles, but
 * the dummy triangles has all vertices set to point O and will not be rendered.
 */
const char* marching_cubes_triangles_vert_shader = "#version 300 es\n"
"\n"
"precision highp isampler2D; /**< Specify high precision for isampler2D type. */\n"
"precision highp isampler3D; /**< Specify high precision for isampler3D type. */\n"
"precision highp sampler2D;  /**< Specify high precision for sampler2D type. */\n"
"precision highp sampler3D;  /**< Specify high precision for sampler3D type. */\n"
"\n"
"/** Precision to avoid division-by-zero errors. */\n"
"#define EPSILON 0.000001f\n"
"\n"
"/** Amount of cells taken for each axis of a scalar field. */\n"
"#define CELLS_PER_AXIS (samples_per_axis - 1)\n"
"\n"
"/** Maximum amount of vertices a single cell can define. */\n"
"const int mc_vertices_per_cell = 15;\n"
"\n"
"/* Uniforms: */\n"
"/** Amount of samples taken for each axis of a scalar field. */\n"
"uniform int samples_per_axis;\n"
"\n"
"/** A signed integer 3D texture is used to deliver cell type data. */\n"
"uniform isampler3D cell_types;\n"
"\n"
"/** A 3D texture is used to deliver scalar field data. */\n"
"uniform sampler3D scalar_field;\n"
"\n"
"/** A 2D texture representing tri_table lookup array. Array contains edge numbers (in sense of Marching Cubes algorithm).\n"
"    As input parameters (indices to texture) should be specified cell type index and combined vertex-triangle number. */\n"
"uniform isampler2D tri_table;\n"
"\n"
"/** Combined model view and projection matrices. */\n"
"uniform mat4 mvp;\n"
"\n"
"/** Isosurface level. */\n"
"uniform float iso_level;\n"
"\n"
"/* Phong shading output variables for fragment shader. */\n"
"out vec4 phong_vertex_position;      /**< position of the vertex in world space.  */\n"
"out vec3 phong_vertex_normal_vector; /**< surface normal vector in world space.   */\n"
"out vec3 phong_vertex_color;         /**< vertex color for fragment colorisation. */\n"
"\n"
"\n"
"/** Function approximates scalar field derivative along begin_vertex<->end_vertex axis.\n"
" *  Field derivative calculated as a scalar field difference between specified vertices\n"
" *  divided by distance between vertices.\n"
" *\n"
" *  @param begin_vertex begin vertex\n"
" *  @param end_vertex   end vertex\n"
" *  @return             scalar field derivative along begin_vertex<->end_vertex axis\n"
" */\n"
"float calc_partial_derivative(vec3 begin_vertex, vec3 end_vertex)\n"
"{\n"
"    float field_value_begin = textureLod(scalar_field, begin_vertex, 0.0).r;\n"
"    float field_value_end   = textureLod(scalar_field, end_vertex,   0.0).r;\n"
"\n"
"    return (field_value_end - field_value_begin) / distance(begin_vertex, end_vertex);\n"
"}\n"
"\n"
"/** Finds normal in given cell corner vertex. Normal calculated as a vec3(dF/dx, dF/dy, dF/dz)\n"
" *  dFs are calculated as difference of scalar field values in corners of this or adjacent cells.\n"
" *\n"
" *  @param p1 vertex for which normal is to be calculated\n"
" *  @return   normal vector to surface in p1\n"
" */\n"
"vec3 calc_cell_corner_normal(in vec3 p1)\n"
"{\n"
"    vec3 result;\n"
"    vec3 delta;\n"
"\n"
"    /* Use neighbour samples to calculate derivative. */\n"
"    delta = vec3(1.0/float(samples_per_axis - 1), 0, 0);\n"
"    result.x = calc_partial_derivative(p1 - delta, p1 + delta);\n"
"\n"
"    delta = vec3(0.0, 1.0/float(samples_per_axis - 1), 0.0);\n"
"    result.y = calc_partial_derivative(p1 - delta, p1 + delta);\n"
"\n"
"    delta = vec3(0.0, 0.0, 1.0/float(samples_per_axis - 1));\n"
"    result.z = calc_partial_derivative(p1 - delta, p1 + delta);\n"
"\n"
"    return result;\n"
"}\n"
"\n"
"/** Calculates normal for an edge vertex like in an orignal SIGGRAPH paper.\n"
" *  First finds normal vectors in edge begin vertex and in edge end vertex, then interpolate.\n"
" *\n"
" *  @param start_vertex_portion influence of edge_start vertex\n"
" *  @param edge_start           normalized coordinates of edge start vertex\n"
" *  @param edge_end             normalized coordinates of edge end vertex\n"
" *  @return                     normal to surface vector in edge position specified\n"
" */\n"
"vec3 calc_phong_normal(in float start_vertex_portion, in vec3 edge_start, in vec3 edge_end)\n"
"{\n"
"    /* Find normal vector in begin vertex of the edge. */\n"
"    vec3 edge_start_normal = calc_cell_corner_normal(edge_start);\n"
"    /* Find normal vector in end vertex of the edge. */\n"
"    vec3 edge_end_normal   = calc_cell_corner_normal(edge_end);\n"
"\n"
"    /* Interpolate normal vector. */\n"
"    return mix(edge_end_normal, edge_start_normal, start_vertex_portion);\n"
"}\n"
"\n"
"/** Decodes cell coordinates from vertex identifier.\n"
" *  Assumes 3D space of CELLS_PER_AXIS cells for each axis and\n"
" *  mc_vertices_per_cell triangles-generating vertices per cell\n"
" *  encoded in vertex identifier according to following formula:\n"
" *    encoded_position = mc_vertex_no + mc_vertices_per_cell * (x + CELLS_PER_AXIS * (y + CELLS_PER_AXIS * z))\n"
" *\n"
" *  @param  encoded_position_argument encoded position\n"
" *  @return                           cell coordinates ranged [0 .. CELLS_PER_AXIS-1] in x,y,z, and decoded vertex number in w.\n"
" */\n"
"/* [Stage 4 decode_cell_position] */\n"
"ivec4 decode_cell_position(in int encoded_position_argument)\n"
"{\n"
"    ivec4 cell_position;\n"
"    int   encoded_position = encoded_position_argument;\n"
"\n"
"    /* Decode combined triangle and vertex number. */\n"
"    cell_position.w  = encoded_position % mc_vertices_per_cell;\n"
"    encoded_position = encoded_position / mc_vertices_per_cell;\n"
"\n"
"    /* Decode coordinates from encoded position. */\n"
"    cell_position.x  = encoded_position % CELLS_PER_AXIS;\n"
"    encoded_position = encoded_position / CELLS_PER_AXIS;\n"
"\n"
"    cell_position.y  = encoded_position % CELLS_PER_AXIS;\n"
"    encoded_position = encoded_position / CELLS_PER_AXIS;\n"
"\n"
"    cell_position.z  = encoded_position;\n"
"\n"
"    return cell_position;\n"
"}\n"
"/* [Stage 4 decode_cell_position] */\n"
"\n"
"/** Identifies cell type for provided cell position.\n"
" *\n"
" *  @param cell_position non-normalized cell position in space\n"
" *  @return              cell type in sense of Macrhing Cubes algorithm\n"
" */\n"
"int get_cell_type(in ivec3 cell_position)\n"
"{\n"
"    vec3 cell_position_normalized = vec3(cell_position) / float(CELLS_PER_AXIS - 1);\n"
"\n"
"    /* Get cell type index of cell to which currently processed vertex (triangle_and_vertex_number) belongs */\n"
"    int  cell_type_index          = textureLod(cell_types, cell_position_normalized, 0.0).r;\n"
"\n"
"    return cell_type_index;\n"
"}\n"
"\n"
"/** Performs a table lookup with cell type index and combined vertex-triangle number specified\n"
" *  to locate an edge number which vertex is currently processed.\n"
" *\n"
" *  @param cell_type_index                    cell type index (in Marching Cubes algorthm sense)\n"
" *  @param combined_triangle_no_and_vertex_no combined vertex and triangle numbers (by formula tringle*3 + vertex)\n"
" *\n"
" *  @return                                   edge number (in sense of Marching Cubes algorithm) or -1 if vertex does not belong to any edge\n"
" */\n"
"int get_edge_number(in int cell_type_index, in int combined_triangle_no_and_vertex_no)\n"
"{\n"
"    /* Normalize indices for texture lookup: [0..14] -> [0.0..1.0], [0..255] -> [0.0..1.0]. */\n"
"    vec2 tri_table_index = vec2(float(combined_triangle_no_and_vertex_no)/14.0, float(cell_type_index)/255.0);\n"
"\n"
"    return textureLod(tri_table, tri_table_index, 0.0).r;\n"
"}\n"
"\n"
"/** Function calculates edge begin or edge end coordinates for specified cell and edge.\n"
" *\n"
" *  @param cell_origin_corner_coordinates normalized cell origin coordinates\n"
" *  @param edge_number                    edge number which coorinates being calculated\n"
" *  @param is_edge_start_vertex           true to request edge start vertex coordinates, false for end edge vertex\n"
" *  @return                               normalized edge start or end vertex coordinates\n"
"*/\n"
"vec3 get_edge_coordinates(in vec3 cell_origin_corner_coordinates, in int edge_number, in bool is_edge_start_vertex)\n"
"{\n"
"    /* These two arrays contain vertex indices which define a cell edge specified by index of arrays. */\n"
"    const int   edge_begins_in_cell_corner[12]  = int[] ( 0,1,2,3,4,5,6,7,0,1,2,3 );\n"
"    const int   edge_ends_in_cell_corner[12]    = int[] ( 1,2,3,0,5,6,7,4,4,5,6,7 );\n"
"    /* Defines offsets by axes for each of 8 cell corneres. */\n"
"    const ivec3 cell_corners_offsets[8]         = ivec3[8]\n"
"    (\n"
"        ivec3(0, 0, 0),\n"
"        ivec3(1, 0, 0),\n"
"        ivec3(1, 0, 1),\n"
"        ivec3(0, 0, 1),\n"
"        ivec3(0, 1, 0),\n"
"        ivec3(1, 1, 0),\n"
"        ivec3(1, 1, 1),\n"
"        ivec3(0, 1, 1)\n"
"    );\n"
"\n"
"    /* Edge corner number (number is in sense of Marching Cubes algorithm). */\n"
"    int edge_corner_no;\n"
"\n"
"    if (is_edge_start_vertex)\n"
"    {\n"
"        /* Use start cell corner of the edge. */\n"
"        edge_corner_no = edge_begins_in_cell_corner[edge_number];\n"
"    }\n"
"    else\n"
"    {\n"
"        /* Use end cell corner of the edge. */\n"
"        edge_corner_no = edge_ends_in_cell_corner[edge_number];\n"
"    }\n"
"\n"
"    /* Normalized cell corner coordinate offsets (to cell origin corner). */\n"
"    vec3 normalized_corner_offsets = vec3(cell_corners_offsets[edge_corner_no]) / float(samples_per_axis - 1);\n"
"\n"
"    /* Normalized cell corner coordinates. */\n"
"    vec3 edge_corner = cell_origin_corner_coordinates + normalized_corner_offsets;\n"
"\n"
"    return edge_corner;\n"
"}\n"
"\n"
"/** Function calculates how close start_corner vertex to intersetction point.\n"
" *\n"
" *  @param start_corner beginning of edge\n"
" *  @param end_corner   end of edge\n"
" *  @param iso_level    scalar field value level defining isosurface\n"
" *  @return             start vertex portion (1.0, if isosurface comes through start vertex)\n"
" */\n"
"float get_start_corner_portion(in vec3 start_corner, in vec3 end_corner, in float iso_level)\n"
"{\n"
"    float result;\n"
"    float start_field_value = textureLod(scalar_field, start_corner, 0.0).r;\n"
"    float end_field_value   = textureLod(scalar_field, end_corner, 0.0).r;\n"
"    float field_delta       = abs(start_field_value - end_field_value);\n"
"\n"
"    if (field_delta > EPSILON)\n"
"    {\n"
"        /* Calculate start vertex portion. */\n"
"        result = abs(end_field_value - iso_level) / field_delta;\n"
"    }\n"
"    else\n"
"    {\n"
"        /* Field values are too close in value to evaluate. Assume middle of an edge. */\n"
"        result = 0.5;\n"
"    }\n"
"\n"
"    return result;\n"
"}\n"
"\n"
"/** Shader entry point. */\n"
"void main()\n"
"{\n"
"    /* [Stage 4 Decode space position] */\n"
"    /* Split gl_vertexID into cell position and vertex number processed by this shader instance. */\n"
"    ivec4 cell_position_and_vertex_no = decode_cell_position(gl_VertexID);\n"
"    ivec3 cell_position               = cell_position_and_vertex_no.xyz;\n"
"    int   triangle_and_vertex_number  = cell_position_and_vertex_no.w;\n"
"    /* [Stage 4 Decode space position] */\n"
"\n"
"    /* [Stage 4 Get cell type and edge number] */\n"
"    /* Get cell type for cell current vertex belongs to. */\n"
"    int   cell_type_index             = get_cell_type(cell_position);\n"
"\n"
"    /* Get edge of the cell to which belongs processed vertex. */\n"
"    int   edge_number                 = get_edge_number(cell_type_index, triangle_and_vertex_number);\n"
"    /* [Stage 4 Get cell type and edge number] */\n"
"\n"
"    /* Check if this is not a vertex of dummy triangle. */\n"
"    if (edge_number != -1)\n"
"    {\n"
"        /* [Stage 4 Calculate cell origin] */\n"
"        /* Calculate normalized coordinates in space of cell origin corner. */\n"
"        vec3 cell_origin_corner    = vec3(cell_position) / float(samples_per_axis - 1);\n"
"        /* [Stage 4 Calculate cell origin] */\n"
"\n"
"        /* [Stage 4 Calculate start and end edge coordinates] */\n"
"        /* Calculate start and end edge coordinates. */\n"
"        vec3 start_corner          = get_edge_coordinates(cell_origin_corner, edge_number, true);\n"
"        vec3 end_corner            = get_edge_coordinates(cell_origin_corner, edge_number, false);\n"
"        /* [Stage 4 Calculate start and end edge coordinates] */\n"
"\n"
"        /* [Stage 4 Calculate middle edge vertex] */\n"
"        /* Calculate share of start point of an edge. */\n"
"        float start_vertex_portion = get_start_corner_portion(start_corner, end_corner, iso_level);\n"
"\n"
"        /* Calculate ''middle'' edge vertex. This vertex is moved closer to start or end vertices of the edge. */\n"
"        vec3 edge_middle_vertex    = mix(end_corner, start_corner, start_vertex_portion);\n"
"        /* [Stage 4 Calculate middle edge vertex] */\n"
"\n"
"        /* [Stage 4 Calculate middle edge normal] */\n"
"        /* Calculate normal to surface in the ''middle'' vertex. */\n"
"        vec3 vertex_normal         = calc_phong_normal(start_vertex_portion, start_corner, end_corner);\n"
"        /* [Stage 4 Calculate middle edge normal] */\n"
"\n"
"        /* Update vertex shader outputs. */\n"
"        gl_Position                = mvp * vec4(edge_middle_vertex, 1.0);        /* Transform vertex position with MVP-matrix.        */\n"
"        phong_vertex_position      = gl_Position;                                /* Set vertex position for fragment shader.          */\n"
"        phong_vertex_normal_vector = vertex_normal;                              /* Set normal vector to surface for fragment shader. */\n"
"        phong_vertex_color         = vec3(0.7);                                  /* Set vertex color for fragment shader.             */\n"
"    }\n"
"    else\n"
"    {\n"
"        /* [Stage 4 Discard dummy triangle] */\n"
"        /* This cell type generates fewer triangles, and this particular one should be discarded. */\n"
"        gl_Position                = vec4(0);                                    /* Discard vertex by setting its coordinate in infinity. */\n"
"        phong_vertex_position      = gl_Position;\n"
"        phong_vertex_normal_vector = vec3(0);\n"
"        phong_vertex_color         = vec3(0);\n"
"        /* [Stage 4 Discard dummy triangle] */\n"
"    }\n"
"}\n";

/**
 * In this shader we render triangles emitted by the mc_triangles_generator_shader vertex shader.
 * The shader uses one directional light source in Phong lighting model.
 * The light source moves on spherical surface around metaballs.
 */
const char* marching_cubes_triangles_frag_shader = "#version 300 es\n"
"\n"
"/** Specify low precision for float type. */\n"
"precision lowp float;\n"
"\n"
"/* Uniforms: */\n"
"/** Current time moment. */\n"
"uniform float time;\n"
"\n"
"/** Position of the vertex (and fragment) in world space. */\n"
"in  vec4 phong_vertex_position;\n"
"\n"
"/** Surface normal vector in world space. */\n"
"in  vec3 phong_vertex_normal_vector;\n"
"\n"
"/** Color passed from vertex shader. */\n"
"in  vec3 phong_vertex_color;\n"
"\n"
"/* Output data: */\n"
"/** Fragment color. */\n"
"out vec4 FragColor;\n"
"\n"
"/** Shader entry point. Main steps are described in comments below. */\n"
"void main()\n"
"{\n"
"    /* Distance to light source. */\n"
"    const float light_distance = 5.0;\n"
"\n"
"    /* Add some movement to light source. */\n"
"    float theta = float(time);\n"
"    float phi   = float(time)/3.0;\n"
"\n"
"    vec3 light_location = vec3\n"
"    (\n"
"        light_distance * cos(theta) * sin(phi),\n"
"        light_distance * cos(theta) * cos(phi),\n"
"        light_distance * sin(theta)\n"
"    );\n"
"\n"
"    /* Scene ambient color. */\n"
"    const vec3  ambient_color = vec3(0.1, 0.1, 0.1);\n"
"    const float attenuation   = 1.0;\n"
"    const float shiness       = 3.0;\n"
"\n"
"    /* Normalize directions. */\n"
"    vec3 normal_direction = normalize(phong_vertex_normal_vector);\n"
"    vec3 view_direction   = normalize(vec3(vec4(0.0, 0.0, 1.0, 0.0) - phong_vertex_position));\n"
"    vec3 light_direction  = normalize(light_location);\n"
"\n"
"    /** Calculate ambient lighting component of directional light. */\n"
"    vec3 ambient_lighting    = ambient_color * phong_vertex_color;\n"
"\n"
"    /** Calculate diffuse reflection lighting component of directional light. */\n"
"    vec3 diffuse_reflection  = attenuation * phong_vertex_color\n"
"                             * max(0.0, dot(normal_direction, light_direction));\n"
"\n"
"    /** Calculate specular reflection lighting component of directional light. */\n"
"    vec3 specular_reflection = vec3(0.0, 0.0, 0.0);\n"
"\n"
"    if (dot(normal_direction, light_direction) >= 0.0)\n"
"    {\n"
"        /* Light source on the right side. */\n"
"        specular_reflection = attenuation * phong_vertex_color\n"
"                            * pow(max(0.0, dot(reflect(-light_direction, normal_direction), view_direction)), shiness);\n"
"    }\n"
"\n"
"    /** Calculate fragment lighting as sum of previous three component. */\n"
"    FragColor = vec4(ambient_lighting + diffuse_reflection + specular_reflection, 1.0);\n"
"}\n";

/* General metaballs example properties. */
GLfloat      model_time        = 0.0f;  /**< Time (in seconds), increased each rendering iteration.                                         */
const GLuint tesselation_level = 32;    /**< Level of details you would like to split model into. Please use values from th range [8..256]. */
GLfloat      isosurface_level  = 12.0f; /**< Scalar field's isosurface level.                                                               */
unsigned int window_width      = 256;   /**< Window width resolution (pixels).                                                              */
unsigned int window_height     = 256;   /**< Window height resolution (pixels).                                                             */

/* Marching Cubes algorithm-specific constants. */
const GLuint samples_per_axis      = tesselation_level;                                      /**< Amount of samples we break scalar space into (per each axis). */
const GLuint samples_in_3d_space   = samples_per_axis * samples_per_axis * samples_per_axis; /**< Amount of samples in 3D space. */
const GLuint cells_per_axis        = samples_per_axis - 1;                                   /**< Amount of cells per each axis. */
const GLuint cells_in_3d_space     = cells_per_axis * cells_per_axis * cells_per_axis;       /**< Amount of cells in 3D space. */
const GLuint vertices_per_triangle = 3;                                                      /**< Amount of vertices that defines one triangle. */
const GLuint triangles_per_cell    = 5;                                                      /**< Amount of triangles that can be generated for a single cell by the Marching Cubes algorithm. */
const GLuint mc_vertices_per_cell  = vertices_per_triangle * triangles_per_cell;             /**< Amount of vertices in tri_table representing triangles by vertices for one cell. */
const GLuint mc_cells_types_count  = 256;                                                    /**< Amount of cell types. */

/** The array that is used for cell triangularization.
 *  Each row in table represents one cell type. Each cell type contains up to 5 triangles.
 *  Each triangle is defined by 3 sequential vertices.
 *  These vertices are "middle" points of the cell edges specified in this table.
 *  For example cell type 0 (see first line) does not define any triangles,
 *  while cell type 1 (see second line) defines one triangle consisting of "middle" points of edges 0,8 and 3 of a cell.
 *  "Middle" points are base points and can be moved closer to edge beginning point or edge ending point.
 *  Edge numeration is according to the Marching Cubes algorithm.
 *  There are exactly 256 cell types due to each vertex having only 2 states: it can be below isosurface or above.
 *  Thus (having 8 corners for each cubic cell) we have 2^8 = 256 cell types.
 *
 *  Table data taken from http://paulbourke.net/geometry/polygonise/
 */
/* [tri_table chosen part for documentation] */
const GLint tri_table[mc_cells_types_count * mc_vertices_per_cell] =
{
  -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  1,  9,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  8,  3,     9,  8,  1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
/* [tri_table chosen part for documentation] */
   1,  2, 10,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,     1,  2, 10,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9,  2, 10,     0,  2,  9,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   2,  8,  3,     2, 10,  8,    10,  9,  8,    -1, -1, -1,    -1, -1, -1,
   3, 11,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0, 11,  2,     8, 11,  0,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  9,  0,     2,  3, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1, 11,  2,     1,  9, 11,     9,  8, 11,    -1, -1, -1,    -1, -1, -1,
   3, 10,  1,    11, 10,  3,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0, 10,  1,     0,  8, 10,     8, 11, 10,    -1, -1, -1,    -1, -1, -1,
   3,  9,  0,     3, 11,  9,    11, 10,  9,    -1, -1, -1,    -1, -1, -1,
   9,  8, 10,    10,  8, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  7,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  3,  0,     7,  3,  4,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  1,  9,     8,  4,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  1,  9,     4,  7,  1,     7,  3,  1,    -1, -1, -1,    -1, -1, -1,
   1,  2, 10,     8,  4,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  4,  7,     3,  0,  4,     1,  2, 10,    -1, -1, -1,    -1, -1, -1,
   9,  2, 10,     9,  0,  2,     8,  4,  7,    -1, -1, -1,    -1, -1, -1,
   2, 10,  9,     2,  9,  7,     2,  7,  3,     7,  9,  4,    -1, -1, -1,
   8,  4,  7,     3, 11,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  11,  4,  7,    11,  2,  4,     2,  0,  4,    -1, -1, -1,    -1, -1, -1,
   9,  0,  1,     8,  4,  7,     2,  3, 11,    -1, -1, -1,    -1, -1, -1,
   4,  7, 11,     9,  4, 11,     9, 11,  2,     9,  2,  1,    -1, -1, -1,
   3, 10,  1,     3, 11, 10,     7,  8,  4,    -1, -1, -1,    -1, -1, -1,
   1, 11, 10,     1,  4, 11,     1,  0,  4,     7, 11,  4,    -1, -1, -1,
   4,  7,  8,     9,  0, 11,     9, 11, 10,    11,  0,  3,    -1, -1, -1,
   4,  7, 11,     4, 11,  9,     9, 11, 10,    -1, -1, -1,    -1, -1, -1,
   9,  5,  4,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9,  5,  4,     0,  8,  3,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  5,  4,     1,  5,  0,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   8,  5,  4,     8,  3,  5,     3,  1,  5,    -1, -1, -1,    -1, -1, -1,
   1,  2, 10,     9,  5,  4,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  0,  8,     1,  2, 10,     4,  9,  5,    -1, -1, -1,    -1, -1, -1,
   5,  2, 10,     5,  4,  2,     4,  0,  2,    -1, -1, -1,    -1, -1, -1,
   2, 10,  5,     3,  2,  5,     3,  5,  4,     3,  4,  8,    -1, -1, -1,
   9,  5,  4,     2,  3, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0, 11,  2,     0,  8, 11,     4,  9,  5,    -1, -1, -1,    -1, -1, -1,
   0,  5,  4,     0,  1,  5,     2,  3, 11,    -1, -1, -1,    -1, -1, -1,
   2,  1,  5,     2,  5,  8,     2,  8, 11,     4,  8,  5,    -1, -1, -1,
  10,  3, 11,    10,  1,  3,     9,  5,  4,    -1, -1, -1,    -1, -1, -1,
   4,  9,  5,     0,  8,  1,     8, 10,  1,     8, 11, 10,    -1, -1, -1,
   5,  4,  0,     5,  0, 11,     5, 11, 10,    11,  0,  3,    -1, -1, -1,
   5,  4,  8,     5,  8, 10,    10,  8, 11,    -1, -1, -1,    -1, -1, -1,
   9,  7,  8,     5,  7,  9,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9,  3,  0,     9,  5,  3,     5,  7,  3,    -1, -1, -1,    -1, -1, -1,
   0,  7,  8,     0,  1,  7,     1,  5,  7,    -1, -1, -1,    -1, -1, -1,
   1,  5,  3,     3,  5,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9,  7,  8,     9,  5,  7,    10,  1,  2,    -1, -1, -1,    -1, -1, -1,
  10,  1,  2,     9,  5,  0,     5,  3,  0,     5,  7,  3,    -1, -1, -1,
   8,  0,  2,     8,  2,  5,     8,  5,  7,    10,  5,  2,    -1, -1, -1,
   2, 10,  5,     2,  5,  3,     3,  5,  7,    -1, -1, -1,    -1, -1, -1,
   7,  9,  5,     7,  8,  9,     3, 11,  2,    -1, -1, -1,    -1, -1, -1,
   9,  5,  7,     9,  7,  2,     9,  2,  0,     2,  7, 11,    -1, -1, -1,
   2,  3, 11,     0,  1,  8,     1,  7,  8,     1,  5,  7,    -1, -1, -1,
  11,  2,  1,    11,  1,  7,     7,  1,  5,    -1, -1, -1,    -1, -1, -1,
   9,  5,  8,     8,  5,  7,    10,  1,  3,    10,  3, 11,    -1, -1, -1,
   5,  7,  0,     5,  0,  9,     7, 11,  0,     1,  0, 10,    11, 10,  0,
  11, 10,  0,    11,  0,  3,    10,  5,  0,     8,  0,  7,     5,  7,  0,
  11, 10,  5,     7, 11,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  10,  6,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,     5, 10,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9,  0,  1,     5, 10,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  8,  3,     1,  9,  8,     5, 10,  6,    -1, -1, -1,    -1, -1, -1,
   1,  6,  5,     2,  6,  1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  6,  5,     1,  2,  6,     3,  0,  8,    -1, -1, -1,    -1, -1, -1,
   9,  6,  5,     9,  0,  6,     0,  2,  6,    -1, -1, -1,    -1, -1, -1,
   5,  9,  8,     5,  8,  2,     5,  2,  6,     3,  2,  8,    -1, -1, -1,
   2,  3, 11,    10,  6,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  11,  0,  8,    11,  2,  0,    10,  6,  5,    -1, -1, -1,    -1, -1, -1,
   0,  1,  9,     2,  3, 11,     5, 10,  6,    -1, -1, -1,    -1, -1, -1,
   5, 10,  6,     1,  9,  2,     9, 11,  2,     9,  8, 11,    -1, -1, -1,
   6,  3, 11,     6,  5,  3,     5,  1,  3,    -1, -1, -1,    -1, -1, -1,
   0,  8, 11,     0, 11,  5,     0,  5,  1,     5, 11,  6,    -1, -1, -1,
   3, 11,  6,     0,  3,  6,     0,  6,  5,     0,  5,  9,    -1, -1, -1,
   6,  5,  9,     6,  9, 11,    11,  9,  8,    -1, -1, -1,    -1, -1, -1,
   5, 10,  6,     4,  7,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  3,  0,     4,  7,  3,     6,  5, 10,    -1, -1, -1,    -1, -1, -1,
   1,  9,  0,     5, 10,  6,     8,  4,  7,    -1, -1, -1,    -1, -1, -1,
  10,  6,  5,     1,  9,  7,     1,  7,  3,     7,  9,  4,    -1, -1, -1,
   6,  1,  2,     6,  5,  1,     4,  7,  8,    -1, -1, -1,    -1, -1, -1,
   1,  2,  5,     5,  2,  6,     3,  0,  4,     3,  4,  7,    -1, -1, -1,
   8,  4,  7,     9,  0,  5,     0,  6,  5,     0,  2,  6,    -1, -1, -1,
   7,  3,  9,     7,  9,  4,     3,  2,  9,     5,  9,  6,     2,  6,  9,
   3, 11,  2,     7,  8,  4,    10,  6,  5,    -1, -1, -1,    -1, -1, -1,
   5, 10,  6,     4,  7,  2,     4,  2,  0,     2,  7, 11,    -1, -1, -1,
   0,  1,  9,     4,  7,  8,     2,  3, 11,     5, 10,  6,    -1, -1, -1,
   9,  2,  1,     9, 11,  2,     9,  4, 11,     7, 11,  4,     5, 10,  6,
   8,  4,  7,     3, 11,  5,     3,  5,  1,     5, 11,  6,    -1, -1, -1,
   5,  1, 11,     5, 11,  6,     1,  0, 11,     7, 11,  4,     0,  4, 11,
   0,  5,  9,     0,  6,  5,     0,  3,  6,    11,  6,  3,     8,  4,  7,
   6,  5,  9,     6,  9, 11,     4,  7,  9,     7, 11,  9,    -1, -1, -1,
  10,  4,  9,     6,  4, 10,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4, 10,  6,     4,  9, 10,     0,  8,  3,    -1, -1, -1,    -1, -1, -1,
  10,  0,  1,    10,  6,  0,     6,  4,  0,    -1, -1, -1,    -1, -1, -1,
   8,  3,  1,     8,  1,  6,     8,  6,  4,     6,  1, 10,    -1, -1, -1,
   1,  4,  9,     1,  2,  4,     2,  6,  4,    -1, -1, -1,    -1, -1, -1,
   3,  0,  8,     1,  2,  9,     2,  4,  9,     2,  6,  4,    -1, -1, -1,
   0,  2,  4,     4,  2,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   8,  3,  2,     8,  2,  4,     4,  2,  6,    -1, -1, -1,    -1, -1, -1,
  10,  4,  9,    10,  6,  4,    11,  2,  3,    -1, -1, -1,    -1, -1, -1,
   0,  8,  2,     2,  8, 11,     4,  9, 10,     4, 10,  6,    -1, -1, -1,
   3, 11,  2,     0,  1,  6,     0,  6,  4,     6,  1, 10,    -1, -1, -1,
   6,  4,  1,     6,  1, 10,     4,  8,  1,     2,  1, 11,     8, 11,  1,
   9,  6,  4,     9,  3,  6,     9,  1,  3,    11,  6,  3,    -1, -1, -1,
   8, 11,  1,     8,  1,  0,    11,  6,  1,     9,  1,  4,     6,  4,  1,
   3, 11,  6,     3,  6,  0,     0,  6,  4,    -1, -1, -1,    -1, -1, -1,
   6,  4,  8,    11,  6,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   7, 10,  6,     7,  8, 10,     8,  9, 10,    -1, -1, -1,    -1, -1, -1,
   0,  7,  3,     0, 10,  7,     0,  9, 10,     6,  7, 10,    -1, -1, -1,
  10,  6,  7,     1, 10,  7,     1,  7,  8,     1,  8,  0,    -1, -1, -1,
  10,  6,  7,    10,  7,  1,     1,  7,  3,    -1, -1, -1,    -1, -1, -1,
   1,  2,  6,     1,  6,  8,     1,  8,  9,     8,  6,  7,    -1, -1, -1,
   2,  6,  9,     2,  9,  1,     6,  7,  9,     0,  9,  3,     7,  3,  9,
   7,  8,  0,     7,  0,  6,     6,  0,  2,    -1, -1, -1,    -1, -1, -1,
   7,  3,  2,     6,  7,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   2,  3, 11,    10,  6,  8,    10,  8,  9,     8,  6,  7,    -1, -1, -1,
   2,  0,  7,     2,  7, 11,     0,  9,  7,     6,  7, 10,     9, 10,  7,
   1,  8,  0,     1,  7,  8,     1, 10,  7,     6,  7, 10,     2,  3, 11,
  11,  2,  1,    11,  1,  7,    10,  6,  1,     6,  7,  1,    -1, -1, -1,
   8,  9,  6,     8,  6,  7,     9,  1,  6,    11,  6,  3,     1,  3,  6,
   0,  9,  1,    11,  6,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   7,  8,  0,     7,  0,  6,     3, 11,  0,    11,  6,  0,    -1, -1, -1,
   7, 11,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   7,  6, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  0,  8,    11,  7,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  1,  9,    11,  7,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   8,  1,  9,     8,  3,  1,    11,  7,  6,    -1, -1, -1,    -1, -1, -1,
  10,  1,  2,     6, 11,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  2, 10,     3,  0,  8,     6, 11,  7,    -1, -1, -1,    -1, -1, -1,
   2,  9,  0,     2, 10,  9,     6, 11,  7,    -1, -1, -1,    -1, -1, -1,
   6, 11,  7,     2, 10,  3,    10,  8,  3,    10,  9,  8,    -1, -1, -1,
   7,  2,  3,     6,  2,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   7,  0,  8,     7,  6,  0,     6,  2,  0,    -1, -1, -1,    -1, -1, -1,
   2,  7,  6,     2,  3,  7,     0,  1,  9,    -1, -1, -1,    -1, -1, -1,
   1,  6,  2,     1,  8,  6,     1,  9,  8,     8,  7,  6,    -1, -1, -1,
  10,  7,  6,    10,  1,  7,     1,  3,  7,    -1, -1, -1,    -1, -1, -1,
  10,  7,  6,     1,  7, 10,     1,  8,  7,     1,  0,  8,    -1, -1, -1,
   0,  3,  7,     0,  7, 10,     0, 10,  9,     6, 10,  7,    -1, -1, -1,
   7,  6, 10,     7, 10,  8,     8, 10,  9,    -1, -1, -1,    -1, -1, -1,
   6,  8,  4,    11,  8,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  6, 11,     3,  0,  6,     0,  4,  6,    -1, -1, -1,    -1, -1, -1,
   8,  6, 11,     8,  4,  6,     9,  0,  1,    -1, -1, -1,    -1, -1, -1,
   9,  4,  6,     9,  6,  3,     9,  3,  1,    11,  3,  6,    -1, -1, -1,
   6,  8,  4,     6, 11,  8,     2, 10,  1,    -1, -1, -1,    -1, -1, -1,
   1,  2, 10,     3,  0, 11,     0,  6, 11,     0,  4,  6,    -1, -1, -1,
   4, 11,  8,     4,  6, 11,     0,  2,  9,     2, 10,  9,    -1, -1, -1,
  10,  9,  3,    10,  3,  2,     9,  4,  3,    11,  3,  6,     4,  6,  3,
   8,  2,  3,     8,  4,  2,     4,  6,  2,    -1, -1, -1,    -1, -1, -1,
   0,  4,  2,     4,  6,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  9,  0,     2,  3,  4,     2,  4,  6,     4,  3,  8,    -1, -1, -1,
   1,  9,  4,     1,  4,  2,     2,  4,  6,    -1, -1, -1,    -1, -1, -1,
   8,  1,  3,     8,  6,  1,     8,  4,  6,     6, 10,  1,    -1, -1, -1,
  10,  1,  0,    10,  0,  6,     6,  0,  4,    -1, -1, -1,    -1, -1, -1,
   4,  6,  3,     4,  3,  8,     6, 10,  3,     0,  3,  9,    10,  9,  3,
  10,  9,  4,     6, 10,  4,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  9,  5,     7,  6, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,     4,  9,  5,    11,  7,  6,    -1, -1, -1,    -1, -1, -1,
   5,  0,  1,     5,  4,  0,     7,  6, 11,    -1, -1, -1,    -1, -1, -1,
  11,  7,  6,     8,  3,  4,     3,  5,  4,     3,  1,  5,    -1, -1, -1,
   9,  5,  4,    10,  1,  2,     7,  6, 11,    -1, -1, -1,    -1, -1, -1,
   6, 11,  7,     1,  2, 10,     0,  8,  3,     4,  9,  5,    -1, -1, -1,
   7,  6, 11,     5,  4, 10,     4,  2, 10,     4,  0,  2,    -1, -1, -1,
   3,  4,  8,     3,  5,  4,     3,  2,  5,    10,  5,  2,    11,  7,  6,
   7,  2,  3,     7,  6,  2,     5,  4,  9,    -1, -1, -1,    -1, -1, -1,
   9,  5,  4,     0,  8,  6,     0,  6,  2,     6,  8,  7,    -1, -1, -1,
   3,  6,  2,     3,  7,  6,     1,  5,  0,     5,  4,  0,    -1, -1, -1,
   6,  2,  8,     6,  8,  7,     2,  1,  8,     4,  8,  5,     1,  5,  8,
   9,  5,  4,    10,  1,  6,     1,  7,  6,     1,  3,  7,    -1, -1, -1,
   1,  6, 10,     1,  7,  6,     1,  0,  7,     8,  7,  0,     9,  5,  4,
   4,  0, 10,     4, 10,  5,     0,  3, 10,     6, 10,  7,     3,  7, 10,
   7,  6, 10,     7, 10,  8,     5,  4, 10,     4,  8, 10,    -1, -1, -1,
   6,  9,  5,     6, 11,  9,    11,  8,  9,    -1, -1, -1,    -1, -1, -1,
   3,  6, 11,     0,  6,  3,     0,  5,  6,     0,  9,  5,    -1, -1, -1,
   0, 11,  8,     0,  5, 11,     0,  1,  5,     5,  6, 11,    -1, -1, -1,
   6, 11,  3,     6,  3,  5,     5,  3,  1,    -1, -1, -1,    -1, -1, -1,
   1,  2, 10,     9,  5, 11,     9, 11,  8,    11,  5,  6,    -1, -1, -1,
   0, 11,  3,     0,  6, 11,     0,  9,  6,     5,  6,  9,     1,  2, 10,
  11,  8,  5,    11,  5,  6,     8,  0,  5,    10,  5,  2,     0,  2,  5,
   6, 11,  3,     6,  3,  5,     2, 10,  3,    10,  5,  3,    -1, -1, -1,
   5,  8,  9,     5,  2,  8,     5,  6,  2,     3,  8,  2,    -1, -1, -1,
   9,  5,  6,     9,  6,  0,     0,  6,  2,    -1, -1, -1,    -1, -1, -1,
   1,  5,  8,     1,  8,  0,     5,  6,  8,     3,  8,  2,     6,  2,  8,
   1,  5,  6,     2,  1,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  3,  6,     1,  6, 10,     3,  8,  6,     5,  6,  9,     8,  9,  6,
  10,  1,  0,    10,  0,  6,     9,  5,  0,     5,  6,  0,    -1, -1, -1,
   0,  3,  8,     5,  6, 10,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  10,  5,  6,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  11,  5, 10,     7,  5, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  11,  5, 10,    11,  7,  5,     8,  3,  0,    -1, -1, -1,    -1, -1, -1,
   5, 11,  7,     5, 10, 11,     1,  9,  0,    -1, -1, -1,    -1, -1, -1,
  10,  7,  5,    10, 11,  7,     9,  8,  1,     8,  3,  1,    -1, -1, -1,
  11,  1,  2,    11,  7,  1,     7,  5,  1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,     1,  2,  7,     1,  7,  5,     7,  2, 11,    -1, -1, -1,
   9,  7,  5,     9,  2,  7,     9,  0,  2,     2, 11,  7,    -1, -1, -1,
   7,  5,  2,     7,  2, 11,     5,  9,  2,     3,  2,  8,     9,  8,  2,
   2,  5, 10,     2,  3,  5,     3,  7,  5,    -1, -1, -1,    -1, -1, -1,
   8,  2,  0,     8,  5,  2,     8,  7,  5,    10,  2,  5,    -1, -1, -1,
   9,  0,  1,     5, 10,  3,     5,  3,  7,     3, 10,  2,    -1, -1, -1,
   9,  8,  2,     9,  2,  1,     8,  7,  2,    10,  2,  5,     7,  5,  2,
   1,  3,  5,     3,  7,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  8,  7,     0,  7,  1,     1,  7,  5,    -1, -1, -1,    -1, -1, -1,
   9,  0,  3,     9,  3,  5,     5,  3,  7,    -1, -1, -1,    -1, -1, -1,
   9,  8,  7,     5,  9,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   5,  8,  4,     5, 10,  8,    10, 11,  8,    -1, -1, -1,    -1, -1, -1,
   5,  0,  4,     5, 11,  0,     5, 10, 11,    11,  3,  0,    -1, -1, -1,
   0,  1,  9,     8,  4, 10,     8, 10, 11,    10,  4,  5,    -1, -1, -1,
  10, 11,  4,    10,  4,  5,    11,  3,  4,     9,  4,  1,     3,  1,  4,
   2,  5,  1,     2,  8,  5,     2, 11,  8,     4,  5,  8,    -1, -1, -1,
   0,  4, 11,     0, 11,  3,     4,  5, 11,     2, 11,  1,     5,  1, 11,
   0,  2,  5,     0,  5,  9,     2, 11,  5,     4,  5,  8,    11,  8,  5,
   9,  4,  5,     2, 11,  3,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   2,  5, 10,     3,  5,  2,     3,  4,  5,     3,  8,  4,    -1, -1, -1,
   5, 10,  2,     5,  2,  4,     4,  2,  0,    -1, -1, -1,    -1, -1, -1,
   3, 10,  2,     3,  5, 10,     3,  8,  5,     4,  5,  8,     0,  1,  9,
   5, 10,  2,     5,  2,  4,     1,  9,  2,     9,  4,  2,    -1, -1, -1,
   8,  4,  5,     8,  5,  3,     3,  5,  1,    -1, -1, -1,    -1, -1, -1,
   0,  4,  5,     1,  0,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   8,  4,  5,     8,  5,  3,     9,  0,  5,     0,  3,  5,    -1, -1, -1,
   9,  4,  5,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4, 11,  7,     4,  9, 11,     9, 10, 11,    -1, -1, -1,    -1, -1, -1,
   0,  8,  3,     4,  9,  7,     9, 11,  7,     9, 10, 11,    -1, -1, -1,
   1, 10, 11,     1, 11,  4,     1,  4,  0,     7,  4, 11,    -1, -1, -1,
   3,  1,  4,     3,  4,  8,     1, 10,  4,     7,  4, 11,    10, 11,  4,
   4, 11,  7,     9, 11,  4,     9,  2, 11,     9,  1,  2,    -1, -1, -1,
   9,  7,  4,     9, 11,  7,     9,  1, 11,     2, 11,  1,     0,  8,  3,
  11,  7,  4,    11,  4,  2,     2,  4,  0,    -1, -1, -1,    -1, -1, -1,
  11,  7,  4,    11,  4,  2,     8,  3,  4,     3,  2,  4,    -1, -1, -1,
   2,  9, 10,     2,  7,  9,     2,  3,  7,     7,  4,  9,    -1, -1, -1,
   9, 10,  7,     9,  7,  4,    10,  2,  7,     8,  7,  0,     2,  0,  7,
   3,  7, 10,     3, 10,  2,     7,  4, 10,     1, 10,  0,     4,  0, 10,
   1, 10,  2,     8,  7,  4,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  9,  1,     4,  1,  7,     7,  1,  3,    -1, -1, -1,    -1, -1, -1,
   4,  9,  1,     4,  1,  7,     0,  8,  1,     8,  7,  1,    -1, -1, -1,
   4,  0,  3,     7,  4,  3,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   4,  8,  7,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   9, 10,  8,    10, 11,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  0,  9,     3,  9, 11,    11,  9, 10,    -1, -1, -1,    -1, -1, -1,
   0,  1, 10,     0, 10,  8,     8, 10, 11,    -1, -1, -1,    -1, -1, -1,
   3,  1, 10,    11,  3, 10,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  2, 11,     1, 11,  9,     9, 11,  8,    -1, -1, -1,    -1, -1, -1,
   3,  0,  9,     3,  9, 11,     1,  2,  9,     2, 11,  9,    -1, -1, -1,
   0,  2, 11,     8,  0, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   3,  2, 11,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   2,  3,  8,     2,  8, 10,    10,  8,  9,    -1, -1, -1,    -1, -1, -1,
   9, 10,  2,     0,  9,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   2,  3,  8,     2,  8, 10,     0,  1,  8,     1, 10,  8,    -1, -1, -1,
   1, 10,  2,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   1,  3,  8,     9,  1,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  9,  1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
   0,  3,  8,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,
  -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1,    -1, -1, -1
};

/** Instance of a timer to measure time moments. */
Timer timer;

/** Amount of spheres defining scalar field. This value should be synchronized between all files. */
const int n_spheres = 3;

/** Amount of components in sphere position varying. */
const int n_sphere_position_components = 4;

/** Matrix that transforms vertices from model space to perspective projected world space. */
Matrix        mvp;


/* 1. Calculate sphere positions stage variable data. */
/** Program object id for sphere update stage. */
GLuint        spheres_updater_program_id                                 = 0;
/** Vertex shader id for sphere update stage. */
GLuint        spheres_updater_vert_shader_id                             = 0;
/** Fragment shader id for sphere update stage. */
GLuint        spheres_updater_frag_shader_id                             = 0;

/** Buffer object id to store calculated sphere positions. */
GLuint        spheres_updater_sphere_positions_buffer_object_id          = 0;

/** Id of transform feedback object to keep sphere update stage buffer bindings. */
GLuint        spheres_updater_transform_feedback_object_id               = 0;

/** Name of time uniform for sphere positions update stage. */
const GLchar* spheres_updater_uniform_time_name                          = "time";
/** Location of time uniform for sphere positions update stage. */
GLuint        spheres_updater_uniform_time_id                            = 0;

/** Sphere position output variable's name. */
const GLchar* sphere_position_varying_name                               = "sphere_position";


/* 2. Scalar field generation stage variable data. */
/** Program object id for scalar field generator stage. */
GLuint        scalar_field_program_id                                    = 0;
/** Vertex shader id for scalar field generator stage. */
GLuint        scalar_field_vert_shader_id                                = 0;
/** Fragment shader id for scalar field generator stage. */
GLuint        scalar_field_frag_shader_id                                = 0;

/** Buffer object id to store calculated values of scalar field. */
GLuint        scalar_field_buffer_object_id                              = 0;

/** Id of transform feedback object to keep scalar field buffer binding. */
GLuint        scalar_field_transform_feedback_object_id                  = 0;

/** Name of samples_per_axis uniform. */
const GLchar* scalar_field_uniform_samples_per_axis_name                 = "samples_per_axis";
/** Location of samples_per_axis uniform. */
GLuint        scalar_field_uniform_samples_per_axis_id                   = 0;

/** Name of uniform block storing sphere data. */
const GLchar* scalar_field_uniform_spheres_name                          = "spheres_uniform_block";
/** Index of uniform block storing sphere data. */
GLuint        scalar_field_uniform_spheres_id                            = 0;

/** Scalar_field_value output variable's name. */
const GLchar* scalar_field_value_varying_name                            = "scalar_field_value";

/** Id of a 3D texture object storing scalar field data. */
GLuint        scalar_field_texture_object_id                             = 0;


/* 3. Marching Cubes cell-splitting stage variable data. */
/** Program object id for cell splitting stage. */
GLuint        marching_cubes_cells_program_id                            = 0;
/** Vertex shader id for cell splitting stage. */
GLuint        marching_cubes_cells_vert_shader_id                        = 0;
/** Fragment shader id for cell splitting stage. */
GLuint        marching_cubes_cells_frag_shader_id                        = 0;

/** Name of cells_per_axis uniform. */
const GLchar* marching_cubes_cells_uniform_cells_per_axis_name           = "cells_per_axis";
/** Location of cells_per_axis uniform. */
GLuint        marching_cubes_cells_uniform_cells_per_axis_id             = 0;

/** Name of iso_level uniform. */
const GLchar* marching_cubes_cells_uniform_isolevel_name                 = "iso_level";
/** Location of iso_level uniform. */
GLuint        marching_cubes_cells_uniform_isolevel_id                   = 0;

/** Name of scalar_field uniform. */
const GLchar* marching_cubes_cells_uniform_scalar_field_sampler_name     = "scalar_field";
/** Location of scalar_field uniform. */
GLuint        marching_cubes_cells_uniform_scalar_field_sampler_id       = 0;

/** Cell_type_index output variable's name. */
const GLchar* marching_cubes_cells_varying_name                          = "cell_type_index";

/** Id of transform feedback object to keep cell types buffer binding. */
GLuint        marching_cubes_cells_transform_feedback_object_id          = 0;

/** Id of a buffer object to hold result cell type data. */
GLuint        marching_cubes_cells_types_buffer_id                       = 0;

/** Id of a texture object to hold result cell type data. */
GLuint        marching_cubes_cells_types_texture_object_id               = 0;


/* 4. Marching Cubes algorithm triangle generation and rendering stage variable data. */
/** Program object id for marching cubes algorthim's for rendering stage. */
GLuint        marching_cubes_triangles_program_id                        = 0;
/** Vertex shader id for marching cubes algorthim's for rendering stage. */
GLuint        marching_cubes_triangles_frag_shader_id                    = 0;
/** Fragment shader id for marching cubes algorthim's for rendering stage. */
GLuint        marching_cubes_triangles_vert_shader_id                    = 0;

/** Name of samples_per_axis uniform. */
const GLchar* marching_cubes_triangles_uniform_samples_per_axis_name     = "samples_per_axis";
/** Location of samples_per_axis uniform. */
GLuint        marching_cubes_triangles_uniform_samples_per_axis_id       = 0;

/** Name of iso_level uniform. */
const GLchar* marching_cubes_triangles_uniform_isolevel_name             = "iso_level";
/** Location of iso_level uniform. */
GLuint        marching_cubes_triangles_uniform_isolevel_id               = 0;

/** Name of time uniform. */
const GLchar* marching_cubes_triangles_uniform_time_name                 = "time";
/** Location of time uniform. */
GLuint        marching_cubes_triangles_uniform_time_id                   = 0;

/** Name of mvp uniform. */
const GLchar* marching_cubes_triangles_uniform_mvp_name                  = "mvp";
/** Location of mvp uniform. */
GLuint        marching_cubes_triangles_uniform_mvp_id                    = 0;

/** Name of cell_types uniform. */
const GLchar* marching_cubes_triangles_uniform_cell_types_sampler_name   = "cell_types";
/** Location of cell_types uniform. */
GLuint        marching_cubes_triangles_uniform_cell_types_sampler_id     = 0;

/** Name of scalar_field uniform. */
const GLchar* marching_cubes_triangles_uniform_scalar_field_sampler_name = "scalar_field";
/** Location of scalar_field uniform. */
GLuint        marching_cubes_triangles_uniform_scalar_field_sampler_id   = 0;

/** Name of sphere_positions_uniform_block uniform block. */
const GLchar* marching_cubes_triangles_uniform_sphere_positions_name     = "sphere_positions_uniform_block";
/** Index of sphere_positions_uniform_block uniform block. */
GLuint        marching_cubes_triangles_uniform_sphere_positions_id       = 0;

/** Name of tri_table uniform. */
const GLchar* marching_cubes_triangles_uniform_tri_table_sampler_name    = "tri_table";
/** Location of tri_table uniform. */
GLuint        marching_cubes_triangles_uniform_tri_table_sampler_id      = 0;

/** Id of a texture object to hold triangle look-up table data. */
GLuint        marching_cubes_triangles_lookup_table_texture_id           = 0;

/** Id of vertex array object. */
GLuint        marching_cubes_triangles_vao_id                            = 0;


/** Calculates combined model view and projection matrix.
 *
 *  @param mvp combined mvp matrix
 */
void calc_mvp(Matrix& mvp)
{
    /* Define projection properties. */
    float degreesToRadiansCoefficient = atanf(1) / 45;                            /* Coefficient to recalculate degrees to radians.      */
    float frustum_fovy                = 45.0f;                                    /* 45 degrees field of view in the y direction.        */
    float frustum_aspect              = (float)window_width/(float)window_height; /* Aspect ratio.                                       */
    float frustum_z_near              = 0.01f;                                    /* How close the viewer is to the near clipping plane. */
    float frustum_z_far               = 100.0f;                                   /* How far the viewer is from the far clipping plane.  */
    float camera_distance             = 2.5f;                                     /* Distance from camera to scene center.               */

    /* Matrix that stores temporary matrix data for translation transformations. */
    Matrix mat4_translate  = Matrix::createTranslation(-0.5, -0.5, -0.5);

    /* Matrix that stores temporary matrix data for scale transformations. */
    Matrix mat4_scale      = Matrix::createScaling    ( 2.0,  2.0,  2.0);

    /* Matrix that transforms the vertices from model space to world space. */
    /* Translate and scale coordinates from [0..1] to [-1..1] range for full visibility. */
    Matrix mat4_model_view = mat4_scale * mat4_translate;

    /* Pull the camera back from the scene center. */
    mat4_model_view[14] -= float(camera_distance);

    /* Create the perspective matrix from frustum parameters. */
    Matrix mat4_perspective = Matrix::matrixPerspective(degreesToRadiansCoefficient * frustum_fovy, frustum_aspect, frustum_z_near, frustum_z_far);

    /* MVP (Model View Perspective) matrix is a result of multiplication of Perspective Matrix by Model View Matrix. */
    mvp = mat4_perspective * mat4_model_view;
}


/** Initialises OpenGL ES and model environments.
 *
 *  @param width  window width reported by operating system
 *  @param height window width reported by operating system
 */
void setupGraphics(int width, int height)
{
    /* Store window width and height. */
    window_width  = width;
    window_height = height;

    /* Specify one byte alignment for pixels rows in memory for pack and unpack buffers. */
    GL_CHECK(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    GL_CHECK(glPixelStorei(GL_PACK_ALIGNMENT,   1));

    /* 1. Calculate sphere positions stage. */
    /* Create sphere updater program object. */
    spheres_updater_program_id = GL_CHECK(glCreateProgram());

    /* Load and compile sphere updater shaders. */
    Shader::processShader(&spheres_updater_vert_shader_id, spheres_updater_vert_shader, GL_VERTEX_SHADER  );
    Shader::processShader(&spheres_updater_frag_shader_id, spheres_updater_frag_shader, GL_FRAGMENT_SHADER);

    /* Attach the shaders. */
    GL_CHECK(glAttachShader(spheres_updater_program_id, spheres_updater_vert_shader_id));
    GL_CHECK(glAttachShader(spheres_updater_program_id, spheres_updater_frag_shader_id));

    /* [Stage 1 Specifying output variables] */
    /* Specify shader varyings (output variables) we are interested in capturing. */
    GL_CHECK(glTransformFeedbackVaryings(spheres_updater_program_id, 1, &sphere_position_varying_name, GL_SEPARATE_ATTRIBS));

    /* Link the program object. */
    GL_CHECK(glLinkProgram(spheres_updater_program_id));
    /* [Stage 1 Specifying output variables] */

    /* [Stage 1 Specifying input variables] */
    /* Get input uniform location. */
    spheres_updater_uniform_time_id = GL_CHECK(glGetUniformLocation(spheres_updater_program_id, spheres_updater_uniform_time_name));
    /* [Stage 1 Specifying input variables] */

    /* Activate spheres updater program. */
    GL_CHECK(glUseProgram(spheres_updater_program_id));

    /* [Stage 1 Allocate buffer for output values] */
    /* Generate buffer object id. Define required storage space sufficient to hold sphere positions data. */
    GL_CHECK(glGenBuffers(1, &spheres_updater_sphere_positions_buffer_object_id));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, spheres_updater_sphere_positions_buffer_object_id));
    GL_CHECK(glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, n_spheres * n_sphere_position_components * sizeof(GLfloat), NULL, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0));
    /* [Stage 1 Allocate buffer for output values] */

    /* [Stage 1 Transform feedback object initialization] */
    /* Generate and bind transform feedback object. */
    GL_CHECK(glGenTransformFeedbacks(1, &spheres_updater_transform_feedback_object_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, spheres_updater_transform_feedback_object_id));

    /* Bind buffers to store calculated sphere positions. */
    GL_CHECK(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, spheres_updater_sphere_positions_buffer_object_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));
    /* [Stage 1 Transform feedback object initialization] */

    /* 2. Scalar field generation stage. */
    /* Create scalar field generator program object. */
    scalar_field_program_id = GL_CHECK(glCreateProgram());

    /* Load and compile scalar field generator shaders. */
    Shader::processShader(&scalar_field_vert_shader_id, scalar_field_vert_shader, GL_VERTEX_SHADER  );
    Shader::processShader(&scalar_field_frag_shader_id, scalar_field_frag_shader, GL_FRAGMENT_SHADER);

    /* Attach the shaders. */
    GL_CHECK(glAttachShader(scalar_field_program_id, scalar_field_vert_shader_id));
    GL_CHECK(glAttachShader(scalar_field_program_id, scalar_field_frag_shader_id));

    /* Specify shader varyings (output variables) we are interested in capturing. */
    GL_CHECK(glTransformFeedbackVaryings(scalar_field_program_id, 1, &scalar_field_value_varying_name, GL_SEPARATE_ATTRIBS));

    /* Link the program object. */
    GL_CHECK(glLinkProgram(scalar_field_program_id));

    /* Get input uniform locations. */
    scalar_field_uniform_samples_per_axis_id = GL_CHECK(glGetUniformLocation  (scalar_field_program_id, scalar_field_uniform_samples_per_axis_name));
    scalar_field_uniform_spheres_id          = GL_CHECK(glGetUniformBlockIndex(scalar_field_program_id, scalar_field_uniform_spheres_name         ));

    /* Activate scalar field generating program. */
    GL_CHECK(glUseProgram(scalar_field_program_id));

    /* Initialize uniforms constant throughout rendering loop. */
    GL_CHECK(glUniform1i(scalar_field_uniform_samples_per_axis_id, samples_per_axis));

    /* Set binding point for uniform block. */
    GL_CHECK(glUniformBlockBinding(scalar_field_program_id, scalar_field_uniform_spheres_id, 0));
    GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, 0, spheres_updater_sphere_positions_buffer_object_id));

    /* Generate buffer object id. Define required storage space sufficient to hold scalar field data. */
    GL_CHECK(glGenBuffers(1, &scalar_field_buffer_object_id));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, scalar_field_buffer_object_id));

    GL_CHECK(glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, samples_in_3d_space * sizeof(GLfloat), NULL, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0));

    /* Generate and bind transform feedback object. */
    GL_CHECK(glGenTransformFeedbacks(1, &scalar_field_transform_feedback_object_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, scalar_field_transform_feedback_object_id));

    /* Bind buffer to store calculated scalar field values. */
    GL_CHECK(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, scalar_field_buffer_object_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));

    /* [Stage 2 Creating texture] */
    /* Generate texture object to hold scalar field data. */
    GL_CHECK(glGenTextures(1, &scalar_field_texture_object_id));

    /* Scalar field uses GL_TEXTURE_3D target of texture unit 1. */
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    GL_CHECK(glBindTexture(GL_TEXTURE_3D, scalar_field_texture_object_id));

    /* Prepare texture storage for scalar field values. */
    GL_CHECK(glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, samples_per_axis, samples_per_axis, samples_per_axis));
    /* [Stage 2 Creating texture] */

    /* Tune texture settings to use it as a data source. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL,  0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE));


    /* 3. Marching Cubes cell-splitting stage. */
    /* Create a program object to execute Marching Cubes algorithm cell splitting stage. */
    marching_cubes_cells_program_id = GL_CHECK(glCreateProgram());

    /* Marching cubes algorithm shaders initialisation. */
    Shader::processShader(&marching_cubes_cells_vert_shader_id, marching_cubes_cells_vert_shader, GL_VERTEX_SHADER  );
    Shader::processShader(&marching_cubes_cells_frag_shader_id, marching_cubes_cells_frag_shader, GL_FRAGMENT_SHADER);

    /* Attach the shaders. */
    GL_CHECK(glAttachShader(marching_cubes_cells_program_id, marching_cubes_cells_vert_shader_id));
    GL_CHECK(glAttachShader(marching_cubes_cells_program_id, marching_cubes_cells_frag_shader_id));

    /* Specify shader varyings (output variables) we are interested in capturing. */
    GL_CHECK(glTransformFeedbackVaryings(marching_cubes_cells_program_id, 1, &marching_cubes_cells_varying_name, GL_SEPARATE_ATTRIBS));

    /* Link the program object. */
    GL_CHECK(glLinkProgram(marching_cubes_cells_program_id));

    /* Get input uniform locations. */
    marching_cubes_cells_uniform_cells_per_axis_id       = GL_CHECK(glGetUniformLocation(marching_cubes_cells_program_id, marching_cubes_cells_uniform_cells_per_axis_name));
    marching_cubes_cells_uniform_scalar_field_sampler_id = GL_CHECK(glGetUniformLocation(marching_cubes_cells_program_id, marching_cubes_cells_uniform_scalar_field_sampler_name));
    marching_cubes_cells_uniform_isolevel_id             = GL_CHECK(glGetUniformLocation(marching_cubes_cells_program_id, marching_cubes_cells_uniform_isolevel_name));

    /* Activate cell-splitting program. */
    GL_CHECK(glUseProgram(marching_cubes_cells_program_id));

    /* Initialize uniforms constant throughout rendering loop. */
    GL_CHECK(glUniform1i(marching_cubes_cells_uniform_cells_per_axis_id,       cells_per_axis  ));
    GL_CHECK(glUniform1f(marching_cubes_cells_uniform_isolevel_id,             isosurface_level));
    GL_CHECK(glUniform1i(marching_cubes_cells_uniform_scalar_field_sampler_id, 1               ));

    /* Generate buffer object id and allocate memory to store scalar field values. */
    GL_CHECK(glGenBuffers(1, &marching_cubes_cells_types_buffer_id));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, marching_cubes_cells_types_buffer_id));
    GL_CHECK(glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, cells_in_3d_space * sizeof(GLint), NULL, GL_STATIC_DRAW));
    GL_CHECK(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0));

    /* Generate and bind transform feedback object. */
    GL_CHECK(glGenTransformFeedbacks(1, &marching_cubes_cells_transform_feedback_object_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, marching_cubes_cells_transform_feedback_object_id));

    /* Bind buffer to store calculated cell type data. */
    GL_CHECK(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, marching_cubes_cells_types_buffer_id));
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));

    /* [Stage 3 Creating texture] */
    /* Generate a texture object to hold cell type data. (We will explain why the texture later). */
    GL_CHECK(glGenTextures(1, &marching_cubes_cells_types_texture_object_id));

    /* Marching cubes cell type data uses GL_TEXTURE_3D target of texture unit 2. */
    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    GL_CHECK(glBindTexture(GL_TEXTURE_3D, marching_cubes_cells_types_texture_object_id));

    /* Prepare texture storage for marching cube cell type data. */
    GL_CHECK(glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32I, cells_per_axis, cells_per_axis, cells_per_axis));
    /* [Stage 3 Creating texture] */

    /* Tune texture settings to use it as a data source. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL,  0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE));


    /* 4. Marching Cubes algorithm triangle generation and rendering stage. */
    /* Create a program object that we will use for triangle generation and rendering stage. */
    marching_cubes_triangles_program_id = GL_CHECK(glCreateProgram());

    /* Initialize shaders for the triangle generation and rendering stage. */
    Shader::processShader(&marching_cubes_triangles_vert_shader_id, marching_cubes_triangles_vert_shader, GL_VERTEX_SHADER  );
    Shader::processShader(&marching_cubes_triangles_frag_shader_id, marching_cubes_triangles_frag_shader, GL_FRAGMENT_SHADER);

    /* Attach the shaders. */
    GL_CHECK(glAttachShader(marching_cubes_triangles_program_id, marching_cubes_triangles_vert_shader_id));
    GL_CHECK(glAttachShader(marching_cubes_triangles_program_id, marching_cubes_triangles_frag_shader_id));

    /* Link the program object. */
    GL_CHECK(glLinkProgram(marching_cubes_triangles_program_id));

    /* Get input uniform locations. */
    marching_cubes_triangles_uniform_time_id                 = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_time_name                ));
    marching_cubes_triangles_uniform_samples_per_axis_id     = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_samples_per_axis_name    ));
    marching_cubes_triangles_uniform_isolevel_id             = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_isolevel_name            ));
    marching_cubes_triangles_uniform_mvp_id                  = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_mvp_name                 ));
    marching_cubes_triangles_uniform_cell_types_sampler_id   = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_cell_types_sampler_name  ));
    marching_cubes_triangles_uniform_tri_table_sampler_id    = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_tri_table_sampler_name   ));
    marching_cubes_triangles_uniform_scalar_field_sampler_id = GL_CHECK(glGetUniformLocation  (marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_scalar_field_sampler_name));
    marching_cubes_triangles_uniform_sphere_positions_id     = GL_CHECK(glGetUniformBlockIndex(marching_cubes_triangles_program_id, marching_cubes_triangles_uniform_sphere_positions_name    ));

    /* Activate triangle generating and rendering program. */
    GL_CHECK(glUseProgram(marching_cubes_triangles_program_id));

    /* Initialize model view projection matrix. */
    calc_mvp(mvp);

    /* Initialize uniforms constant throughout rendering loop. */
    GL_CHECK(glUniform1f(marching_cubes_triangles_uniform_isolevel_id,             isosurface_level));
    GL_CHECK(glUniform1i(marching_cubes_triangles_uniform_samples_per_axis_id,     samples_per_axis));
    GL_CHECK(glUniform1i(marching_cubes_triangles_uniform_tri_table_sampler_id,    4               ));
    GL_CHECK(glUniform1i(marching_cubes_triangles_uniform_cell_types_sampler_id,   2               ));
    GL_CHECK(glUniform1i(marching_cubes_triangles_uniform_scalar_field_sampler_id, 1               ));
    GL_CHECK(glUniformMatrix4fv(marching_cubes_triangles_uniform_mvp_id, 1, GL_FALSE, mvp.getAsArray()));

    /* Allocate memory for buffer */
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, spheres_updater_sphere_positions_buffer_object_id));

    /* Generate an Id for a texture object to hold look-up array data (tri_table). */
    GL_CHECK(glGenTextures(1, &marching_cubes_triangles_lookup_table_texture_id));

    /* Lookup array (tri_table) uses GL_TEXTURE_2D target of texture unit 4. */
    GL_CHECK(glActiveTexture(GL_TEXTURE4));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, marching_cubes_triangles_lookup_table_texture_id));

    /* Tune texture settings to use it as a data source. */
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST      ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  0               ));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE));
    GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE));

    /* Load lookup table (tri_table) into texture. */
    GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, mc_vertices_per_cell, mc_cells_types_count));
    GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D,        /* Use texture bound to GL_TEXTURE_2D               */
                             0,                    /* Base image level                                 */
                             0,                    /* From the texture origin                          */
                             0,                    /* From the texture origin                          */
                             mc_vertices_per_cell, /* Width will represent vertices in all 5 triangles */
                             mc_cells_types_count, /* Height will represent cell type                  */
                             GL_RED_INTEGER,       /* Texture will have only one component             */
                             GL_INT,               /* ... of type int                                  */
                             tri_table             /* Data will be copied directly from tri_table      */
                            ));

    /* Generate a vertex array object. We'll go with the explanation later. */
    GL_CHECK(glGenVertexArrays(1, &marching_cubes_triangles_vao_id));

    /* In OpenGL ES, draw calls require a bound vertex array object.
     * Even though we're not using any per-vertex attribute data, we still need to bind a vertex array object.
     */
    GL_CHECK(glBindVertexArray(marching_cubes_triangles_vao_id));

    /* Enable facet culling, depth testing and specify front face for polygons. */
    GL_CHECK(glEnable   (GL_DEPTH_TEST));
    GL_CHECK(glEnable   (GL_CULL_FACE ));
    GL_CHECK(glFrontFace(GL_CW        ));

    /* Start counting time. */
    timer.reset();
}

/** Draws one frame. */
void renderFrame(void)
{
    /* Update time. */
    model_time = timer.getTime();

    /*
     * Rendering section
     */
    /* Clear the buffers that we are going to render to in a moment. */
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    /* [Stage 1 Calculate sphere positions stage] */
    /* 1. Calculate sphere positions stage.
     *
     * At this stage we calculate new sphere positions in space
     * according to current time moment.
     */
    /* [Stage 1 Bind buffers to store calculated sphere position values] */
    /* Bind buffers to store calculated sphere position values. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, spheres_updater_transform_feedback_object_id));
    /* [Stage 1 Bind buffers to store calculated sphere position values] */

    /* [Stage 1 Enable GL_RASTERIZER_DISCARD] */
    /* Shorten GL pipeline: we will use vertex shader only. */
    GL_CHECK(glEnable(GL_RASTERIZER_DISCARD));
    /* [Stage 1 Enable GL_RASTERIZER_DISCARD] */
    {
        /* Select program for sphere positions generation stage. */
        GL_CHECK(glUseProgram(spheres_updater_program_id));

        /* [Stage 1 Specify input arguments to vertex shader] */
        /* Specify input arguments to vertex shader. */
        GL_CHECK(glUniform1f(spheres_updater_uniform_time_id, model_time));
        /* [Stage 1 Specify input arguments to vertex shader] */

        /* [Stage 1 Activate transform feedback mode] */
        /* Activate transform feedback mode. */
        GL_CHECK(glBeginTransformFeedback(GL_POINTS));
        /* [Stage 1 Activate transform feedback mode] */
        {
            /* [Stage 1 Execute n_spheres times vertex shader] */
            /* Run sphere positions calculation. */
            GL_CHECK(glDrawArrays(GL_POINTS, 0, n_spheres));
            /* [Stage 1 Execute n_spheres times vertex shader] */
        }
        /* [Stage 1 Deactivate transform feedback mode] */
        GL_CHECK(glEndTransformFeedback());
        /* [Stage 1 Deactivate transform feedback mode] */
    }
    /* [Stage 1 Disable GL_RASTERIZER_DISCARD] */
    GL_CHECK(glDisable(GL_RASTERIZER_DISCARD));
    /* [Stage 1 Disable GL_RASTERIZER_DISCARD] */

    /* Unbind buffers used at this stage. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));
    /* [Stage 1 Calculate sphere positions stage] */


    /* [Stage 2 Scalar field generation stage] */
    /* 2. Scalar field generation stage.
     *
     * At this stage we calculate scalar field and store it in buffer
     * and later copy from buffer to texture.
     */
    /* Bind sphere positions data buffer to GL_UNIFORM_BUFFER. */
    GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, spheres_updater_sphere_positions_buffer_object_id));

    /* Bind buffer object to store calculated scalar field values. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, scalar_field_transform_feedback_object_id));

    /* Shorten GL pipeline: we will use vertex shader only. */
    GL_CHECK(glEnable(GL_RASTERIZER_DISCARD));
    {
        /* Select program for scalar field generation stage. */
        GL_CHECK(glUseProgram(scalar_field_program_id));

        /* Activate transform feedback mode. */
        GL_CHECK(glBeginTransformFeedback(GL_POINTS));
        {
            /* Run scalar field calculation for all vertices in space. */
            GL_CHECK(glDrawArrays(GL_POINTS, 0, samples_in_3d_space));
        }
        GL_CHECK(glEndTransformFeedback());
    }
    GL_CHECK(glDisable(GL_RASTERIZER_DISCARD));

    /* Unbind buffers used at this stage. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));
    /* [Stage 2 Scalar field generation stage] */

    /* Copy scalar field values from buffer into texture bound to target GL_TEXTURE_3D of texture unit 1.
     * We need to move this data to a texture object, as there is no way we could access data
     * stored within a buffer object in an OpenGL ES 3.0 shader.
     */
    /* [Stage 2 Scalar field generation stage move data to texture] */
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    GL_CHECK(glBindBuffer   (GL_PIXEL_UNPACK_BUFFER, scalar_field_buffer_object_id));
    GL_CHECK(glTexSubImage3D(GL_TEXTURE_3D,    /* Use texture bound to GL_TEXTURE_3D                                     */
                             0,                /* Base image level                                                       */
                             0,                /* From the texture origin                                                */
                             0,                /* From the texture origin                                                */
                             0,                /* From the texture origin                                                */
                             samples_per_axis, /* Texture have same width as scalar field in buffer                      */
                             samples_per_axis, /* Texture have same height as scalar field in buffer                     */
                             samples_per_axis, /* Texture have same depth as scalar field in buffer                      */
                             GL_RED,           /* Scalar field gathered in buffer has only one component                 */
                             GL_FLOAT,         /* Scalar field gathered in buffer is of float type                       */
                             NULL              /* Scalar field gathered in buffer bound to GL_PIXEL_UNPACK_BUFFER target */
                            ));
    /* [Stage 2 Scalar field generation stage move data to texture] */


    /* 3. Marching cube algorithm cell splitting stage.
     *
     * At this stage we analyze isosurface in each cell of space and
     * assign one of 256 possible types to each cell. Cell type data
     * for each cell is stored in attached buffer.
     */
    /* Bind buffer to store cell type data. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, marching_cubes_cells_transform_feedback_object_id));

    /* Shorten GL pipeline: we will use vertex shader only. */
    GL_CHECK(glEnable(GL_RASTERIZER_DISCARD));
    {
        /* Select program for Marching Cubes algorthim's cell splitting stage. */
        GL_CHECK(glUseProgram(marching_cubes_cells_program_id));

        /* Activate transform feedback mode. */
        GL_CHECK(glBeginTransformFeedback(GL_POINTS));
        {
            /* [Stage 3 Execute vertex shader] */
            /* Run Marching Cubes algorithm cell splitting stage for all cells. */
            GL_CHECK(glDrawArrays(GL_POINTS, 0, cells_in_3d_space));
            /* [Stage 3 Execute vertex shader] */
        }
        GL_CHECK(glEndTransformFeedback());
    }
    GL_CHECK(glDisable(GL_RASTERIZER_DISCARD));

    /* Unbind buffers used at this stage. */
    GL_CHECK(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0));

    /* Copy data from buffer into texture bound to target GL_TEXTURE2 in texture unit 2.
     * We need to move this data to a texture object, as there is no way we could access data
     * stored within a buffer object in a OpenGL ES 3.0 shader.
     */
    GL_CHECK(glActiveTexture(GL_TEXTURE2));
    GL_CHECK(glBindBuffer   (GL_PIXEL_UNPACK_BUFFER, marching_cubes_cells_types_buffer_id));
    GL_CHECK(glTexSubImage3D(GL_TEXTURE_3D,  /* Use texture bound to GL_TEXTURE_3D                                   */
                             0,              /* Base image level                                                     */
                             0,              /* From the texture origin                                              */
                             0,              /* From the texture origin                                              */
                             0,              /* From the texture origin                                              */
                             cells_per_axis, /* Texture have same width as cells by width in buffer                  */
                             cells_per_axis, /* Texture have same height as cells by height in buffer                */
                             cells_per_axis, /* Texture have same depth as cells by depth in buffer                  */
                             GL_RED_INTEGER, /* Cell types gathered in buffer have only one component                */
                             GL_INT,         /* Cell types gathered in buffer are of int type                        */
                             NULL            /* Cell types gathered in buffer bound to GL_PIXEL_UNPACK_BUFFER target */
                            ));


    /* 4. Marching Cubes algorithm triangle generation stage.
     *
     * At this stage, we render exactly (3 vertices * 5 triangles per cell *
     * amount of cells the scalar field is split to) triangle vertices.
     * Then render triangularized geometry.
     */
    GL_CHECK(glActiveTexture(GL_TEXTURE0));

    /* Activate triangle generating and rendering program. */
    GL_CHECK(glUseProgram(marching_cubes_triangles_program_id));

    /* Specify input arguments to vertex shader. */
    GL_CHECK(glUniform1f(marching_cubes_triangles_uniform_time_id, model_time));

    /* [Stage 4 Run triangle generating and rendering program] */
    /* Run triangle generating and rendering program. */
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, cells_in_3d_space * triangles_per_cell * vertices_per_triangle));
    /* [Stage 4 Run triangle generating and rendering program] */
}

/** Deinitialises OpenGL ES environment. */
void cleanup()
{
    GL_CHECK(glDeleteVertexArrays      (1, &marching_cubes_triangles_vao_id                  ));
    GL_CHECK(glDeleteShader            (    marching_cubes_triangles_frag_shader_id          ));
    GL_CHECK(glDeleteShader            (    marching_cubes_triangles_vert_shader_id          ));
    GL_CHECK(glDeleteProgram           (    marching_cubes_triangles_program_id              ));
    GL_CHECK(glDeleteTextures          (1, &marching_cubes_triangles_lookup_table_texture_id ));
    GL_CHECK(glDeleteTextures          (1, &marching_cubes_cells_types_texture_object_id     ));
    GL_CHECK(glDeleteTransformFeedbacks(1, &marching_cubes_cells_transform_feedback_object_id));
    GL_CHECK(glDeleteBuffers           (1, &marching_cubes_cells_types_buffer_id             ));
    GL_CHECK(glDeleteShader            (    marching_cubes_cells_frag_shader_id              ));
    GL_CHECK(glDeleteShader            (    marching_cubes_cells_vert_shader_id              ));
    GL_CHECK(glDeleteProgram           (    marching_cubes_cells_program_id                  ));
    GL_CHECK(glDeleteTextures          (1, &scalar_field_texture_object_id                   ));
    GL_CHECK(glDeleteTransformFeedbacks(1, &scalar_field_transform_feedback_object_id        ));
    GL_CHECK(glDeleteBuffers           (1, &scalar_field_buffer_object_id                    ));
    GL_CHECK(glDeleteShader            (    scalar_field_frag_shader_id                      ));
    GL_CHECK(glDeleteShader            (    scalar_field_vert_shader_id                      ));
    GL_CHECK(glDeleteProgram           (    scalar_field_program_id                          ));
    GL_CHECK(glDeleteTransformFeedbacks(1, &spheres_updater_transform_feedback_object_id     ));
    GL_CHECK(glDeleteBuffers           (1, &spheres_updater_sphere_positions_buffer_object_id));
    GL_CHECK(glDeleteShader            (    spheres_updater_frag_shader_id                   ));
    GL_CHECK(glDeleteShader            (    spheres_updater_vert_shader_id                   ));
    GL_CHECK(glDeleteProgram           (    spheres_updater_program_id                       ));
}


extern "C"
{
    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_metaballs_NativeLibrary_init
    (JNIEnv *env, jclass jcls, jint width, jint height)
    {
        /* Initialze OpenGL ES and model environment for metaballs: allocate buffers and textures, bind them, etc. */
        setupGraphics(width, height);
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_metaballs_NativeLibrary_step
    (JNIEnv *env, jclass jcls)
    {
        /* Render a frame */
        renderFrame();
    }

    JNIEXPORT void JNICALL Java_com_arm_malideveloper_openglessdk_metaballs_NativeLibrary_uninit
    (JNIEnv *, jclass)
    {
        cleanup();
    }
}
