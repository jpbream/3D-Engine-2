#include "Mat4.h"
#include "Mat3.h"
#include <memory>
#include <math.h>

Mat3 Mat3::Identity({ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 });

Mat3::Mat3() {
	memset(data, 0, sizeof(Mat3));
}

Mat3::Mat3(const Vec3& c1, const Vec3& c2, const Vec3& c3)
{

	cols[0] = c1;
	cols[1] = c2;
	cols[2] = c3;

}

Mat3::Mat3(const Mat3& m) {

	memcpy(data, m.data, sizeof(Mat3));

}

Mat3& Mat3::operator=(const Mat3& m) {

	memcpy(data, m.data, sizeof(Mat3));
	return *this;

}

float& Mat3::operator()(int r, int c) {

	// data is stored column major
	return data[c * 3 + r];

}
float Mat3::operator()(int r, int c) const {

	// data is stored column major
	return data[c * 3 + r];

}

Vec3& Mat3::operator[](int c) {

	return cols[c];

}
Vec3 Mat3::operator[](int c) const {

	return cols[c];

}

Mat3 Mat3::operator*(const Mat3& m) const {

	Vec3 thisRow1(data[0], data[3], data[6]);
	Vec3 thisRow2(data[1], data[4], data[7]);
	Vec3 thisRow3(data[2], data[5], data[8]);

	Vec3 newCol1(thisRow1 * m.cols[0], thisRow2 * m.cols[0], thisRow3 * m.cols[0]);
	Vec3 newCol2(thisRow1 * m.cols[1], thisRow2 * m.cols[1], thisRow3 * m.cols[1]);
	Vec3 newCol3(thisRow1 * m.cols[2], thisRow2 * m.cols[2], thisRow3 * m.cols[2]);

	return { newCol1, newCol2, newCol3 };

}

Vec3 Mat3::operator*(const Vec3& v) const {

	Vec3 thisRow1(data[0], data[3], data[6]);
	Vec3 thisRow2(data[1], data[4], data[7]);
	Vec3 thisRow3(data[2], data[5], data[8]);
 
	return { thisRow1 * v, thisRow2 * v, thisRow3 * v };

}

Mat3 Mat3::operator*(float c) const {
	
	return {cols[0] * c, cols[1] * c, cols[2] * c};

}

float Mat3::GetDeterminant() const {
	
	Vec3 cp = cols[1] % cols[2];

	//walk down the first column, use the cross product of the last two columns
	return cols[0].x * cp.x + cols[0].y * cp.y + cols[0].z * cp.z;

}
Mat3 Mat3::GetInverse() const {
	
	//adjucate matrix method

	float iDeterminant = 1 / GetDeterminant();
	Mat3 transpose = GetTranspose();

	Vec3 adjCol1(transpose.cols[1].y * transpose.cols[2].z - transpose.cols[1].z * transpose.cols[2].y,
		         transpose.cols[1].x * transpose.cols[2].z - transpose.cols[1].z * transpose.cols[2].x,
		         transpose.cols[1].x * transpose.cols[2].y - transpose.cols[1].y * transpose.cols[2].x);

	Vec3 adjCol2(transpose.cols[0].y * transpose.cols[2].z - transpose.cols[0].z * transpose.cols[2].y,
		         transpose.cols[0].x * transpose.cols[2].z - transpose.cols[0].z * transpose.cols[2].x,
		         transpose.cols[0].x * transpose.cols[2].y - transpose.cols[0].y * transpose.cols[2].x);

	Vec3 adjCol3(transpose.cols[0].y * transpose.cols[1].z - transpose.cols[0].z * transpose.cols[1].y,
		         transpose.cols[0].x * transpose.cols[1].z - transpose.cols[0].z * transpose.cols[1].x,
		         transpose.cols[0].x * transpose.cols[1].y - transpose.cols[0].y * transpose.cols[1].x);

	Mat3 adjucate(adjCol1, adjCol2, adjCol3);

	adjucate.cols[0].y *= -1;
	adjucate.cols[1].x *= -1;
	adjucate.cols[1].z *= -1;
	adjucate.cols[2].y *= -1;

	return adjucate * iDeterminant;

}
Mat3 Mat3::GetTranspose() const {
	return 
	{ 
		{data[0], data[3], data[6]}, 
	    {data[1], data[4], data[7]}, 
	    {data[2], data[5], data[8]} 
	};
}

Mat4 Mat3::Augment() const {

	return { {cols[0].x, cols[0].y, cols[0].z, 0},
			 {cols[1].x, cols[1].y, cols[1].z, 0},
			 {cols[2].x, cols[2].y, cols[2].z, 0},
			 {0, 0, 0, 1}
	};

}

Mat3 Mat3::Get2DTranslation(float dx, float dy) {

	return { {1, 0, 0}, {0, 1, 0}, {dx, dy, 1} };

}
Mat3 Mat3::GetRotation(float rx, float ry, float rz) {

	Mat3 matX({1, 0, 0}, {0, cosf(rx), sinf(rx)}, {0, -sinf(rx), cosf(rx)});
	Mat3 matY({cosf(ry), 0, -sinf(ry)}, {0, 1, 0}, {sinf(ry), 0, cosf(ry)});
	Mat3 matZ({cosf(rz), sinf(rz), 0}, {-sinf(rz), cosf(rz), 0}, {0, 0, 1});

	return matZ * matY * matX;
}
Mat3 Mat3::GetScale(float sx, float sy, float sz) {

	return { {sx, 0, 0}, {0, sy, 0}, {0, sz, 0} };

}

std::ostream& operator<<(std::ostream& os, const Mat3& m) {

	os << "[ " << m.data[0] << " " << m.data[3] << " " << m.data[6] << " ]" << std::endl;
	os << "| " << m.data[1] << " " << m.data[4] << " " << m.data[7] << " |" << std::endl;
	os << "[ " << m.data[2] << " " << m.data[5] << " " << m.data[8] << " ]";

	return os;

}
