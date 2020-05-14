#include "Light.h"
#include "Mat3.h"
#include <math.h>
#include <iostream>

#define OFFSET 10
#define FRUSTUM_SCALE 1

#define LARGE_DEPTH 1e30f

static Mat4 viewportMatrix = { 
	{.5, 0, 0, 0}, 
	{0, -.5, 0, 0}, 
	{0, 0, .5, 0}, 
	{.5, .5, .5, 1} 
};

static Box CalculateFrustumBoundingBox(Frustum frustum, const Mat4& camToWorldMatrix, const Mat4& lightViewMatrix) {
	
	// this actually works
	frustum.far /= 3;

	// there are 8 corners on the view frustum
	Vec4 frustumPoints[8];

	// find the frustum points in camera space
	// left/right top/bottom are the x y coordinates of the points on the near plane
	frustumPoints[0] = { frustum.left, frustum.bottom, -frustum.near, 1 };
	frustumPoints[1] = { frustum.right, frustum.bottom, -frustum.near, 1 };
	frustumPoints[2] = { frustum.right, frustum.top, -frustum.near, 1 };
	frustumPoints[3] = { frustum.left, frustum.top, -frustum.near, 1 };

	// what to multiply left/right top/bottom by to get the x y values for the points on the far plane
	// derived using similar triangles
	float farFactor = (frustum.near + frustum.far) / frustum.near;

	frustumPoints[4] = { frustum.left * farFactor, frustum.bottom * farFactor, -frustum.far, 1 };
	frustumPoints[5] = { frustum.right * farFactor, frustum.bottom * farFactor, -frustum.far, 1 };
	frustumPoints[6] = { frustum.right * farFactor, frustum.top * farFactor, -frustum.far, 1 };
	frustumPoints[7] = { frustum.left * farFactor, frustum.top * farFactor, -frustum.far, 1 };
	
	// convert to light space
	for (int i = 0; i < 8; ++i) {

		frustumPoints[i] = lightViewMatrix * camToWorldMatrix * frustumPoints[i];
	}

	// hold the max x y and z values of the 8 points
	float maxX = frustumPoints[0].x; float minX = frustumPoints[0].x;
	float maxY = frustumPoints[0].y; float minY = frustumPoints[0].y;
	float maxZ = frustumPoints[0].z; float minZ = frustumPoints[0].z;

	for (int i = 1; i < 8; ++i) {

		if (frustumPoints[i].x > maxX)
			maxX = frustumPoints[i].x;
		if (frustumPoints[i].x < minX)
			minX = frustumPoints[i].x;

		if (frustumPoints[i].y > maxY)
			maxY = frustumPoints[i].y;
		if (frustumPoints[i].y < minY)
			minY = frustumPoints[i].y;

		if (frustumPoints[i].z > maxZ)
			maxZ = frustumPoints[i].z;
		if (frustumPoints[i].z < minZ)
			minZ = frustumPoints[i].z;

	}

	Box f;

	// subtract an offset from front to prevent pop in of shadows being cast into the scene
	// from behind the near plane. Got the idea from ThinMatrix's video on shadow mapping.
	f.front = -maxZ - OFFSET;
	f.back = -minZ;
	f.left = minX;
	f.right = maxX;
	f.top = minY;
	f.bottom = maxY;

	return f;

}

DirectionalLight::DirectionalLight(const Vec3& color, const Vec3& rotation)
	:
	shadowMapRenderer(0, 0), color(color)
{
	SetRotation(rotation);
}

DirectionalLight::DirectionalLight(const Vec3& color, const Vec3& rotation, int shadowMapWidth, int shadowMapHeight)
	:
	shadowMapRenderer(shadowMapWidth, shadowMapHeight), color(color), rotation(rotation)
{
	SetRotation(rotation);
}

const Vec3& DirectionalLight::GetColor() const
{
	return color;
}

const Vec3& DirectionalLight::GetRotation() const
{
	return rotation;
}

Vec3 DirectionalLight::GetColorAt(const Vec3& surfaceNormal) const
{
	float dot = -surfaceNormal * direction;
	if (dot < 0)
		dot = 0;

	return color * dot;
}

void DirectionalLight::SetColor(const Vec3& color)
{
	this->color = color;
}

void DirectionalLight::SetRotation(const Vec3& rotation)
{
	this->rotation = rotation;

	direction = Mat3::GetRotation(rotation.x, rotation.y, rotation.z) * Vec3(0, 0, -1);
	viewMatrix = Mat4::GetRotation(rotation.x, rotation.y, rotation.z).GetInverse();
}

void DirectionalLight::UpdateShadowBox(const Frustum& viewFrustum, const Mat4& camToWorldMatrix)
{

	// calculate the bounding box for the passed in view frustum
	//Vec4 worldCenter;
	Box boundingBox = CalculateFrustumBoundingBox(viewFrustum, camToWorldMatrix, viewMatrix);

	// use this box to set up the orthographic projection
	// FRUSTUM SCALE will scale the box by a scale factor
	orthographicProjection = Mat4::GetOrthographicProjection(
		boundingBox.front * FRUSTUM_SCALE,
		boundingBox.back * FRUSTUM_SCALE,
		boundingBox.left * FRUSTUM_SCALE,
		boundingBox.right * FRUSTUM_SCALE,
		boundingBox.top * FRUSTUM_SCALE,
		boundingBox.bottom * FRUSTUM_SCALE
	);
}

float DirectionalLight::SampleShadowMap(float s, float t) const
{
	
	if (s >= 0 && s < 1 && t >= 0 && t < 1) {

		return shadowMapRenderer.GetDepthBuffer().GetPixel(
			
			s * shadowMapRenderer.GetDepthBuffer().GetWidth(),
			t * shadowMapRenderer.GetDepthBuffer().GetHeight()
		
		);

	}
	else {

		// texel is out of range of shadow frustum, return value that will
		// not be shadowed
		return LARGE_DEPTH;
	}
}

void DirectionalLight::ClearShadowMap()
{
	shadowMapRenderer.ClearDepthBuffer();

}

Vec4 DirectionalLight::WorldToShadowTransform(const Vec4& worldSpaceCoord) const
{
	return orthographicProjection * viewMatrix * worldSpaceCoord;
}

Vec4 DirectionalLight::WorldToViewportTransform(const Vec4& worldSpaceCoord) const 
{
	return viewportMatrix * orthographicProjection * viewMatrix * worldSpaceCoord;
}
