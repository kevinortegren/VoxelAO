#pragma once

#include <d3d11.h>
#include <vector>
#include "Vertex.h"
#include "Texture.h"
#include <map>
#include <string>


struct Material
{
	Texture* diffuse;
	Texture* clipMask;
};

struct MeshGroup 
{
	//The index where the draw command should start
	unsigned startIndex;
	//Number of indices that will be rendered
	unsigned numIndices;

	//Texture handles
	std::string material;
};

class Mesh
{
public:
	Mesh(std::vector<Vertex>* vertexList, std::vector<unsigned>* indexList, std::vector<MeshGroup> pmeshGroups, std::map<std::string, Material> pmaterialMap);
	~Mesh();

	void Apply();

	void DrawIndexed();
	void Draw();

private:
	ID3D11Buffer* vertexBuffer;
	ID3D11Buffer* indexBuffer;

	unsigned numVertices;
	unsigned numIndices;
	unsigned offset;
	unsigned stride;

	std::vector<MeshGroup> meshGroups;
	std::map<std::string, Material> materialMap;
};