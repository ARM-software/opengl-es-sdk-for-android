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

#ifndef WIREFRAME_TORUS_H
#define WIREFRAME_TORUS_H

#include "Torus.h"

/** 
 * \brief Class derived from the Torus abstract class. It manages drawing of a rotating wireframed
 *        unicolor torus. Apart from inherited components, it manages a buffer that stores 
 *        indices needed for the glDrawElements() call and also is of determining those indices.
 *        As input attributes, it directly passes the vertices of a torus.
 */
class WireframeTorus : public Torus
{
private:

    /* Number of indices needed for a single glDrawElements() call. */
    static const unsigned int indicesCount = 4 * circlesCount * pointsPerCircleCount;

    /* Index of a GL_ELEMENT_ARRAY_BUFFER buffer, to store determined indices. */
    GLuint indicesBufferID;

    /**
     * \brief Determine indices needed for a single glDrawElements() call in GL_LINES mode.
     */
    void initializeBufferForIndices(void);

    bool initializeVertexAttribs(void);

public:

    /**
     * \brief Instantiates a representation of a solid torus, using user-provided radius and tube radius.
     *
     * \param torusRadius  [in] Distance between the center of torus and the center of its tube.
     * \param circleRadius [in] Radius of the circle that models the tube.
     */
    WireframeTorus(float torusRadius, float circleRadius);

    ~WireframeTorus(void);

    void draw(float* rotationVector);
};

#endif /* WIREFRAME_TORUS_H */
