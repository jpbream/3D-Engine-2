#include "Cubemap.h"

#define FAR 25

const int Cubemap::NUM_TRIANGLES = 12;

int Cubemap::INDICES[36] = {

		0, 1, 2,
		0, 2, 3,

		1, 5, 6,
		1, 6, 2,

		3, 6, 7,
		3, 2, 6,

		4, 1, 0,
		4, 5, 1,

		0, 3, 7,
		0, 7, 4,

		4, 7, 6,
		4, 6, 5

};

Cubemap::C_Vertex Cubemap::VERTICES[8] = {
	{ Vec4(-.5, -.5, -.5, 1) },
	{ Vec4(.5, -.5, -.5, 1) },
	{ Vec4(.5, .5, -.5, 1) },
	{ Vec4(-.5, .5, -.5, 1) },
	{ Vec4(-.5, -.5, .5, 1) },
	{ Vec4(.5, -.5, .5, 1) },
	{ Vec4(.5, .5, .5, 1) },
	{ Vec4(-.5, .5, .5, 1) }
};

Cubemap* Cubemap::boundObject = nullptr;

Cubemap::Cubemap(
	const std::string& posx,
	const std::string& negx,
	const std::string& posy,
	const std::string& negy,
	const std::string& posz,
	const std::string& negz
) :
	posx(posx), negx(negx), posy(posy), negy(negy), posz(posz), negz(negz),
	transform(Mat4::GetScale(FAR, FAR, FAR))

{

}

Cubemap::C_Vertex::C_Vertex(const Vec4& position) : position(position) {}
Cubemap::C_Pixel::C_Pixel() {}
Cubemap::C_Pixel::C_Pixel(const Vec4& position, const Vec4& texDirection) : position(position), texDirection(texDirection) {}
Vec4& Cubemap::C_Pixel::GetPos() { return position; }

const Surface* Cubemap::GetPlanes() const {
	return &posx;
}

void Cubemap::Render(Renderer& renderer, const Mat4& rotView, const Mat4& projection) {

	this->projection = &projection;
	this->view = &rotView;

	boundObject = this;

	renderer.DrawElementArray<C_Vertex, C_Pixel>(NUM_TRIANGLES, INDICES, VERTICES, VertexShader, PixelShader);

	boundObject = nullptr;
}

Cubemap::C_Pixel Cubemap::VertexShader(C_Vertex& vertex) {

	Vec4 pos = *(boundObject->projection) * *(boundObject->view) * boundObject->transform * vertex.position;
	return { pos, vertex.position };

}
Vec4 Cubemap::PixelShader(C_Pixel& pixel, const Renderer::Sampler<C_Pixel>& sampler) {

	Vec4& dir = pixel.texDirection;

	// this scheme came from LearnOpenGL.com
	return sampler.SampleCubeMap(boundObject->GetPlanes(), dir.x, dir.y, dir.z);
 }
