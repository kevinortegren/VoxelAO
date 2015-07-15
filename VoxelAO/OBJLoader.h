#pragma once

#include <vector>
#include <d3d11.h>
#include "Mesh.h"
#include "Texture.h"
#include <map>

class OBJLoader
{
public:
	OBJLoader();
	~OBJLoader();
	
	Mesh* LoadOBJ(const char* file);
private:
	void LoadMTL(std::map<std::string, Material>& materialMap, const char* file);
};