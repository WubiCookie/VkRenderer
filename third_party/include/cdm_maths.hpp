/* cdm_maths - v0.1 - geometric library - https://github.com/WubiCookie/cdm
   no warranty implied; use at your own risk

LICENSE

       DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                   Version 2, December 2004

Copyright (C) 2021 Charles Seizilles de Mazancourt <charles DOT de DOT mazancourt AT hotmail DOT fr>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

 0. You just DO WHAT THE FUCK YOU WANT TO.

CREDITS

Written by Charles Seizilles de Mazancourt
*/

#ifndef CDM_MATHS_HPP
#define CDM_MATHS_HPP

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#undef min
#undef max
#undef near
#undef far

namespace cdm
{
template<typename T>
class normalized;
struct complex;
struct radian;
struct degree;
struct vector2;
struct vector3;
struct vector4;
struct matrix2;
struct matrix3;
struct matrix3x4;
struct matrix4x3;
struct matrix4;
struct perspective;
struct euler_angles;
struct quaternion;
struct cartesian_direction2d;
struct polar_direction2d;
struct line;
struct plane;
//struct rect;
struct aa_rect;
struct circle;
//struct ellipse;
//struct aa_ellipse;
struct ray2d;
struct ray3d;
struct aabb;
struct transform2d;
struct transform3d;
struct uniform_transform2d;
struct uniform_transform3d;
struct unscaled_transform2d;
struct unscaled_transform3d;

namespace detail
{
template<unsigned char x, unsigned char y>
float get_quaternion_matrix_element(quaternion q);
} // namespace detail

inline constexpr float pi = 3.141592653589793238462643f;
inline constexpr float deg_to_rad = pi / 180.0f;
inline constexpr float rad_to_deg = 180.0f / pi;

template<typename T>
T clamp(T f, T min_, T max_) { return std::min(max_, std::max(min_, f)); }

template<typename T, typename U>
T lerp(T begin, T end, U percent) { return (end - begin) * percent + begin; }

template<typename T>
bool nearly_equal_epsilon(T f1, T f2, T epsilon) { return std::abs(f1 - f2) < epsilon; }

template<typename T>
bool nearly_equal(T f1, T f2) { return nearly_equal_epsilon(f1, f2, std::numeric_limits<float>::epsilon()); }

template<typename T>
constexpr int sign(T val);
template<typename T>
constexpr int signnum(T val);

struct complex
{
    float r = 0.0f;
    float i = 0.0f;

    complex() = default;
    complex(const complex&) = default;
    complex(complex&&) = default;
    complex(float real, float imaginary);
    explicit complex(radian angle);

    float norm() const;
    float norm_squared() const;
    complex& normalize();
    complex get_normalized() const;
    complex& conjugate();
    complex get_conjugated() const;

    complex operator+(complex c) const;
    complex operator-(complex c) const;

    complex& operator+=(complex c);
    complex& operator-=(complex c);
    complex& operator*=(complex c);

    complex& operator=(const complex&) = default;
    complex& operator=(complex&&) = default;
};

complex operator*(complex c1, complex c2);
complex operator*(complex c, float f);
complex operator*(const normalized<complex>& c1, complex c2);
complex operator*(complex c1, const normalized<complex>& c2);
normalized<complex> operator*(const normalized<complex>& c1, const normalized<complex>& c2);
vector2 operator*(const normalized<complex>& c, vector2 v);

struct radian
{
    float angle = 0.0f;

    radian() = default;
    explicit radian(float f);
    radian(const radian& r);
    radian(degree d);

    explicit operator float() const;

    float sin() const;
    float cos() const;
    float tan() const;
    float asin() const;
    float acos() const;
    float atan() const;
    float sinh() const;
    float cosh() const;
    float tanh() const;
    float asinh() const;
    float acosh() const;
    float atanh() const;

    radian& operator+=(float f);
    radian& operator+=(radian r);
    radian& operator-=(float f);
    radian& operator-=(radian r);
    radian& operator*=(float f);
    radian& operator*=(radian r);
    radian& operator/=(float f);
    radian& operator/=(radian r);

    radian operator-() const;

