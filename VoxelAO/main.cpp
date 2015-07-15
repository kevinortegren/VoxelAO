#define _USE_MATH_DEFINES
#include <iostream>
#include <SDL.h>
#include <SDL_syswm.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <SimpleMath.h>
#include "Camera.h"
#include "Vertex.h"
#include "OBJLoader.h"
#include <SDL_image.h>
#include "RenderTarget.h"
#include "Effects.h"
#include "PrimitiveBatch.h"
#include <GeometricPrimitive.h>
#include "VertexTypes.h"
#include <string>
#include <array>
#include "D3D11Timer.h"
#include <cmath>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define CHECKDX(x) if (x != S_OK) std::cerr << "DirectX has returned an ERROR on line: " << __LINE__ << " in: " << __FUNCTION__ << std::endl;

const int WINDOW_WIDTH = 1536;
const int WINDOW_HEIGHT = 768;
const float FARZ = 3000.0f;
const float NEARZ = 0.5f;

//DX11
IDXGISwapChain* swapChain;
ID3D11Device* device;
ID3D11DeviceContext* context;
ID3D11RenderTargetView* backBuffer;
ID3D11DepthStencilView* depthStencilView;

ID3D11RasterizerState* RSStateSolid;
ID3D11RasterizerState* RSStateWireframe;

//Shaders
ID3D11VertexShader* VS;
ID3D11PixelShader* PS;

ID3D11VertexShader* fsqVS;
ID3D11PixelShader* fsqPS;

ID3D11Buffer* camConstBuffer;

ID3D11ShaderResourceView* depthBufferView;

ID3D11InputLayout *layout;
ID3D11InputLayout *fsqlayout;

ID3D11SamplerState* linearSamplerState;

RenderTarget colorRT;
RenderTarget normalRT;
RenderTarget positionRT;

Camera camera;

Mesh* mesh;

SDL_Window* window;

//DXTK
DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* primitiveBatch;
DirectX::BasicEffect* basicEffect;
ID3D11InputLayout* basicInputLayout;

D3D11Timer* timer;



#undef main
bool HandleEvents();
void InitD3D(HWND winHandle);

