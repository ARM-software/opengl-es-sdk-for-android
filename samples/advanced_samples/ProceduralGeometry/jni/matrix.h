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
                {
                    m[rcol][lrow] += (*this)[k][lrow] * rhs[rcol][k];
                }
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

static vec3 normalize(const vec3 &v)
{
    return v / sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

static mat4 transpose(const mat4 &m)
{
    vec4 a = m.x;
    vec4 b = m.y;
    vec4 c = m.z;
    vec4 d = m.w;
    mat4 result;
    result.x = vec4(a.x, b.x, c.x, d.x);
    result.y = vec4(a.y, b.y, c.y, d.y);
    result.z = vec4(a.z, b.z, c.z, d.z);
    result.w = vec4(a.w, b.w, c.w, d.w);
    return result;
}

static mat4 perspective(float fovy, float aspect, float z_near, float z_far)
{
    mat4 m(1.0f);
    float invtf = 1.0f / tan(fovy * 0.5f);
    m[0].x = invtf / aspect;
    m[1].y = invtf;
    m[2].z = -(z_far + z_near) / (z_far - z_near);
    m[2].w = -1.0f;
    m[3].z = (-2.0f * z_far * z_near) / (z_far - z_near);
    m[3].w = 0.0f;
    return m;
}

static mat4 orthographic(float left, float right, float bottom, float top, float z_near, float z_far)
{
    mat4 m(1.0f);
    m[0].x = 2.0f / (right - left);
    m[3].x = -(right + left) / (right - left);
    m[1].y = 2.0f / (top - bottom);
    m[3].y = -(top + bottom) / (top - bottom);
    m[2].z = -2.0f / (z_far - z_near);
    m[3].z = -(z_far + z_near) / (z_far - z_near);
    return m;
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
