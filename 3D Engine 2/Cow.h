#pragma once
#include "Renderer.h"
#include "Light.h"

class Cow
{

private:

	Vec3 diffuseColor = { 1, 0, 0 };

	class CowVertex {
	public:
		Vec4 position;
		Vec3 normal;
		Vec2 texel;

		CowVertex();
		CowVertex(const Vec4& position, const Vec3& normal, const Vec2& texel);
	};

	class CowPixel : public Renderer::PixelShaderInput {
	public:
		Vec4 position;
		Vec3 normal;
		Vec3 worldPos;
		Vec2 texel;
		Vec3 shadow;

		CowPixel();

		Vec4& GetPos() override;
	};

	int nTriangles;
	int* pIndices;
	CowVertex* pVertices;

	static const Cow* boundObject;
	static Mat4 boundMatrices[10];
	static Vec3 boundVectors[5];
	static const SpotLight* boundLight;

	static CowPixel MainVertexShader(CowVertex& vertex);
	static Vec4 MainPixelShader(CowPixel& pixel, const Renderer::Sampler<CowPixel>& sampler);

	static CowPixel ShadowVertexShader(CowVertex& vertex);
	static Vec4 ShadowPixelShader(CowPixel& pixel, const Renderer::Sampler<CowPixel>& sampler);

public:

	Cow(const Vec3& position, const Vec3& rotation, const Vec3& scale);
	~Cow();

	Vec3 position;
	Vec3 rotation;
	Vec3 scale;

	void AddToShadowMap(SpotLight& light);
	void Render(Renderer& renderer, const Mat4& proj, const Mat4& view, const SpotLight& light, const Vec3& cameraPos);

};

