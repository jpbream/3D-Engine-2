#include "Vec3.h"
#include "Vec4.h"
#include <math.h>

Vec3::Vec3() : x(0), y(0), z(0) {}

Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

Vec3::Vec3(const Vec3& v) : x(v.x), y(v.y), z(v.z) {}

Vec3& Vec3::operator=(const Vec3& v) {

	x = v.x;
	y = v.y;
	z = v.z;

	return *this;

}

float Vec3::operator*(const Vec3& v) const {

	return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::operator%(const Vec3& v) const {

	return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};

}

Vec3 Vec3::operator+(const Vec3& v) const {

	return {x + v.x, y + v.y, z + v.z};

}
Vec3 Vec3::operator-(const Vec3& v) const {

	return {x - v.x, y - v.y, z - v.z};

}

Vec3 Vec3::operator*(float s) const {

	return {x * s, y * s, z * s};

}
Vec3 Vec3::operator/(float s) const {

	return {x / s, y / s, z / s};

}
Vec3& Vec3::operator*=(float s) {

	x *= s;
	y *= s;
	z *= s;

	return *this;

}
Vec3& Vec3::operator/=(float s) {

	x /= s;
	y /= s;
	z /= s;

	return *this;

}

Vec3& Vec3::operator+=(const Vec3& v) {

	x += v.x;
	y += v.y;
	z += v.z;

	return *this;

}
Vec3& Vec3::operator-=(const Vec3& v) {

	x -= v.x;
	y -= v.y;
	z -= v.z;

	return *this;

}

Vec3 Vec3::operator-() const {

	return {-x, -y, -z};

}

float Vec3::Length() const {

	return sqrtf(x * x + y * y + z * z);

}
Vec3 Vec3::Normalized() const {
	return *this * (1 / Length());
}

Vec3 Vec3::Reflect(const Vec3& normal) const {

	// Equations from "Mathematics for 3D Game Programming and Computer Graphics" by Eric Lengyel

	Vec3 thisNormal = Normalized();
	Vec3 normNormal = normal.Normalized();

	return normNormal * (normNormal * thisNormal * 2) - thisNormal;

}

Vec3 Vec3::Refract(const Vec3& normal, float n1, float n2) const {

	// Equations from "Mathematics for 3D Game Programming and Computer Graphics" by Eric Lengyel

	Vec3 thisNormal = Normalized();
	Vec3 normNormal = normal.Normalized();

	float nRatio = n1 / n2;

	float coefficient = ((normNormal * nRatio) * thisNormal) - (sqrt(1 - (nRatio * nRatio) * (1 - pow(normNormal * thisNormal, 2))));

	return normNormal * coefficient - thisNormal * nRatio;

}

Vec3 Vec3::Modulate(const Vec3& v1, const Vec3& v2) {

	return { v1.r * v2.r, v1.g * v2.g, v1.b * v2.b };
}

Vec4 Vec3::Vec4() const {
	return { x, y, z, 1 };
}

std::ostream& operator<<(std::ostream& os, const Vec3& v) {

	os << "[ " << v.x << " " << v.y << " " << v.z << " ]";
	return os;

}
