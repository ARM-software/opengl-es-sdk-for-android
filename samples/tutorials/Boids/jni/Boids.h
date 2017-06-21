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

#ifndef BOIDS_H
#define BOIDS_H

namespace MaliSDK
{
    /** Name of a fragment shader file. */
    #define FRAGMENT_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.boids/files/fragment_shader_source.frag")
    /** Name of a vertex shader file. */
    #define VERTEX_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.boids/files/vertex_shader_source.vert")
    /** Name of a movement fragment shader file. */
    #define MOVEMENT_FRAGMENT_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.boids/files/movement.frag")
    /** Name of a movement vertex shader file. */
    #define MOVEMENT_VERTEX_SHADER_FILE_NAME ("/data/data/com.arm.malideveloper.openglessdk.boids/files/movement.vert")
}
#endif /* BOIDS_H */
