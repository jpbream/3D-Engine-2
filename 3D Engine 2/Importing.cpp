#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Importing.h"


/////// MESH ////////

Mesh::Mesh(const aiMesh* ai_mesh)
	:
	ai_mesh(ai_mesh)
{
}

int Mesh::NumTriangles() const
{
	return ai_mesh->mNumFaces;
}

int Mesh::Indices(int index) const
{
	return ai_mesh->mFaces[index / 3].mIndices[index % 3];
}

int Mesh::NumVertices() const
{
	return ai_mesh->mNumVertices;
}

Vec3 Mesh::Positions(int index) const
{
	aiVector3D vec = ai_mesh->mVertices[index];
	return { vec.x, vec.y, vec.z };
}

bool Mesh::HasNormals() const
{
	return ai_mesh->HasNormals();
}

Vec3 Mesh::Normals(int index) const
{
	aiVector3D vec = ai_mesh->mNormals[index];
	return { vec.x, vec.y, vec.z };
}

bool Mesh::HasTextureCoords() const
{
	return ai_mesh->HasTextureCoords(0);
}

Vec2 Mesh::TextureCoords(int index) const
{
	aiVector3D vec = ai_mesh->mTextureCoords[0][index];
	return { vec.x, 1 - vec.y };
}

bool Mesh::HasColors() const
{
	return ai_mesh->HasVertexColors(0);
}

Vec3 Mesh::Colors(int index) const
{
	aiColor4D vec = ai_mesh->mColors[0][index];
	return { vec.r, vec.g, vec.b };
}

bool Mesh::HasTangentsAndBitangents() const
{
	return ai_mesh->HasTangentsAndBitangents();
}

Vec3 Mesh::Tangents(int index) const
{
	aiVector3D vec = ai_mesh->mTangents[index];
	return { vec.x, vec.y, vec.z };
}

Vec3 Mesh::Bitangents(int index) const
{
	aiVector3D vec = ai_mesh->mBitangents[index];
	return { vec.x, vec.y, vec.z };
}

int Mesh::MaterialID() const
{
	return ai_mesh->mMaterialIndex;
}


/////// MATERIAL ////////

Material::Material(const aiMaterial* ai_material, const std::string& sceneDir)
	:
	ai_material(ai_material), sceneDir(sceneDir)
{
}

Vec3 Material::Diffuse() const
{
	aiColor3D color;
	ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, color);

	return { color.r, color.g, color.b };
}

Vec3 Material::Ambient() const
{
	aiColor3D color;
	ai_material->Get(AI_MATKEY_COLOR_AMBIENT, color);

	return { color.r, color.g, color.b };
}

Vec3 Material::Specular() const
{
	aiColor3D color;
	ai_material->Get(AI_MATKEY_COLOR_SPECULAR, color);

	return { color.r, color.g, color.b };
}

Vec3 Material::Emissive() const
{
	aiColor3D color;
	ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, color);

	return { color.r, color.g, color.b };
}

float Material::SpecularExponent() const
{
	float exponent;
	ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, exponent);

	return exponent;
}

std::string Material::AmbientTexture() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_AMBIENT, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::DiffuseTexture() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::SpecularTexture() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_SPECULAR, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::EmissiveTexture() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_EMISSIVE, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::HeightMap() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_HEIGHT, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::NormalMap() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}

std::string Material::GlossMap() const
{
	aiString texPath;
	ai_material->GetTexture(aiTextureType::aiTextureType_SHININESS, 0, &texPath);

	return (sceneDir + texPath.C_Str());
}



/////// SCENE ////////

Scene::Scene(const std::string& filepath)
	:
	directory(filepath.substr(0, filepath.find_last_of("/") + 1))
{
	ai_scene = ai_importer.ReadFile(filepath, aiProcess_Triangulate);
}

int Scene::NumMeshes() const
{
	return ai_scene->mNumMeshes;
}

Mesh Scene::MeshAt(int meshIndex) const
{
	return { ai_scene->mMeshes[meshIndex] };
}

int Scene::NumMaterials() const
{
	return ai_scene->mNumMaterials;
}

Material Scene::MaterialAt(int materialIndex) const
{
	const aiMaterial* material = ai_scene->mMaterials[materialIndex];

	return { material, directory };
}
