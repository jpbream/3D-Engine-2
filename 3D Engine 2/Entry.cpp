#include "Engine.h"
#include <iostream>
#include "Renderer.h"
#include "Mat4.h"
#include "Manager.h"
#include "Cubemap.h"
#include "Shapes.h"
#include "Light.h"
#include "Cow.h"
#include "Queue.h"

Window* pWindow = nullptr;

float fov = 90;

Mat4 rotation2 = Mat4::GetRotation(0, 0, 0);
Mat4 translation2 = Mat4::Get3DTranslation(0, -5, 0);
Mat4 scale2 = Mat4::GetScale(10, 1, 10);

Frustum projFrustum;
Mat4 projection;
Mat4 view;

Vec3 cameraPos(0, 6, 0);
Vec3 cameraRot(-PI / 2, 0, 0);

//Vec3 lightDir(0, 0, -1);
Vec3 lightRot(-PI * 2 / 2.2 + 2 * PI, 0, 0);
//DirectionalLight dl({ 1, 0, 0 }, lightRot, 2048, 2048);
SpotLight sl({ 1, 1, 1 }, { 0, 6, 0}, { 11 * PI / 6, 0, 0 }, 1.0f, 0.00f, 0.0f, 1, 2048, 2048);

Cow cow({0, -2.2, -5}, {0, PI / 4, 0}, {1, 1, 1});

Surface texture("images/grass.jpg");

Cubemap cb("cube/posx.jpg", 
	"cube/negx.jpg", 
	"cube/posy.jpg", 
	"cube/negy.jpg", 
	"cube/posz.jpg", 
	"cube/negz.jpg");

Sphere sphere(20, 2);

class TestVertex {

public:
	Vec4 position;
	Vec3 normal;
	Vec2 texel;

	TestVertex() {}
	TestVertex(const Vec4& position, const Vec3& normal, const Vec2& texel) : position(position), normal(normal), texel(texel) {}

};

class TestPixel : public Renderer::PixelShaderInput {

public:
	Vec4 position;
	Vec3 normal;
	Vec3 worldPos;
	Vec2 texel;
	Vec3 shadow;

	TestPixel() {}
	TestPixel(const Vec4& v, const Vec3& normal) : position(v), normal(normal) {}

	Vec4& GetPos() override {
		return position;
	}

};
TestPixel TestVertexShader(TestVertex& vertex) {

	Vec4 worldPos = translation2 * rotation2 * scale2 * vertex.position;
	Vec4 thing = projection * view * worldPos;
	Vec3 norm = rotation2.Truncate() * vertex.normal;

	TestPixel tp(thing, norm);
	tp.worldPos = Vec3(worldPos.x, worldPos.y, worldPos.z);
	tp.texel = vertex.texel * 25;

	Mat4 mObject = translation2 * rotation2 * scale2;

	Vec4 s = Mat4::Viewport * sl.WorldToShadowMatrix() * mObject * vertex.position;
	tp.shadow = { s.s, s.t, s.p };

	return tp;

}

Vec4 TestPixelShader(TestPixel& pixel, const Renderer::Sampler<TestPixel>& sampler2d) {
	
	float fracInShadow = sl.MultiSampleShadowMap(pixel.shadow, 1);
	

	//float fracInShadow = 0;
	//float compDepth = dl.SampleShadowMap(pixel.shadow.s, pixel.shadow.t);
	//if (pixel.shadow.p > compDepth + 0.01)
		//fracInShadow = 1;
	
	//Vec3 col = dl.GetColorAt(pixel.normal.Normalized());
	Vec3 col = sl.GetColorAt(pixel.worldPos);

	col.r -= fracInShadow * 0.1f;
	col.g -= fracInShadow * 0.1f;
	col.b -= fracInShadow * 0.1f;

	if (col.r < 0) col.r = 0;
	if (col.g < 0) col.g = 0;
	if (col.b < 0) col.b = 0;

	return { col.r, col.g, col.b, 1 };
	
	//Vec3 reflect = toCam.Reflect(pixel.normal);

	//Vec4 col = sampler2d.SampleTex2D(texture, 11);
	//return { col.r * dot, col.g * dot, col.b * dot, 1 };

}

class SphereVertex {

public:
	Vec4 position;
	SphereVertex() {}
	SphereVertex(const Vec4& pos) : position(pos) {}
};

class SpherePixel : public Renderer::PixelShaderInput {

public:
	Vec4 position;
	Vec3 normal;
	Vec3 worldPos;
	Vec3 shadow;
	SpherePixel() {}
	SpherePixel(const Vec4& v, const Vec3& normal) : position(v), normal(normal) {}

	Vec4& GetPos() { return position; }
};

float r = 0.1;

TestVertex terrainVerts[4] = { 
	{{-1, 0, -1, 1}, {0, 1, 0}, {0, 0}},
{{1, 0, -1, 1}, {0, 1, 0}, {1, 0}},
{{1, 0, 1, 1}, {0, 1, 0}, {1, 1}},
{{-1, 0, 1, 1}, {0, 1, 0}, {0, 1}}
};
int terrainIndices[6] = {3, 2, 1, 3, 1, 0};

