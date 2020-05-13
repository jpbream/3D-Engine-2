#include "Engine.h"
#include <iostream>
#include "Renderer.h"
#include "Mat4.h"
#include "Manager.h"
#include "Cubemap.h"
#include "Shapes.h"
#include "Frustum.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Mat4 rotation = Mat4::GetRotation(0, PI / 4, 0);
Mat4 rotation2 = Mat4::GetRotation(0, 0, 0);
Mat4 translation = Mat4::Get3DTranslation(0, -2.2, -25);
Mat4 translation2 = Mat4::Get3DTranslation(0, -5, 0);
Mat4 scale = Mat4::GetScale(1, 1, 1);
Mat4 scale2 = Mat4::GetScale(100, 1, 100);

Frustum projFrustum;
Mat4 projection;
Mat4 view;

Mat4 lightView;

Vec3 cameraPos(0, 6, 0);
Vec3 cameraRot(-PI / 6, 0, 0);

Mat4 mViewport = { {.5, 0, 0, 0}, {0, -.5, 0, 0}, {0, 0, .5, 0}, {.5, .5, .5, 1} };

Frustum boundingBox;
Mat4 orthographic;
Renderer::DepthBuffer shadowMap;

//Vec3 lightDir(0, 0, -1);
Vec3 lightRot(-PI * 2 / 2.2, 0, 0);

Surface texture("images/grass.jpg");

Cubemap cb("cube/posx.jpg", 
	"cube/negx.jpg", 
	"cube/posy.jpg", 
	"cube/negy.jpg", 
	"cube/posz.jpg", 
	"cube/negz.jpg");

int* indices;
Vec4* vertices;
int numI;
int numV;

Frustum GetFrustumBoundingBox(const Frustum& frustum, const Mat4& boxSpaceViewMatrix, const Mat4& camToWorldMatrix) {

	Vec4 frustumPoints[8];

	frustumPoints[0] = {frustum.left, frustum.bottom, -frustum.near, 1};
	frustumPoints[1] = {frustum.right, frustum.bottom, -frustum.near, 1};
	frustumPoints[2] = { frustum.right, frustum.top, -frustum.near, 1 };
	frustumPoints[3] = { frustum.left, frustum.top, -frustum.near, 1 };

	float farFactor = (frustum.near + frustum.far) / frustum.near;

	frustumPoints[4] = { frustum.left * farFactor, frustum.bottom * farFactor, -frustum.far, 1 };
	frustumPoints[5] = { frustum.right * farFactor, frustum.bottom * farFactor, -frustum.far, 1 };
	frustumPoints[6] = { frustum.right * farFactor, frustum.top * farFactor, -frustum.far, 1 };
	frustumPoints[7] = { frustum.left * farFactor, frustum.top * farFactor, -frustum.far, 1 };

	// convert to the box space
	for (int i = 0; i < 8; ++i) {
		frustumPoints[i] = boxSpaceViewMatrix * camToWorldMatrix * frustumPoints[i];
	}

	float maxX = frustumPoints[0].x; float minX = frustumPoints[0].x;
	float maxY = frustumPoints[0].y; float minY = frustumPoints[0].y;
	float maxZ = frustumPoints[0].z; float minZ = frustumPoints[0].z;

	for (int i = 1; i < 8; ++i) {

		if (frustumPoints[i].x > maxX)
			maxX = frustumPoints[i].x;
		if (frustumPoints[i].x < minX)
			minX = frustumPoints[i].x;

		if (frustumPoints[i].y > maxY)
			maxY = frustumPoints[i].y;
		if (frustumPoints[i].y < minY)
			minY = frustumPoints[i].y;

		if (frustumPoints[i].z > maxZ)
			maxZ = frustumPoints[i].z;
		if (frustumPoints[i].z < minZ)
			minZ = frustumPoints[i].z;

	}

	Frustum f;
	f.near = -maxZ - 10;
	f.far = -minZ;
	f.left = minX;
	f.right = maxX;
	f.top = maxY;
	f.bottom = minY;

	return f;
}

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

	Vec4 s = mViewport * orthographic * lightView  * mObject * vertex.position;
	tp.shadow = { s.s, s.t, s.p };

	return tp;

}