int main(int argc, char* argv[])
{
	bool running = true;

	//Setup SDL window
	SDL_SysWMinfo info;

	HWND handle;
	
	if (SDL_Init(SDL_INIT_TIMER) != 0)
	{
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
	}

	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);

	window = SDL_CreateWindow("Clustered Prototype", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

	//Must init info struct with SDL version info, see documentation for explanation
	SDL_VERSION(&info.version);

	//If succeeded, init d3d11 and other things
	if (SDL_GetWindowWMInfo(window, &info))
	{
		handle = info.info.win.window;
	}
	else
	{
		std::cerr << "Failed to get WMInfo: " << SDL_GetError() << std::endl;
		running = false;
	}
	
	InitD3D(handle);

	timer = new D3D11Timer(device, context);

	{
		ID3DBlob* vsblob,* psblob;
		UINT flags = 0;

#ifndef _DEBUG // IF NOT DEBUG
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif 
	
		//Create shaders
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/simple.vertex", nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsblob, nullptr));

		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/simple.pixel", nullptr, nullptr, "main", "ps_5_0", flags, 0, &psblob, nullptr));

		CHECKDX(device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), nullptr, &VS));
		CHECKDX(device->CreatePixelShader(psblob->GetBufferPointer(), psblob->GetBufferSize(), nullptr, &PS));

		D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		CHECKDX(device->CreateInputLayout(ied, 3, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &layout));

		//Full screen quad shader
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/fsq.vertex", nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsblob, nullptr));
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/fsq.pixel", nullptr, nullptr, "main", "ps_5_0", flags, 0, &psblob, nullptr));

		CHECKDX(device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), nullptr, &fsqVS));
		CHECKDX(device->CreatePixelShader(psblob->GetBufferPointer(), psblob->GetBufferSize(), nullptr, &fsqPS));

		D3D11_INPUT_ELEMENT_DESC iedfsq[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		CHECKDX(device->CreateInputLayout(iedfsq, 1, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &fsqlayout));
	}

	{
		OBJLoader objLoader;

		mesh = objLoader.LoadOBJ("../Assets/Models/sponza.obj");
	}

	//Camera 
	camera.SetLens((float)M_PI_4, NEARZ, FARZ, WINDOW_WIDTH, WINDOW_HEIGHT);

	std::unique_ptr<DirectX::GeometricPrimitive> shape(DirectX::GeometricPrimitive::CreateSphere(context, 10.0f, 8));

	{
		//Init primitive spritebatch from DirectXTK
		primitiveBatch = new DirectX::PrimitiveBatch<DirectX::VertexPositionColor>(context);
		basicEffect = new DirectX::BasicEffect(device);
	
		basicEffect->SetProjection(XMLoadFloat4x4(&camera.GetCamData().projMat));

		basicEffect->SetVertexColorEnabled(true);

		void const* shaderByteCode;
		size_t byteCodeLength;

		basicEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		CHECKDX(device->CreateInputLayout(DirectX::VertexPositionColor::InputElements,
			DirectX::VertexPositionColor::InputElementCount,
			shaderByteCode, byteCodeLength,
			&basicInputLayout));


		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(CameraData);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		CHECKDX(device->CreateBuffer(&cbDesc, nullptr, &camConstBuffer));

		//Set up sampler state
		D3D11_SAMPLER_DESC samplerDesc;
		ZeroMemory(&samplerDesc, sizeof(samplerDesc));
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		CHECKDX(device->CreateSamplerState(&samplerDesc, &linearSamplerState));

		//Rasterizer state
		D3D11_RASTERIZER_DESC RSdesc;
		ZeroMemory(&RSdesc, sizeof(RSdesc));
		RSdesc.FillMode = D3D11_FILL_SOLID;
		RSdesc.CullMode = D3D11_CULL_NONE;
		RSdesc.DepthClipEnable = TRUE;

		CHECKDX(device->CreateRasterizerState(&RSdesc, &RSStateSolid));

		RSdesc.FillMode = D3D11_FILL_WIREFRAME;
		CHECKDX(device->CreateRasterizerState(&RSdesc, &RSStateWireframe));
	}

	//Set states, these are static states for the moment
	context->RSSetState(RSStateSolid);
	
	//RenderTarget clear color
	float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };

	//Timer
	uint32_t oldTime, currentTime;
	float dt;

	currentTime = SDL_GetTicks();

	while (running)
	{
		oldTime = currentTime;
		currentTime = SDL_GetTicks();
		dt = (currentTime - oldTime) / 1000.0f;

		running = HandleEvents();

		const Uint8* keystate = SDL_GetKeyboardState(NULL);

		//Check keys and move camera
		if (keystate[SDL_SCANCODE_A])
			camera.StrafeLeft(dt);
		if (keystate[SDL_SCANCODE_D])
			camera.StrafeRight(dt);
		if (keystate[SDL_SCANCODE_W])
			camera.MoveForward(dt);
		if (keystate[SDL_SCANCODE_S])
			camera.MoveBackward(dt);
		if (keystate[SDL_SCANCODE_LSHIFT])
			camera.MoveDown(dt);
		if (keystate[SDL_SCANCODE_SPACE])
			camera.MoveUp(dt);
			
		camera.Update();

		D3D11_MAPPED_SUBRESOURCE camResource;
		CHECKDX(context->Map(camConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &camResource));
		memcpy(camResource.pData, &camera.GetCamData(), sizeof(CameraData));
		context->Unmap(camConstBuffer, 0);


		//Clear RTs before drawing
		colorRT.Clear();
		normalRT.Clear();
		positionRT.Clear();
		context->ClearRenderTargetView(backBuffer, clearColor);
		context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//Set Gbuffer RTs
		ID3D11RenderTargetView* RTs[] = {colorRT.GetRenderTargetView(), normalRT.GetRenderTargetView(), positionRT.GetRenderTargetView()};
		ID3D11RenderTargetView* RTzero[] = { 0, 0, 0 };
		context->OMSetRenderTargets(3, RTs, depthStencilView);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//Set shaders
		context->VSSetShader(VS, 0, 0);
		context->PSSetShader(PS, 0, 0);
		context->IASetInputLayout(layout);
		context->PSSetSamplers(0, 1, &linearSamplerState);
		context->VSSetConstantBuffers(0, 1, &camConstBuffer);
		context->RSSetState(RSStateSolid);
		
		timer->Start();
		//Draw geometry
		mesh->Apply();
		mesh->DrawIndexed();

		context->OMSetRenderTargets(3, RTzero, 0);

		LARGE_INTEGER frequency;
		if (::QueryPerformanceFrequency(&frequency) == FALSE)
			throw "foo";

		LARGE_INTEGER start;
		if (::QueryPerformanceCounter(&start) == FALSE)
			throw "foo";


		LARGE_INTEGER end;
		if (::QueryPerformanceCounter(&end) == FALSE)
			throw "foo";

		double interval = (static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart) * 1000.0;

		context->OMSetRenderTargets(1, &backBuffer, 0);
		//Draw to backbuffer
		context->VSSetShader(fsqVS, 0, 0);
		context->PSSetShader(fsqPS, 0, 0);
		context->IASetInputLayout(fsqlayout);
		colorRT.PSSetSRV(0);
		normalRT.PSSetSRV(1);
		positionRT.PSSetSRV(2);
		context->PSSetShaderResources(3, 1, &depthBufferView);

		
		context->DrawInstanced(3, 1, 0, 0);
		timer->Stop();

		double gpuTime = timer->GetTime();

		std::string outstr = "LA: " + std::to_string(interval) + "Shading time: " + std::to_string(gpuTime);
		SDL_SetWindowTitle(window, outstr.c_str());

		////////DRAW LINES//////
		//Set depth
		context->OMSetRenderTargets(1, &backBuffer, depthStencilView);

		//basicEffect->SetView(XMLoadFloat4x4(&camera.GetCamData().viewMat));
		//basicEffect->Apply(context);
		//context->IASetInputLayout(basicInputLayout);
		
		swapChain->Present(0, 0);

		ID3D11ShaderResourceView* SRVzero[] = { 0 };
		context->PSSetShaderResources(0, 1, SRVzero);
		context->PSSetShaderResources(1, 1, SRVzero);
		context->PSSetShaderResources(2, 1, SRVzero);
		context->PSSetShaderResources(3, 1, SRVzero);
		context->PSSetShaderResources(4, 1, SRVzero);
		context->PSSetShaderResources(5, 1, SRVzero);
		context->PSSetShaderResources(6, 1, SRVzero);
	}

	std::cout << "Exiting program!" << std::endl;

	delete mesh;
	delete primitiveBatch;
	delete basicEffect;
	delete timer;

	depthStencilView->Release();
	RSStateSolid->Release();
	RSStateWireframe->Release();
	fsqVS->Release();
	fsqPS->Release();
	camConstBuffer->Release();
	depthBufferView->Release();
	layout->Release();
	fsqlayout->Release();
	linearSamplerState->Release();
	VS->Release();
	PS->Release();
	swapChain->Release();
	backBuffer->Release();
	device->Release();
	context->Release();
	basicInputLayout->Release();
	SDL_DestroyWindow(window);

	return 0;
}

