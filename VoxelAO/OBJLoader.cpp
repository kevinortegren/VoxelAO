#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <string>
#include <SimpleMath.h>

struct MatMap
{
	char mat[64];
	char tex[64];
};

OBJLoader::OBJLoader()
{

}

OBJLoader::~OBJLoader()
{

}

std::unique_ptr<Mesh> OBJLoader::LoadBIN(const char* file)
{
	VertexIndexData data;
	std::vector<MeshGroup> meshGroups;
	std::vector<MatMap> matMap;
	std::ifstream binFile;

	binFile.open(file, std::ifstream::in | std::ifstream::binary);

	if (binFile.is_open())
	{
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;

		binFile.read((char*)&vertexCount, sizeof(uint32_t));
		binFile.read((char*)&indexCount, sizeof(uint32_t));

		data.vertexData.resize(vertexCount);
		data.indexData.resize(indexCount);

		binFile.read((char*)&data.vertexData[0], vertexCount * sizeof(Vertex));
		binFile.read((char*)&data.indexData[0], indexCount * sizeof(uint32_t));

		uint32_t groupCount = 0;

		binFile.read((char*)&groupCount, sizeof(uint32_t));

		meshGroups.resize(groupCount);

		binFile.read((char*)&meshGroups[0], groupCount * sizeof(MeshGroup));

		//read materials
		uint32_t materialCount = 0;
		binFile.read((char*)&materialCount, sizeof(uint32_t));

		matMap.resize(materialCount);

		binFile.read((char*)&matMap[0], materialCount * sizeof(MatMap));

		binFile.close();
	}


	std::map<std::string, Material> materialMap;
	for (auto mm : matMap)
	{
		std::string textureName(mm.tex);
		std::string filePath = "../Assets/Models/" + textureName;

		Material mat;
		if (textureName == "")
		{
			mat.diffuse = nullptr;
		}
		else
		{
			mat.diffuse = new Texture();
			mat.diffuse->CreateTextureFromFile(filePath.c_str());
		}

		materialMap[mm.mat] = mat;
	}

	return  std::make_unique<Mesh>(&data, meshGroups, materialMap);
}