TestPixel TestVertexShader2(TestVertex& vertex) {

	Vec4 worldPos = translation * rotation * scale * vertex.position;
	Vec4 thing = projection * view * worldPos;
	Vec3 norm = rotation2.Truncate() * vertex.normal;

	TestPixel tp(thing, norm);
	tp.worldPos = Vec3(worldPos.x, worldPos.y, worldPos.z);
	tp.texel = vertex.texel * 25;

	Mat4 mObject = translation2 * rotation2 * scale2;

	Vec4 s = mViewport * orthographic * lightView * mObject * vertex.position;
	tp.shadow = { s.s, s.t, s.p };

	return tp;

}

Vec4 TestPixelShader(TestPixel& pixel, const Renderer::Sampler<TestPixel>& sampler2d) {
	
	float dot = -(Mat3::GetRotation(lightRot.r, lightRot.g, lightRot.b) * Vec3(0, 0, -1)) * pixel.normal.Normalized();
	if (dot < 0)
		dot = 0;
	
	//if (pixel.shadow.s >= 0 && pixel.shadow.s < 1 && pixel.shadow.t >= 0 && pixel.shadow.t < 1)
	int x = pixel.shadow.s * shadowMap.GetWidth();
	int y = pixel.shadow.t * shadowMap.GetHeight();
	float fracInShadow = 0;
	for (int i = x - 1; i < x + 1; ++i) {
		for (int j = y - 1; j < y + 1; ++j) {

			if (i >= 0 && i < shadowMap.GetWidth() && j >= 0 && j < shadowMap.GetHeight()) {

				float sample = shadowMap.GetPixel(i, j);

				if (pixel.shadow.p > sample + 0.001)
					fracInShadow += 1.0 / 9;

			}

		}
	}
	
	//else {
		//return { 1, 0, 0, 1 };
	

	Vec4 col = { dot, dot, dot, 1 };

	col *= (1 - fracInShadow * 0.5);


	return col;
	
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
SphereVertex* sv;

SpherePixel SphereVertexShader(SphereVertex& v) {

	Vec4 worldPos = translation * rotation * scale * v.position;
	Vec4 thing = projection * view * worldPos;
	Vec3 norm =  rotation.Truncate() * Vec3(v.position.x, v.position.y, v.position.z);

	SpherePixel tp(thing, norm);
	tp.worldPos = Vec3(worldPos.x, worldPos.y, worldPos.z);

	Mat4 mObject = translation * rotation * scale;
	Vec4 s = mViewport * orthographic * lightView * mObject * v.position;

	tp.shadow = { s.s, s.t, s.p };

	return tp;
}

Vec4 SpherePixelShader(SpherePixel& sp, const Renderer::Sampler<SpherePixel>& samp) {

	float dot = -(Mat3::GetRotation(lightRot.r, lightRot.g, lightRot.b) * Vec3(0, 0, -1)) * sp.normal.Normalized();
	if (dot < 0)
		dot = 0;

	float compDepth = 1e99;
	//if (sp.shadow.s >= 0 && sp.shadow.s < 1 && sp.shadow.t >= 0 && sp.shadow.t < 1)
		compDepth = shadowMap.GetPixel(sp.shadow.s * (shadowMap.GetWidth()), (sp.shadow.t * shadowMap.GetHeight()));
	//else
		//return { 1, 0, 0, 1 };
	
	Vec4 col = { dot, dot, dot, 1 };

	if (sp.shadow.p > compDepth + 0.1)
		col *= 0.9;

	return col;

	//Vec3 reflect = toCam.Reflect(sp.normal);
	//Vec3 reflect = toCam.Refract(sp.normal, 1, 1.33);

	//Vec4 col = samp.SampleCubeMap(cb.GetPlanes(), reflect.x, reflect.y, reflect.z);

	//return { col.r, col.g * 0.5f, col.b * 0.5f, 1 };
}

TestPixel ShadowVertexShader(TestVertex& v) {

	Mat4 mObject = translation * rotation * scale;
	Vec4 pos = orthographic * lightView * mObject * v.position;
	
	return {pos , {0, 0, 0} };

}

Vec4 ShadowPixelShader(TestPixel& sp, const Renderer::Sampler<TestPixel>& samp) {

	return { 0, 0, 0, 0 };

}

int numBoxIndices = 0;
int* boxIndices;
TestVertex* boxVerts;

float r = 0.1;

TestVertex terrainVerts[4] = { 
	{{-1, 0, -1, 1}, {0, 1, 0}, {0, 0}},
{{1, 0, -1, 1}, {0, 1, 0}, {1, 0}},
{{1, 0, 1, 1}, {0, 1, 0}, {1, 1}},
{{-1, 0, 1, 1}, {0, 1, 0}, {0, 1}}
};
int terrainIndices[6] = {3, 2, 1, 3, 1, 0};

bool RenderLogic(Renderer& renderer, float deltaTime) {

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	if (keyboard[SDL_SCANCODE_Q]) {
		//rotation2 = rotation2 * Mat4::GetRotation(0.01, 0, 0);
		lightRot.r += deltaTime;
	}
	if (keyboard[SDL_SCANCODE_E]) {
		//rotation2 = rotation2 * Mat4::GetRotation(-0.01, 0, 0);
		lightRot.r -= deltaTime;
	}
	if (keyboard[SDL_SCANCODE_W]) {
		
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraPos += cameraDir * deltaTime * 2;

	}
	if (keyboard[SDL_SCANCODE_S]) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraPos -= cameraDir * deltaTime * 2;
	}

	if (keyboard[SDL_SCANCODE_A]) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraDir = Mat3::GetRotation(0, PI / 2, 0) * cameraDir;
		cameraPos += cameraDir * deltaTime * 2;
	}
	if (keyboard[SDL_SCANCODE_D]) {
		Vec3 cameraDir = Mat3::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z) * Vec3(0, 0, -1);
		cameraDir.y = 0;
		cameraDir = Mat3::GetRotation(0, PI / 2, 0) * cameraDir;
		cameraPos -= cameraDir * deltaTime * 2;
	}

	if (keyboard[SDL_SCANCODE_SPACE]) {

		cameraPos.y += 2 * deltaTime;
	}
	if (keyboard[SDL_SCANCODE_LSHIFT]) {

		cameraPos.y -= 2 * deltaTime;
	}

	if (keyboard[SDL_SCANCODE_R]) {
		translation(1, 3) -= deltaTime;
	}
	if (keyboard[SDL_SCANCODE_F]) {
		translation(1, 3) += deltaTime;
	}

	if (keyboard[SDL_SCANCODE_I]) {
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	if (keyboard[SDL_SCANCODE_O]) {
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	//rotation = Mat4::GetRotation(r, 0, 0);

	view = (Mat4::Get3DTranslation(cameraPos.x, cameraPos.y, cameraPos.z) * Mat4::GetRotation(cameraRot.x, cameraRot.y, cameraRot.z)).GetInverse();

	lightView = Mat4::GetRotation(lightRot.x, lightRot.y, lightRot.z).GetInverse();

	Mat4 camToWorld = view.GetInverse();
	Frustum f = GetFrustumBoundingBox(projFrustum, lightView, camToWorld);

	orthographic = Mat4::GetOrthographicProjection(f.near, f.far, f.left, f.right, f.top, f.bottom);

	//renderer.SetFlags(RF_WIREFRAME);
	//renderer.ClearFlags(RF_BACKFACE_CULL);

	//renderer.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, TestVertexShader, TestPixelShader);
	renderer.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, ShadowVertexShader, ShadowPixelShader);
	
	//renderer.DrawElementArray<SphereVertex, SpherePixel>(numI, indices, sv, ShadowVertexShader, ShadowPixelShader);

	shadowMap = renderer.GetDepthBuffer();
	//shadowMap.SaveToFile("images/shadowmap.bmp");
	//exit(0);
	renderer.ClearDepthBuffer();

	//renderer.DrawElementArray<SphereVertex, SpherePixel>(numI, indices, sv, SphereVertexShader, SpherePixelShader);
	renderer.DrawElementArray<TestVertex, TestPixel>(numBoxIndices / 3, boxIndices, boxVerts, TestVertexShader2, TestPixelShader);
	renderer.DrawElementArray<TestVertex, TestPixel>(2, terrainIndices, terrainVerts, TestVertexShader, TestPixelShader);
	//cb.Render(renderer, view, projection);

	SDL_Event event = {};
	while (SDL_PollEvent(&event)) {

		switch (event.type) {

		case SDL_QUIT:
			return true;
			break;

		case SDL_MOUSEMOTION:

			if ((event.motion.xrel != 0 || event.motion.yrel != 0) && SDL_GetRelativeMouseMode() == SDL_TRUE) {

				cameraRot.x -= event.motion.yrel * 3.0f / 720;
				cameraRot.y -= event.motion.xrel * 3.0f / 720;

			}

			break;
		}
			
	}
	return false;

}

