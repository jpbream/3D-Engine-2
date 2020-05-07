#include "Engine.h"
#include <iostream>
#include "Renderer.h"
#include "Mat4.h"
#include "Manager.h"

Mat4 rotation = Mat4::GetRotation(0, 0, .000001);
Mat4 translation = Mat4::Get3DTranslation(0, 0, -4);
Mat4 scale = Mat4::GetScale(5, 5, 5);
Mat4 projection;
Mat4 view;
float camXRot = 0;
float camYRot = 0;

Surface texture("texture.png");

Surface posX("cube/posx.jpg");
Surface negX("cube/negx.jpg");
Surface posY("cube/posy.jpg");
Surface negY("cube/negy.jpg");
Surface posZ("cube/posz.jpg");
Surface negZ("cube/negz.jpg");

class TestVertex {

public:
	Vec4 position;
	Vec2 texel;

	TestVertex(const Vec4& position, const Vec2& texel) : position(position), texel(texel) {}

};

class TestPixel : public Renderer::PixelShaderInput {

public:
	Vec4 position;
	Vec2 texel;

	TestPixel() {}
	TestPixel(const Vec4& v, const Vec2& t) : position(v), texel(t) {}

	Vec4& GetPos() override {
		return position;
	}

};

TestPixel TestVertexShader(TestVertex& vertex) {

	Vec4 thing = projection * view * translation * rotation * scale * vertex.position;
	return { thing, vertex.texel };

}

Vec4 TestPixelShader(TestPixel& pixel, const Renderer::Sampler<TestPixel>& sampler2d) {

	//return texture.GetPixel(pixel.texel.x * (texture.GetWidth() - 1), pixel.texel.y * (texture.GetHeight() - 1));
	return sampler2d.SampleTex2D(texture, 5);

}

class CubeMapVertex {
public:
	Vec4 position;
	CubeMapVertex(const Vec4& p) : position(p) {}
};

class CubeMapPixel : public Renderer::PixelShaderInput {
	
public:
	Vec4 position;
	Vec4 texel;
	CubeMapPixel() {}
	CubeMapPixel(const Vec4& p, const Vec4& t) : position(p), texel(t) {}

	Vec4& GetPos() override {
		return position;
	}

};

CubeMapPixel CubeMapVertexShader(CubeMapVertex& vertex) {
	
	Vec4 pos = projection * view * scale * vertex.position;
	return { pos, vertex.position };
}
Vec4 CubeMapPixelShader(CubeMapPixel& pixel, const Renderer::Sampler<CubeMapPixel>& sampler) {
	return sampler.SampleCubeMap(posX, negX, posY, negY, posZ, negZ, pixel.texel.x, pixel.texel.y, pixel.texel.z);
}

int indices[] = { 0, 1, 2, 0, 2, 3 };
TestVertex vertices[] = { {Vec4(-.5, -.5, -.5, 1), Vec2(0, 1)}, {Vec4(.5, -.5, -.5, 1), Vec2(1, 1)}, {Vec4(.5, .5, -.5, 1), Vec2(1, 0)} , {Vec4(-.5, .5, -.5, 1), Vec2(0, 0)} };

int cubeMapIndices[] = {

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
CubeMapVertex cubeMapVerts[] = { {Vec4(-.5, -.5, -.5, 1)}, {Vec4(.5, -.5, -.5, 1)}, {Vec4(.5, .5, -.5, 1)} , {Vec4(-.5, .5, -.5, 1)},
							   {Vec4(-.5, -.5, .5, 1)}, {Vec4(.5, -.5, .5, 1)}, {Vec4(.5, .5, .5, 1)} , {Vec4(-.5, .5, .5, 1)} };

bool RenderLogic(Surface& backBuffer, Renderer& renderer, float deltaTime) {


	float r = 0.1;

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	if (keyboard[SDL_SCANCODE_Q]) {
		r -= .8 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_E]) {
		r += .8 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_W]) {
		translation(2, 3) -= .5 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_S]) {
		translation(2, 3) += .5 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_A]) {
		translation(0, 3) -= .5 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_D]) {
		translation(0, 3) += .5 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_R]) {
		translation(1, 3) -= .5 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_F]) {
		translation(1, 3) += .5 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_I]) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	if (keyboard[SDL_SCANCODE_O]) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	rotation = Mat4::GetRotation(0, r, 0);
		
	//renderer.DrawElementArray<TestVertex, TestPixel>(2, indices, vertices, TestVertexShader, TestPixelShader);
	renderer.DrawElementArray<CubeMapVertex, CubeMapPixel>(12, cubeMapIndices, cubeMapVerts, CubeMapVertexShader, CubeMapPixelShader);
		

	SDL_Event event = {};
	while (SDL_PollEvent(&event)) {

		switch (event.type) {

		case SDL_QUIT:
			return true;
			break;

		case SDL_MOUSEMOTION:

			if ((event.motion.xrel != 0 || event.motion.yrel != 0) && SDL_GetRelativeMouseMode() == SDL_TRUE) {

				camXRot -= event.motion.yrel * PI / 720;
				camYRot -= event.motion.xrel * PI / 720;

				view = Mat4::GetRotation(camXRot, camYRot, 0);
				view = view.GetInverse();

			}

			break;
		}
			
	}

}


int main(int argc, char* argv[]) {

	Instance::Init();

	texture.GenerateMipMaps();
	texture.Tint({ 1, 0, 0, 1 }, 0.2);
	
	Window window("My Window", 100, 20, 1920, 1080, 0);

	projection = Mat4::GetPerspectiveProjection(1, 100, -1, 1, (float)window.GetHeight() / window.GetWidth(), -(float)window.GetHeight() / window.GetWidth());

	StartDoubleBufferedInstance(window, RenderLogic, RF_BILINEAR | RF_MIPMAP | RF_TRILINEAR);

	//projection = Mat4::GetPerspectiveProjection(1, 100, -1, 1, 2160.0 / 3840, -2160.0 / 3840);
	//Surface s("images/Features.jpg");
	//Renderer r(s);
	//r.DrawElementArray<CubeMapVertex, CubeMapPixel>(12, cubeMapIndices, cubeMapVerts, CubeMapVertexShader, CubeMapPixelShader);

	//s.SetContrast(1);
	//s.Resize(1920, 1080, true);
	//s.Rescale(0.1, 0.1);
	//s.Invert();
	//s.GaussianBlur(2, 1, Surface::BLUR_BOTH);
	//s.SaveToFile("images/blur.bmp");
	//window.DrawSurface(s);
	//window.BlockUntilQuit();

	return 0;
}