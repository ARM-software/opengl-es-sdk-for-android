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

precision highp float;

// Pass from vertex
highp in vec3 vEyeVec;
highp in vec2 vUV;
mediump in vec3 vNormal;
mediump in vec3 vTangent;
mediump in vec3 vCamPos;

const vec3 uLightPos0 = vec3(0.0, 0.0, -50.0);
const vec3 uLightColor0 = vec3(232.0/255.0, 234.0/255.0, 178.0/255.0);
const float uInvRadius0 = 0.01;
const vec3 uLightPos1 = vec3(-20.0, 5.0, -50.0);
const vec3 uLightColor1 = vec3(22.0/255.0, 71.0/255.0, 247.0/255.0);
const float uInvRadius1 = 0.02;

// Moving light
vec3 uLightPos2 = vec3(35.0, -20.0, -50.0);
const vec3 uLightColor2 = vec3(244.0/255.0, 125.0/255.0, 125.0/255.0);
const float uInvRadius2 = 0.025;

uniform mediump sampler2D TexDiffuse;
uniform mediump sampler2D TexNormal;
uniform mediump sampler2D TexMetallicRoughness;
uniform mediump sampler2D TexBump;

uniform mediump float Time;

layout(location = 0) out vec4 FragColor;

#define saturate(x) clamp(x, 0.0, 1.0)
#define PI 3.14159265359

struct Material
{
	vec4 Diffuse;
	vec3 Normal;
	float Metallic;
	float Roughness;
	float ReflectionCoefficient;
};

struct LightingData
{
	vec3 WorldPos;
	vec3 EyeDir;
	vec3 ReflDir;
	vec2 BRDFlut;

	vec3 Diffuse;
	vec3 Reflectance;
	vec3 Normal;
	float Transparency;
	float Roughness;
	float Metallic;

	float n_dot_v;
};

LightingData SetupLightingData(vec3 WorldPos, vec3 EyeDir, Material mat)
{
	LightingData ld;

	ld.WorldPos = WorldPos;
	ld.Normal = mat.Normal;
	ld.Roughness = mat.Roughness;
	ld.Metallic = mat.Metallic;

	ld.Reflectance = mix(vec3(mat.ReflectionCoefficient), mat.Diffuse.xyz, vec3(mat.Metallic));
	ld.Diffuse = mat.Diffuse.xyz * (1.0 - mat.Metallic);
	ld.Transparency = mat.Diffuse.w;

	ld.EyeDir = EyeDir;
	ld.ReflDir = reflect(EyeDir, mat.Normal);

	ld.n_dot_v = saturate(abs(dot(-EyeDir, mat.Normal)));
	return ld;
}

float ComputeFalloff(float Distance, highp float InvRadius)
{
	return 1.0 - saturate(Distance * InvRadius);
}

float ComputeDistrubution(LightingData ld, vec3 l, vec3 h, float sq_rough)
{
	// Compute n dot h
	float n_dot_h = dot(ld.Normal, h);
	float sq_n_dot_h = n_dot_h * n_dot_h;

	float d = sq_rough + ( (1.0 - sq_n_dot_h) / (sq_n_dot_h * sq_rough) );
	float D = sq_rough / (PI * sq_n_dot_h * sq_n_dot_h * d * d );

	return D;
}

float ComputeGeometric(LightingData ld, float sq_rough)
{
	float sq_n_dot_v = ld.n_dot_v * ld.n_dot_v;
	float g = 1.0 + sqrt(1.0 + sq_rough * ( (1.0 - sq_n_dot_v) / sq_n_dot_v) );
	float G = 2.0 / g;
	return G;
}

vec3 ComputeSchlickFresnel(LightingData ld, float dot, vec3 F0)
{
	vec3 fresnel = F0 + (1.0 - F0) * pow( (1.0 - dot), 5.0);
	return fresnel;
}

vec3 ComputePointLight(vec3 LightPos, float InvRadius, vec3 LightColor, LightingData ld)
{
	vec3 Light = LightPos - ld.WorldPos;
	vec3 nLight = normalize(Light);

	// Compute incident light angle and scattered light angle to the normal
	float iV = abs(dot(nLight, ld.Normal ));
	float oV = abs(dot(-ld.EyeDir, ld.Normal));

	// Compute the light falloff
	float fFalloff = ComputeFalloff(length(Light), InvRadius);

	vec3 h = normalize( nLight + -ld.EyeDir);
	float sq_rough = ld.Roughness*ld.Roughness;

	// Compute Schlick fresnell for Cook-Torrance
	vec3 Fc = ComputeSchlickFresnel(ld, abs(dot(ld.Normal,h)), ld.Reflectance);

	// Compute Schlick Fresnell for Lambertian
	vec3 Fl = ComputeSchlickFresnel(ld, abs(dot(ld.Normal,nLight)), vec3(0.5));

	// Compute distribution and geometry function using GGX model
	float D = ComputeDistrubution(ld, nLight, h, sq_rough);
	float G = ComputeGeometric(ld, sq_rough);

	// Compute Cook-Torrance BRDF for reflective surfaces
	vec3 result = (Fc * D * G) / (4.0 * iV * oV);

	// Compute Lambertian BRDF for matte surfaces
	vec3 lambertian = ld.Diffuse * (1.0 - Fl);

	// Boost specular and light intensity
	return (result * 2.0 + lambertian) * LightColor * 5.0 * abs(dot(ld.Normal, nLight)) * fFalloff;
}

void main()
{
	// Make the third light rotate around the scene
	uLightPos2 = vec3(cos(Time / 60.0) * 35.0, -20.0, -50.0 + sin(Time / 60.0) * 35.0);

	Material mat;

	vec4 RawNormal = texture(TexNormal, vUV);
	vec3 TangentNormal = RawNormal.xyz * 2.0 - 1.0;
	vec3 Binormal = cross(vTangent, vNormal);
	vec3 WorldPos = vEyeVec + vCamPos;

	mat.Normal = normalize(
		TangentNormal.x * vTangent.xyz +
		TangentNormal.y * Binormal.xyz +
		TangentNormal.z * vNormal.xyz);

	mat.Diffuse = texture(TexDiffuse, vUV);
	mat.Metallic = texture(TexMetallicRoughness, vUV).z;
	mat.Roughness = texture(TexMetallicRoughness, vUV).y;
	mat.ReflectionCoefficient = 0.04;

	vec3 Lighting;
	LightingData ld = SetupLightingData(WorldPos, normalize(vEyeVec), mat);

	vec3 light1 = ComputePointLight(uLightPos0, uInvRadius0, uLightColor0, ld);
	vec3 light2 = ComputePointLight(uLightPos1, uInvRadius1, uLightColor1, ld);
	vec3 light3 = ComputePointLight(uLightPos2, uInvRadius2, uLightColor2, ld);

	Lighting += light1 * 0.60 + light2 * 0.45 + light3 * 0.25;

	FragColor = vec4(Lighting, 1.0);
}
