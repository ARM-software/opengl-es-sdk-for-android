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

#ifndef SHADOW_MAPPING_H
#define SHADOW_MAPPING_H

#include "VectorTypes.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
    
namespace MaliSDK
{
    /** Name of a fragment shader file that will be used to render a cube representing the spot light source. */
    #define SPOT_LIGHT_CUBE_FRAGMENT_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.shadowMapping/files/cube_light_fragment_shader_source.frag")
    /** Name of a vertex shader file that will be used to render a cube representing the spot light source. */
    #define SPOT_LIGHT_CUBE_VERTEX_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.shadowMapping/files/cube_light_vertex_shader_source.vert")
    /** Name of a fragment shader file that will be used to render a scene. */
    #define FRAGMENT_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.shadowMapping/files/lighting_fragment_shader_source.frag")
    /** Name of a vertex shader file that will be used to render a scene. */
    #define VERTEX_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.shadowMapping/files/model_vertex.vert")
}
#endif /* SHADOW_MAPPING_H */