    radian& operator=(radian r);
    radian& operator=(degree d);
};

radian operator+(radian, radian);
radian operator-(radian, radian);
radian operator*(radian, radian);
radian operator/(radian, radian);
radian operator*(radian, float);
radian operator/(radian, float);
radian operator*(float, radian);
radian operator/(float, radian);
bool operator<(float, radian);
bool operator>(float, radian);
bool operator==(float, radian);
bool operator!=(float, radian);
bool operator>=(float, radian);
bool operator<=(float, radian);
bool operator<(radian, float);
bool operator>(radian, float);
bool operator==(radian, float);
bool operator!=(radian, float);
bool operator>=(radian, float);
bool operator<=(radian, float);
bool operator<(radian, radian);
bool operator>(radian, radian);
bool operator==(radian, radian);
bool operator!=(radian, radian);
bool operator>=(radian, radian);
bool operator<=(radian, radian);

struct degree
{
    float angle = 0.0f;

    degree() = default;
    explicit degree(float f);
    degree(const degree& d);
    degree(radian r);

    explicit operator float() const;

    float sin() const;
    float cos() const;
    float tan() const;
    float asin() const;
    float acos() const;
    float atan() const;
    float sinh() const;
    float cosh() const;
    float tanh() const;
    float asinh() const;
    float acosh() const;
    float atanh() const;

    degree& operator+=(float f);
    degree& operator+=(degree d);
    degree& operator-=(float f);
    degree& operator-=(degree d);
    degree& operator*=(float f);
    degree& operator*=(degree d);
    degree& operator/=(float f);
    degree& operator/=(degree d);

    degree operator-() const;

    degree& operator=(degree d);
    degree& operator=(radian r);
};

degree operator+(degree, degree);
degree operator-(degree, degree);
degree operator*(degree, degree);
degree operator/(degree, degree);
degree operator*(degree, float);
degree operator/(degree, float);
degree operator*(float, degree);
degree operator/(float, degree);
bool operator<(float, degree);
bool operator>(float, degree);
bool operator==(float, degree);
bool operator!=(float, degree);
bool operator>=(float, degree);
bool operator<=(float, degree);
bool operator<(degree, float);
bool operator>(degree, float);
bool operator==(degree, float);
bool operator!=(degree, float);
bool operator>=(degree, float);
bool operator<=(degree, float);
bool operator<(degree, degree);
bool operator>(degree, degree);
bool operator==(degree, degree);
bool operator!=(degree, degree);
bool operator>=(degree, degree);
bool operator<=(degree, degree);

struct vector2
{
    float x = 0.0f, y = 0.0f;

    vector2() = default;
    vector2(float x, float y);

    float norm() const;
    float norm_squared() const;
    vector2& normalize();
    vector2 get_normalized() const;
    vector2& clamp(vector2 min, vector2 max);
    vector2 get_clamped(vector2 min, vector2 max) const;
    vector2& negate();
    vector2 get_negated() const;
    float dot(vector2 v) const;
    static vector2 lerp(vector2 begin, vector2 end, float percent);
    static vector2 nlerp(vector2 begin, vector2 end, float percent);
    static vector2 slerp(vector2 begin, vector2 end, float percent);
    float distance_from(vector2 v) const;
    static float distance_between(vector2 v1, vector2 v2);
    static vector2 from_to(vector2 from, vector2 to);
    vector2 to(vector2 v) const;
    static radian angle(vector2 v1, vector2 v2);
    static bool nearly_equal(vector2 v1, vector2 v2);

    bool lies_on(const line& l);
    //bool lies_on(const rect& r);
    bool lies_on(const aa_rect& r);
    bool lies_on(const circle& c);
    //bool lies_on(const ellipe& e);
    //bool lies_on(const aa_ellipe& e);

    vector2 operator+(vector2 v) const;
    vector2 operator-(vector2 v) const;
    vector2 operator*(float f) const;
    vector2 operator/(float f) const;

    vector2& operator+=(vector2 v);
    vector2& operator-=(vector2 v);
    vector2& operator*=(float f);
    vector2& operator/=(float f);

    vector2 operator-() const;

