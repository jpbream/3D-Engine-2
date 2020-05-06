#include "Vec4.h"
#include <math.h>

Vec4::Vec4() : x(0), y(0), z(0), w(1) {}

Vec4::Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

Vec4::Vec4(const Vec4& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

Vec4& Vec4::operator=(const Vec4& v) {

	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;

	return *this;

}

Vec4 Vec4::operator%(const Vec4& v) const {

	return { y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x, w};

}

float Vec4::operator*(const Vec4& v) const {

	return x * v.x + y * v.y + z * v.z + w * v.w;
}

Vec4 Vec4::operator+(const Vec4& v) const {

	return { x + v.x, y + v.y, z + v.z, w + v.w };
	
}
Vec4 Vec4::operator-(const Vec4& v) const {

	return { x - v.x, y - v.y, z - v.z, w - v.w };

}

Vec4 Vec4::operator*(float s) const {

	return { x * s, y * s, z * s, w * s };

}
Vec4 Vec4::operator/(float s) const {

	return { x / s, y / s, z / s, w / s };

}
Vec4& Vec4::operator*=(float s) {

	x *= s;
	y *= s;
	z *= s;
	w *= s;

	return *this;

}
Vec4& Vec4::operator/=(float s) {

	x /= s;
	y /= s;
	z /= s;
	w /= s;

	return *this;

}

Vec4& Vec4::operator+=(const Vec4& v) {

	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;

	return *this;

}
Vec4& Vec4::operator-=(const Vec4& v) {

	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;

	return *this;

}

Vec4 Vec4::operator-() const {

	return { -x, -y, -z , -w};

}

float Vec4::Length() const {

	return sqrtf(x * x + y * y + z * z + w * w);

}
Vec4 Vec4::Normalized() const {
	return *this * (1 / Length());
}

std::ostream& operator<<(std::ostream& os, const Vec4& v) {

	os << "[ " << v.x << " " << v.y << " " << v.z << " " << v.w << " ]";
	return os;

}
