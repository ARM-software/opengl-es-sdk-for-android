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
 * The main idea of the sorting algorithm is based on counting.
 * For example, say that we need to put the number 3 in sorted order,
 * and we know that there are in total 5 numbers less than 3 in the input.
 * Then we know that our number must come after all of these - that is, in position 5!
 *
 * To determine such a value for each number in our input, we use a prefix sum
 * (also known as a scan).
 *
 * It works like this, let's make this our input that we want to sort:
 *     1 3 2 0 1 0 2 2
 *
 * (the actual input may have values greater than 3, but the scan only operates
 * on 2 bit values, because the radix sort works on 2-bit stages. We mask out
 * the interesting digit based on the <bitOffset> value using bitfieldExtract().)
 *
 * We then construct four flag arrays, one for each possible digit,
 * that has a 1 where the key matches the digit, and 0 elsewhere:
 *     0 0 0 1 0 1 0 0 <- 0
 *     1 0 0 0 1 0 0 0 <- 1
 *     0 0 1 0 0 0 1 1 <- 2
 *     0 1 0 0 0 0 0 0 <- 3
 *
 * If we do an exclusive prefix sum over these arrays (carrying over the sum
 * from each array to the next) we get:
 *     0 0 0 0 1 1 2 2 <- 0
 *     2 3 3 3 3 4 4 4 <- 1
 *     4 4 4 5 5 5 5 6 <- 2
 *     7 7 8 8 8 8 8 8 <- 3 (note that 7 was carried over instead of 6)
 *
 * Now we have all we need!
 * We then go through each element in the input, and look at this table to find
 * out where the element should go in the sorted output.
 *
 * For example, the first 0 is located in the fourth column (as marked by the flag).
 * The scan array that corresponds to 0 contains the number 0 at this columns.
 * Thus, the first 0 should go to location 0.
 *
 * Not too bad!
 * What about the first 1?
 *
 * It is masked in the first column, second row.
 * We then look at the second row to determine its offset.
 * The scan value there is 2. So the first 1 should go to index 2 in the output.
*/

/*
 * For efficiency, we perform the prefix sum in blocks over multiple stages.
 * For example, with blocks of 4-by-4 elements, calculating the prefix sum of
 *     0 0 0 1 0 1 0 0
 * is done by first scanning each block individually
 *     0 0 0 0 | 0 0 1 1
 *
 * This can be done highly parallel over multiple shader cores and lots of threads
 * if the work set is large enough.
 *
 * To merge these two we need to add the sum of elements in the first block,
 * to each element in the second block. So first we compute the sums
 *     1 | 1
 * then we take the prefix sum of this again!
 *     0 | 1
 * Finally we add blocksum[i] to each element in block[i], and so on
 *     0 0 0 0 (+ 0) | 0 0 1 1 (+ 1)
 *     0 0 0 0 | 1 1 2 2
 *
 * This is a recursive problem since we need to take the prefix sum of block sums.
 * We might have enough blocks that this problem can also be quite parallel.
 * Our implementation takes a recursive approach where we keep scanning the
 * residual block sums in parallel until the blocksum becomes a single value.
 * Once we have worked our way down, we need to "resolve" the prefix sum
 * in the smaller blocks all the way up so we can get the complete prefix sum again.
 *
 * Because we need to perform four prefix sums, one for every digit, the sums array is a uvec4 array.
 * The xyzw components correspond to the digit 0, 1, 2 and 3
 * scan arrays, respectively. Storing it as a vector allows us to use vector math
 * to operate on each array in single expressions.
 *
 * In the implementation, we use inclusive scan (postfix sum) instead of exclusive scan (prefix sum).
 * The difference between these is trivially resolved in scan_reorder.cs.
 */

layout(local_size_x = 32) in; // We work on 4 items at once, so this value should be BLOCK_SIZE / 4.
#define NUM_STEPS 4u
layout(binding = 0, std430) readonly buffer Data
{
    vec4 in_points[];
};

layout(binding = 1, std430) writeonly buffer OutData
{
    uvec4 outbuf[];
};

layout(binding = 2, std430) writeonly buffer BlockSumData
{
    uvec4 blocksum[];
};

layout(binding = 3, std430) writeonly buffer FlagsData
{
    uvec4 flags[];
};

