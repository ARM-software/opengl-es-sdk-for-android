#version 310 es

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

/*
 * Take a scan array which has been scanned per-block and sum array (inclusive scan for per-block results)
 * and resolve the result to get a complete inclusive scan result.
 * Needed if we need to do scan in multiple stages.
 */

layout(local_size_x = 32) in; // We work on 4 items at once, so this value should be BLOCK_SIZE / 4.
layout(binding = 0, std430) readonly buffer Data
{
    uvec4 buf[];
};

layout(binding = 1, std430) readonly buffer BlockSumData
{
    uvec4 blocksum[];
};

layout(binding = 2, std430) writeonly buffer OutData
{
    uvec4 outbuf[];
};

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    uint wg_ident = gl_WorkGroupID.x;
    uvec4 miniblock0 = buf[4u * ident + 0u];
    uvec4 miniblock1 = buf[4u * ident + 1u];
    uvec4 miniblock2 = buf[4u * ident + 2u];
    uvec4 miniblock3 = buf[4u * ident + 3u];
    if (wg_ident != 0u) {
        uvec4 prev_sum = blocksum[wg_ident - 1u];
        miniblock0 += prev_sum;
        miniblock1 += prev_sum;
        miniblock2 += prev_sum;
        miniblock3 += prev_sum;
    }
    outbuf[4u * ident + 0u] = miniblock0;
    outbuf[4u * ident + 1u] = miniblock1;
    outbuf[4u * ident + 2u] = miniblock2;
    outbuf[4u * ident + 3u] = miniblock3;
}
