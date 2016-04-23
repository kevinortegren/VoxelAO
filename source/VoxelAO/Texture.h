#pragma once

#include <SDL.h>
#include <D3D11.h>
#include "Types.h"

class Texture
{
public:
	Texture();
	~Texture();

	void CreateTextureFromFile(const char* file);
	void PSSetSRV(uint32 slot);
	void CreateRenderTarget(uint32 width, uint32 height, DXGI_FORMAT format);

	ID3D11ShaderResourceView* GetRSV();

private:
	ID3D11ShaderResourceView* textureRSV;

};