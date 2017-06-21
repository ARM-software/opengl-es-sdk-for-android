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

#include "Frustum.h"
#include "AABB.h"

Frustum::Frustum() {}

//! [Compute plane equations]
Frustum::Frustum(const mat4& view_projection)
{
    // Frustum planes are in world space.
    mat4 inv = mat_inverse(view_projection);

    // Get world-space coordinates for clip-space bounds.
    vec4 lbn = inv * vec4(-1, -1, -1, 1);
    vec4 ltn = inv * vec4(-1,  1, -1, 1);
    vec4 lbf = inv * vec4(-1, -1,  1, 1);
    vec4 rbn = inv * vec4( 1, -1, -1, 1);
    vec4 rtn = inv * vec4( 1,  1, -1, 1);
    vec4 rbf = inv * vec4( 1, -1,  1, 1);
    vec4 rtf = inv * vec4( 1,  1,  1, 1);

    // Divide by w.
    vec3 lbn_pos = vec_project(lbn);
    vec3 ltn_pos = vec_project(ltn);
    vec3 lbf_pos = vec_project(lbf);
    vec3 rbn_pos = vec_project(rbn);
    vec3 rtn_pos = vec_project(rtn);
    vec3 rbf_pos = vec_project(rbf);
    vec3 rtf_pos = vec_project(rtf);

    // Get plane normals for all sides of frustum.
    vec3 left_normal   = vec_normalize(vec_cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos));
    vec3 right_normal  = vec_normalize(vec_cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos));
    vec3 top_normal    = vec_normalize(vec_cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos));
    vec3 bottom_normal = vec_normalize(vec_cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos));
    vec3 near_normal   = vec_normalize(vec_cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos));
    vec3 far_normal    = vec_normalize(vec_cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos));

    // Plane equations compactly represent a plane in 3D space.
    // We want a way to compute the distance to the plane while preserving the sign to know which side we're on.
    // Let:
    //    O: an arbitrary point on the plane
    //    N: the normal vector for the plane, pointing in the direction
    //       we want to be "positive".
    //    X: Position we want to check.
    //
    // Distance D to the plane can now be expressed as a simple operation:
    // D = dot((X - O), N) = dot(X, N) - dot(O, N)
    //
    // We can reduce this to one dot product by assuming that X is four-dimensional (4th component = 1.0).
    // The normal can be extended to four dimensions as well:
    // X' = vec4(X, 1.0)
    // N' = vec4(N, -dot(O, N))
    //
    // The expression now reduces to: D = dot(X', N')
    planes[0] = vec4(near_normal,   -vec_dot(near_normal, lbn_pos));   // Near
    planes[1] = vec4(far_normal,    -vec_dot(far_normal, lbf_pos));    // Far
    planes[2] = vec4(left_normal,   -vec_dot(left_normal, lbn_pos));   // Left
    planes[3] = vec4(right_normal,  -vec_dot(right_normal, rbn_pos));  // Right
    planes[4] = vec4(top_normal,    -vec_dot(top_normal, ltn_pos));    // Top
    planes[5] = vec4(bottom_normal, -vec_dot(bottom_normal, lbn_pos)); // Bottom
}
//! [Compute plane equations]

//! [Test for intersection]
bool Frustum::intersects_aabb(const AABB& aabb) const
{
    // If all corners of an axis-aligned bounding box are on the "wrong side" (negative distance)
    // of at least one of the frustum planes, we can safely cull the mesh.
    vec4 corners[8];
    for (unsigned int c = 0; c < 8; c++)
    {
        // Require 4-dimensional coordinates for plane equations.
        corners[c] = vec4(aabb.corner(c), 1.0f);
    }

    for (unsigned int p = 0; p < 6; p++)
    {
        bool inside_plane = false;
        for (unsigned int c = 0; c < 8; c++)
        {
            // If dot product > 0, we're "inside" the frustum plane,
            // otherwise, outside.
            if (vec_dot(corners[c], planes[p]) > 0.0f)
            {
                inside_plane = true;
                break;
            }
        }

        if (!inside_plane)
            return false;
    }

    return true;
}
//! [Test for intersection]
