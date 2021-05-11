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

#include "AstcTextures.h"
#include "Matrix.h"

#include <cmath>
#include <cstdlib>
#include <string>

namespace AstcTextures
{
    /* Identity matrix. */
    const float Matrix::identityArray[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    /* Please see header for specification. */
    Matrix::Matrix(const float* array)
    {
        memcpy(elements, array, 16 * sizeof(float));
    }

    /* Please see header for specification. */
    Matrix Matrix::identityMatrix = Matrix(identityArray);

    /* Please see header for specification. */
    Matrix::Matrix(void){}

    /* Please see header for specification. */
    float* Matrix::getAsArray(void)
    {
        return elements;
    }

    /* Please see header for specification. */
    float& Matrix::operator[](unsigned element)
    { 
        if (element > 15)
        {
            LOGE("Matrix only has 16 elements, tried to access element %d", element);
            exit(1);
        }

        return elements[element]; 
    }

    /* Please see header for specification. */
    Matrix Matrix::operator*(Matrix right)
    {
        return multiply(this, &right);
    }

    /* Please see header for specification. */
    Matrix& Matrix::operator=(const Matrix &another)
    {
        if(this != &another)
        {
            memcpy(this->elements, another.elements, 16 * sizeof(float));
        }

        return *this;
    }

    /* Please see header for specification. */
    Matrix Matrix::matrixOrthographic(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        Matrix result = identityMatrix;

        result.elements[0]  = 2.0f / (right - left);
        result.elements[12] = -(right + left) / (right - left);

        result.elements[5]  = 2.0f / (top - bottom);
        result.elements[13] = -(top + bottom) / (top - bottom);

        result.elements[10] = -2.0f / (zFar - zNear);
        result.elements[14] = -(zFar + zNear) / (zFar - zNear);

        return result;
    }

    /* Please see header for specification. */
    Matrix Matrix::matrixPerspective(float FOV, float ratio, float zNear, float zFar)
    {
        Matrix result = identityMatrix;

        FOV = 1.0f / tan(FOV * 0.5f);

        result.elements[ 0] = FOV / ratio;
        result.elements[ 5] = FOV;
        result.elements[10] = -(zFar + zNear) / (zFar - zNear);
        result.elements[11] = -1.0f;
        result.elements[14] = (-2.0f * zFar * zNear) / (zFar - zNear);
        result.elements[15] = 0.0f;

        return result;
    }

    /* Please see header for specification. */
    Matrix Matrix::createRotationX(float angle)
    {
        Matrix result = identityMatrix;
        float angle_converted_to_radians = M_PI * angle / 180.0f;

        result.elements[5]  = cosf(angle_converted_to_radians);
        result.elements[9]  = -sinf(angle_converted_to_radians);
        result.elements[6]  = sinf(angle_converted_to_radians);
        result.elements[10] = cosf(angle_converted_to_radians);

        return result;
    }

    /* Please see header for specification. */
    Matrix Matrix::createRotationY(float angle)
    {
        Matrix result = identityMatrix;
        float angle_converted_to_radians = M_PI * angle / 180.0f;

        result.elements[0]  = cosf(angle_converted_to_radians);
        result.elements[8]  = sinf(angle_converted_to_radians);
        result.elements[2]  = -sinf(angle_converted_to_radians);
        result.elements[10] = cosf(angle_converted_to_radians);

        return result;
    }

    /* Please see header for specification. */
    Matrix Matrix::createRotationZ(float angle)
    {
        Matrix result = identityMatrix;
        float angle_converted_to_radians = M_PI * angle / 180.0f;

        result.elements[0] = cosf(angle_converted_to_radians);
        result.elements[4] = -sinf(angle_converted_to_radians);
        result.elements[1] = sinf(angle_converted_to_radians);
        result.elements[5] = cosf(angle_converted_to_radians);

        return result;
    }

    /* Please see header for specification. */
    Matrix Matrix::multiply(Matrix *left, Matrix *right)
    {
        Matrix result;

        for(int row = 0; row < 4; row++)
        {
            for(int column = 0; column < 4; column ++)
            {
                float accumulator = 0.0f;

                for(int allElements = 0; allElements < 4; allElements ++)
                {
                    accumulator += left->elements[allElements * 4 + row] * right->elements[column * 4 + allElements];
                }

                result.elements[column * 4 + row] = accumulator;
            }
        }

        return result;
    }
}
