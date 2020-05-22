#pragma once
#include <assimp/Importer.hpp>
#include "Vec3.h"
#include "Vec2.h"
#include <string>

// WRAPPER FOR THE ASSIMP LIBRARY

// assimp structs used by these classes
class aiMesh;
class aiMaterial;
class aiScene;

class Scene;
class Mesh;

class Mesh {

	friend class Scene;

private:

	const aiMesh* ai_mesh;

	Mesh(const aiMesh* ai_mesh);

public:

	int NumTriangles() const;
	int Indices(int index) const;

	int NumVertices() const;

	Vec3 Positions(int index) const;

	bool HasNormals() const;
	Vec3 Normals(int index) const;

	bool HasTextureCoords() const;
	Vec2 TextureCoords(int index) const;

	bool HasColors() const;
	Vec3 Colors(int index) const;

	bool HasTangentsAndBitangents() const;
	Vec3 Tangents(int index) const;
	Vec3 Bitangents(int index) const;

	int MaterialID() const;

};

class Material {

	friend class Scene;

private:
	const aiMaterial* ai_material;
	const std::string sceneDir;

	Material(const aiMaterial* ai_material, const std::string& sceneDir);

public:

	Vec3 Diffuse() const;
	Vec3 Ambient() const;
	Vec3 Specular() const;
	Vec3 Emissive() const;

	float SpecularExponent() const;

	std::string AmbientTexture() const;
	std::string DiffuseTexture() const;
	std::string SpecularTexture() const;
	std::string EmissiveTexture() const;
	std::string HeightMap() const;
	std::string NormalMap() const;
	std::string GlossMap() const;

};

class Scene {

private:
	
	Assimp::Importer ai_importer;

	const aiScene* ai_scene;
	const std::string directory;

public:

	Scene(const std::string& filepath);

	int NumMeshes() const;
	Mesh MeshAt(int meshIndex) const;

	int NumMaterials() const;
	Material MaterialAt(int materialIndex) const;

};
