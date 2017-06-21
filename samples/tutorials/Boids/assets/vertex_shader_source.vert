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

/* [Vertex shader source] */
const int numberOfSpheres = 30;
const float pi = 3.14159265358979323846;

in      vec4 attributePosition;
in      vec4 attributeColor;
out     vec4 vertexColor;
uniform vec4 perspectiveVector;
uniform vec3 scalingVector;
uniform vec3 cameraVector;

/* 
 * We use uniform block in order to reduce amount of memory transfers to minimum. 
 * The uniform block uses data taken directly from a buffer object. 
 */
uniform BoidsUniformBlock
{
    vec4 sphereLocation[numberOfSpheres];
};

void main()
{
    float fieldOfAngle     = 1.0 / tan(perspectiveVector.x * 0.5);
    vec3  locationOfSphere = vec3 (sphereLocation[gl_InstanceID].x, sphereLocation[gl_InstanceID].y, sphereLocation[gl_InstanceID].z);
    
    /* Set red color for leader and green color for followers. */
    if(gl_InstanceID == 0)
    {
        vertexColor = vec4(attributeColor.x, 0.5 * attributeColor.y, 0.5 * attributeColor.z, attributeColor.w);
    }
    else
    {
        vertexColor = vec4(0.5 * attributeColor.x, attributeColor.y, 0.5 * attributeColor.z, attributeColor.w);
    }
    
    /* Create transformation matrices. */
    mat4 translationMatrix = mat4(1.0,                 0.0,                   0.0,                 0.0, 
                                  0.0,                 1.0,                   0.0,                 0.0, 
                                  0.0,                 0.0,                   1.0,                 0.0, 
                                  locationOfSphere.x,  locationOfSphere.y,    locationOfSphere.z,  1.0);
                                  
    mat4 cameraMatrix      = mat4(1.0,                 0.0,                   0.0,                 0.0, 
                                  0.0,                 1.0,                   0.0,                 0.0, 
                                  0.0,                 0.0,                   1.0,                 0.0, 
                                  cameraVector.x,      cameraVector.y,        cameraVector.z,      1.0);
                                  
    mat4 scalingMatrix     = mat4(scalingVector.x,     0.0,                   0.0,                 0.0, 
                                  0.0,                 scalingVector.y,       0.0,                 0.0, 
                                  0.0,                 0.0,                   scalingVector.z,     0.0, 
                                  0.0,                 0.0,                   0.0,                 1.0);
                                  
    mat4 perspectiveMatrix = mat4(fieldOfAngle/perspectiveVector.y,  0.0,            0.0,                                                                                              0.0, 
                                  0.0,                               fieldOfAngle,   0.0,                                                                                              0.0, 
                                  0.0,                               0.0,            -(perspectiveVector.w + perspectiveVector.z) / (perspectiveVector.w - perspectiveVector.z),       -1.0, 
                                  0.0,                               0.0,            (-2.0 * perspectiveVector.w * perspectiveVector.z) / (perspectiveVector.w - perspectiveVector.z), 0.0);
    /* Compute scaling. */
    mat4 tempMatrix = scalingMatrix;
    
    /* Compute translation. */
    tempMatrix      = translationMatrix * tempMatrix;
    tempMatrix      = cameraMatrix      * tempMatrix;
                
    /* Compute perspective. */
    tempMatrix      = perspectiveMatrix * tempMatrix;
                
    /* Return gl_Position. */
    gl_Position     = tempMatrix * attributePosition;
}
/* [Vertex shader source] */
