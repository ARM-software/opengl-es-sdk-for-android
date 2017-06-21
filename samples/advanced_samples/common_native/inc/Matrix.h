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

#ifndef MATRIX_H
#define MATRIX_H

#include "VectorTypes.h"

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
         * \brief Transform a 4D vertex by a matrix.
         * \param[in] vector The 4D vector to be transformed.
         * \param[in] matrix The transformation matrix.
         * \return The result of matrix x vector
         */
        static Vec4f vertexTransform(Vec4f *vector, Matrix *matrix);

        /**
         * \brief Transform a 3D vertex by a matrix.
         * \param[in] vector The 3D vector to be transformed.
         * \param[in] matrix The transformation matrix.
         * \return The result of matrix x vector
         */
        static Vec3f vertexTransform(Vec3f *vector, Matrix *matrix);

        /**
         * \brief Transpose a matrix in-place.
         * \param[in,out] matrix The matrix to transpose.
         */
        static void matrixTranspose(Matrix *matrix);


        /**
         * \brief Create and return a rotation matrix around the x-axis matrix.
         * \param[in] angle Angle of rotation (in degrees).
         * \return A rotation matrix around the x-axis matrix with the required angle of rotation.
         */
        static Matrix createRotationX(float angle);

        /**
         * \brief Create and return a rotation matrix around the y-axis matrix.
         * \param[in] angle Angle of rotation (in degrees).
         * \return A rotation matrix around the y-axis matrix with the required angle of rotation.
         */
        static Matrix createRotationY(float angle);

        /**
         * \brief Create and return a rotation matrix around the z-axis matrix.
         * \param[in] angle Angle of rotation (in degrees).
         * \return A rotation matrix around the z-axis matrix with the required angle of rotation.
         */
        static Matrix createRotationZ(float angle);

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
         * \brief Print the matrix. 
         */
        void print(void);

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

        /**
		 * \brief Create and return a camera matrix.
		 * \param[in] eye Point vector which determines the camera position.
		 * \param[in] center Point vector which determines where camera is looking at.
		 * \param[in] up Vector which determines the orientation of the "head".
		 * \return A camera matrix.
		 */
		static Matrix matrixCameraLookAt(Vec3f eye, Vec3f center, Vec3f up); 

        /**
         * \brief Create and return an orthographic projection matrix. 
         *
         * Assumes Z is positive going away from the user's view (left-hand coordinate system).
         * \param[in] left The coordinate for the left vertical clipping plane. 
         * \param[in] right The coordinate for the right vertical clipping plane. 
         * \param[in] bottom The coordinate for the bottom horizontal clipping plane. 
         * \param[in] top The coordinate for the top horizontal clipping plane. 
         * \param[in] zNear The distance from the camera to the near clipping plane. 
         * \param[in] zFar the distance from the camera to the far clipping plane. 
         * \return An orthographic projection matrix. 
         */
        static Matrix matrixOrthographic(float left, float right, float bottom, float top, float zNear, float zFar);

        /**
         * \brief Get the inverse of a matrix.
         * \param[in] matrix The matrix to invert.
         * \return The inverse matrix of matrix.
         */
        static Matrix matrixInvert(Matrix *matrix);

        /**
         * \brief Calculate determinant of supplied 3x3 matrix.
         * \param[in] matrix The matrix to calculate the determinant of (supplied as a 9 element float array in column major order).
         * \return The determinant of the supplied matrix.
         */
        static float matrixDeterminant(float *matrix);

        /**
         * \brief Calculate determinant of supplied 4x4 matrix.
         * \param[in] matrix The matrix to calculate the determinant of.
         * \return The determinant of the supplied matrix.
         */
        static float matrixDeterminant(Matrix *matrix);

        /**
         * \brief Scale each element in a matrix by a constant.
         * \param[in] matrix The matrix to scale.
         * \param[in] scale The scale factor
         * \return The matrix matrix scaled by scale
         */
        static Matrix matrixScale(Matrix *matrix, float scale);
    };
}
#endif  /* MATRIX_H */