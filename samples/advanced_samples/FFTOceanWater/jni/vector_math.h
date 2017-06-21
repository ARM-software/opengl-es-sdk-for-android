/* Copyright (c) 2015-2017, ARM Limited and Contributors
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

#ifndef VECTOR_MATH_H__
#define VECTOR_MATH_H__

#include <string.h>
#include <math.h>
#include <stdint.h>

#ifndef PI
#define PI 3.141592653f
#endif

// Basic vector math.

template<typename T>
const typename T::data_type *value_ptr(const T& vec)
{
    return vec.data;
}

struct vec2
{
    union
    {
        struct
        {
            float x, y;
        };
        // Allow us to use vec2, vec3 and vec4 directly in uniform buffers.
        // GLSL using std140 packing demands that packing of vectors is four floats.
        // The rules on packing with arrays can be slightly complicated howewer.
        float data[2];
    };
    enum { vector_size = 2 };
    typedef float data_type;
    typedef vec2 vector_type;
    vec2() {}
    vec2(float s) { this->x = this->y = s; }
    vec2(float x, float y) { this->x = x; this->y = y; }
    vec2(const float *vec) { memcpy(data, vec, 2 * sizeof(float)); }
};

namespace detail
{
    template<typename T>
    struct ivec2
    {
        union
        {
            struct
            {
                T x, y;
            };
            // Allow us to use vec2, vec3 and vec4 directly in uniform buffers.
            // GLSL using std140 packing demands that packing of vectors is four floats.
            // The rules on packing with arrays can be slightly complicated howewer.
            T data[2];
        };
        enum { vector_size = 2 };
        typedef T data_type;
        typedef ivec2<T> vector_type;
        ivec2() {}
        ivec2(T s) { x = y = s; }
        ivec2(T x, T y) { this->x = x; this->y = y; }
        ivec2(const T *vec) { memcpy(data, vec, 2 * sizeof(T)); }
        ivec2(vec2 s) { x = T(s.x); y = T(s.y); } 
        operator vec2() const { return vec2(x, y); }
    };
}

typedef detail::ivec2<uint32_t> uvec2;
typedef detail::ivec2<int32_t> ivec2;
typedef detail::ivec2<uint8_t> ubvec2;

struct vec3
{
    union
    {
        struct
        {
            float x, y, z;
        };
        float data[4];
    };
    enum { vector_size = 3 };
    typedef float data_type;
    typedef vec3 vector_type;
    vec3() {}
    vec3(float s) { x = y = z = s; }
    vec3(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }
    vec3(const float *vec) { memcpy(data, vec, 3 * sizeof(float)); }
};

struct vec4
{
    union
    {
        struct
        {
            float x, y, z, w;
        };
        float data[4];
    };
    enum { vector_size = 4 };
    typedef float data_type;
    typedef vec4 vector_type;
    vec4() {}
    vec4(float s) { x = y = z = w = s; }
    vec4(float x, float y, float z, float w) { this->x = x; this->y = y; this->z = z; this->w = w; }
    vec4(const float *vec) { memcpy(data, vec, 4 * sizeof(float)); }

    vec4(const vec3& vec, float v)
    {
        *this = vec4(vec.x, vec.y, vec.z, v);
    }

    vec4(const vec2& a, const vec2& b)
    {
        *this = vec4(a.x, a.y, b.x, b.y);
    }

    vec4(const vec2& a, float b, float c)
    {
        *this = vec4(a.x, a.y, b, c);
    }

    operator vec3() const { return vec3(data); }
};

struct mat4
{
    float data[16];

    mat4() {}
    mat4(float s) { for (unsigned int i = 0; i < 16; i++) data[i] = s; }
    mat4(float c00, float c01, float c02, float c03,
        float c10, float c11, float c12, float c13,
        float c20, float c21, float c22, float c23,
        float c30, float c31, float c32, float c33)
    {
        data[ 0] = c00; data[ 1] = c01; data[ 2] = c02; data[ 3] = c03;
        data[ 4] = c10; data[ 5] = c11; data[ 6] = c12; data[ 7] = c13;
        data[ 8] = c20; data[ 9] = c21; data[10] = c22; data[11] = c23;
        data[12] = c30; data[13] = c31; data[14] = c32; data[15] = c33;
    }
    mat4(const float *mat) { memcpy(data, mat, 16 * sizeof(float)); } 
    typedef float data_type;
};

// Use SFINAE to avoid dubious overloads.

template<typename T>
inline typename T::vector_type operator-(const T& a)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = -a.data[i];
    return res;
}

template<typename T>
inline typename T::vector_type operator*(const T& a, const T& b)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = a.data[i] * b.data[i];
    return res;
}

template<typename T>
inline typename T::vector_type operator/(const T& a, const T& b)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = a.data[i] / b.data[i];
    return res;
}

template<typename T>
inline typename T::vector_type operator+(const T& a, const T& b)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = a.data[i] + b.data[i];
    return res;
}

template<typename T>
inline typename T::vector_type operator-(const T& a, const T& b)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = a.data[i] - b.data[i];
    return res;
}

template<typename T>
inline typename T::vector_type& operator*=(T& a, const T& b)
{
    for (unsigned int i = 0; i < T::vector_size; i++)
        a.data[i] *= b.data[i];
    return a;
}

template<typename T>
inline typename T::vector_type& operator/=(T& a, const T& b)
{
    for (unsigned int i = 0; i < T::vector_size; i++)
        a.data[i] /= b.data[i];
    return a;
}

template<typename T>
inline typename T::vector_type& operator+=(T& a, const T& b)
{
    for (unsigned int i = 0; i < T::vector_size; i++)
        a.data[i] += b.data[i];
    return a;
}

template<typename T>
inline typename T::vector_type& operator-=(T& a, const T& b)
{
    for (unsigned int i = 0; i < T::vector_size; i++)
        a.data[i] -= b.data[i];
    return a;
}

inline mat4 operator*(const mat4& a, const mat4& b)
{
    mat4 res;
    for (unsigned int r = 0; r < 4; r++)
    {
        for (unsigned int c = 0; c < 4; c++)
        {
            float sum = 0.0f;
            for (unsigned int k = 0; k < 4; k++)
                sum += a.data[r + 4 * k] * b.data[4 * c + k];
            res.data[r + 4 * c] = sum;
        }
    }

    return res;
}

inline vec4 operator*(const mat4& mat, const vec4& vec)
{
    vec4 res(0.0f);
    for (unsigned int i = 0; i < 4; i++)
        res += vec4(mat.data + 4 * i) * vec4(vec.data[i]);
    return res;
}

inline mat4& operator*=(mat4& mat, float v)
{
    for (unsigned int i = 0; i < 16; i++)
        mat.data[i] *= v;
    return mat;
}

inline vec3 vec_cross(const vec3& a, const vec3& b)
{
    return vec3(
        a.y * b.z - b.y * a.z,
        a.z * b.x - b.z * a.x,
        a.x * b.y - b.x * a.y);
}

template<typename T>
inline float vec_dot(const T& a, const T& b)
{
    float sum = 0.0f;
    for (unsigned int i = 0; i < T::vector_size; i++)
        sum += a.data[i] * b.data[i];
    return sum;
}

template<typename T>
inline float vec_length(const T& vec)
{
    return sqrt(vec_dot(vec, vec));
}

template<typename T>
inline T vec_normalize(const T& vec)
{
    return vec / T(vec_length(vec));
}

template<typename T>
inline T vec_floor(const T& vec)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = floor(vec.data[i]);
    return res;
}

template<typename T>
inline T vec_round(const T& vec)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = round(vec.data[i]);
    return res;
}

template<typename T>
inline T vec_fract(const T& vec)
{
    return vec - vec_floor(vec);
}

inline vec3 vec_project(const vec4& vec)
{
    return vec3(vec.data) / vec3(vec.w);
}

template<typename T>
inline T clamp(T value, T lo, T hi)
{
    if (value < lo)
        return lo;
    else if (value > hi)
        return hi;
    else
        return value;
}

template<typename T>
inline T vec_clamp(const T& vec, const T& lo, const T& hi)
{
    T res;
    for (unsigned int i = 0; i < T::vector_size; i++)
        res.data[i] = clamp(vec.data[i], lo.data[i], hi.data[i]);
    return res;
}

inline mat4 mat_look_at(const vec3& eye, const vec3& center,
                        const vec3& up)
{
    vec3 zaxis = vec_normalize(center - eye);
    vec3 xaxis = vec_normalize(vec_cross(zaxis, up));
    vec3 yaxis = vec_cross(xaxis, zaxis);
    return mat4(
        xaxis.x,               yaxis.x,             -zaxis.x,             0.0f,
        xaxis.y,               yaxis.y,             -zaxis.y,             0.0f,
        xaxis.z,               yaxis.z,             -zaxis.z,             0.0f,
        -vec_dot(xaxis, eye), -vec_dot(yaxis, eye), -vec_dot(-zaxis, eye), 1.0f);
}

inline mat4 mat_rotate_x(float radians)
{
    float cos_r = cos(radians);
    float sin_r = sin(radians);

    return mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, cos_r, sin_r, 0.0f,
                0.0f, -sin_r, cos_r, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

inline mat4 mat_rotate_y(float radians)
{
    float cos_r = cos(radians);
    float sin_r = sin(radians);

    return mat4(cos_r, 0.0f, sin_r, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                -sin_r, 0.0f, cos_r, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

inline mat4 mat_rotate_z(float radians)
{
    float cos_r = cos(radians);
    float sin_r = sin(radians);

    return mat4(cos_r, sin_r, 0.0f, 0.0f,
                -sin_r, cos_r, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

inline vec3 vec_rotateX(const vec3& v, float radians)
{
    return vec3(mat_rotate_x(radians) * vec4(v, 1.0f));
}

inline vec3 vec_rotateY(const vec3& v, float radians)
{
    return vec3(mat_rotate_y(radians) * vec4(v, 1.0f));
}

inline vec3 vec_rotateZ(const vec3& v, float radians)
{
    return vec3(mat_rotate_z(radians) * vec4(v, 1.0f));
}

inline mat4 mat_perspective_fov(float fovy, float aspect,
                                float zn, float zf)
{
    float yFac = tanf(fovy * PI / 360.0f);
    float xFac = yFac * aspect;
    return mat4(1.0f / xFac, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f / yFac, 0.0f, 0.0f,
        0.0f, 0.0f, -(zf + zn) / (zf - zn), -1.0f,
        0.0f, 0.0f, -(2.0f * zf * zn) / (zf - zn), 0.0f);
}

inline mat4 mat_inverse(const mat4& a)
{
    float a0 = a.data[ 0] * a.data[ 5] - a.data[ 4] * a.data[ 1];
    float a1 = a.data[ 0] * a.data[ 9] - a.data[ 8] * a.data[ 1];
    float a2 = a.data[ 0] * a.data[13] - a.data[12] * a.data[ 1];
    float a3 = a.data[ 4] * a.data[ 9] - a.data[ 8] * a.data[ 5];
    float a4 = a.data[ 4] * a.data[13] - a.data[12] * a.data[ 5];
    float a5 = a.data[ 8] * a.data[13] - a.data[12] * a.data[ 9];
    float b0 = a.data[ 2] * a.data[ 7] - a.data[ 6] * a.data[ 3];
    float b1 = a.data[ 2] * a.data[11] - a.data[10] * a.data[ 3];
    float b2 = a.data[ 2] * a.data[15] - a.data[14] * a.data[ 3];
    float b3 = a.data[ 6] * a.data[11] - a.data[10] * a.data[ 7];
    float b4 = a.data[ 6] * a.data[15] - a.data[14] * a.data[ 7];
    float b5 = a.data[10] * a.data[15] - a.data[14] * a.data[11];

    float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    float inv_det = 1.0f / det;

    mat4 inv;
    inv.data[ 0] = + a.data[5] * b5 - a.data[ 9] * b4 + a.data[13] * b3;
    inv.data[ 1] = - a.data[1] * b5 + a.data[ 9] * b2 - a.data[13] * b1;
    inv.data[ 2] = + a.data[1] * b4 - a.data[ 5] * b2 + a.data[13] * b0;
    inv.data[ 3] = - a.data[1] * b3 + a.data[ 5] * b1 - a.data[ 9] * b0;
    inv.data[ 4] = - a.data[4] * b5 + a.data[ 8] * b4 - a.data[12] * b3;
    inv.data[ 5] = + a.data[0] * b5 - a.data[ 8] * b2 + a.data[12] * b1;
    inv.data[ 6] = - a.data[0] * b4 + a.data[ 4] * b2 - a.data[12] * b0;
    inv.data[ 7] = + a.data[0] * b3 - a.data[ 4] * b1 + a.data[ 8] * b0;
    inv.data[ 8] = + a.data[7] * a5 - a.data[11] * a4 + a.data[15] * a3;
    inv.data[ 9] = - a.data[3] * a5 + a.data[11] * a2 - a.data[15] * a1;
    inv.data[10] = + a.data[3] * a4 - a.data[ 7] * a2 + a.data[15] * a0;
    inv.data[11] = - a.data[3] * a3 + a.data[ 7] * a1 - a.data[11] * a0;
    inv.data[12] = - a.data[6] * a5 + a.data[10] * a4 - a.data[14] * a3;
    inv.data[13] = + a.data[2] * a5 - a.data[10] * a2 + a.data[14] * a1;
    inv.data[14] = - a.data[2] * a4 + a.data[ 6] * a2 - a.data[14] * a0;
    inv.data[15] = + a.data[2] * a3 - a.data[ 6] * a1 + a.data[10] * a0;

    inv *= inv_det;
    return inv;
}

#endif
