#pragma once
#include <iostream>

class Vec3
{

public:
	
	union {
		float x;
		float r;
		float s;
	};

	union {
		float y;
		float g;
		float t;
	};

	union {
		float z;
		float b;
		float p;
	};

	Vec3();
	Vec3(float x, float y, float z);
	Vec3(const Vec3& v);

	Vec3& operator=(const Vec3& v);

	float operator*(const Vec3& v) const;
	Vec3 operator%(const Vec3& v) const;
	Vec3 operator+(const Vec3& v) const;
	Vec3 operator-(const Vec3& v) const;

	Vec3 operator*(float s) const;
	Vec3 operator/(float s) const;
	Vec3& operator*=(float s);
	Vec3& operator/=(float s);

	Vec3& operator+=(const Vec3& v);
	Vec3& operator-=(const Vec3& v);

	Vec3 operator-() const;

	float Length() const;
	Vec3 Normalized() const;

	Vec3 Reflect(const Vec3& normal) const;
	Vec3 Refract(const Vec3& normal, float n1, float n2) const;

};

std::ostream& operator<<(std::ostream& os, const Vec3& v);

