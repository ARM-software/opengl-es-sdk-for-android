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

#ifndef OCCLUSION_QUERIES_H
#define OCCLUSION_QUERIES_H

    /* Interval expressed in seconds in which we change between modes. */
    #define TIME_INTERVAL (10.0f)

    /* Determines number of cubes that are going to be rendered per frame. */
    #define NUMBER_OF_CUBES (20)

    /* Determines accuracy of rounded cubes - number of sample triangles that will make up a super ellipsoid. */
    #define NUMBER_OF_SAMPLES (256)

    /* These two "squareness" parameters determine what kind of figure we will get.
     * Different values can create for example a sphere, rounded cube, something like a star, cylinder, etc.
     * These given values (0.3f and 0.3f) will create a rounded cube. */
    #define SQUARENESS_1 (0.3f)
    #define SQUARENESS_2 (0.3f)

    /* These variables are used to scale up cubes (normal and rounded).
     * normalCubeScaleFactor has to be smaller than roundedCubeScaleFactor
     * to avoid blinking effect (some cubes disappear some appear). */
    #define ROUNDED_CUBE_SCALE_FACTOR (2.5f)
    #define NORMAL_CUBE_SCALE_FACTOR  (2.3f)

#endif /* OCCLUSION_QUERIES_H */
