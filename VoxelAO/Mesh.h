#pragma once

#include <d3d11.h>
#include <vector>
#include "Vertex.h"
#include "Texture.h"
#include <map>
#include <string>
#include "Types.h"

struct VertexIndexData
{
	std::vector<Vertex> vertexData;
	std::vector<uint32> indexData;
};

struct Material
{
	Texture* diffuse;
};

struct MeshGroup
{
	//The index where the draw command should start
	UINT startIndex;
	//Number of indices that will be rendered
	UINT numIndices;

	//Texture handles
	char material[64];
};

class Mesh
{
public:
	Mesh(VertexIndexData* vert_ind_data, std::vector<MeshGroup> pmeshGroups, std::map<std::string, Material> pmaterialMap);
	~Mesh();

	void Apply();

	void DrawIndexed();
	void Draw();

private:
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	uint32 numVertices;
	uint32 numIndices;
	uint32 offset;
	uint32 stride;

	std::vector<MeshGroup> meshGroups;
	std::map<std::string, Material> materialMap;
};