    bool operator==(vector2 v) const;
    bool operator!=(vector2 v) const;
};

vector2 operator*(float f, vector2 v);
float dot(vector2 lhs, vector2 rhs);

struct vector3
{
    float x = 0.0f, y = 0.0f, z = 0.0f, _ = 0.0f;

    vector3() = default;
    vector3(float x, float y, float z);
    vector3(vector2 v, float z);

    float norm() const;
    float norm_squared() const;
    vector3& normalize();
    vector3 get_normalized() const;
    vector3& clamp(vector3 min, vector3 max);
    vector3 get_clamped(vector3 min, vector3 max) const;
    vector3& negate();
    vector3 get_negated() const;
    float dot(vector3 v) const;
    vector3 cross(vector3 v) const;
    static vector3 lerp(vector3 begin, vector3 end, float percent);
    static vector3 nlerp(vector3 begin, vector3 end, float percent);
    float distance_from(vector3 v) const;
    static float distance_between(vector3 v1, vector3 v2);
    static float distance_squared_between(vector3 v1, vector3 v2);
    static vector3 from_to(vector3 from, vector3 to);
    vector3 to(vector3 v) const;
    radian angle(vector3 v) const;
    radian angle_around_axis(vector3 v, vector3 axis);

    vector3 operator+(vector3 v) const;
    vector3 operator-(vector3 v) const;
    vector3 operator*(float f) const;
    vector3 operator/(float f) const;

    vector3& operator+=(vector3 v);
    vector3& operator-=(vector3 v);
    vector3& operator*=(float f);
    vector3& operator/=(float f);

    vector3 operator-() const;

    bool operator==(vector3 v) const;
    bool operator!=(vector3 v) const;
};

vector3 operator*(float f, vector3 v);
float dot(vector3 lhs, vector3 rhs);

struct vector4
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    vector4() = default;
    vector4(float x, float y, float z, float w);
    vector4(vector2 v, float z, float w);
    vector4(vector3 v, float w);

    float norm() const;
    float norm_squared() const;
    vector4& normalize();
    vector4 get_normalized() const;
    vector4& clamp(vector4 min, vector4 max);
    vector4 get_clamped(vector4 min, vector4 max) const;
    vector4& negate();
    vector4 get_negated() const;
    float dot(vector4 v) const;
    static vector4 lerp(vector4 begin, vector4 end, float percent);
    static vector4 nlerp(vector4 begin, vector4 end, float percent);
    float distance_from(vector4 v) const;
    static float distance_between(vector4 v1, vector4 v2);
    static vector4 from_to(vector4 from, vector4 to);
    vector4 to(vector4 v) const;
    static radian angle(vector4 v1, vector4 v2);

    vector2 xy() const;
    vector3 xyz() const;

    vector4 operator+(vector4 v) const;
    vector4 operator-(vector4 v) const;
    vector4 operator*(float f) const;
    vector4 operator/(float f) const;

    vector4& operator+=(vector4 v);
    vector4& operator-=(vector4 v);
    vector4& operator*=(float f);
    vector4& operator/=(float f);

    vector4 operator-() const;

    bool operator==(vector4 v) const;
    bool operator!=(vector4 v) const;
};

float dot(vector4 lhs, vector4 rhs);

template<typename T>
class normalized
{
    T vector;

public:
    normalized() = default;
    normalized(const normalized&) = default;
    normalized(normalized&&) = default;
    normalized(const T& t);
    normalized(T&& t) noexcept;

    static normalized already_normalized(const T& t);
    static normalized already_normalized(T&& t) noexcept;

    const T& operator*() const;
    const T* operator->() const;
    operator const T&() const;

    normalized operator+(const T& v) const;
    normalized operator-(const T& v) const;
    T operator*(float f) const;
    T operator/(float f) const;

    normalized& operator+=(const T& v);
    normalized& operator-=(const T& v);

    normalized operator-() const;

    bool operator==(const T& v) const;
    bool operator!=(const T& v) const;

    normalized& operator=(const normalized&) = default;
    normalized& operator=(normalized&&) = default;
    normalized& operator=(const T& t);
    normalized& operator=(T&& t) noexcept;
};

