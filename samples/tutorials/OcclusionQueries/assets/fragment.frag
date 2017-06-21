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

/* These values are passed to the fragment shader from the vertex shader. */
in vec3 normalOut;
in vec4 modelPosition;

/* Color of a geometry. */
uniform vec4 color;
/* Inverted model-view-projection matrix that will be used to compute light. */
uniform mat4 worldInverseMatrix;

/* Output object's color. */
out vec4 outColor;

/* Structure that describes the light source. */
struct lightSource
{
    vec4 position;
    vec4 diffuse;
    vec4 specular;
};

/* Structure that describes material. */
struct material
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};

void main()
{
    /* Ambient light factor. */
    vec4 sceneAmbient = vec4(0.2, 0.2, 0.2, 1.0);

    /* Light source with hard coded parameters. */
    lightSource light = lightSource(vec4(50.0, 100.0, 50.0, 0.0),
                                    vec4( 1.0,   1.0,  1.0, 1.0),
                                    vec4( 1.0,   1.0,  1.0, 1.0));

    /* New material with hard coded parameters. */
    material frontMaterial = material(vec4(0.2, 0.2, 0.2, 1.0),
                                      color,
                                      vec4(1.0, 1.0, 1.0, 1.0),
                                      25.0);

    /* Set vectors up. */
    vec3 normalDirection = normalize(normalOut);
    /* We need a direction to the viewer. That's why we compute difference between the camera position (worldInverseMatrix) and vertex position (modelPosition). */
    vec3 viewDirection   = normalize(vec3(worldInverseMatrix * vec4(0.0, 0.0, 0.0, 1.0) - modelPosition));
    
    vec3 lightDirection;
    
    float attenuation;

    /* Check if it's a directional light. */
    if (0.0 == light.position.w)
    {
        /* No attenuation. */
        attenuation = 1.0;

        /* Setup light direction. */
        lightDirection = normalize(vec3(light.position));
    } 

    /* Compute ambient factor of the light. It's done by multiplying sceneAmbient color and material ambient color. */
    vec3 ambientLighting = vec3(sceneAmbient) * vec3(frontMaterial.ambient);

    /* Compute diffuse reflection. Diffuse reflection = attenuation * lightDiffuse * materialDiffuse * (normalDirection o lightDirection). */
    vec3 diffuseReflection = attenuation                 *
                             vec3(light.diffuse)         *
                             vec3(frontMaterial.diffuse) *
                             clamp(dot(normalDirection, lightDirection), 0.0, 1.0);
    
    /* Check if the light source is on the proper side. */
    vec3 specularReflection = vec3(0.0, 0.0, 0.0);

    if (dot(normalDirection, lightDirection) >= 0.0)
    {
        /* If it's on the right side, compute specularReflection. Specular reflection = attenuation * lightSpecular * materialSpecular * (-lightDirection o normalDirection). */
        specularReflection = attenuation                  *
                             vec3(light.specular)         *
                             vec3(frontMaterial.specular) *
                             pow(max(0.0, dot(reflect(-lightDirection, normalDirection), viewDirection)), frontMaterial.shininess);
    }

    /* Set the final color of the object. */
    outColor = vec4(ambientLighting + diffuseReflection + specularReflection, 1.0);
}
/* [Fragment shader code] */