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

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifndef M_PI
/**
 * \brief PI value approximation.
 */
#define M_PI 3.14159265358979323846f
#endif /* M_PI */

namespace MaliSDK
{
    /**
     * \brief Convert an angle in degrees to radians.
     * \param[in] degrees The angle (in degrees) to convert to radians.
     */
    inline float degreesToRadians(float degrees)
    {
        return M_PI * degrees / 180.0f;
    }

    /**
     * \brief A 3D floating point vector
     *
     * Class containing three floating point numbers, useful for representing 3D coordinates.
     */
    class Vec3f
    {
    public:
        float x, y, z;

        /**
         * \brief Normalize 3D floating point vector.
         */
        void normalize(void)
        {
            float length = sqrt(x * x + y * y + z * z);

            x /= length;
            y /= length;
            z /= length;
        }

       /**
        * \brief Calculate cross product between two 3D floating point vectors.
        *
        * \param[in] vector1 First floating point vector that will be used to compute cross product.
        * \param[in] vector2 Second floating point vector that will be used to compute cross product.
        *
        * \return Floating point vector that is a result of cross product of vector1 and vector2.
        */
        static Vec3f cross(const Vec3f& vector1, const Vec3f& vector2)
	    {
		    /* Floating point vector to be returned. */
		    Vec3f crossProduct;

		    crossProduct.x = (vector1.y * vector2.z) - (vector1.z * vector2.y);
		    crossProduct.y = (vector1.z * vector2.x) - (vector1.x * vector2.z);
		    crossProduct.z = (vector1.x * vector2.y) - (vector1.y * vector2.x);

		    return crossProduct;
	    }
    };


   /**
    * \brief A 4D floating point vector
    *
    * Class containing four floating point numbers.
    */
   class Vec4f
   {
   public:
       float x, y, z, w;

       /**
        * \brief Normalize 4D floating point vector.
        */
       void normalize(void)
       {
           float length = sqrt(x * x + y * y + z * z + w * w);

           x /= length;
           y /= length;
           z /= length;
           w /= length;
       }
   };

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
         *
         * \param[in] left First matrix to multiply.
         * \param[in] right Second matrix to multiply.
         *
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
         *
         * \return A pointer to the matrix elements.
         */
        float* getAsArray(void);

        /**
         * \brief Default constructor.
         */
        Matrix(void);

        /**
         * \brief Array operator for accessing the elements of the matrix.
         *
         * \param[in] element The element index of the matrix (accepts 0-15).
         *
         * \return The element of the matrix.
         */
        float& operator[] (unsigned element);

        /**
         * \brief Multiply operator to post multiply a matrix by another.
         *
         * \param[in] right The matrix to post multiply by.
         *
         * \return The result of matrix * right.
         */
        Matrix operator* (Matrix right);

        /**
         * \brief Overloading assingment operater to do deep copy of the Matrix elements.
         */
        Matrix& operator=(const Matrix &another);

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
         * \brief Create and return a perspective projection matrix.
         *
         * \param[in] FOV   The field of view angle (in degrees) in the y direction.
         * \param[in] ratio The ratio used to calculate the field of view in the x direction.
         *                  The ratio of x (width) to y (height).
         * \param[in] zNear The distance from the camera to the near clipping plane.
         * \param[in] zFar  The distance from the camera to the far clipping plane.
         *
         * \return A perspective projection matrix.
         */
        static Matrix matrixPerspective(float FOV, float ratio, float zNear, float zFar);

        /**
		 * \brief Create and return a camera matrix.
		 *
		 * \param[in] eye Point vector which determines the camera position.
		 * \param[in] center Point vector which determines where camera is looking at.
		 * \param[in] up Vector which determines the orientation of the "head".
		 *
		 * \return A camera matrix.
		 */
		static Matrix matrixCameraLookAt(Vec3f eye, Vec3f center, Vec3f up);
    };
}
#endif  /* MATRIX_H */