struct matrix2
{
    float
        m00 = 0.0f, m10 = 0.0f,
        m01 = 0.0f, m11 = 0.0f;

    matrix2() = default;
    matrix2(float m00, float m10, float m01, float m11);
    explicit matrix2(const matrix3& m);
    explicit matrix2(const matrix4& m);

    static matrix2 zero(void);
    static matrix2 identity(void);
    static matrix2 rotation(radian angle);
    static matrix2 rotation(const normalized<complex>& angle);
    matrix2& transpose();
    matrix2 get_transposed() const;
    float get_determinant() const;

    float& at(uint8_t x, uint8_t y);
    const float& at(uint8_t x, uint8_t y) const;

    vector2 operator*(vector2 v) const;
    matrix2 operator*(matrix2 m) const;
};

struct matrix3
{
    float
        m00 = 0.0f, m10 = 0.0f, m20 = 0.0f,
        m01 = 0.0f, m11 = 0.0f, m21 = 0.0f,
        m02 = 0.0f, m12 = 0.0f, m22 = 0.0f;

    matrix3() = default;
    matrix3(float m00, float m10, float m20,
        float m01, float m11, float m21,
        float m02, float m12, float m22);

    explicit matrix3(matrix2 m);
    explicit matrix3(const matrix4& m);
    explicit matrix3(const transform2d& t);
    explicit matrix3(uniform_transform2d t);
    explicit matrix3(unscaled_transform2d t);

    static matrix3 zero();
    static matrix3 identity();
    static matrix3 rotation(euler_angles r);
    static matrix3 rotation(quaternion q);
    static matrix3 rotation_around_x(radian angle);
    static matrix3 rotation_around_y(radian angle);
    static matrix3 rotation_around_z(radian angle);
    static matrix3 rotation_around_x(normalized<complex> angle);
    static matrix3 rotation_around_y(normalized<complex> angle);
    static matrix3 rotation_around_z(normalized<complex> angle);
    matrix3& inverse();
    matrix3 get_inversed() const;
    matrix3& transpose();
    matrix3 get_transposed() const;
    float get_determinant() const;
    bool is_orthogonal() const;

    float& at(uint8_t x, uint8_t y);
    const float& at(uint8_t x, uint8_t y) const;

    matrix3 operator*(float f) const;
    vector3 operator*(vector3 v) const;
    matrix3 operator*(const matrix3& m) const;
    matrix4x3 operator*(const matrix4x3& m) const;
};

struct matrix3x4
{
    float
        m00 = 0.0f, m10 = 0.0f, m20 = 0.0f,
        m01 = 0.0f, m11 = 0.0f, m21 = 0.0f,
        m02 = 0.0f, m12 = 0.0f, m22 = 0.0f,
        m03 = 0.0f, m13 = 0.0f, m23 = 0.0f;

    matrix3x4() = default;
    matrix3x4(float m00, float m10, float m20,
        float m01, float m11, float m21,
        float m02, float m12, float m22,
        float m03, float m13, float m23);

    explicit matrix3x4(matrix2 m);
    explicit matrix3x4(const matrix3& m);
    explicit matrix3x4(const matrix4& m);
    explicit matrix3x4(const transform3d& t);
    explicit matrix3x4(const uniform_transform3d& t);
    explicit matrix3x4(const unscaled_transform3d& t);

    static matrix3x4 zero();
    static matrix3x4 identity();
    static matrix3x4 rotation(euler_angles r);
    static matrix3x4 rotation(quaternion q);
    static matrix3x4 rotation_around_x(radian angle);
    static matrix3x4 rotation_around_y(radian angle);
    static matrix3x4 rotation_around_z(radian angle);
    static matrix3x4 rotation_around_x(const normalized<complex>& angle);
    static matrix3x4 rotation_around_y(const normalized<complex>& angle);
    static matrix3x4 rotation_around_z(const normalized<complex>& angle);
    matrix4x3 get_transposed() const;

    float& at(uint8_t x, uint8_t y);
    const float& at(uint8_t x, uint8_t y) const;

    matrix3x4 operator*(float f) const;
    vector4 operator*(vector3 v) const;
    matrix3x4 operator*(const matrix3& m) const;
    matrix4 operator*(const matrix4x3& m) const;
};

