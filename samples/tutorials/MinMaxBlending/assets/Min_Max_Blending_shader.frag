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

/* [Fragment shader code] */
precision mediump float;
precision mediump int;
precision mediump isampler3D;

/* Value used to normalize colour values. */
const highp int maxShort = 32768;
/* Value used to brighten output colour. */
const float contrastModifier = 3.0;
 
/* Input taken from vertex shader. */ 
in vec3 uvwCoordinates;

/* 3D integer texture sampler. */
uniform isampler3D textureSampler;
/* Boolean value indicating current blending equation. */
uniform bool isMinBlending;
/* Threshold used for min blending. */
uniform float minBlendingThreshold;

/* Output variable. */
out vec4 fragColor;

void main()
{
    /* Loaded texture short integer data are in big endian order. Swap the bytes. */
    ivec4 initialTexture          = ivec4(texture(textureSampler, uvwCoordinates).rrr, 1.0);
    ivec4 swappedBytesTextureTemp =  (initialTexture << 8) & ivec4(0xFF00);
    ivec4 swappedBytesTexture     = ((initialTexture >> 8) & ivec4(0x00FF)) | swappedBytesTextureTemp;

    /* Determine output fragment colour. */
    fragColor = vec4(swappedBytesTexture) / float(maxShort) * contrastModifier;

    /* If min blending is set, discard fragments that are not bright enough. */
    if (isMinBlending && length(fragColor) < minBlendingThreshold)
    {
        discard;
    }
}
/* [Fragment shader code] */