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

#ifndef INTEGER_LOGIC_H
#define INTEGER_LOGIC_H

#include "VectorTypes.h"
#include <EGL/egl.h>
#include <GLES3/gl3.h>
    
namespace MaliSDK
{
    /* Name of the file in which "rule 30" vertex shader's body is located. */
    #define  VERTEX_RULE_30_SHADER_FILENAME ("/data/data/com.arm.malideveloper.openglessdk.integerLogic/files/IntegerLogic_Rule30_shader.vert")
    /* Name of the file in which "merge" vertex shader's body is located. */
    #define  VERTEX_MERGE_SHADER_FILENAME ("/data/data/com.arm.malideveloper.openglessdk.integerLogic/files/IntegerLogic_Merge_shader.vert")
    /* Name of the file in which "rule 30" fragment shader's body is located. */
    #define  FRAGMENT_RULE_30_SHADER_FILENAME ("/data/data/com.arm.malideveloper.openglessdk.integerLogic/files/IntegerLogic_Rule30_shader.frag")
    /* Name of the file in which "merge" fragment shader's body is located. */
    #define  FRAGMENT_MERGE_SHADER_FILENAME ("/data/data/com.arm.malideveloper.openglessdk.integerLogic/files/IntegerLogic_Merge_shader.frag")

    /* Structure storing locations of attributes and uniforms for merge program. */
    struct MergeProgramLocations
    {
        GLint mvpMatrixLocation;
        GLint pingTextureLocation;
        GLint pongTextureLocation;
        GLint positionLocation;
        GLint texCoordLocation;

        MergeProgramLocations()
        {
            mvpMatrixLocation   = -1;
            pingTextureLocation = -1;
            pongTextureLocation = -1;
            positionLocation    = -1;
            texCoordLocation    = -1;
        }
    };

    /* Structure storing locations of attributes and uniforms for rule30 program. */
    struct Rule30ProgramLocations
    {
        GLint inputTextureLocation;
        GLint verticalOffsetLocation;
        GLint mvpMatrixLocation;
        GLint inputVerticalOffsetLocation;
        GLint inputNeighbourLocation;
        GLint positionLocation;
        GLint texCoordLocation;

        Rule30ProgramLocations()
        {
            inputTextureLocation        = -1;
            verticalOffsetLocation      = -1;
            mvpMatrixLocation           = -1;
            inputVerticalOffsetLocation = -1;
            inputNeighbourLocation      = -1;
            positionLocation            = -1;
            texCoordLocation            = -1;
        }
    };

    /*
     * Vertex array, storing coordinates for a single line filling whole row.
     * The 4th coordinate is set to 0.5 to reduce clip coordinates to [-0.5, 0.5].
     * It is necessary so the offsets are the same as for UV coordinatesin Rule30 shader.
     */
    static const float lineVertices[] =
    {
        -0.5f, 1.0f, 0.0f, 0.5f,
         0.5f, 1.0f, 0.0f, 0.5f,
    };

    /*
     * Vertex array, storing coordinates for the quad in the fullscreen.
     * The 4th coordinate is set to 0.5 to keep compatibility with line vertices.
     */
    static const float quadVertices[] =
    {
        -0.5f,  1.0f, 0.0f, 0.5f,
         0.5f,  1.0f, 0.0f, 0.5f,
        -0.5f, -1.0f, 0.0f, 0.5f,
         0.5f, -1.0f, 0.0f, 0.5f,
    };

    /* UV array. Used to map texture on both line ends. */
    static const float lineTextureCoordinates[] =
    {
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    /* UV array. Used to map texture on the whole quad. */
    static const float quadTextureCoordinates[] =
    {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
    };
}
#endif /* INTEGER_LOGIC_H */
