#pragma once

#include <D3D11.h>
#include "Types.h"

class RenderTarget
{
public:
	RenderTarget();
	~RenderTarget();

	void CreateRenderTarget(uint32 width, uint32 height, DXGI_FORMAT format);
	ID3D11RenderTargetView* GetRenderTargetView();
	void PSSetSRV(uint32 slot);
	void Clear();
private:

	ID3D11RenderTargetView* renderTargetView;
	ID3D11ShaderResourceView* renderSRV;
};