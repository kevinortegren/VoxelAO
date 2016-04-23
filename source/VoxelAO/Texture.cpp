#include "Texture.h"
#include <iostream>
#include <DDSTextureLoader.h>

extern ID3D11Device* device;
extern ID3D11DeviceContext* context;

#define CHECKDX(x) if (x != S_OK) std::cerr << "DirectX has returned an ERROR on line: " << __LINE__ << " in: " << __FUNCTION__ << std::endl;

Texture::Texture()
{
	textureRSV = nullptr;
}

Texture::~Texture()
{
	if (textureRSV)
		textureRSV->Release();
}

void Texture::CreateTextureFromFile(const char* file)
{
	wchar_t wstring[150];

	MultiByteToWideChar(CP_ACP, 0, file, -1, wstring, 150);

	CHECKDX(DirectX::CreateDDSTextureFromFile(device, wstring, nullptr, &textureRSV, 0, nullptr));
}

void Texture::PSSetSRV(uint32 slot)
{
	context->PSSetShaderResources(slot, 1, &textureRSV);
}
