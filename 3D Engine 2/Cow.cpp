#include "Cow.h"
#include "Mat3.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define MODEL 0
#define MVP 1
#define NORM 2
#define SHADOW 3

#define CAMERA 0

#define SHADOW_SAMPLE 1

const Cow* Cow::boundObject = nullptr;
Mat4 Cow::boundMatrices[10];
Vec3 Cow::boundVectors[5];
const SpotLight* Cow::boundLight = nullptr;

Cow::CowPixel Cow::MainVertexShader(CowVertex& vertex)
{
	// homogeneous clip space position
	Vec4 hcs = boundMatrices[MVP] * vertex.position;

	// rotate the surface normal
	Vec3 norm = boundMatrices[NORM].Truncate() * vertex.normal;

	CowPixel tp;
	tp.position = hcs;
	tp.normal = norm;
	tp.worldPos = (boundMatrices[MODEL] * vertex.position).Vec3();
	tp.texel = vertex.texel;

	// the shadow map coordinate in viewport space
	Vec4 s = Mat4::Viewport * boundMatrices[SHADOW] * boundMatrices[MODEL] * vertex.position;
	tp.shadow = { s.s, s.t, s.p };

	return tp;
}

Vec4 Cow::MainPixelShader(CowPixel& pixel, const Renderer::Sampler<CowPixel>& sampler)
{
	// fraction of the pixels that lie in shadow
	float fracInShadow = boundLight->MultiSampleShadowMap(pixel.shadow, SHADOW_SAMPLE);

	// color of the light
	Vec3 lightCol = boundLight->GetColorAt(pixel.worldPos);

	// how much the surface faces the light
	float facingFactor = boundLight->FacingFactor(pixel.normal.Normalized());

	Vec3 normal = pixel.normal.Normalized();
	Vec3 toCamera = (boundVectors[CAMERA] - pixel.worldPos).Normalized();
	Vec3 toLight = (boundLight->GetPosition() - pixel.worldPos).Normalized();

	// specular reflection model described in
	// "Mathematics for 3D Game Programming and Computer Graphics" by Eric Lengyel
	// Section 7.4

	// vector halfway between to-viewer and to-light vector
	Vec3 halfway = (toLight + toCamera).Normalized();

	// spec factor is how much to scale the specular color by
	float specFactor = normal * halfway;
	if ( specFactor < 0 )
		specFactor = 0;
	else
		specFactor = powf(specFactor, 15);

	specFactor = normal * toLight > 0 ? specFactor : 0;

	// the colors based on the materials surface properties
	// diffuse, specular (specular color will be the light color)
	Vec3 nonLightCol = boundObject->diffuseColor * facingFactor + boundLight->GetColor() * specFactor;

	// final non ambient color is the color of the light modulated with
	// the colors not contributed by the light
	Vec3 nonAmbientColor = Vec3::Modulate(
		lightCol,
		nonLightCol
	);

	// darken area if it is in shadow
	nonAmbientColor.r -= fracInShadow * 0.1f;
	nonAmbientColor.g -= fracInShadow * 0.1f;
	nonAmbientColor.b -= fracInShadow * 0.1f;

	// ambient and emmissive color would be added to this
	Vec3 finalColor = nonAmbientColor;

	finalColor.Clamp();
	return finalColor.Vec4();
}

Cow::CowPixel Cow::ShadowVertexShader(CowVertex& vertex)
{
	// shadow coordinate in light space
	Vec4 pos = boundMatrices[SHADOW] * boundMatrices[MODEL] * vertex.position;

	CowPixel p;
	p.position = pos;

	return p;
}

Vec4 Cow::ShadowPixelShader(CowPixel& pixel, const Renderer::Sampler<CowPixel>& sampler)
{
	return { 0, 0, 0, 0 };
}

Cow::Cow(const Vec3& position, const Vec3& rotation, const Vec3& scale)
	:
	position(position), rotation(rotation), scale(scale)
{

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(FILE, aiProcess_Triangulate);

	const aiMesh* mesh = scene->mMeshes[0];

	nTriangles = mesh->mNumFaces;
	pIndices = new int[nTriangles * 3];

	// read indices
	for ( unsigned int i = 0; i < mesh->mNumFaces; ++i ) {

		pIndices[3 * i] = mesh->mFaces[i].mIndices[0];
		pIndices[3 * i + 1] = mesh->mFaces[i].mIndices[1];
		pIndices[3 * i + 2] = mesh->mFaces[i].mIndices[2];

	}

	// read vertices
	pVertices = new CowVertex[mesh->mNumVertices];
	for ( unsigned int i = 0; i < mesh->mNumVertices; ++i ) {

		Vec4 pos;
		pos.x = mesh->mVertices[i].x;
		pos.y = mesh->mVertices[i].y;
		pos.z = mesh->mVertices[i].z;
		pos.w = 1;

		Vec3 norm;
		norm.x = mesh->mNormals[i].x;
		norm.y = mesh->mNormals[i].y;
		norm.z = mesh->mNormals[i].z;

		pVertices[i] = { pos, norm, {0, 0} };
	}

}

Cow::~Cow()
{
	delete[] pIndices;
	delete[] pVertices;
}

void Cow::AddToShadowMap(SpotLight& light)
{
	boundObject = this;
	boundLight = &light;

	boundMatrices[MODEL] = Mat4::Get3DTranslation(position.x, position.y, position.z) *
		Mat4::GetRotation(rotation.x, rotation.y, rotation.z) *
		Mat4::GetScale(scale.x, scale.y, scale.z);

	boundMatrices[SHADOW] = light.WorldToShadowMatrix();

	light.DrawToShadowMap<CowVertex, CowPixel>(nTriangles, pIndices, pVertices, ShadowVertexShader, ShadowPixelShader);

	boundObject = nullptr;
	boundLight = nullptr;
}

void Cow::Render(Renderer& renderer, const Mat4& proj, const Mat4& view, const SpotLight& light, const Vec3& cameraPos)
{
	boundObject = this;
	boundLight = &light;

	boundMatrices[MODEL] = Mat4::Get3DTranslation(position.x, position.y, position.z) *
		Mat4::GetRotation(rotation.x, rotation.y, rotation.z) *
		Mat4::GetScale(scale.x, scale.y, scale.z);

	boundMatrices[MVP] = proj * view * boundMatrices[MODEL];

	boundMatrices[NORM] = Mat4::GetRotation(rotation.x, rotation.y, rotation.z);

	boundMatrices[SHADOW] = light.WorldToShadowMatrix();

	boundVectors[CAMERA] = cameraPos;

	renderer.DrawElementArray<CowVertex, CowPixel>(nTriangles, pIndices, pVertices, MainVertexShader, MainPixelShader);

	boundObject = nullptr;
	boundLight = nullptr;
}

Cow::CowVertex::CowVertex()
{
}

Cow::CowVertex::CowVertex(const Vec4& position, const Vec3& normal, const Vec2& texel)
	:
	position(position), normal(normal), texel(texel)
{
}

Cow::CowPixel::CowPixel()
{
}

Vec4& Cow::CowPixel::GetPos()
{
	return position;
}