#include "Light.h"
#include "Mat3.h"
#include <math.h>
#include <iostream>

#define OFFSET 10
#define RANGE 25
#define LARGE_DEPTH 1e30f

#define SHADOW_DEPTH_OFFSET 0.007

static Box CalculateFrustumBoundingBox(const Frustum& frustum, const Mat4& camToWorldMatrix, const Mat4& lightViewMatrix) {

	// in this function we fake the far plane as being at RANGE to artificially limit the
	// shadow casting range along the z axis, since the far plane is usually very far away

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
	float farFactor = (frustum.near + RANGE) / frustum.near;

	frustumPoints[4] = { frustum.left * farFactor, frustum.bottom * farFactor, -RANGE, 1 };
	frustumPoints[5] = { frustum.right * farFactor, frustum.bottom * farFactor, -RANGE, 1 };
	frustumPoints[6] = { frustum.right * farFactor, frustum.top * farFactor, -RANGE, 1 };
	frustumPoints[7] = { frustum.left * farFactor, frustum.top * farFactor, -RANGE, 1 };
	
	// convert to light space
	for (int i = 0; i < 8; ++i)
		frustumPoints[i] = lightViewMatrix * camToWorldMatrix * frustumPoints[i];

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

	// if the y's are not reversed, the shadow map is drawn to a file upside down for some reason
	// it makes no visual difference in the program though
	f.top = minY;
	f.bottom = maxY;

	return f;

}

/////// DIRECTIONAL LIGHT /////////

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

float DirectionalLight::FacingFactor(const Vec3& surfaceNormal) const
{
	float ff = -direction * surfaceNormal;
	if ( ff < 0 )
		return 0;
	return ff;
}

