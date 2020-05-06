#pragma once
#include <iostream>

class Vec2
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

	Vec2();
	Vec2(float x, float y);
	Vec2(const Vec2& v);

	Vec2& operator=(const Vec2& v);

	float operator*(const Vec2& v) const;
	Vec2 operator+(const Vec2& v) const;
	Vec2 operator-(const Vec2& v) const;

	Vec2 operator*(float s) const;
	Vec2 operator/(float s) const;
	Vec2& operator*=(float s);
	Vec2& operator/=(float s);

	Vec2& operator+=(const Vec2& v);
	Vec2& operator-=(const Vec2& v);

	Vec2 operator-() const;

	float Length() const;
	Vec2 Normalized() const;

};

std::ostream& operator<<(std::ostream& os, const Vec2& v);

