#pragma once

#include <vector>
#include <d3d11.h>
#include "Mesh.h"
#include "Texture.h"
#include <map>
#include <memory>

using namespace DirectX::SimpleMath;

class OBJLoader
{
public:
	OBJLoader();
	~OBJLoader();

	std::unique_ptr<Mesh> LoadBIN(const char* file);
};