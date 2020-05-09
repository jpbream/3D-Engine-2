#include "Mat2.h"
#include <memory>

Mat2 Mat2::Identity({ 1, 0 }, { 0, 1 });

Mat2::Mat2() {
	memset(data, 0, sizeof(Mat2));
}

Mat2::Mat2(const Vec2& c1, const Vec2& c2)
{

	cols[0] = c1;
	cols[1] = c2;

}

Mat2::Mat2(const Mat2& m) {

	memcpy(data, m.data, sizeof(Mat2));

}

Mat2& Mat2::operator=(const Mat2& m) {

	memcpy(data, m.data, sizeof(Mat2));
	return *this;

}

float& Mat2::operator()(int r, int c) {

	// data is stored column major
	return data[c * 2 + r];

}
float Mat2::operator()(int r, int c) const {

	// data is stored column major
	return data[c * 2 + r];

}

Vec2& Mat2::operator[](int c) {

	return cols[c];

}
Vec2 Mat2::operator[](int c) const {

	return cols[c];

}

Mat2 Mat2::operator*(const Mat2& m) const {

	Vec2 thisRow1(data[0], data[2]);
	Vec2 thisRow2(data[1], data[3]);

	Vec2 newCol1(thisRow1 * m.cols[0], thisRow2 * m.cols[0]);
	Vec2 newCol2(thisRow1 * m.cols[1], thisRow2 * m.cols[1]);

	return { newCol1, newCol2 };

}

Vec2 Mat2::operator*(const Vec2& v) const {

	Vec2 thisRow1(data[0], data[2]);
	Vec2 thisRow2(data[1], data[3]);

	return { thisRow1 * v, thisRow2 * v };

}

Mat2 Mat2::operator*(float c) const {

	return { cols[0] * c, cols[1] * c };

}

float Mat2::GetDeterminant() const {
	return data[0] * data[3] - data[2] * data[1];
}
Mat2 Mat2::GetInverse() const {

	Mat2 i({ data[3], -data[1] }, { -data[2], data[0] });
	return i * (1 / GetDeterminant());
}
Mat2 Mat2::GetTranspose() const {
	return
	{
		{data[0], data[2]},
		{data[1], data[3]},
	};
}
