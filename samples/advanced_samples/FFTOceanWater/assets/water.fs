#version 310 es

/* Copyright (c) 2015-2017, ARM Limited and Contributors
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

precision mediump float;

in highp vec3 vWorld;
in highp vec4 vGradNormalTex;

mediump out vec3 FragColor;

layout(location = 4) uniform highp vec3 uCamPos;

layout(binding = 1) uniform mediump sampler2D uGradJacobian;
layout(binding = 2) uniform sampler2D uNormal;
layout(binding = 3) uniform samplerCube uSkydome;

void main()
{
    vec3 vGradJacobian = texture(uGradJacobian, vGradNormalTex.xy).xyz;
    vec2 noise_gradient = 0.30 * texture(uNormal, vGradNormalTex.zw).xy;

    float jacobian = vGradJacobian.z;
    float turbulence = max(2.0 - jacobian + dot(abs(noise_gradient), vec2(1.2)), 0.0);

    // Add low frequency gradient from heightmap with gradient from high frequency noisemap.
    vec3 normal = vec3(-vGradJacobian.x, 1.0, -vGradJacobian.y);
    normal.xz -= noise_gradient;
    normal = normalize(normal);

    // Make water brighter based on how turbulent the water is.
    // This is rather "arbitrary", but looks pretty good in practice.
    float color_mod = 1.0 + 3.0 * smoothstep(1.2, 1.8, turbulence);

    // Compute fog by sampling skydome.
    highp vec3 world_to_cam = uCamPos - vWorld;
    highp float fog = exp2(-0.000008 * dot(world_to_cam, world_to_cam));

    world_to_cam = normalize(world_to_cam);
    float cosTheta = clamp(dot(world_to_cam, normal), 0.0, 1.0);
    vec3 reflected = reflect(-world_to_cam, normal);

    // If a noisy normal makes us reflect downwards, just make it point upwards instead.
    reflected.y = abs(reflected.y);

    // Sample cubemap.
    vec3 spec = texture(uSkydome, reflected).rgb;

    // Schlick's approximation for fresnel term.
    const float c_spec = 0.02; // F(0) for water.
    float rTheta = c_spec + (1.0 - c_spec) * pow(1.0 - cosTheta, 5.0);

    // Do not add in diffuse term.
    vec3 color = color_mod * spec * rTheta;

    // Blend in fog.
    FragColor = mix(texture(uSkydome, -world_to_cam).rgb, color, fog);

    // Simple gamma correction.
    FragColor = sqrt(FragColor);
}
