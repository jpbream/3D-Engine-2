#pragma once
#include "Vec3.h"
#include "Mat2.h"
#include <iostream>

class Mat4;

class Mat3
{

	friend std::ostream& operator<<(std::ostream& os, const Mat3& dt);

private:

	union {

		float data[9];
		Vec3 cols[3];

	};

public:
	
	static Mat3 Identity;

	Mat3();
	Mat3(const Vec3& c1, const Vec3& c2, const Vec3& c3);
	Mat3(const Mat3& m);

	Mat3& operator=(const Mat3& m);

	float& operator()(int r, int c);
	float operator()(int r, int c) const;

	Vec3& operator[](int c);
	Vec3 operator[](int c) const;

	Mat3 operator*(const Mat3& m) const;
	Vec3 operator*(const Vec3& v) const;
	Mat3 operator*(float c) const;

	float GetDeterminant() const;
	Mat3 GetInverse() const;
	Mat3 GetTranspose() const;

	Mat4 Augment() const;

	static Mat3 Get2DTranslation(float dx, float dy);
	static Mat3 GetRotation(float rx, float ry, float rz);
	static Mat3 GetScale(float sx, float sy, float sz);

};

std::ostream& operator<<(std::ostream& os, const Mat3& dt);