struct matrix4x3
{
    float
        m00 = 0.0f, m10 = 0.0f, m20 = 0.0f, m30,
        m01 = 0.0f, m11 = 0.0f, m21 = 0.0f, m31,
        m02 = 0.0f, m12 = 0.0f, m22 = 0.0f, m32;

    matrix4x3() = default;
    matrix4x3(float m00, float m10, float m20, float m30,
        float m01, float m11, float m21, float m31,
        float m02, float m12, float m22, float m32);

    explicit matrix4x3(matrix2 m);
    explicit matrix4x3(const matrix3& m);
    explicit matrix4x3(const matrix4& m);
    explicit matrix4x3(const transform3d& t);
    explicit matrix4x3(const uniform_transform3d& t);
    explicit matrix4x3(const unscaled_transform3d& t);

    static matrix4x3 zero();
    static matrix4x3 identity();
    static matrix4x3 rotation(euler_angles r);
    static matrix4x3 rotation(quaternion q);
    static matrix4x3 translation(vector3 t);
    static matrix4x3 rotation_around_x(radian angle);
    static matrix4x3 rotation_around_y(radian angle);
    static matrix4x3 rotation_around_z(radian angle);
    static matrix4x3 rotation_around_x(const normalized<complex>& angle);
    static matrix4x3 rotation_around_y(const normalized<complex>& angle);
    static matrix4x3 rotation_around_z(const normalized<complex>& angle);
    matrix3x4 get_transposed() const;

    float& at(uint8_t x, uint8_t y);
    const float& at(uint8_t x, uint8_t y) const;

    matrix4x3 operator*(float f) const;
    vector3 operator*(vector4 v) const;
    matrix3 operator*(const matrix3x4& m) const;
    matrix4x3 operator*(const matrix4& m) const;
};

struct matrix4
{
    float
        m00 = 0.0f, m10 = 0.0f, m20 = 0.0f, m30 = 0.0f,
        m01 = 0.0f, m11 = 0.0f, m21 = 0.0f, m31 = 0.0f,
        m02 = 0.0f, m12 = 0.0f, m22 = 0.0f, m32 = 0.0f,
        m03 = 0.0f, m13 = 0.0f, m23 = 0.0f, m33 = 0.0f;

    matrix4() = default;
    matrix4(float m00, float m10, float m20, float m30,
        float m01, float m11, float m21, float m31,
        float m02, float m12, float m22, float m32,
        float m03, float m13, float m23, float m33);
    explicit matrix4(matrix2 m);
    explicit matrix4(const matrix3& m);
    matrix4(const matrix4& m) = default;
    explicit matrix4(const perspective& p);
    explicit matrix4(const transform3d& t);
    explicit matrix4(const uniform_transform3d& t);
    explicit matrix4(const unscaled_transform3d& t);

    static matrix4 zero();
    static matrix4 identity();
    static matrix4 rotation(euler_angles r);
    static matrix4 rotation(quaternion q);
    static matrix4 translation(vector3 t);
    static matrix4 scale(vector3 t);
    static matrix4 look_at(vector3 from, vector3 to, vector3 up = { 0.0f, 1.0f, 0.0f });
    static matrix4 orthographic(float left, float right, float top, float bottom, float near, float far);
    static matrix4 rotation_around_x(radian angle);
    static matrix4 rotation_around_y(radian angle);
    static matrix4 rotation_around_z(radian angle);
    static matrix4 rotation_around_x(const normalized<complex>& angle);
    static matrix4 rotation_around_y(const normalized<complex>& angle);
    static matrix4 rotation_around_z(const normalized<complex>& angle);
    bool is_orthogonal() const;
    bool is_homogenous() const;
    matrix4& inverse();
    matrix4 get_inversed() const;
    matrix4& transpose();
    matrix4 get_transposed() const;
    float get_determinant() const;

    float& at(uint8_t x, uint8_t y);
    const float& at(uint8_t x, uint8_t y) const;

    matrix4 operator*(float f) const;
    matrix4 operator/(float f) const;
    vector4 operator*(vector4 v) const;
    matrix3x4 operator*(const matrix3x4& m) const;
    matrix4 operator*(const matrix4& m) const;
};

