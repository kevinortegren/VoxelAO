#include "RenderTarget.h"
#include <iostream>

extern ID3D11Device* device;
extern ID3D11DeviceContext* context;

RenderTarget::RenderTarget()
{
	renderTargetView = nullptr;
	renderSRV = nullptr;
}

RenderTarget::~RenderTarget()
{
	renderTargetView->Release();
	renderSRV->Release();
}

void RenderTarget::CreateRenderTarget(unsigned width, unsigned height, DXGI_FORMAT format)
{
	HRESULT result;

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	D3D11_RENDER_TARGET_VIEW_DESC rtdesc;
	ZeroMemory(&rtdesc, sizeof(rtdesc));
	rtdesc.Format = format;
	rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ID3D11Texture2D* testTex;
	result = device->CreateTexture2D(&desc, nullptr, &testTex);
	if (result != S_OK)
		std::cerr << "CreateTexture2D Failed!" << std::endl;

	result = device->CreateRenderTargetView(testTex, &rtdesc, &renderTargetView);
	if (result != S_OK)
		std::cerr << "CreateRenderTargetView Failed!" << std::endl;

	result = device->CreateShaderResourceView(testTex, nullptr, &renderSRV);
	if (result != S_OK)
		std::cerr << "CreateShaderResourceView Failed!" << std::endl;

	testTex->Release();
}

ID3D11RenderTargetView* RenderTarget::GetRenderTargetView()
{
	return renderTargetView;
}

void RenderTarget::Clear()
{
	float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(renderTargetView, clearColor);
}

void RenderTarget::PSSetSRV(unsigned slot)
{
	context->PSSetShaderResources(slot, 1, &renderSRV);
}

