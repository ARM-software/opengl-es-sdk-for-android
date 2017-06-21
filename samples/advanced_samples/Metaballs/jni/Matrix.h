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

namespace MaliSDK
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
         * \brief Multiply 2 matrices to return a third.
         * \note When multiplying matrices the ordering of the parameters affects the result.
         * \param[in] left First matrix to multiply.
         * \param[in] right Second matrix to multiply.
         * \return The result of left * right
         */
        static Matrix multiply(Matrix *left, Matrix *right);

        /**
         * \brief A 4x4 identity Matrix;
         */
        static const float identityArray[];
    public:
        /**
         * \brief Get the matrix elements as a column major order array.
         * \return A pointer to the matrix elements.
         */
        float* getAsArray(void);

        /**
         * \brief Default constructor.
         */
        Matrix(void);

        /**
         * \brief Array operator for accessing the elements of the matrix.
         * \param[in] element The element index of the matrix (accepts 0-15).
         * \return The element of the matrix.
         */
        float& operator[] (unsigned element);

        /**
         * \brief Multiply operator to post multiply a matrix by another.
         * \param[in] right The matrix to post multiply by.
         * \return The result of matrix * right.
         */
        Matrix operator* (Matrix right);

        /**
         * \brief Overloading assingment operater to do deep copy of the Matrix elements.
         */
        Matrix& operator=(const Matrix &another);

        /**
         * \brief Constructor from element array.
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
         * \brief Create and return a translation matrix.
         * \param[in] x Distance to translate in the x-axis.
         * \param[in] y Distance to translate in the y-axis.
         * \param[in] z Distance to translate in the z-axis.
         * \return A translation matrix with the required translation distances.
         */
        static Matrix createTranslation(float x, float y, float z);

        /**
         * \brief Create and return a scaling matrix.
         * \param[in] x Scale factor in the x-axis.
         * \param[in] y Scale factor in the y-axis.
         * \param[in] z Scale factor in the z-axis.
         * \return A scaling matrix with the required scaling factors.
         */
        static Matrix createScaling(float x, float y, float z);

        /**
         * \brief Create and return a perspective projection matrix.
         * \param[in] FOV The field of view angle (in degrees) in the y direction.
         * \param[in] ratio The ratio used to calculate the field of view in the x direction.
         * The ratio of x (width) to y (height).
         * \param[in] zNear The distance from the camera to the near clipping plane.
         * \param[in] zFar the distance from the camera to the far clipping plane.
         * \return A perspective projection matrix.
         */
        static Matrix matrixPerspective(float FOV, float ratio, float zNear, float zFar);
    };
}
#endif  /* MATRIX_H */