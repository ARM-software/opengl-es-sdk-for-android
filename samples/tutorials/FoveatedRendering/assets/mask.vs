#version 310 es

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

#extension GL_OVR_multiview2 : enable

in vec3 vertexPosition;

uniform uint maskType;

layout(num_views = 4) in;

void main()
{
	vec3 position = vertexPosition;

	// Force the mask to be rendered in front of everything by default
	position.z = 0.0f;

	// If it's not the right mask for the view, push it to the background
	if((gl_ViewID_OVR == uint(2) || gl_ViewID_OVR == uint(3)) && maskType == uint(1)){
		position.z = 1.0f;
	}
	if((gl_ViewID_OVR == uint(0) || gl_ViewID_OVR == uint(1)) && maskType == uint(2)){
		position.z = 1.0f;
	}

	gl_Position = vec4(position, 1.0);
}