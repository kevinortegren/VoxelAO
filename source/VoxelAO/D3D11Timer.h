#pragma once
#include <d3d11.h>

class D3D11Timer
{
	ID3D11Query*	mDisjoint;
	ID3D11Query*	mStart;
	ID3D11Query*	mStop;

	ID3D11Device*			mDevice;
	ID3D11DeviceContext*	mDeviceContext;

public:
	explicit D3D11Timer(ID3D11Device* d3dDevice, ID3D11DeviceContext* d3dDeviceContext);
	~D3D11Timer();

	void Start();
	void Stop();

	double GetTime();
};
 