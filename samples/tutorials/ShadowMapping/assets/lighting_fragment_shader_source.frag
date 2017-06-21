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
precision highp float;
precision highp sampler2DShadow;

in vec4 outputLightPosition;       /* Vector of the spot light position translated into eye-space. */
in vec3 outputNormal;              /* Normal vector for the coordinates. */
in vec4 outputPosition;            /* Vertex coordinates expressed in eye space. */
in mat4 outputViewToTextureMatrix; /* Matrix we will use in the fragment shader to sample the shadow map for given fragment. */

uniform vec4            colorOfGeometry; /* Colour of the geometry. */
uniform vec3            lightDirection;  /* Normalized direction vector for the spot light. */
uniform sampler2DShadow shadowMap;       /* Sampler of the depth texture used for shadow-mapping. */

out vec4 color; /* Output colour variable. */

#define PI 3.14159265358979323846

/* Structure holding properties of the directional light. */
struct DirectionalLight
{
    float ambient;   /* Value of ambient intensity for directional lighting of a scene. */
    vec3  color;     /* Colour of the directional light. */
    vec3  direction; /* Direction for the directional light. */
};

/* Structure holding properties of spot light. */
struct SpotLight
{
    float ambient;              /* Value of ambient intensity for spot lighting. */
    float angle;                /* Angle between spot light direction and cone face. */
    float spotExponent;         /* Value indicating intensity distribution of light. */
    float constantAttenuation;  /* Value of light's attenuation. */
    float linearAttenuation;    /* Value of linear light's attenuation. */
    float quadraticAttenuation; /* Value of quadratic light's attenuation. */
    vec3  direction;            /* Vector of direction of spot light. */
    vec4  position;             /* Coordinates of position of spot light source. */
};

void main()
{
    DirectionalLight directionalLight;

    directionalLight.ambient   = 0.01;
    directionalLight.color     = vec3(1.0,  1.0,  1.0);
    directionalLight.direction = vec3(0.2, -1.0, -0.2);

    SpotLight spotLight;

    spotLight.ambient              = 0.1;
    spotLight.angle                = 30.0;
    spotLight.spotExponent         = 2.0;
    spotLight.constantAttenuation  = 1.0;
    spotLight.linearAttenuation    = 0.1;
    spotLight.quadraticAttenuation = 0.9;
    spotLight.direction            = lightDirection;
    spotLight.position             = outputLightPosition;

    /* Compute distance between the light position and the fragment position. */
    float xDistanceFromLightToVertex = (spotLight.position.x - outputPosition.x);
    float yDistanceFromLightToVertex = (spotLight.position.y - outputPosition.y);
    float zDistanceFromLightToVertex = (spotLight.position.z - outputPosition.z);
    float distanceFromLightToVertex  = sqrt((xDistanceFromLightToVertex * xDistanceFromLightToVertex) +
                                            (yDistanceFromLightToVertex * yDistanceFromLightToVertex) +
                                            (zDistanceFromLightToVertex * zDistanceFromLightToVertex));
    /* Directional light. */
    /* Calculate the value of diffuse intensity. */
    float diffuseIntensity = max(0.0, -dot(outputNormal, normalize(directionalLight.direction)));

    /* Calculate colour for directional lighting. */
    color = colorOfGeometry * vec4(directionalLight.color * (directionalLight.ambient + diffuseIntensity), 1.0);

    /* Spot light. */
    /* Compute the dot product between normal and light direction. */
    float normalDotLight = max(dot(normalize(outputNormal), normalize(-spotLight.direction)), 0.0);

    /* Shadow. */
    /* Position of the vertex translated to texture space. */
    vec4 vertexPositionInTexture = outputViewToTextureMatrix * outputPosition;
    /* Normalized position of the vertex translated to texture space. */
    vec4 normalizedVertexPositionInTexture = vec4(vertexPositionInTexture.x / vertexPositionInTexture.w,
                                                  vertexPositionInTexture.y / vertexPositionInTexture.w,
                                                  vertexPositionInTexture.z / vertexPositionInTexture.w,
                                                  1.0);

    /* Depth value retrieved from the shadow map. */
    float shadowMapDepth = textureProj(shadowMap, normalizedVertexPositionInTexture);
    /* Depth value retrieved from drawn model. */
    float modelDepth = normalizedVertexPositionInTexture.z;

    /* Calculate vector from position of light to position of fragment. */
    vec3 vectorFromLightToFragment = vec3(outputPosition.x - spotLight.position.x,
                                        outputPosition.y - spotLight.position.y,
                                        outputPosition.z - spotLight.position.z);

    /* Calculate cosine value of angle between vectorFromLightToFragment and vector of spot light direction. */
    float cosinusAlpha = dot(spotLight.direction, vectorFromLightToFragment) /
                                (sqrt(dot(spotLight.direction, spotLight.direction)) *
                            sqrt(dot(vectorFromLightToFragment, vectorFromLightToFragment)));
    /* Calculate angle for cosine value. */
    float alpha = acos(cosinusAlpha);

    /*
     * Check angles. If alpha is less than spotLight.angle then the fragment is inside light cone.
     * Otherwise the fragment is outside the cone - it is not lit by spot light.
     */
    const float shadowMapBias = 0.00001;

    if (alpha < spotLight.angle)
    {
        if (modelDepth < shadowMapDepth + shadowMapBias)
        {
            float spotEffect = dot(normalize(spotLight.direction), normalize(vectorFromLightToFragment));

            spotEffect = pow(spotEffect, spotLight.spotExponent);

            /* Calculate total value of light's attenuation. */
            float attenuation = spotEffect /
                                (spotLight.constantAttenuation  +
                                spotLight.linearAttenuation    * distanceFromLightToVertex +
                                spotLight.quadraticAttenuation * distanceFromLightToVertex * distanceFromLightToVertex);

            /* Calculate colour for spot lighting.
             * Scale the colour by 0.5 to make the shadows more obvious. */
            color = color / 0.5 + (attenuation * (normalDotLight + spotLight.ambient));
        }
    }

    /* Angle (in radians) between the surfaces normal and the light direction. */
    float angle = acos(dot(normalize(outputNormal), normalize(spotLight.direction)));

    /*
     * Reduce the intensity of the colour if the object is facing away from the light.
     * scaleIntensity is 1 when the light is facing the surface, 0 when its facing the opposite direction.
     */
    float scaleIntensity = smoothstep(0.0, PI, angle);
    vec4 scaleVector = vec4(scaleIntensity, scaleIntensity, scaleIntensity, 1.0);
    color *= scaleVector;
}
/* [Fragment shader code] */
