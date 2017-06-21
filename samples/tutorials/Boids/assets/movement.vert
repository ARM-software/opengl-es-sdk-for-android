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

/* [Movement vertex shader source] */
const int numberOfSpheres = 30;

/* 
 * We use uniform block in order to reduce amount of memory transfers to minimum. 
 * The uniform block uses data taken directly from a buffer object. 
 */
uniform inputData
{
  vec4 inLocation[numberOfSpheres]; /* Current location of spheres. */
  vec4 inVelocity[numberOfSpheres]; /* Current velocity of spheres. */
};

out vec4 location; /* Transformed sphere location. */
out vec4 velocity; /* Transformed sphere velocity. */

uniform float time; /* Time value used for determining new leader's position. */

/* Boids fly toward center of the mass. */
vec4 moveToCenter()
{
    vec4 center = vec4(0.0);

    /* Calculate the center of mass for all other boids (average of their locations). */
    for (int i = 0; i < numberOfSpheres; i++)
    {
        if (i != gl_InstanceID)
        {
            center = center + inLocation[i];
        }
    }

    center = center / float(numberOfSpheres - 1);

    return (center - inLocation[gl_InstanceID]) / 100.0;
}

/* Boids keep their distance from other boids. */
vec4 keepDistanceBetweenBoids()
{
    vec4 result = vec4(0.0);
    
    for (int i = 0; i < numberOfSpheres; i++)
    {
        if (i != gl_InstanceID)
        {
            /* Compute distance between boids. */
            float xDistance   = inLocation[i].x - inLocation[gl_InstanceID].x;
            float yDistance   = inLocation[i].y - inLocation[gl_InstanceID].y;
            float zDistance   = inLocation[i].z - inLocation[gl_InstanceID].z;
            float xyzDistance = sqrt(xDistance * xDistance + yDistance * yDistance + zDistance * zDistance);
            
            /* If distance between boids is too small, update result vector. */
            /* Radius of sphere (which represents a single boid) is set to 10, scaling factor is set to 0.1, which means that the boids start to overlap if the distance gets below 2. 
            * Minimum distance is set to 4 so that boids start to run away from each other if the distance between them is too low. 
            * We use smoothstep() function to smoothen the "run-away".
            */
            if (xyzDistance < 4.0)
            {
                result = result - (1.1 - smoothstep(0.0, 4.0, xyzDistance)) * (inLocation[i] - inLocation[gl_InstanceID]);
            }
        }
    }
    
    return result;
}

/* Boids try to match velocity with other boids. */
vec4 matchVelocity()
{
    vec4 result = vec4(0.0);
    
    /* Compute average velocity of all other boids. */
    for (int i = 0; i < numberOfSpheres; i++)
    {
        if (i != gl_InstanceID)
        {
            result = result + inVelocity[i];
        }
    }
    
    result = result / float(numberOfSpheres - 1);
    
    return (result - inVelocity[gl_InstanceID]) / 2.0;	
}

/* Compute followers' positions and velocities. */
void setFollowersPosition()
{
    vec4 result1 = moveToCenter();
    vec4 result2 = keepDistanceBetweenBoids();
    vec4 result3 = matchVelocity();
        
    velocity = inVelocity[gl_InstanceID] + result1 + result2 + result3;
    location = inLocation[gl_InstanceID] + velocity;
}

/* Calculate leader's position using a certain closed curve. */ 
void setLeaderPosition()
{
    location = vec4(15.0 * (1.0 + cos(time) - 1.0), 
                    15.0 * sin(time), 
                    2.0 * 15.0 * sin(time / 2.0), 
                    1.0);
    velocity = vec4(0.0);
}

void main()
{
    /* Use a different approach depending on whether we are dealing with a leader or a follower. */
    if (gl_InstanceID == 0)
    {
        setLeaderPosition();
    }
    else
    {
        setFollowersPosition();
    }
}
/* [Movement vertex shader source] */