#include "Vec3.h"
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

std::ostream& operator<<(std::ostream& os, const Vec3& v) {

	os << "[ " << v.x << " " << v.y << " " << v.z << " ]";
	return os;

}