struct perspective
{
private:
    radian m_angle = degree(90.0f);
    float m_ratio = 1.0f;
    float m_inv_ratio = 1.0f;
    float m_near = 0.01f;
    float m_far = 10000.0f;
    float m_invTanHalfFovy = 1.0f / (m_angle / 2.0f).tan();

public:
    perspective() = default;
    perspective(radian angle, float ratio, float near, float far);
    perspective(const perspective&) = default;
    perspective(perspective&&) = default;

    perspective& operator=(const perspective&) = default;
    perspective& operator=(perspective&&) = default;

    void set_angle(radian angle);
    radian get_angle() const;

    void set_ratio(float ratio);
    float get_ratio() const;

    void set_near(float near_plane);
    float get_near() const;

    void set_far(float far_plane);
    float get_far() const;

    matrix4 to_matrix4() const;

    friend matrix4 operator*(const matrix4& m, const perspective& p);
    friend matrix4 operator*(const perspective& p, const matrix4& m);

    friend matrix4 operator*(const unscaled_transform3d& t, const perspective& p);
    friend matrix4 operator*(const perspective& p, const unscaled_transform3d& t);
};

matrix4 operator*(const matrix4& m, const perspective& p);
matrix4 operator*(const perspective& p, const matrix4& m);

matrix4 operator*(const unscaled_transform3d& t, const perspective& p);
matrix4 operator*(const perspective& p, const unscaled_transform3d& t);

struct euler_angles
{
    radian x, y, z;

    euler_angles() = default;
    euler_angles(radian x, radian y, radian z);
};

struct quaternion
{
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;

    quaternion() = default;
    quaternion(float x, float y, float z, float w);
    quaternion(const normalized<vector3>& axis, radian angle);

    static quaternion zero();
    static quaternion identity();
    quaternion& inverse();
    quaternion get_inversed() const;
    quaternion& conjugate();
    quaternion get_conjugated() const;
    quaternion& normalize();
    quaternion get_normalized() const;
    quaternion& negate();
    quaternion get_negated() const;
    quaternion& clamp(quaternion min, quaternion max);
    quaternion get_clamped(quaternion min, quaternion max) const;
    float norm() const;
    float norm_squared() const;
    static quaternion lerp(quaternion begin, quaternion end, float percent);
    static quaternion nlerp(quaternion begin, quaternion end, float percent);
    static quaternion slerp(quaternion begin, quaternion end, float percent);
    float dot(quaternion) const;
    quaternion cross(quaternion) const;

    quaternion operator+(quaternion q) const;
    quaternion operator-(quaternion q) const;
    quaternion operator*(quaternion q) const; // TODO: implement operator* for normalized<> if result is normalized too
    quaternion operator*(float f) const;
    quaternion operator/(float f) const;

    quaternion& operator+=(quaternion q);
    quaternion& operator-=(quaternion q);
    quaternion& operator*=(float f);
    quaternion& operator/=(float f);

    quaternion operator-() const;

    bool operator==(quaternion v) const;
    bool operator!=(quaternion v) const;
};

vector3 operator*(const normalized<quaternion>& q, vector3 v);

struct cartesian_direction2d
{
    float x, y;

    cartesian_direction2d() = default;
    cartesian_direction2d(float x, float y);
    cartesian_direction2d(const normalized<vector2>& direction);
    cartesian_direction2d(polar_direction2d direction);
    cartesian_direction2d(radian angle);
    cartesian_direction2d(const cartesian_direction2d&) = default;
    cartesian_direction2d(cartesian_direction2d&&) noexcept = default;
    ~cartesian_direction2d() = default;

    cartesian_direction2d& operator=(const cartesian_direction2d&) = default;
    cartesian_direction2d& operator=(cartesian_direction2d&&) noexcept = default;

    cartesian_direction2d& rotate(radian angle);
    cartesian_direction2d get_rotated(radian angle);
};

struct polar_direction2d
{
    radian angle;

