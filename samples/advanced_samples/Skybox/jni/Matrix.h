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

#ifndef MATRIX_H
#define MATRIX_H

namespace Skybox
{
    /**
     * \brief Functions for manipulating matrices.
     */
    class Matrix
    {
        private:
            /**
             * \brief A 16 element floating point array used to represent a 4x4 matrix.
             * \note Items are stored in column major order as OpenGL ES expects them.
             */
            float elements[16];

            /**
             * \brief A 4x4 identity Matrix;
             */
            static const float identityArray[];

        public:
            /**
             * \brief Get the matrix elements as a column major order array.
             *
             * \return A pointer to the matrix elements.
             */
            float* getAsArray(void);

            /**
             * \brief Default constructor.
             */
            Matrix(void);

            /**
             * \brief Overloading assingment operater to do deep copy of the Matrix elements.
             */
            Matrix& operator=(const Matrix& another);

            /**
             * \brief Constructor from element array.
             *
             * \param[in] array A column major order array to use as the matrix elements.
             */
            Matrix(const float* array);

            /**
             * \brief The identity matrix.
             *
             * A matrix with 1's on the main diagonal and 0's everywhere else.
             */
            static Matrix identityMatrix;

            /**
             * \brief Create and return an orthographic projection matrix.
             *
             * Assumes Z is positive going away from the user's view (left-hand coordinate system).
             *
             * \param[in] left The coordinate for the left vertical clipping plane.
             * \param[in] right The coordinate for the right vertical clipping plane.
             * \param[in] bottom The coordinate for the bottom horizontal clipping plane.
             * \param[in] top The coordinate for the top horizontal clipping plane.
             * \param[in] zNear The distance from the camera to the near clipping plane.
             * \param[in] zFar the distance from the camera to the far clipping plane.
             * \return An orthographic projection matrix.
             */
            static Matrix matrixOrthographic(float left, float right, float bottom, float top, float zNear, float zFar);
    };
}
#endif  /* MATRIX_H */
