#pragma once
#include <iostream>

class Vec4
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
		float r;
	};

	union {
		float w;
		float a;
		float q;
	};

	Vec4();
	Vec4(float x, float y, float z, float w);
	Vec4(const Vec4& v);

	Vec4& operator=(const Vec4& v);

	Vec4 operator%(const Vec4& v) const;

	float operator*(const Vec4& v) const;
	Vec4 operator+(const Vec4& v) const;
	Vec4 operator-(const Vec4& v) const;

	Vec4 operator*(float s) const;
	Vec4 operator/(float s) const;
	Vec4& operator*=(float s);
	Vec4& operator/=(float s);

	Vec4& operator+=(const Vec4& v);
	Vec4& operator-=(const Vec4& v);

	Vec4 operator-() const;

	float Length() const;
	Vec4 Normalized() const;

};

std::ostream& operator<<(std::ostream& os, const Vec4& v);