    polar_direction2d() = default;
    polar_direction2d(float x, float y);
    polar_direction2d(const normalized<vector2>& direction);
    polar_direction2d(cartesian_direction2d direction);
    polar_direction2d(radian angle);
    polar_direction2d(const polar_direction2d&) = default;
    polar_direction2d(polar_direction2d&&) noexcept = default;
    ~polar_direction2d() = default;

    polar_direction2d& operator=(const polar_direction2d&) = default;
    polar_direction2d& operator=(polar_direction2d&&) noexcept = default;

    polar_direction2d& rotate(radian angle);
    polar_direction2d get_rotated(radian angle);

    polar_direction2d& wrap();
    polar_direction2d get_wrapped();
};

struct line
{
    float coefficient;
    float offset;

    static bool is_parallel(line l1, line l2);
};

inline bool collides(line l1, line l2);
inline bool collides(line l1, line l2, vector2& intersection);
inline bool collides(line l, vector2 v);
inline bool collides(vector2 v, line l);

struct plane
{
    vector3 normal;
    float distance = 0.0f;
};

inline float distance_between(const plane& p, vector3 v);
inline float distance_between(vector3 v, const plane& p);

//struct rect
//{
//  vector2 origin;
//  vector2 dimention;
//  complex angle;
//};
//
//bool collides(const rect& r1, const rect& r2);
//std::size_t collides(const rect& r1, const rect& r2, vector2* intersection1, vector2* insersection2);
//bool collides(const rect& r, const line& l);
//std::size_t collides(const rect& r, const line& l, vector2* intersection1, vector2* insersection2);
//bool collides(const line& l, const rect& r);
//std::size_t collides(const line& l, const rect& r, vector2* intersection1, vector2* insersection2);
//bool collides(const rect& r, vector2 v);
//bool collides(vector2 v, const rect& r);

struct aa_rect
{
    vector2 origin;
    vector2 dimention;

    bool contains(vector2 v) const;
};

inline std::size_t collides(const aa_rect& r1, const aa_rect& r2, vector2* intersection1, vector2* insersection2);
//std::size_t collides(const aa_rect& r1, const rect& r2, vector2* intersection1, vector2* insersection2);
//std::size_t collides(const rect& r1, const aa_rect& r2, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const aa_rect& r, const line& l, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const line& l, const aa_rect& r, vector2* intersection1, vector2* insersection2);
inline bool collides(const aa_rect& r, vector2 v);
inline bool collides(vector2 v, const aa_rect& r);

struct circle
{
    vector2 origin;
    float radius;
};

inline std::size_t collides(const circle& c1, const circle& c2, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const circle& c, const aa_rect& r, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const aa_rect& r, const circle& c, vector2* intersection1, vector2* insersection2);
//std::size_t collides(const circle& c, const rect& r, vector2* intersection1, vector2* insersection2);
//std::size_t collides(const rect& r, const circle& c, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const circle& c, const line& l, vector2* intersection1, vector2* insersection2);
inline std::size_t collides(const line& l, const circle& c, vector2* intersection1, vector2* insersection2);
inline bool collides(const circle& c, vector2 v);
inline bool collides(vector2 v, const circle& c);

//struct ellipse
//{
//  vector2 origin;
//  float semi_minor = 1.0f;
//  float semi_major = 1.0f;
//  complex angle;
//};
//
//inline std::size_t collides(const ellipse& e1, const ellipse& e2, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const ellipse& e, const circle& c, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const circle& c, const ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const ellipse& e, const aa_rect& r, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_rect& r, const ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const ellipse& e, const rect& r, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const rect& r, const ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const ellipse& e, const line& l, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const line& l, const ellipse& e, vector2* intersection1, vector2* insersection2);
//inline bool collides(const ellipse& e, vector2 v);
//inline bool collides(vector2 v, const ellipse& e);
//
//struct aa_ellipse
//{
//  vector2 origin;
//  float semi_minor = 1.0f;
//  float semi_major = 1.0f;
//};
//
//inline std::size_t collides(const aa_ellipse& e1, const aa_ellipse& e2, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_ellipse& e1, const ellipse& e2, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const ellipse& e1, const aa_ellipse& e2, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_ellipse& e, const circle& c, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const circle& c, const aa_ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_ellipse& e, const aa_rect& r, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_rect& r, const aa_ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_ellipse& e, const rect& r, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const rect& r, const aa_ellipse& e, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const aa_ellipse& e, const line& l, vector2* intersection1, vector2* insersection2);
//inline std::size_t collides(const line& l, const aa_ellipse& e, vector2* intersection1, vector2* insersection2);
//inline bool collides(const aa_ellipse& e, vector2 v);
//inline bool collides(vector2 v, const aa_ellipse& e);

