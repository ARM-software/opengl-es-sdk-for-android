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

#include "Matrix.h"

namespace MaliSDK
{
    /* Identity matrix. */
    const float Matrix::identityArray[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    Matrix Matrix::identityMatrix = Matrix(identityArray);

    /** Please see header for the specification. */
    Matrix::Matrix(const float* array)
    {
        memcpy(elements, array, 16 * sizeof(float));
    }

    /** Please see header for the specification. */
    Matrix::Matrix(void)
    {
    }

    /** Please see header for the specification. */
    float& Matrix::operator[] (unsigned element)
    {
        if (element > 15)
        {
            exit(1);
        }
        return elements[element];
    }

    /** Please see header for the specification. */
    Matrix Matrix::operator* (Matrix right)
    {
        return multiply(this, &right);
    }

    /** Please see header for the specification. */
    Matrix& Matrix::operator= (const Matrix &another)
    {
        if(this != &another)
        {
            memcpy(this->elements, another.elements, 16 * sizeof(float));
        }

        return *this;
    }

    float* Matrix::getAsArray(void)
    {
        return elements;
    }

    /** Please see header for the specification. */
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

    /** Please see header for the specification. */
    Matrix Matrix::matrixCameraLookAt(Vec3f eye, Vec3f center, Vec3f up)
	{
		Matrix result = identityMatrix;

		Vec3f cameraX, cameraY;

        Vec3f cameraZ = {center.x - eye.x, center.y - eye.y, center.z - eye.z};
        cameraZ.normalize();

        cameraX = Vec3f::cross(cameraZ, up);
        cameraX.normalize();

		cameraY = Vec3f::cross(cameraX, cameraZ);

		/*
         * The final cameraLookAt should look like:
		 *
		 * cameraLookAt[] = { cameraX.x,	cameraY.x,   -cameraZ.x,  0.0f,
		 *					  cameraX.y,	cameraY.y,   -cameraZ.y,  0.0f,
		 *					  cameraX.z,	cameraY.z,   -cameraZ.z,  0.0f,
		 *					 -eye.x,	   -eye.y,		 -eye.z,	  1.0f };
		 */

		result[0]  = cameraX.x;
		result[1]  = cameraY.x;
		result[2]  = -cameraZ.x;

		result[4]  = cameraX.y;
		result[5]  = cameraY.y;
		result[6]  = -cameraZ.y;

		result[8]  = cameraX.z;
		result[9]  = cameraY.z;
		result[10] = -cameraZ.z;

		result[12] = -eye.x;
		result[13] = -eye.y;
		result[14] = -eye.z;

		return result;
	}

    /** Please see header for the specification. */
    Matrix Matrix::multiply(Matrix *left, Matrix *right)
    {
        Matrix result;

        for(int row = 0; row < 4; row ++)
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
