#include "Engine.h"
#include <iostream>
#include "Renderer.h"
#include "Mat4.h"
#include "Manager.h"
#include "Cubemap.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Mat4 rotation = Mat4::GetRotation(0, 0, .000001);
Mat4 translation = Mat4::Get3DTranslation(0, 0, -10);
Mat4 scale = Mat4::GetScale(1, 1, 1);
Mat4 projection;
Mat4 view;
float camXRot = 0;
float camYRot = 0;

Surface texture("texture.png");

Cubemap cb("cube/posx.jpg", 
	"cube/negx.jpg", 
	"cube/posy.jpg", 
	"cube/negy.jpg", 
	"cube/posz.jpg", 
	"cube/negz.jpg");

class TestVertex {

public:
	Vec4 position;
	Vec3 normal;

	TestVertex() {}
	TestVertex(const Vec4& position, const Vec3& normal) : position(position), normal(normal) {}

};

class TestPixel : public Renderer::PixelShaderInput {

public:
	Vec4 position;
	Vec3 normal;
	Vec3 worldPos;

	TestPixel() {}
	TestPixel(const Vec4& v, const Vec3& normal) : position(v), normal(normal) {}

	Vec4& GetPos() override {
		return position;
	}

};
TestPixel TestVertexShader(TestVertex& vertex) {

	Vec4 worldPos = view * translation * rotation * scale * vertex.position;
	Vec4 thing = projection * worldPos;
	Vec3 norm = rotation.Truncate() * vertex.normal;

	TestPixel tp(thing, norm);
	tp.worldPos = Vec3(worldPos.x, worldPos.y, worldPos.z);

	return tp;

}

Vec4 TestPixelShader(TestPixel& pixel, const Renderer::Sampler<TestPixel>& sampler2d) {

	//return texture.GetPixel(pixel.texel.x * (texture.GetWidth() - 1), pixel.texel.y * (texture.GetHeight() - 1));
	//return sampler2d.SampleTex2D(texture, 5);

	//float dot = (pixel.normal * Vec3(0, 0, 1));
	//if (dot < 0)
		//dot = 0;
	
	
	//return { 0, dot, 0, 1 };
	Vec3 toCam = Vec3(0, 0, 0) - pixel.worldPos;
	Vec3 reflect = toCam.Reflect(pixel.normal);

	return sampler2d.SampleCubeMap(cb.GetPlanes(), reflect.x, reflect.y, reflect.z);

}

int numBoxIndices = 0;
int* boxIndices;
TestVertex* boxVerts;

float r = 0.1;

bool RenderLogic(Renderer& renderer, float deltaTime) {

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	if (keyboard[SDL_SCANCODE_Q]) {
		r -= .8 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_E]) {
		r += .8 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_W]) {
		translation(2, 3) -= 1 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_S]) {
		translation(2, 3) += 1 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_A]) {
		translation(0, 3) -= 1 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_D]) {
		translation(0, 3) += 1 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_R]) {
		translation(1, 3) -= 1 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_F]) {
		translation(1, 3) += 1 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_I]) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	if (keyboard[SDL_SCANCODE_O]) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	rotation = Mat4::GetRotation(0, r, 0);

	renderer.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, TestVertexShader, TestPixelShader);
	cb.Render(renderer, view, projection);
	

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

void PostProcess(Surface& frontBuffer) {

	//frontBuffer.GaussianBlur(10, 3, Surface::BLUR_BOTH);
	//frontBuffer.Invert();
	//frontBuffer.SetContrast(0.3);

}


int main(int argc, char* argv[]) {

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("models/OBJ/Cow2.obj", aiProcess_Triangulate);

	const aiMesh* mesh = scene->mMeshes[0];

	numBoxIndices = mesh->mNumFaces * 3;
	boxIndices = new int[ numBoxIndices ];

	for (int i = 0; i < mesh->mNumFaces; ++i) {

		boxIndices[3 * i] = mesh->mFaces[i].mIndices[0];
		boxIndices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
		boxIndices[3 * i + 2] = mesh->mFaces[i].mIndices[2];

	}

	boxVerts = new TestVertex[mesh->mNumVertices];
	for (int i = 0; i < mesh->mNumVertices; ++i) {

		Vec4 pos; 
		pos.x = mesh->mVertices[i].x;
		pos.y = mesh->mVertices[i].y;
		pos.z = mesh->mVertices[i].z;
		pos.w = 1;

		Vec3 norm;
		norm.x = mesh->mNormals[i].x;
		norm.y = mesh->mNormals[i].y;
		norm.z = mesh->mNormals[i].z;

		boxVerts[i] = { pos, norm };

	}
	
	Instance::Init();

	//texture.GenerateMipMaps();
	//texture.Tint({ 1, 0, 0, 1 }, 0.2);
	
	Window window("My Window", 20, 20, 1000, 1000, 0);

	projection = Mat4::GetPerspectiveProjection(1, 100, -1, 1, (float)window.GetHeight() / window.GetWidth(), -(float)window.GetHeight() / window.GetWidth());

	StartDoubleBufferedInstance(window, RenderLogic, PostProcess, RF_BILINEAR | RF_MIPMAP | RF_TRILINEAR | RF_BACKFACE_CULL);

	

	//Surface s(window.GetWidth(), window.GetHeight());
	//Renderer r(s);
	//r.SetFlags(RF_BACKFACE_CULL);
	//r.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, TestVertexShader, TestPixelShader);
	//cb.Render(r, view, projection);
	////r.DrawElementArray<CubeMapVertex, CubeMapPixel>(12, cubeMapIndices, cubeMapVerts, CubeMapVertexShader, CubeMapPixelShader);

	//window.DrawSurface(s);
	//window.BlockUntilQuit();


	delete[] boxIndices;
	delete[] boxVerts;

	return 0;
}