#pragma once

#include <D3D11.h>

class RenderTarget
{
public:
	RenderTarget();
	~RenderTarget();

	void CreateRenderTarget(unsigned width, unsigned height, DXGI_FORMAT format);
	ID3D11RenderTargetView* GetRenderTargetView();
	void PSSetSRV(unsigned slot);
	void Clear();
private:

	ID3D11RenderTargetView* renderTargetView;
	ID3D11ShaderResourceView* renderSRV;
};