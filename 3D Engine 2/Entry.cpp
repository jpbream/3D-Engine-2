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
#include "Utility.h"

Window* pWindow = nullptr;

float fov = 90;

Mat4 rotation2 = Mat4::GetRotation(0, 0, 0);
Mat4 translation2 = Mat4::Get3DTranslation(0, -5, 0);
Mat4 scale2 = Mat4::GetScale(15, 1, 15);

Frustum projFrustum;
Mat4 projection;
Mat4 view;

Vec3 cameraPos(0, 6, 20);
Vec3 cameraRot(-PI / 2, 0, 0);

//DirectionalLight dl({ 1, 0, 0 }, lightRot, 2048, 2048);
SpotLight sl({ 1, 1, 1 }, { 0, 0, 25}, { 0, 0, 0 }, 1.0f, 0.00f, 0.0f, 6, 2048, 2048);

Cow cow({0, 0, 15}, {0, PI / 4, 0}, {1, 1, 1});

Surface texture("images/norm.png");

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
	Vec3 tangent;
	Vec3 bitangent;

	float padding = 0;

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
	Vec3 toCam;
	Vec3 toLight;

	float padding[3];

	TestPixel() {}
	TestPixel(const Vec4& v, const Vec3& normal) : position(v), normal(normal) {}

	Vec4& GetPos() override {
		return position;
	}

};

Vec3 lightD;
TestPixel TestVertexShader(TestVertex& vertex) {

	Vec4 worldPos = translation2 * rotation2 * scale2 * vertex.position;
	Vec4 thing = projection * view * worldPos;
	Vec3 norm = rotation2.Truncate() * vertex.normal;

	TestPixel tp(thing, norm);
	tp.worldPos = Vec3(worldPos.x, worldPos.y, worldPos.z);
	tp.texel = vertex.texel;

	Mat4 mObject = translation2 * rotation2 * scale2;

	Vec4 s = Mat4::Viewport * sl.WorldToShadowMatrix() * mObject * vertex.position;
	tp.shadow = { s.s, s.t, s.p };

	// get the to cam and to light and lightDirection vectors in OBJECT space
	tp.toCam = ((translation2 * rotation2 * scale2).GetInverse() * (cameraPos.Vec4() - worldPos)).Vec3();
	tp.toLight = ((translation2 * rotation2 * scale2).GetInverse() * (sl.GetPosition().Vec4() - worldPos)).Vec3();
	//tp.toLight = (rotation2.GetInverse() * sl.GetPosition().Vec4()).Vec3();

	lightD = (rotation2.GetInverse() * sl.GetDirection().Vec4()).Vec3();

	// then transform them to tangent space
	Mat3 objToTan(vertex.tangent, vertex.bitangent, vertex.normal);
	objToTan = objToTan.GetTranspose();

	tp.toCam = objToTan * tp.toCam;
	tp.toLight = objToTan * tp.toLight;

	lightD = objToTan * lightD;
	lightD = lightD.Normalized();

	return tp;

}

Vec4 TestPixelShader(TestPixel& pixel, const Renderer::Sampler<TestPixel>& sampler2d) {
	
	float fracInShadow = sl.MultiSampleShadowMap(pixel.shadow, 5);
	
	Vec4 normSample = sampler2d.SampleTex2D(texture, FLOAT_OFFSET(pixel, texel));
	normSample *= 2;
	normSample -= Vec4(1, 1, 1, 1);

	Vec3 lightCol = sl.GetColorAt(pixel.worldPos);

	// how much the surface faces the light
	float facingFactor = Light::FacingFactor(lightD, normSample.Vec3());

	pixel.toCam = pixel.toCam.Normalized();
	pixel.toLight = pixel.toLight.Normalized();

	// spec factor is how much to scale the specular color by
	float specFactor = Light::SpecularFactor(pixel.toLight, normSample.Vec3(), pixel.toCam, 35);

	if ( fracInShadow > 0 )
		specFactor = 0;


	// the colors based on the materials surface properties
	// diffuse, specular (specular color will be the light color)
	Vec3 nonLightCol = Vec3(1, 1, 1) * facingFactor + sl.GetColor() * specFactor;

	// final non ambient color is the color of the light modulated with
	// the colors not contributed by the light
	Vec3 nonAmbientColor = Vec3::Modulate(
		lightCol,
		nonLightCol
	);

	// darken area if it is in shadow
	nonAmbientColor.r -= fracInShadow * 0.3f;
	nonAmbientColor.g -= fracInShadow * 0.3f;
	nonAmbientColor.b -= fracInShadow * 0.3f;

	// ambient and emmissive color would be added to this
	Vec3 finalColor = nonAmbientColor;

	finalColor.Clamp();
	return finalColor.Vec4();

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

	if ( keyboard[SDL_SCANCODE_Q] ) {
		cow.rotation.y += 0.1;
		rotation2 = Mat4::GetRotation(-.2, 0, 0) * rotation2;
	}
	if ( keyboard[SDL_SCANCODE_E] ) {
		cow.rotation.y -= 0.1;
		rotation2 = Mat4::GetRotation(.2, 0, 0) * rotation2;
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
	
	sl.UpdateShadowBox(projFrustum, camToWorld);

	cow.AddToShadowMap(sl);

	cow.Render(renderer, projection, view, sl, cameraPos);
	//cb.Render(renderer, Mat4::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z).GetInverse(), projection);
	
	renderer.DrawElementArray<TestVertex, TestPixel>(2, terrainIndices, terrainVerts, TestVertexShader, TestPixelShader);

	sl.ClearShadowMap();

	
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

		case SDL_KEYDOWN:

			if ( event.key.keysym.scancode == SDL_SCANCODE_J ) {
				commandQueue.Enqueue(C_TOGGLE_FLAG);
				commandQueue.Enqueue(RF_WIREFRAME);
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

	/*TestVertex v1;
	TestVertex v2;

	double start = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
	for ( int i = 0; i < 10000000; ++i ) {
		TestVertex v3 = Lerp(v1, v2, 0.5);
	}
	double end = (double)SDL_GetPerformanceCounter() / SDL_GetPerformanceFrequency();
	std::cout << (end - start) << std::endl;
	return 0;*/

	SDL_Init(SDL_INIT_VIDEO);

	texture.GenerateMipMaps();
	
	Instance::Init();

	CalculateNormals(2, terrainIndices, 4, terrainVerts, FLOAT_OFFSET(terrainVerts[0], position), FLOAT_OFFSET(terrainVerts[0], normal));
	CalculateTangentsAndBitangents(
		2, terrainIndices, 4, terrainVerts,
		FLOAT_OFFSET(terrainVerts[0], position),
		FLOAT_OFFSET(terrainVerts[0], texel),
		FLOAT_OFFSET(terrainVerts[0], normal),
		FLOAT_OFFSET(terrainVerts[0], tangent),
		FLOAT_OFFSET(terrainVerts[0], bitangent)
	);

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