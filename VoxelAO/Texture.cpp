#include "Texture.h"
#include <iostream>

extern ID3D11Device* device;
extern ID3D11DeviceContext* context;

Texture::Texture()
{
	textureRSV = nullptr;
}

Texture::~Texture()
{
	textureRSV->Release();
}

void Texture::CreateTextureFromFile(const char* file)
{
	HRESULT result;

	SDL_Surface *image;
	image = IMG_Load(file);
	if (!image)
	{
		std::cout << "IMG_Load: " << IMG_GetError() << std::endl;
	}

	char* dstPixels = new char[image->w *  image->h * 4];
	int res = SDL_ConvertPixels(image->w, image->h, image->format->format, image->pixels, image->pitch, SDL_PIXELFORMAT_ABGR8888, dstPixels, image->w * 4);

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image->w;
	desc.Height = image->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA texData;
	texData.pSysMem = dstPixels;
	texData.SysMemPitch = image->w * 4;
	texData.SysMemSlicePitch = 0;

	ID3D11Texture2D* testTex;
	result = device->CreateTexture2D(&desc, &texData, &testTex);
	if (result != S_OK)
		std::cerr << "CreateTexture2D Failed!" << std::endl;
	
	result = device->CreateShaderResourceView(testTex, 0, &textureRSV);
	if (result != S_OK)
		std::cerr << "CreateShaderResourceView Failed!" << std::endl;

	//Clean up
	delete[] dstPixels;
	testTex->Release();
	SDL_FreeSurface(image);
}

void Texture::PSSetSRV(unsigned slot)
{
	context->PSSetShaderResources(slot, 1, &textureRSV);
}
