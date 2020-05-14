#pragma once
#include "Vec3.h"
#include "Mat4.h"
#include "Shapes.h"
#include "Renderer.h"
#include "Mat4.h"
#include "Vec3.h"

class DirectionalLight {

private:

	Vec3 color;
	Vec3 rotation;

	Vec3 direction;
	Mat4 viewMatrix;

	Mat4 orthographicProjection;
	Renderer shadowMapRenderer;

public:

	DirectionalLight(const Vec3& color, const Vec3& rotation);
	DirectionalLight(const Vec3& color, const Vec3& rotation, int shadowMapWidth, int shadowMapHeight);

	const Vec3& GetColor() const;
	const Vec3& GetRotation() const;

	Vec3 GetColorAt(const Vec3& surfaceNormal) const;

	void SetColor(const Vec3& color);
	void SetRotation(const Vec3& rotation);

	void UpdateShadowBox(const Frustum& viewFrustum, const Mat4& camToWorldMatrix);

	template <class Vertex, class Pixel>
	void DrawToShadowMap(int numIndexGroups, int* indices, Vertex* vertices, Renderer::VS_TYPE<Vertex, Pixel> VertexShadowShader, Renderer::PS_TYPE<Pixel> PixelShadowShader) {

		shadowMapRenderer.DrawElementArray<Vertex, Pixel>(numIndexGroups, indices, vertices, VertexShadowShader, PixelShadowShader);

	}

	template <class Vertex, class Pixel, class Mesh>
	void DrawToShadowMap(Mesh* mesh, int numIndexGroups, int* indices, Vertex* vertices) {

		shadowMapRenderer.DrawElementArray<Vertex, Pixel, Mesh>(mesh, numIndexGroups, indices, vertices);

	}

	float SampleShadowMap(float s, float t) const;

	void ClearShadowMap();

	Vec4 WorldToShadowTransform(const Vec4& worldSpaceCoord) const;
	Vec4 WorldToViewportTransform(const Vec4& worldSpaceCoord) const;

};

class SpotLight {

private:

	Vec4 position;
	Vec3 rotation;

	Vec3 direction;
	Mat4 viewMatrix;

	float constant;
	float linear;
	float quadratic;

	float exponent;

	Mat4 perspectiveProjection;

	Renderer shadowMapRenderer;

public:

	SpotLight(const Vec3& color, const Vec4& position, const Vec3& rotation, float constant, float linear, float quadratic, float exponent);
	SpotLight(const Vec3& color, const Vec4& position, const Vec3& rotation, float constant, float linear, float quadratic, float exponent, int shadowMapWidth, int shadowMapHeight);

	const Vec3& GetColor() const;
	const Vec4& GetPosition() const;
	const Vec3& GetRotation() const;

	Vec3 GetColorAt(const Vec4& position) const;

	void SetColor(const Vec3& color);
	void SetPosition(const Vec4& position);
	void SetRotation(const Vec3& rotation);
	void SetConstants(float constant, float linear, float quadratic);
	void SetExponent(float exponent);

	void UpdateShadowBox(const Frustum& viewFrustum, const Mat4& camToWorldMatrix);

	template <class Vertex, class Pixel>
	void DrawToShadowMap(int numIndexGroups, int* indices, Vertex* vertices, Renderer::VS_TYPE<Vertex, Pixel> VertexShadowShader, Renderer::PS_TYPE<Pixel> PixelShadowShader);

	template <class Vertex, class Pixel, class Mesh>
	void DrawToShadowMap(Mesh* mesh, int numIndexGroups, int* indices, Vertex* vertices);

	float SampleShadowMap(float s, float t) const;

	void ClearShadowMap();

	const Mat4& GetViewMatrix() const;


};

class PointLight {

private:

	Vec3 color;
	Vec4 position;

	float constant;
	float linear;
	float quadratic;

	Mat4 perspectiveProjection;

	Renderer posXRenderer;
	Renderer negXRenderer;
	Renderer posYRenderer;
	Renderer negYRenderer;
	Renderer posZRenderer;
	Renderer negZRenderer;

public:

	PointLight(const Vec3& color, const Vec4& position, float constant, float linear, float quadratic);
	PointLight(const Vec3& color, const Vec4& position, float constant, float linear, float quadratic, int shadowMapWidth, int shadowMapHeight);

	const Vec3& GetColor() const;
	const Vec4& GetPosition() const;

	Vec3 GetColorAt(const Vec4& position) const;
	
	void SetColor(const Vec3& color);
	void SetPosition(const Vec4& position);
	void SetConstants(float constant, float linear, float quadratic);

	void UpdateShadowBox(const Frustum& viewFrustum, const Mat4& camToWorldMatrix);

	template <class Vertex, class Pixel>
	void DrawToShadowMap(int numIndexGroups, int* indices, Vertex* vertices, Renderer::VS_TYPE<Vertex, Pixel> VertexShadowShader, Renderer::PS_TYPE<Pixel> PixelShadowShader);

	template <class Vertex, class Pixel, class Mesh>
	void DrawToShadowMap(Mesh* mesh, int numIndexGroups, int* indices, Vertex* vertices);

	float SampleShadowMap(float x, float y, float z) const;

	void ClearShadowMap();

	Mat4 GetViewMatrix() const;

};

//template<class Vertex, class Pixel>
//inline void DirectionalLight::DrawToShadowMap(int numIndexGroups, int* indices, Vertex* vertices, Renderer::VS_TYPE<Vertex, Pixel> VertexShadowShader, Renderer::PS_TYPE<Pixel> PixelShadowShader)
//{
//}
//
//template<class Vertex, class Pixel, class Mesh>
//inline void DirectionalLight::DrawToShadowMap(Mesh* mesh, int numIndexGroups, int* indices, Vertex* vertices)
//{
//}
