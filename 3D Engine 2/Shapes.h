#pragma once
#include "Vec4.h"

struct Box {

	float left;
	float right;
	float bottom;
	float top;
	float front;
	float back;

};

struct Frustum {

	float near;
	float far;

	float left;
	float right;

	float bottom;
	float top;


};

struct Sphere {

	float radius;

	int nTriangles;
	int nVertices;

	int* pIndices = nullptr;
	Vec4* pVertices = nullptr;

	Sphere(int resolution, float radius);
	~Sphere();

};


