#pragma once
#include "Vec4.h"
#include <iostream>

class Mat3;

class Mat4
{

	friend std::ostream& operator<<(std::ostream& os, const Mat4& dt);

private:

	union {

		float data[16];
		Vec4 cols[4];

	};

public:

	static const Mat4 Identity;
	static const Mat4 Viewport;

	Mat4();
	Mat4(const Vec4& c1, const Vec4& c2, const Vec4& c3, const Vec4& c4);
	Mat4(const Mat4& m);

	Mat4& operator=(const Mat4& m);

	float& operator()(int r, int c);
	float operator()(int r, int c) const;

	Vec4& operator[](int c);
	Vec4 operator[](int c) const;

	Mat4 operator*(const Mat4& m) const;
	Vec4 operator*(const Vec4& v) const;
	Mat4 operator*(float c) const;

	float GetDeterminant() const;
	Mat4 GetInverse() const;
	Mat4 GetTranspose() const;

	Mat3 Truncate() const;

	static Mat4 Get3DTranslation(float dx, float dy, float dz);
	static Mat4 GetRotation(float rx, float ry, float rz);
	static Mat4 GetScale(float sx, float sy, float sz);
	static Mat4 GetPerspectiveProjection(float n, float f, float l, float r, float t, float b);
	static Mat4 GetOrthographicProjection(float n, float f, float l, float r, float t, float b);

};

std::ostream& operator<<(std::ostream& os, const Mat4& dt);