bool HandleEvents()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_QUIT:
				return false;
			case SDL_KEYDOWN:
			{
				switch (e.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					return false;
				case SDLK_h:
				{
					camera.camSpeed = 300.0f;

				}break;
				case SDLK_j:
				{
					camera.camSpeed = 0.1f;

				}break;
				}
			} break;
			//Move camera when left mouse button is pressed and mouse is moving!
			case SDL_MOUSEMOTION:
			{
				if (e.motion.state & SDL_BUTTON_LMASK)
				{
					float pitch = (float)e.motion.yrel / 200.0f;
					float yaw = (float)e.motion.xrel / 200.0f;

					camera.Pitch(pitch);
					camera.Yaw(-yaw);
				}
			} break;
		}
	}

	return true;
}

void InitD3D(HWND winHandle)
{
	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = winHandle;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Windowed = true;

	UINT flags = 0;

#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	CHECKDX(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, &swapDesc, &swapChain, &device, nullptr, &context));

	ID3D11Texture2D* pBackBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	backBuffer = nullptr;
	CHECKDX(device->CreateRenderTargetView(pBackBuffer, nullptr, &backBuffer));
	pBackBuffer->Release();

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = WINDOW_WIDTH;
	depthStencilDesc.Height = WINDOW_HEIGHT;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	dsv_desc.Flags = 0;
	dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
	dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Texture2D.MipSlice = 0;

	D3D11_SHADER_RESOURCE_VIEW_DESC sr_desc;
	sr_desc.Format = DXGI_FORMAT_R32_FLOAT;
	sr_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sr_desc.Texture2D.MostDetailedMip = 0;
	sr_desc.Texture2D.MipLevels = -1;

	ID3D11Texture2D* DSB;
	CHECKDX(device->CreateTexture2D(&depthStencilDesc, NULL, &DSB));
	CHECKDX(device->CreateDepthStencilView(DSB, &dsv_desc, &depthStencilView));
	CHECKDX(device->CreateShaderResourceView(DSB, &sr_desc, &depthBufferView));
	DSB->Release();

	//Create renderTagets for GBuffer
	colorRT.CreateRenderTarget(WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
	normalRT.CreateRenderTarget(WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R16G16B16A16_FLOAT);
	positionRT.CreateRenderTarget(WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R16G16B16A16_FLOAT);

	D3D11_VIEWPORT viewPort;
	ZeroMemory(&viewPort, sizeof(D3D11_VIEWPORT));

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;
	viewPort.Width = WINDOW_WIDTH;
	viewPort.Height = WINDOW_HEIGHT;

	context->RSSetViewports(1, &viewPort);
}