#include "Shapes.h"
#include "Mat4.h"

#define PI 3.14159265358979323846

static void MakeSphere(int vertsDown, float radius, int* numTriangles, int* numVertices, int** indices, Vec4** vertices) {

	// helper function for the sphere constructor

	// the number of vertices around the sphere
	int vertsAround = vertsDown * 2 + 2;

	// the distance in radians between any adjacent vertex
	float radianGap = PI / (vertsDown + 1);

	// number down * number around plus top and bottom
	*numVertices = vertsDown * vertsAround + 2;

	*vertices = new Vec4[*numVertices];

	Vec4 top = { 0, radius, 0, 1 };
	Vec4 bottom = { 0, -radius, 0, 1 };

	// what index into the vertex or index we are
	int count = 0;

	// store top as the first vertex
	*vertices[0] = top;

	// i loops down the sphere
	for (int i = 0; i < vertsDown; ++i) {

		// the z rotation multiplication must be applied first
		float rotz = (i + 1) * radianGap;
		Vec4 down = Mat4::GetRotation(0, 0, rotz) * top;

		// j loops around the sphere
		for (int j = 0; j < vertsAround; ++j) {

			float roty = (j + 1) * radianGap;

			// generate the position
			Vec4 vertex = Mat4::GetRotation(0, roty, 0) * down;

			(*vertices)[1 + count] = vertex;
			++count;
		}
	}

	// store bottom as the last vertex
	(*vertices)[*numVertices - 1] = bottom;

	// number of triangles that connect the top and bottom vertices to the rest of the sphere, excluding
	// the final gap
	int topAndBottom = 2 * (vertsAround - 1);

	// number of triangles that connect the main mesh of the sphere, excluding the final gap
	int mainBody = 2 * (vertsDown - 1) * (vertsAround - 1);

	// number of triangles that connect the final gap, excluding the triangles
	// connecting to the top and bottom
	int finalGap = 2 * (vertsDown - 1);

	// total number of triangles in the sphere is the sum of all of these
	// extra 2 is the two triangles that connect the final gap to the top and botton
	*numTriangles = topAndBottom + mainBody + finalGap + 2;

	// total number of indices is 3 times that
	*indices = new int[*numTriangles * 3];


	count = 0;

	// all indices from here out must add 1 to them to skip over the top index
	// that is first in the list

	// connect to top and bottom (first and last row)
	for (int j = 0; j < vertsAround - 1; ++j) {

		(*indices)[count++] = j + 1;
		(*indices)[count++] = j + 2;
		(*indices)[count++] = 0;

		// vertsdown - 1 is the last row
		(*indices)[count++] = (vertsDown - 1) * vertsAround + j + 1;
		(*indices)[count++] = *numVertices - 1;
		(*indices)[count++] = (vertsDown - 1) * vertsAround + j + 2;
		
	}

	// connect all except last gap
	for (int i = 0; i < vertsDown - 1; ++i) {

		for (int j = 0; j < vertsDown * 2 + 1; ++j) {

			// multiplying a number by vertsAround selects
			// that row

			int i1 = i * vertsAround + j;
			int i2 = (i + 1) * vertsAround + j;
			int i3 = (i + 1) * vertsAround + (j + 1);
			int i4 = i * vertsAround + (j + 1);


			(*indices)[count++] = i1 + 1;
			(*indices)[count++] = i3 + 1;
			(*indices)[count++] = i4 + 1;

			(*indices)[count++] = i1 + 1;
			(*indices)[count++] = i2 + 1;
			(*indices)[count++] = i3 + 1;
		}
	}

	// connect last gap (first column to last column)
	for (int i = 0; i < vertsDown - 1; ++i) {

		int i1 = i * vertsAround;
		int i2 = (i + 1) * vertsAround;
		int i3 = (i + 1) * vertsAround + vertsAround - 1;
		int i4 = i * vertsAround + vertsAround - 1;

		(*indices)[count++] = i1 + 1;
		(*indices)[count++] = i4 + 1;
		(*indices)[count++] = i3 + 1;

		(*indices)[count++] = i1 + 1;
		(*indices)[count++] = i3 + 1;
		(*indices)[count++] = i2 + 1;
		
	}

	// fill in last 2 triangles

	// first row, first and last column
	(*indices)[count++] = 1;
	(*indices)[count++] = 0;
	(*indices)[count++] = (vertsAround - 1) + 1;
	
	// last row, first and last column
	(*indices)[count++] = (vertsDown - 1) * vertsAround + 1;
	(*indices)[count++] = (vertsDown - 1) * vertsAround + (vertsAround - 1) + 1;
	(*indices)[count++] = *numVertices - 1;

}

Sphere::Sphere(int resolution, float radius) : radius(radius) {

	// call make sphere instead of having all that code in this constructor
	MakeSphere(resolution, radius, &nTriangles, &nVertices, &pIndices, &pVertices);

}

Sphere::~Sphere() {

	if (pIndices != nullptr)
		delete[] pIndices;

	if (pVertices != nullptr)
		delete[] pVertices;

}

