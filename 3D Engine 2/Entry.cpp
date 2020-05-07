#include "Engine.h"
#include <iostream>
#include "Renderer.h"
#include "Mat4.h"
#include <thread>

Mat4 rotation = Mat4::GetRotation(0, 0, .000001);
Mat4 translation = Mat4::Get3DTranslation(0, 0, -4);
Mat4 scale = Mat4::GetScale(5, 5, 5);
Mat4 projection;
Mat4 view;
float camXRot = 0;
float camYRot = 0;

//Surface texture("texture.png");
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

Surface* front = nullptr;
Surface* back = nullptr;

volatile bool shouldQuit = false;
volatile bool drawing = false;
volatile bool rendering = false;

void DrawLoop(Window& window) {

	while (!shouldQuit) {
		
		drawing = true;
		window.DrawSurface(*front);
		drawing = false;

		while (rendering);
	}
	std::cout << "Stopping Thread" << std::endl;
}

int main(int arc, char* argv[]) {

	//double testStart = SDL_GetPerformanceCounter();
	////

	//texture.GenerateMipMaps();
	//texture.Tint({ 1, 0, 0, 1 }, 0.2);

	////
	////std::cout << (SDL_GetPerformanceCounter() - testStart) / SDL_GetPerformanceFrequency() << " seconds" << std::endl;
	//return 0;
	
	
	//texture.RotateLeft();

	texture.GenerateMipMaps();
	texture.Tint({1, 0, 0, 1}, 0.2);
	unsigned int numThreads = std::thread::hardware_concurrency();
	std::cout << numThreads << " Threads" << std::endl;

	SDL_Log("Starting the Engine");
	Instance::Init();

	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);

	Window window("My Window", 1000, 20, 750, 750, 0);
	Surface surface1(window.GetWidth(), window.GetHeight());
	Surface surface2(window.GetWidth(), window.GetHeight());

	projection = Mat4::GetPerspectiveProjection(1, 100, -1, 1, (float)window.GetHeight() / window.GetWidth(), -(float)window.GetHeight() / window.GetWidth());

	int mouseX = 0;
	int mouseY = 0;

	int relX = 0;
	int relY = 0;

	Vec3 red(1, 0, 0);

	front = &surface1;
	back = &surface2;

	Renderer renderer(*back);
	renderer.ClearFlags(RF_BACKFACE_CULL);

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
	
	float r = 0.1;

	std::thread drawLoop(DrawLoop, std::ref(window));

	float deltaTime = 0;

	while (!shouldQuit) {

		static int frames = 0;
		static double totalTime = 0;
		Uint64 start = SDL_GetPerformanceCounter();

		static double totalRenderTime = 0;

		rendering = true;
		Uint64 renderStart = SDL_GetPerformanceCounter();

		SDL_GetMouseState(&mouseX, &mouseY);

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

		

		//r = 3.14159 / 3;
		//translation(2, 3) = -.2;
		//r += deltaTime;
		rotation = Mat4::GetRotation(0, r, 0);
		renderer.SetRenderTarget(*back);
		back->BlackOut();
		//renderer.DrawElementArray<TestVertex, TestPixel>(2, indices, vertices, TestVertexShader, TestPixelShader);
		renderer.DrawElementArray<CubeMapVertex, CubeMapPixel>(12, cubeMapIndices, cubeMapVerts, CubeMapVertexShader, CubeMapPixelShader);
		renderer.ClearZBuffer();
		back->Tint({1, 0, 0, 1}, 0.2);
		back->RotateRight();
		std::swap(front, back);
		

		Uint64 renderEnd = SDL_GetPerformanceCounter();

		totalRenderTime += (renderEnd - renderStart) / (double)SDL_GetPerformanceFrequency();

		rendering = false;
		while (drawing);

		Uint64 end = SDL_GetPerformanceCounter();
		static Uint64 interval = SDL_GetPerformanceFrequency();

		frames++;
		deltaTime = (end - start) / (double)interval;
		totalTime += deltaTime;

		if (totalTime > 1.0) {
			std::cout << "Frame Time: " << totalTime * 1000 / frames << " ms.   " << frames << " frames.   " << totalRenderTime * 1000 / frames << " ms render time.   (" << (int)(totalRenderTime * 100 / totalTime) << "%)" << std::endl;
			frames = 0;
			totalTime = 0;
			totalRenderTime = 0;
		}

		SDL_Event event = {};
		while (SDL_PollEvent(&event)) {

			switch (event.type) {

			case SDL_QUIT:
				shouldQuit = true;
				break;

			case SDL_MOUSEMOTION:
				relX = event.motion.xrel;
				relY = event.motion.yrel;

				if ((relX != 0 || relY != 0) && SDL_GetRelativeMouseMode() == SDL_TRUE) {

					camXRot -= relY / 2 * 3.14159 / 360;
					camYRot -= relX / 2 * 3.14159 / 360;

					view = Mat4::GetRotation(camXRot, camYRot, 0);
					view = view.GetInverse();

				}

				break;
			}
			


		}

	}

	drawLoop.join();

	Instance::Quit();
	return 0;
}