shared uvec4 sharedData[gl_WorkGroupSize.x];

uniform int bitOffset;
uniform vec3 axis;
uniform float zMin;
uniform float zMax;

/*
 * The particles are sorted by increasing distance along the sorting axis.
 * We find the distance by a simple dot product. The sorting algorithm
 * needs integer keys (16-bit in this case), so we convert the distance from
 * the range [zMin, zMax] -> [0, 65535].
 * Finally we extract the current working digits (2-bit in this case) from the key.
 *
 * Read four particles at once to improve vectorization a bit.
 */
uvec4 decodeKeys(uint index)
{
    vec4 z = vec4(
            dot(in_points[4u * index + 0u].xyz, axis),
            dot(in_points[4u * index + 1u].xyz, axis),
            dot(in_points[4u * index + 2u].xyz, axis),
            dot(in_points[4u * index + 3u].xyz, axis));
    z = 65535.0 * clamp((z - zMin) / (zMax - zMin), vec4(0.0), vec4(1.0));
    return bitfieldExtract(uvec4(z), bitOffset, 2);
}

void main()
{
    uint ident = gl_GlobalInvocationID.x;
    uint local_ident = gl_LocalInvocationID.x;

    uvec4 flagdata = decodeKeys(ident);
    // Save the flags data for later. Will be used in scan_reorder.cs.
    // We could recompute in scan_reorder.cs of course if we wanted.
    flags[ident] = flagdata;

    // Create flags by comparing against the possible 2-bit values.
    uvec4 miniblock0 = uvec4(equal(flagdata.xxxx, uvec4(0u, 1u, 2u, 3u)));
    uvec4 miniblock1 = uvec4(equal(flagdata.yyyy, uvec4(0u, 1u, 2u, 3u)));
    uvec4 miniblock2 = uvec4(equal(flagdata.zzzz, uvec4(0u, 1u, 2u, 3u)));
    uvec4 miniblock3 = uvec4(equal(flagdata.wwww, uvec4(0u, 1u, 2u, 3u)));

    // First we want to do the scan in registers to reduce shared memory pressure a bit.
    miniblock1 += miniblock0;
    miniblock2 += miniblock1;
    miniblock3 += miniblock2;

    // We have now done inclusive scan for our "miniblock".
    // We only share our accumulated sum (miniblock3) with other threads.

    // Share miniblock sum with other threads
    sharedData[local_ident] = miniblock3;
    memoryBarrierShared();
    barrier();

    for (uint step = 0u; step < NUM_STEPS; step++) {
        // Half the threads will have something useful to do every step.
        // Branching like this is a not an issue on Mali Midgard as long as we keep enough threads busy doing something useful.
        if ((local_ident & (1u << step)) != 0u) {
            // Get previous index. This value will be the same for every thread within this "block".
            uint prev = ((local_ident >> step) << step) - 1u;

            // Update our block. Always accumulate data in registers.
            uvec4 sum_prev = sharedData[prev];
            miniblock0 += sum_prev;
            miniblock1 += sum_prev;
            miniblock2 += sum_prev;
            miniblock3 += sum_prev;

            // Write out current value.
            sharedData[local_ident] = miniblock3;
        }
        memoryBarrierShared();
        barrier();
    }

    // We don't need barrier after last iteration, so unroll that manually.
    if ((local_ident & (1u << NUM_STEPS)) != 0u) {
        // Get previous index. This value will be the same for every thread within this "block".
        uint prev = ((local_ident >> NUM_STEPS) << NUM_STEPS) - 1u;

        // Update our block. Always accumulate data in registers.
        uvec4 sum_prev = sharedData[prev];
        miniblock0 += sum_prev;
        miniblock1 += sum_prev;
        miniblock2 += sum_prev;
        miniblock3 += sum_prev;
    }

    // Write out inclusive scan results.
    outbuf[4u * ident + 0u] = miniblock0;
    outbuf[4u * ident + 1u] = miniblock1;
    outbuf[4u * ident + 2u] = miniblock2;
    outbuf[4u * ident + 3u] = miniblock3;

    // Last thread knows the inclusive scan for this work group, so write out to blocksum.
    if (local_ident == (gl_WorkGroupSize.x - 1u))
        blocksum[gl_WorkGroupID.x] = miniblock3;
}
