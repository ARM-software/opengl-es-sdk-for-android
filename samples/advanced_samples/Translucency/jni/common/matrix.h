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
#ifndef PI
#define PI 3.141592653f
#endif
#include <math.h>

struct vec2
{
    float x;
    float y;

    vec2() : x(0.0f), y(0.0f) { }
    vec2(float X, float Y) : x(X), y(Y){ }
    explicit vec2(float S) : x(S), y(S) { }
    vec2 operator + (const vec2 &rhs) const { return vec2(x + rhs.x, y + rhs.y); }
    vec2 operator * (const vec2 &rhs) const { return vec2(x * rhs.x, y * rhs.y); }
    vec2 operator - (const vec2 &rhs) const { return vec2(x - rhs.x, y - rhs.y); }
    vec2 operator * (const float s)  const  { return vec2(x * s, y * s); }
    vec2 operator / (const float s)  const  { return vec2(x / s, y / s); }

    vec2 &operator *= (const float s)   { *this = *this * s; return *this; }
    vec2 &operator += (const vec2 &rhs) { *this = *this + rhs; return *this; }
    vec2 &operator *= (const vec2 &rhs) { *this = *this * rhs; return *this; }
    vec2 &operator -= (const vec2 &rhs) { *this = *this - rhs; return *this; }

    float &operator [] (unsigned int i)             { return (&x)[i]; }
    const float &operator [] (unsigned int i) const { return (&x)[i]; }
};

struct vec3
{
    float x;
    float y;
    float z;

