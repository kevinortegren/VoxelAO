#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <D3D11.h>

class Texture
{
public:
	Texture();
	~Texture();

	void CreateTextureFromFile(const char* file);
	void PSSetSRV(unsigned slot);
	void CreateRenderTarget(unsigned width, unsigned height, DXGI_FORMAT format);

	ID3D11ShaderResourceView* GetRSV();

private:
	ID3D11ShaderResourceView* textureRSV;

};