float DirectionalLight::SpecularFactor(const Vec3& surfaceNormal, const Vec3& toCamera, float specularExponent) const
{
	// specular reflection model described in
	// "Mathematics for 3D Game Programming and Computer Graphics" by Eric Lengyel
	// Section 7.4

	Vec3 toLight = -direction;

	// vector halfway between to-viewer and to-light vector
	Vec3 halfway = (toLight + toCamera).Normalized();

	// spec factor is how much to scale the specular color by
	float specFactor = surfaceNormal * halfway;
	if ( specFactor < 0 )
		specFactor = 0;
	else
		specFactor = powf(specFactor, specularExponent);

	return surfaceNormal * toLight > 0 ? specFactor : 0;
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
	Box boundingBox = CalculateFrustumBoundingBox(viewFrustum, camToWorldMatrix, viewMatrix);

	// use this box to set up the orthographic projection
	// FRUSTUM SCALE will scale the box by a scale factor
	orthographicProjection = Mat4::GetOrthographicProjection(
		boundingBox.front,
		boundingBox.back,
		boundingBox.left,
		boundingBox.right,
		boundingBox.top,
		boundingBox.bottom
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

float DirectionalLight::MultiSampleShadowMap(const Vec3& shadowCoord, int sampleWidth) const
{
	// if the light is not using a shadow map, indicate that no pixels are in shadow
	if ( GetShadowMapWidth() == 0 || GetShadowMapHeight() == 0 )
		return 0.0f;

	float xoff = 1.0f / GetShadowMapWidth();
	float yoff = 1.0f / GetShadowMapHeight();

	float fracInShadow = 0;

	for ( int i = 0; i < sampleWidth; i++ ) {
		for ( int j = 0; j < sampleWidth; j++ ) {

			float s = shadowCoord.s + (sampleWidth / 2.0f) * xoff - i * xoff;
			float t = shadowCoord.t + (sampleWidth / 2.0f) * yoff - j * yoff;

			float sample = SampleShadowMap(s, t);

			if ( shadowCoord.p > sample + SHADOW_DEPTH_OFFSET )
				fracInShadow += 1.0f / (sampleWidth * sampleWidth);

		}
	}

	return fracInShadow;
}

int DirectionalLight::GetShadowMapWidth() const {
	return shadowMapRenderer.GetDepthBuffer().GetWidth();
}

int DirectionalLight::GetShadowMapHeight() const {
	return shadowMapRenderer.GetDepthBuffer().GetHeight();
}

void DirectionalLight::ClearShadowMap()
{
	shadowMapRenderer.ClearDepthBuffer();

}

Mat4 DirectionalLight::WorldToShadowMatrix() const
{
	return orthographicProjection * viewMatrix;
}



/////// SPOT LIGHT /////////

SpotLight::SpotLight(const Vec3& color, const Vec3& position, const Vec3& rotation, float constant, float linear, float quadratic, float exponent)

	: color(color), shadowMapRenderer(0, 0), constant(constant), linear(linear), quadratic(quadratic), exponent(exponent)

{
	SetPosition(position);
	SetRotation(rotation);
}

SpotLight::SpotLight(const Vec3& color, const Vec3& position, const Vec3& rotation, float constant, float linear, float quadratic, float exponent, int shadowMapWidth, int shadowMapHeight)

	: color(color), shadowMapRenderer(shadowMapWidth, shadowMapHeight), constant(constant), linear(linear), quadratic(quadratic), exponent(exponent)
{
	SetPosition(position);
	SetRotation(rotation);
}

const Vec3& SpotLight::GetColor() const
{
	return color;
}

const Vec3& SpotLight::GetPosition() const
{
	return position;
}

const Vec3& SpotLight::GetRotation() const
{
	return rotation;
}

Vec3 SpotLight::GetColorAt(const Vec3& position) const
{
	
	// distance from the point to the light
	float distance = (this->position - position).Length();

	// intensity based on the attenuation constants
	float intensity = 1.0f / (constant + linear * distance + quadratic * distance * distance);

	// how much the point to light vector lines up with the spot direction
	// raised to the concentration exponent
	float directionFactor = -direction * (this->position - position).Normalized();
	if ( directionFactor <= 0 )
		return { 0, 0, 0 };

	directionFactor = powf(directionFactor, exponent);

	return color * directionFactor * intensity;

}

float SpotLight::FacingFactor(const Vec3& surfaceNormal) const
{
	float ff = -direction * surfaceNormal;
	if ( ff < 0 )
		return 0;
	return ff;
}

float SpotLight::SpecularFactor(const Vec3& worldPosition, const Vec3& surfaceNormal, const Vec3& toCamera, float specularExponent) const
{
	// specular reflection model described in
	// "Mathematics for 3D Game Programming and Computer Graphics" by Eric Lengyel
	// Section 7.4

	Vec3 toLight = (position - worldPosition).Normalized();

	// vector halfway between to-viewer and to-light vector
	Vec3 halfway = (toLight + toCamera).Normalized();

	// spec factor is how much to scale the specular color by
	float specFactor = surfaceNormal * halfway;
	if ( specFactor < 0 )
		specFactor = 0;
	else
		specFactor = powf(specFactor, specularExponent);

	return surfaceNormal * toLight > 0 ? specFactor : 0;
}

void SpotLight::SetColor(const Vec3& color)
{
	this->color = color;
}

void SpotLight::SetPosition(const Vec3& position)
{
	this->position = position;

	viewMatrix = (Mat4::Get3DTranslation(position.x, position.y, position.z) * Mat4::GetRotation(rotation.x, rotation.y, rotation.z)).GetInverse();
}

void SpotLight::SetRotation(const Vec3& rotation)
{
	this->rotation = rotation;

	direction = Mat3::GetRotation(rotation.x, rotation.y, rotation.z) * Vec3(0, 0, -1);
	viewMatrix = (Mat4::Get3DTranslation(position.x, position.y, position.z) * Mat4::GetRotation(rotation.x, rotation.y, rotation.z)).GetInverse();
}

void SpotLight::SetConstants(float constant, float linear, float quadratic)
{
	this->constant = constant;
	this->linear = linear;
	this->quadratic = quadratic;
}

void SpotLight::SetExponent(float exponent)
{
	this->exponent = exponent;
}

void SpotLight::UpdateShadowBox(const Frustum& viewFrustum, const Mat4& camToWorldMatrix)
{
	// calculate the bounding box for the passed in view frustum
	Box boundingBox = CalculateFrustumBoundingBox(viewFrustum, camToWorldMatrix, viewMatrix);

	// only care about the back of the bounding box
	/*perspectiveProjection = Mat4::GetPerspectiveProjection(
		1.0f,
		-boundingBox.back,
		-1.0f,
		1.0f,
		(float)GetShadowMapHeight() / GetShadowMapWidth(),
		-(float)GetShadowMapHeight() / GetShadowMapWidth()
	);*/

	perspectiveProjection = Mat4::GetOrthographicProjection(
		boundingBox.front,
		boundingBox.back,
		boundingBox.left,
		boundingBox.right,
		boundingBox.top,
		boundingBox.bottom
	);

}

float SpotLight::SampleShadowMap(float s, float t) const
{
	if ( s >= 0 && s < 1 && t >= 0 && t < 1 ) {

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

float SpotLight::MultiSampleShadowMap(const Vec3& shadowCoord, int sampleWidth) const
{
	// if the light is not using a shadow map, indicate that no pixels are in shadow
	if ( GetShadowMapWidth() == 0 || GetShadowMapHeight() == 0 )
		return 0.0f;

	float xoff = 1.0f / GetShadowMapWidth();
	float yoff = 1.0f / GetShadowMapHeight();

	float fracInShadow = 0;

	for ( int i = 0; i < sampleWidth; i++ ) {
		for ( int j = 0; j < sampleWidth; j++ ) {

			float s = shadowCoord.s + (sampleWidth / 2.0f) * xoff - i * xoff;
			float t = shadowCoord.t + (sampleWidth / 2.0f) * yoff - j * yoff;

			float sample = SampleShadowMap(s, t);

			if ( shadowCoord.p > sample + SHADOW_DEPTH_OFFSET )
				fracInShadow += 1.0f / (sampleWidth * sampleWidth);

		}
	}

	return fracInShadow;
}

int SpotLight::GetShadowMapWidth() const
{
	return shadowMapRenderer.GetDepthBuffer().GetWidth();
}

int SpotLight::GetShadowMapHeight() const
{
	return shadowMapRenderer.GetDepthBuffer().GetHeight();
}

void SpotLight::ClearShadowMap()
{
	shadowMapRenderer.ClearDepthBuffer();
}

Mat4 SpotLight::WorldToShadowMatrix() const
{
	return perspectiveProjection * viewMatrix;
}
