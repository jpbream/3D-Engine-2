#pragma once
#include "Vec4.h"
#include "Surface.h"
#include "Mat4.h"
#include "Renderer.h"

class Cubemap
{

private:

	class C_Vertex {

	public:
		Vec4 position;

		C_Vertex(const Vec4& position);

	};

	class C_Pixel : public Renderer::PixelShaderInput {

	public:
		Vec4 position;
		Vec4 texDirection;

		C_Pixel();
		C_Pixel(const Vec4& position, const Vec4& texDirection);

		Vec4& GetPos() override;

	};

	Surface posx;
	Surface negx;
	Surface posy;
	Surface negy;
	Surface posz;
	Surface negz;

	static const int NUM_TRIANGLES;


	const Mat4 transform;

	const Mat4* projection = nullptr;
	const Mat4* view = nullptr;

	static int INDICES[36];
	static C_Vertex VERTICES[8];

	static Cubemap* boundObject;

	static C_Pixel VertexShader(C_Vertex& vertex);
	static Vec4 PixelShader(C_Pixel& pixel, const Renderer::Sampler<C_Pixel>& sampler);

public:

	Cubemap(
		const std::string& posx,
		const std::string& negx,
		const std::string& posy,
		const std::string& negy,
		const std::string& posz,
		const std::string& negz
	);

	void Render(Renderer& renderer, const Mat4& rotView, const Mat4& projection);

	const Surface* GetPlanes() const;

};
