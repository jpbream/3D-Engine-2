#include "Cubemap.h"

#define FAR 50

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

Cubemap::Vertex Cubemap::VERTICES[8] = {
	{ Vec4(-.5, -.5, -.5, 1) },
	{ Vec4(.5, -.5, -.5, 1) },
	{ Vec4(.5, .5, -.5, 1) },
	{ Vec4(-.5, .5, -.5, 1) },
	{ Vec4(-.5, -.5, .5, 1) },
	{ Vec4(.5, -.5, .5, 1) },
	{ Vec4(.5, .5, .5, 1) },
	{ Vec4(-.5, .5, .5, 1) }
};

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

Cubemap::Vertex::Vertex(const Vec4& position) : position(position) {}
Cubemap::Pixel::Pixel() {}
Cubemap::Pixel::Pixel(const Vec4& position, const Vec4& texDirection) : position(position), texDirection(texDirection) {}

Vec4& Cubemap::Pixel::GetPos() { return position; }

const Surface* Cubemap::GetPlanes() const {
	return &posx;
}

void Cubemap::Render(Renderer& renderer, const Mat4& view, const Mat4& projection) {

	this->projection = &projection;
	this->view = &view;

	renderer.DrawElementArray<Vertex, Pixel, Cubemap>(this, NUM_TRIANGLES, INDICES, VERTICES);

}

Cubemap::Pixel Cubemap::VertexShader(Vertex& vertex) {

	Vec4 pos = *projection * *view * transform * vertex.position;
	return { pos, vertex.position };

}
Vec4 Cubemap::PixelShader(Pixel& pixel, const Renderer::Sampler<Pixel>& sampler) {

	// this scheme came from LearnOpenGL.com
	return sampler.SampleCubeMap(GetPlanes(), pixel.texDirection.x, pixel.texDirection.y, pixel.texDirection.z);
 }
