#version 300 es

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

precision highp float;
precision lowp usampler2D;

/* Determines the resulting colour by taking appropriate data from inputTexture. */
uvec4 determineResultColor();

/* Apply Rule 30 using the pixels from one line above.
 * Since the input textures contain only red component, the implementation checks
 * if its value in the upper line pixels is higher or lower than 0.5. Then it returns
 * black or white colour vectors, following the Rule 30.
 */
uvec4 applyRule30(uvec4 upperLeftPixel, uvec4 upperCenterPixel, uvec4 upperRightPixel);

/* UV coordinates received from vertex shader. */
in vec2 fragmentTexCoord;

/* Sampler holding current input texture, from which the output is determined. */
uniform usampler2D inputTexture;
/* A uniform used to determine UV coordinates of the input line. */
uniform float     inputVerticalOffset;
/* A uniform used to point to neighbouring pixels. */
uniform float     inputNeighbour;

/* Output variable. */
out uvec4 fragColor;

void main()
{
    fragColor = determineResultColor();
}

/* See the description of the declaration. */
uvec4 determineResultColor()
{
    uvec4 upperLeftPixel   = texture(inputTexture, fragmentTexCoord - vec2(inputNeighbour,  inputVerticalOffset));
    uvec4 upperCenterPixel = texture(inputTexture, fragmentTexCoord - vec2(0.0,             inputVerticalOffset));
    uvec4 upperRightPixel  = texture(inputTexture, fragmentTexCoord - vec2(-inputNeighbour, inputVerticalOffset));

    return applyRule30(upperLeftPixel, upperCenterPixel, upperRightPixel);
}

/* See the description of the declaration. */
uvec4 applyRule30(uvec4 upperLeftPixel, uvec4 upperCenterPixel, uvec4 upperRightPixel)
{
    /* Value of upper left pixel to be compared. */
    uint upperLeftCompare   = upperLeftPixel.r;
    /* Value of upper center pixel to be compared. */
    uint upperCenterCompare = upperCenterPixel.r;
    /* Value of upper right pixel to be compared. */
    uint upperRightCompare  = upperRightPixel.r;

    if (upperLeftCompare == 0u   && upperCenterCompare == 0u   && upperRightCompare == 0u   ||
        upperLeftCompare == 255u && upperCenterCompare == 0u   && upperRightCompare == 255u ||
        upperLeftCompare == 255u && upperCenterCompare == 255u && upperRightCompare == 0u   ||
        upperLeftCompare == 255u && upperCenterCompare == 255u && upperRightCompare == 255u)
    {
        return uvec4(0u, 0u, 0u, 255u);
    }
    else
    {
        return uvec4(255u, 255u, 255u, 255u);
    }
}
