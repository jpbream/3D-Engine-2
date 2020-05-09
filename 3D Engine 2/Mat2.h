#pragma once
#include "Vec2.h"

class Mat2
{

private:

	union {

		float data[4];
		Vec2 cols[2];

	};

public:

	static Mat2 Identity;

	Mat2();
	Mat2(const Vec2& c1, const Vec2& c2);
	Mat2(const Mat2& m);

	Mat2& operator=(const Mat2& m);

	float& operator()(int r, int c);
	float operator()(int r, int c) const;

	Vec2& operator[](int c);
	Vec2 operator[](int c) const;

	Mat2 operator*(const Mat2& m) const;
	Vec2 operator*(const Vec2& v) const;
	Mat2 operator*(float c) const;

	float GetDeterminant() const;
	Mat2 GetInverse() const;
	Mat2 GetTranspose() const;

};



