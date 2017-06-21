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

layout(num_views = 2) in;

in vec4 vertexPosition;
in vec3 vertexNormal;
in vec3 vertexTangent;
in vec2 uvCoordinates;

uniform mat4 View[2];
uniform mat4 Projection[2];
uniform mat4 ModelView[2];
uniform mat4 ModelViewProjection[2];
uniform mat4 Model;

out vec2 vUV;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vEyeVec;
out vec3 vCamPos;

const vec3 cameraLocation = vec3(0.0,0.0,0.0);

void main()
{
    gl_Position = ModelViewProjection[gl_ViewID_OVR] * vertexPosition;

    vNormal = normalize(( vec4(vertexNormal, 0.0)) * inverse(Model)).xyz;
    vTangent = normalize((Model * vec4(vertexTangent, 0.0)).xyz);
    vEyeVec = (Model * vertexPosition).xyz - cameraLocation;
    vUV = uvCoordinates;
    vCamPos = cameraLocation;
}