/* Copyright (c) 2012-2017, ARM Limited and Contributors
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

varying vec3 v_v3Position;
varying vec3 v_v3Normal;

uniform vec3 u_v3Color;

const vec3 lightPos = vec3(0.0, 1.0, -0.25);
const vec3 ambientColor = vec3(0.1, 0.1, 0.1);
const vec3 specularColor = vec3(1.0, 1.0, 1.0);

const float shininess = 16.0;

void main()
{
	vec3 diffuseColor = u_v3Color;
	vec3 lightDirection = normalize(lightPos - v_v3Position);

	float lambert = max(dot(lightDirection, v_v3Normal), 0.0);

	float specular = 0.0;
	if (lambert > 0.0)
	{
		vec3 viewDirection = normalize(-v_v3Position);
		vec3 halfDirection = normalize(lightDirection + viewDirection);

		float specAngle = max(dot(halfDirection, normalize(v_v3Normal)), 0.0);
		specular = pow(specAngle, shininess);
	}

	gl_FragColor = vec4(ambientColor * diffuseColor + lambert * diffuseColor + specular * specularColor, 1.0);
}