struct ray2d
{
    vector2 origin;
    normalized<vector2> direction;
};

struct ray3d
{
    vector3 origin;
    normalized<vector3> direction;
};

inline bool collides(const ray3d& r, const plane& p);
inline bool collides(const ray3d& r, const plane& p, vector3& intersection);
inline bool collides(const plane& p, const ray3d& r);
inline bool collides(const plane& p, const ray3d& r, vector3& intersection);

struct aabb
{
    vector3 min;
    vector3 max;

    bool contains(vector3 p) const;
    vector3 get_center() const;

    aabb operator+(aabb rhs) const;
};

inline bool collides(aabb b, ray3d r);
inline bool collides(const aabb& b1, const aabb& b2);

struct transform2d
{
    vector2 position;
    normalized<complex> rotation;
    vector2 scale = {1.0f, 1.0f};

    transform2d() = default;
    transform2d(vector2 position, normalized<complex> rotation, vector2 scale);

    transform2d operator*(const transform2d& t) const;
    vector2 operator*(vector2 v) const;
};

struct transform3d
{
    vector3 position;
    quaternion rotation;
    vector3 scale = {1.0f, 1.0f, 1.0f};

    transform3d() = default;
    transform3d(vector3 position, quaternion rotation, vector3 scale);
    explicit transform3d(const unscaled_transform3d& t);

    transform3d operator*(const transform3d& t) const;
    vector3 operator*(vector3 v) const;
    quaternion operator*(quaternion q) const;
};

struct uniform_transform2d
{
    vector2 position;
    radian rotation;
    float scale = 1.0f;

    uniform_transform2d() = default;
    uniform_transform2d(vector2 position, radian rotation, float scale);

    uniform_transform2d operator*(uniform_transform2d t) const;
    vector2 operator*(vector2 v) const;
};

struct uniform_transform3d
{
    vector3 position;
    quaternion rotation;
    float scale = 1.0f;

    uniform_transform3d() = default;
    uniform_transform3d(vector3 position, quaternion rotation, float scale);

    uniform_transform3d operator*(const uniform_transform3d& t) const;
    vector3 operator*(vector3 v) const;
    quaternion operator*(quaternion q) const;
};

struct unscaled_transform2d
{
    vector2 position;
    radian rotation;

    unscaled_transform2d() = default;
    unscaled_transform2d(vector2 position, radian rotation);

    unscaled_transform2d operator*(unscaled_transform2d t) const;
    vector2 operator*(vector2 v) const;
};

struct unscaled_transform3d
{
    vector3 position;
    quaternion rotation;

    unscaled_transform3d() = default;
    unscaled_transform3d(vector3 position, quaternion rotation);

    unscaled_transform3d& translate_absolute(vector3 t);
    unscaled_transform3d& translate_relative(vector3 t);
    unscaled_transform3d& rotate(quaternion r);

    unscaled_transform3d& inverse();
    unscaled_transform3d get_inversed() const;

    matrix4 to_matrix() const;

    unscaled_transform3d operator*(const unscaled_transform3d& t) const;
    vector3 operator*(vector3 v) const;
    quaternion operator*(quaternion q) const;
};
} // namespace cdm

inline cdm::radian operator""_rad(long double d);
inline cdm::radian operator""_rad(unsigned long long int i);
inline cdm::radian operator""_pi(long double d);
inline cdm::radian operator""_pi(unsigned long long int i);
inline cdm::degree operator""_deg(long double d);
inline cdm::degree operator""_deg(unsigned long long int i);

#ifdef CDM_MATHS_VECTORIZED
#include "cdm_maths_vectorized.hpp"
#else
#include "cdm_maths_implementation.hpp"
#endif

#endif // CDM_MATHS_HPP