void PostProcess(Surface& frontBuffer) {

	//frontBuffer.GaussianBlur(10, 3, Surface::BLUR_BOTH);
	//frontBuffer.Invert();
	//frontBuffer.SetContrast(0.3);

	int numKeys;
	const Uint8* keyboard = SDL_GetKeyboardState(&numKeys);

	if (keyboard[SDL_SCANCODE_P]) {
		frontBuffer.SaveToFile("images/Screenshot.bmp");
	}

}


int main(int argc, char* argv[]) {

	texture.GenerateMipMaps();

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile("models/OBJ/Cow2.obj", aiProcess_Triangulate);

	const aiMesh* mesh = scene->mMeshes[0];

	numBoxIndices = mesh->mNumFaces * 3;
	boxIndices = new int[ numBoxIndices ];

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {

		boxIndices[3 * i] = mesh->mFaces[i].mIndices[0];
		boxIndices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
		boxIndices[3 * i + 2] = mesh->mFaces[i].mIndices[2];

	}

	boxVerts = new TestVertex[mesh->mNumVertices];
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {

		Vec4 pos; 
		pos.x = mesh->mVertices[i].x;
		pos.y = mesh->mVertices[i].y;
		pos.z = mesh->mVertices[i].z;
		pos.w = 1;

		Vec3 norm;
		norm.x = mesh->mNormals[i].x;
		norm.y = mesh->mNormals[i].y;
		norm.z = mesh->mNormals[i].z;

		boxVerts[i] = { pos, norm, {0, 0} };

	}
	
	Instance::Init();

	//texture.GenerateMipMaps();
	//texture.Tint({ 1, 0, 0, 1 }, 0.2);
	
	// 2560, 1440
	Window window("My Window", 20, 20, 750, 750, 0);

	projection = Mat4::GetPerspectiveProjection(1, 100, -1, 1, (float)window.GetHeight() / window.GetWidth(), -(float)window.GetHeight() / window.GetWidth());
	projFrustum.near = 1;
	projFrustum.far = 100;
	projFrustum.left = -1;
	projFrustum.right = 1;
	projFrustum.top = (float)window.GetHeight() / window.GetWidth();
	projFrustum.bottom = -(float)window.GetHeight() / window.GetWidth();

	Shapes::MakeSphere(20, 2, &numI, &numV, &indices, &vertices);
	sv = new SphereVertex[numV];
	for (int i = 0; i < numV; ++i) {
		sv[i].position = vertices[i];
	}

	StartDoubleBufferedInstance(window, RenderLogic, PostProcess,  RF_MIPMAP | RF_TRILINEAR | RF_BACKFACE_CULL);

	

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

	delete[] indices;
	delete[] vertices;
	delete[] sv;

	delete[] boxIndices;
	delete[] boxVerts;

	return 0;
}