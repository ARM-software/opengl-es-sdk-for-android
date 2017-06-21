/* Copyright (c) 2017, ARM Limited and Contributors
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

#pragma once
#include <time.h>

namespace Stats
{
	timespec endFrame;
	timespec startFrame;
	uint32_t nbFrame = 0;
	uint32_t totalNbFrame;
	double TotalFrameTime = 0;

	inline void StartFrame()
	{
		startFrame = endFrame;
	}

	inline void EndFrame()
	{
		clock_gettime(CLOCK_REALTIME, &endFrame );
		float diff = (endFrame.tv_sec - startFrame.tv_sec) + (endFrame.tv_nsec - startFrame.tv_nsec);
		if (diff < 0)
			diff += 1000000000;

		TotalFrameTime += diff;
		++nbFrame;
		++totalNbFrame;
	}

	inline float GetFPS()
	{
		return  1000000000 / (TotalFrameTime / nbFrame);
	}

	inline float GetAverageFrameTime()
	{
		return (TotalFrameTime / nbFrame) / 1000000;
	}

	inline void Clear()
	{
		TotalFrameTime = 0;
		nbFrame = 0;
	}

}