    vec3() : x(0.0f), y(0.0f), z(0.0f) { }
    vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) { }
    explicit vec3(float S) : x(S), y(S), z(S) { }
    vec3 operator - () const { return vec3(-x, -y, -z); }
    vec3 operator + (const vec3 &rhs) const { return vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
    vec3 operator * (const vec3 &rhs) const { return vec3(x * rhs.x, y * rhs.y, z * rhs.z); }
    vec3 operator - (const vec3 &rhs) const { return vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
    vec3 operator * (const float s)  const  { return vec3(x * s, y * s, z * s); }
    vec3 operator / (const float s)  const  { return vec3(x / s, y / s, z / s); }

    vec3 &operator += (const vec3 &rhs) { *this = *this + rhs; return *this; }
    vec3 &operator *= (const vec3 &rhs) { *this = *this * rhs; return *this; }
    vec3 &operator -= (const vec3 &rhs) { *this = *this - rhs; return *this; }

    float &operator [] (unsigned int i)             { return (&x)[i]; }
    const float &operator [] (unsigned int i) const { return (&x)[i]; }
};

struct vec4
{
    float x;
    float y;
    float z;
    float w;

    vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
    vec4(vec3 V, float W) : x(V.x), y(V.y), z(V.z), w(W) { }
    vec4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) { }
    explicit vec4(float S) : x(S), y(S), z(S), w(S) { }
    vec4 operator - () const { return vec4(-x, -y, -z, -w); }
    vec4 operator + (const vec4 &rhs) const { return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
    vec4 operator * (const vec4 &rhs) const { return vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
    vec4 operator - (const vec4 &rhs) const { return vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
    vec4 operator * (const float s)  const  { return vec4(x * s, y * s, z * s, w * s); }
    vec4 operator / (const float s)  const  { return vec4(x / s, y / s, z / s, w / s); }

    vec4 &operator *= (const float s)   { *this = *this * s; return *this; }
    vec4 &operator += (const vec4 &rhs) { *this = *this + rhs; return *this; }
    vec4 &operator *= (const vec4 &rhs) { *this = *this * rhs; return *this; }
    vec4 &operator -= (const vec4 &rhs) { *this = *this - rhs; return *this; }

    float &operator [] (unsigned int i)             { return (&x)[i]; }
    const float &operator [] (unsigned int i) const { return (&x)[i]; }

    vec3 xyz() const { return vec3(x, y, z); }
};

struct mat4
{
    vec4 x, y, z, w; // columns

    mat4() { }
    explicit mat4(float s) : x(0.0f), y(0.0f), z(0.0f), w(0.0f)
    {
        x.x = s;
        y.y = s;
        z.z = s;
        w.w = s;
    }

    mat4 operator * (const mat4 &rhs)
    {
        mat4 m;
        for (int lrow = 0; lrow < 4; ++lrow)
        {
            for (int rcol = 0; rcol < 4; ++rcol)
            {
                m[rcol][lrow] = 0.0f;
                for (int k = 0; k < 4; ++k)
                    m[rcol][lrow] += (*this)[k][lrow] * rhs[rcol][k];

            }
        }
        return m;
    }

    mat4 operator * (const float s)
    {
        mat4 m = *this;
        m.x *= s;
        m.y *= s;
        m.z *= s;
        m.w *= s;
        return m;
    }

    vec4 operator * (const vec4 &rhs)
    {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }

    vec4 &operator [] (unsigned int i) { return (&x)[i]; }
    const vec4 &operator [] (unsigned int i) const { return (&x)[i]; }
    const float *value_ptr() const { return &(x[0]); }
    float *value_ptr() { return &(x[0]); }
};

static float smoothstep(float edge0, float edge1, float t)
{
    t = (t - edge0) / (edge1 - edge0);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

static vec3 normalize(const vec3 &v)
{
    return v / sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static mat4 perspective(float fovy, float aspect, float near, float far)
{
    mat4 m(1.0f);
    float invtf = 1.0f / tan(fovy * 0.5f);
    m[0].x = invtf / aspect;
    m[1].y = invtf;
    m[2].z = -(far + near) / (far - near);
    m[2].w = -1.0f;
    m[3].z = (-2.0f * far * near) / (far - near);
    m[3].w = 0.0f;
    return m;
}

static mat4 orthographic(float left, float right, float bottom, float top, float near, float far)
{
    mat4 m(1.0f);
    m[0].x = 2.0f / (right - left);
    m[3].x = -(right + left) / (right - left);
    m[1].y = 2.0f / (top - bottom);
    m[3].y = -(top + bottom) / (top - bottom);
    m[2].z = -2.0f / (far - near);
    m[3].z = -(far + near) / (far - near);
    return m;
}

// http://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
static mat4 inverse(const mat4 &op)
{
    mat4 inv_mat(0.0f);
    const float *m = op.value_ptr();
    float *inv = inv_mat.value_ptr();

    inv[0] = m[5]  * m[10] * m[15] -
    m[5]  * m[11] * m[14] -
    m[9]  * m[6]  * m[15] +
    m[9]  * m[7]  * m[14] +
    m[13] * m[6]  * m[11] -
    m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] +
    m[4]  * m[11] * m[14] +
    m[8]  * m[6]  * m[15] -
    m[8]  * m[7]  * m[14] -
    m[12] * m[6]  * m[11] +
    m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] -
    m[4]  * m[11] * m[13] -
    m[8]  * m[5] * m[15] +
    m[8]  * m[7] * m[13] +
    m[12] * m[5] * m[11] -
    m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] +
    m[4]  * m[10] * m[13] +
    m[8]  * m[5] * m[14] -
    m[8]  * m[6] * m[13] -
    m[12] * m[5] * m[10] +
    m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] +
    m[1]  * m[11] * m[14] +
    m[9]  * m[2] * m[15] -
    m[9]  * m[3] * m[14] -
    m[13] * m[2] * m[11] +
    m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] -
    m[0]  * m[11] * m[14] -
    m[8]  * m[2] * m[15] +
    m[8]  * m[3] * m[14] +
    m[12] * m[2] * m[11] -
    m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] +
    m[0]  * m[11] * m[13] +
    m[8]  * m[1] * m[15] -
    m[8]  * m[3] * m[13] -
    m[12] * m[1] * m[11] +
    m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] -
    m[0]  * m[10] * m[13] -
    m[8]  * m[1] * m[14] +
    m[8]  * m[2] * m[13] +
    m[12] * m[1] * m[10] -
    m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] -
    m[1]  * m[7] * m[14] -
    m[5]  * m[2] * m[15] +
    m[5]  * m[3] * m[14] +
    m[13] * m[2] * m[7] -
    m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] +
    m[0]  * m[7] * m[14] +
    m[4]  * m[2] * m[15] -
    m[4]  * m[3] * m[14] -
    m[12] * m[2] * m[7] +
    m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] -
    m[0]  * m[7] * m[13] -
    m[4]  * m[1] * m[15] +
    m[4]  * m[3] * m[13] +
    m[12] * m[1] * m[7] -
    m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] +
    m[0]  * m[6] * m[13] +
    m[4]  * m[1] * m[14] -
    m[4]  * m[2] * m[13] -
    m[12] * m[1] * m[6] +
    m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] +
    m[1] * m[7] * m[10] +
    m[5] * m[2] * m[11] -
    m[5] * m[3] * m[10] -
    m[9] * m[2] * m[7] +
    m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] -
    m[0] * m[7] * m[10] -
    m[4] * m[2] * m[11] +
    m[4] * m[3] * m[10] +
    m[8] * m[2] * m[7] -
    m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] +
    m[0] * m[7] * m[9] +
    m[4] * m[1] * m[11] -
    m[4] * m[3] * m[9] -
    m[8] * m[1] * m[7] +
    m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] -
    m[0] * m[6] * m[9] -
    m[4] * m[1] * m[10] +
    m[4] * m[2] * m[9] +
    m[8] * m[1] * m[6] -
    m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return mat4(1.0f);

    det = 1.0f / det;
    return inv_mat * det;
}

static mat4 rotateX(float rad)
{
    float co = cosf(rad); float si = sinf(rad);
    mat4 m(1.0f);
    m[1][1] = co; m[1][2] = -si; m[2][1] = si; m[2][2] = co;
    return m;
}

static mat4 rotateY(float rad)
{
    float co = cosf(rad); float si = sinf(rad);
    mat4 m(1.0f);
    m[0][0] = co; m[0][2] = si; m[2][0] = -si; m[2][2] = co;
    return m;
}

static mat4 rotateZ(float rad)
{
    float co = cosf(rad); float si = sinf(rad);
    mat4 m(1.0f);
    m[0][0] = co; m[1][0] = -si; m[0][1] = si; m[1][1] = co;
    return m;
}

static mat4 translate(float x, float y, float z)
{
    mat4 m(1.0f);
    m[3][0] = x; m[3][1] = y; m[3][2] = z; m[3][3] = 1.0f;
    return m;
}

static mat4 translate(const vec3 &v)
{
    mat4 m(1.0f);
    m[3][0] = v.x; m[3][1] = v.y; m[3][2] = v.z;
    return m;
}

static mat4 scale(float x, float y, float z)
{
    mat4 m(1.0f);
    m[0][0] = x; m[1][1] = y; m[2][2] = z;
    return m;
}

static mat4 scale(float s)
{
    return scale(s, s, s);
}

#endif