bool RenderLogic(Renderer& renderer, float deltaTime) {

	projection = Mat4::GetPerspectiveProjection(1, 75, (float)pWindow->GetHeight() / pWindow->GetWidth(), fov, projFrustum);

	/*projection = Mat4::GetPerspectiveProjection(1, 75, -1, 1, (float)pWindow->GetHeight() / pWindow->GetWidth(), -(float)pWindow->GetHeight() / pWindow->GetWidth());
	projFrustum.near = 1;
	projFrustum.far = 75;
	projFrustum.left = -1;
	projFrustum.right = 1;
	projFrustum.top = (float)pWindow->GetHeight() / pWindow->GetWidth();
	projFrustum.bottom = -(float)pWindow->GetHeight() / pWindow->GetWidth();*/

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	cow.rotation.y += 1 * deltaTime;

	if ( keyboard[SDL_SCANCODE_Q] ) {
		cow.rotation.y += 0.1;
		lightRot.r += deltaTime;
	}
	if ( keyboard[SDL_SCANCODE_E] ) {
		cow.rotation.y -= 0.1;
		lightRot.r -= deltaTime;
	}
	if ( keyboard[SDL_SCANCODE_W] ) {

		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraPos += cameraDir * deltaTime * 2;

	}
	if ( keyboard[SDL_SCANCODE_S] ) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraPos -= cameraDir * deltaTime * 2;
	}

	if ( keyboard[SDL_SCANCODE_A] ) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraDir = Mat3::GetRotation(0, PI / 2, 0) * cameraDir;
		cameraPos += cameraDir * deltaTime * 2;
	}
	if ( keyboard[SDL_SCANCODE_D] ) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraDir = Mat3::GetRotation(0, PI / 2, 0) * cameraDir;
		cameraPos -= cameraDir * deltaTime * 2;
	}

	if ( keyboard[SDL_SCANCODE_SPACE] ) {

		cameraPos.y += 2 * deltaTime;
	}
	if ( keyboard[SDL_SCANCODE_LSHIFT] ) {

		cameraPos.y -= 2 * deltaTime;
	}

	if ( keyboard[SDL_SCANCODE_R] ) {
		fov += 1;
	}
	if ( keyboard[SDL_SCANCODE_F] ) {
		fov -= 1;
	}

	if ( keyboard[SDL_SCANCODE_I] ) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	if ( keyboard[SDL_SCANCODE_O] ) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	//rotation = Mat4::GetRotation(r, 0, 0);

	view = (Mat4::Get3DTranslation(cameraPos.x, cameraPos.y, cameraPos.z) * Mat4::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z)).GetInverse();

	Mat4 camToWorld = view.GetInverse();
	
	//sl.UpdateShadowBox(projFrustum, camToWorld);

	//cow.AddToShadowMap(sl);
	//cow.Render(renderer, projection, view, sl, cameraPos);
	cb.Render(renderer, Mat4::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z).GetInverse(), projection);
	
	//renderer.DrawElementArray<TestVertex, TestPixel>(2, terrainIndices, terrainVerts, TestVertexShader, TestPixelShader);

	//sl.ClearShadowMap();

	
	return false;

}

bool PostProcess(Surface& frontBuffer) {

	//frontBuffer.GaussianBlur(10, 3, Surface::BLUR_BOTH);
	//frontBuffer.Invert();
	//frontBuffer.SetContrast(0.3);

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	if (keyboard[SDL_SCANCODE_P]) {
		frontBuffer.SaveToFile("images/Screenshot.bmp");
	}

	return false;
}

bool EventHandler(Queue<int>& commandQueue)
{
	
	SDL_Event event = {};
	while ( SDL_PollEvent(&event) ) {

		switch ( event.type ) {

		case SDL_QUIT:
			return true;
			break;

		case SDL_MOUSEMOTION:

			if ( (event.motion.xrel != 0 || event.motion.yrel != 0) && SDL_GetRelativeMouseMode() == SDL_TRUE ) {

				cameraRot.x -= event.motion.yrel * 3.0f / 720;
				cameraRot.y -= event.motion.xrel * 3.0f / 720;

			}

			break;

		case SDL_WINDOWEVENT:
			switch ( event.window.event ) {

			case SDL_WINDOWEVENT_RESIZED:
				commandQueue.Enqueue(C_RESIZE);
				commandQueue.Enqueue(event.window.data1);
				commandQueue.Enqueue(event.window.data2);
				break;

			}
			
			break;
		}

	}
	return false;
}


int main(int argc, char* argv[]) {

	SDL_Init(SDL_INIT_VIDEO);

	texture.GenerateMipMaps();
	
	Instance::Init();

	//texture.GenerateMipMaps();
	//texture.Tint({ 1, 0, 0, 1 }, 0.2);

	// 2560, 1440
	Window window("My Window", 20, 20, 750, 750, SDL_WINDOW_RESIZABLE);
	pWindow = &window;

	projection = Mat4::GetPerspectiveProjection(1, 75, -1, 1, (float)window.GetHeight() / window.GetWidth(), -(float)window.GetHeight() / window.GetWidth());
	projFrustum.near = 1;
	projFrustum.far = 75;
	projFrustum.left = -1;
	projFrustum.right = 1;
	projFrustum.top = (float)window.GetHeight() / window.GetWidth();
	projFrustum.bottom = -(float)window.GetHeight() / window.GetWidth();

	StartDoubleBufferedInstance(window, EventHandler, RenderLogic, PostProcess, RF_MIPMAP | RF_TRILINEAR | RF_BACKFACE_CULL);

	

	//Surface s(window.GetWidth(), window.GetHeight());
	//Renderer r(s);
	//r.SetFlags(RF_BACKFACE_CULL);
	//r.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, TestVertexShader, TestPixelShader);
	//cb.Render(r, view, projection);
	////r.DrawElementArray<CubeMapVertex, CubeMapPixel>(12, cubeMapIndices, cubeMapVerts, CubeMapVertexShader, CubeMapPixelShader);

	//window.DrawSurface(s);
	//window.BlockUntilQuit();

	//window.DrawSurface(s);
	//window.BlockUntilQuit();

	return 0;
}