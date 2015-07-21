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
#include <memory>


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")


//#define DEBUG_DRAW

#define CHECKDX(x) if (x != S_OK) std::cerr << "DirectX has returned an ERROR on line: " << __LINE__ << " in: " << __FUNCTION__ << std::endl;

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const float FARZ = 3000.0f;
const float NEARZ = 0.5f;
const int VOXELSIZE = 1024;
const float VOXEL_STRUCTURE_WIDTH = 1024.0f * 3;
const float VOXELWIDTH = VOXEL_STRUCTURE_WIDTH / (float)VOXELSIZE;
const int NUM_VOXELS = VOXELSIZE * VOXELSIZE * VOXELSIZE;
const int VOXEL_ELEMENTS = VOXELSIZE * VOXELSIZE * (VOXELSIZE / 32);

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

ID3D11VertexShader* voxelVS;
ID3D11GeometryShader* voxelGS;
ID3D11PixelShader* voxelPS;

ID3D11Buffer* camConstBuffer;
ID3D11Buffer* voxelStructure;
ID3D11Buffer* voxelStagingStructure;
ID3D11Buffer* voxelMatBuffer;
ID3D11Buffer* voxelClearBuffer;
ID3D11Buffer* voxelSampleKernel;

RenderTarget voxelVoidRT;

ID3D11BlendState* voxelBlendState;

ID3D11UnorderedAccessView* voxelUAV;

ID3D11ShaderResourceView* depthBufferView;
ID3D11ShaderResourceView* voxelSRV;

ID3D11InputLayout *layout;
ID3D11InputLayout *fsqlayout;
ID3D11InputLayout *voxelLayout;


ID3D11SamplerState* linearSamplerState;

RenderTarget colorRT;
RenderTarget normalRT;
RenderTarget positionRT;

Camera camera;

std::unique_ptr<Mesh> mesh;

SDL_Window* window;

//Voxel matrix
Matrix voxelMatrix;
Matrix voxelViewProjMatrix[3];

Vector4 sampleKernel[25];

uint32_t* voxelCPUstructure = new uint32_t[VOXEL_ELEMENTS];

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

	window = SDL_CreateWindow("VoxelAO", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

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
		ID3DBlob* vsblob,* psblob, *gsblob;
		ID3DBlob* errblob = nullptr;
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
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/fsq.pixel", nullptr, nullptr, "main", "ps_5_0", flags, 0, &psblob, &errblob));
		if (errblob)
			std::cout << (char*)errblob->GetBufferPointer() << std::endl;

		CHECKDX(device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), nullptr, &fsqVS));
		CHECKDX(device->CreatePixelShader(psblob->GetBufferPointer(), psblob->GetBufferSize(), nullptr, &fsqPS));

		D3D11_INPUT_ELEMENT_DESC iedfsq[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		CHECKDX(device->CreateInputLayout(iedfsq, 1, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &fsqlayout));

		//Create voxelizer shaders
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/voxelizing.vertex", nullptr, nullptr, "main", "vs_5_0", flags, 0, &vsblob, &errblob));
		if (errblob)
			std::cout << (char*)errblob->GetBufferPointer() << std::endl;
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/voxelizing.geometry", nullptr, nullptr, "main", "gs_5_0", flags, 0, &gsblob, &errblob));
		if (errblob)
			std::cout << (char*)errblob->GetBufferPointer() << std::endl;
		CHECKDX(D3DCompileFromFile(L"../Assets/Shaders/voxelizing.pixel", nullptr, nullptr, "main", "ps_5_0", flags, 0, &psblob, &errblob));
		if (errblob)
			std::cout << (char*)errblob->GetBufferPointer() << std::endl;

		CHECKDX(device->CreateVertexShader(vsblob->GetBufferPointer(), vsblob->GetBufferSize(), nullptr, &voxelVS));
		CHECKDX(device->CreateGeometryShader(gsblob->GetBufferPointer(), gsblob->GetBufferSize(), nullptr, &voxelGS));
		CHECKDX(device->CreatePixelShader(psblob->GetBufferPointer(), psblob->GetBufferSize(), nullptr, &voxelPS));

		D3D11_INPUT_ELEMENT_DESC iedvox[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		CHECKDX(device->CreateInputLayout(ied, 3, vsblob->GetBufferPointer(), vsblob->GetBufferSize(), &voxelLayout));

		vsblob->Release();
		gsblob->Release();
		psblob->Release();

	}

	{
		OBJLoader objLoader;

		mesh = objLoader.LoadBIN("../Assets/Models/sponza.bin");
	}

	//Camera 
	camera.SetLens((float)M_PI_4, NEARZ, FARZ, WINDOW_WIDTH, WINDOW_HEIGHT);

	std::unique_ptr<DirectX::GeometricPrimitive> shape(DirectX::GeometricPrimitive::CreateSphere(context, 10.0f, 8));

	{
		//Init primitive spritebatch from DirectXTK
		primitiveBatch = new DirectX::PrimitiveBatch<DirectX::VertexPositionColor>(context, 65536, (size_t)(65536/3));
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

	//Init voxel stuff
	{
		voxelMatrix = Matrix::CreateOrthographic(VOXEL_STRUCTURE_WIDTH, VOXEL_STRUCTURE_WIDTH, 0.0f, VOXEL_STRUCTURE_WIDTH);
		voxelViewProjMatrix[0] = Matrix::CreateLookAt(Vector3(-VOXEL_STRUCTURE_WIDTH/2, 0, 0), Vector3(0, 0, 0), Vector3(0, 1, 0)) * voxelMatrix;
		voxelViewProjMatrix[1] = Matrix::CreateLookAt(Vector3(0, -VOXEL_STRUCTURE_WIDTH / 2, 0), Vector3(0, 0, 0), Vector3(1, 0, 0)) * voxelMatrix;
		voxelViewProjMatrix[2] = Matrix::CreateLookAt(Vector3(0, 0, -VOXEL_STRUCTURE_WIDTH / 2), Vector3(0, 0, 0), Vector3(0, 1, 0)) * voxelMatrix;

		D3D11_BUFFER_DESC voxmatbuffdesc;
		voxmatbuffdesc.ByteWidth = 3 * sizeof(Matrix);
		voxmatbuffdesc.Usage = D3D11_USAGE_IMMUTABLE;
		voxmatbuffdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		voxmatbuffdesc.CPUAccessFlags = 0;
		voxmatbuffdesc.MiscFlags = 0;
		voxmatbuffdesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA voxmatdata;
		voxmatdata.pSysMem = &voxelViewProjMatrix[0];
		voxmatdata.SysMemPitch = 3 * sizeof(Matrix);
		voxmatdata.SysMemSlicePitch = 0;

		CHECKDX(device->CreateBuffer(&voxmatbuffdesc, &voxmatdata, &voxelMatBuffer));


		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = VOXEL_ELEMENTS * sizeof(uint32_t);
		cbDesc.Usage = D3D11_USAGE_DEFAULT;
		cbDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		cbDesc.StructureByteStride = 0;

		CHECKDX(device->CreateBuffer(&cbDesc, nullptr, &voxelStructure));

		D3D11_BUFFER_DESC voxelClearDesc;
		voxelClearDesc.ByteWidth = VOXEL_ELEMENTS * sizeof(uint32_t);
		voxelClearDesc.Usage = D3D11_USAGE_IMMUTABLE;
		voxelClearDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		voxelClearDesc.CPUAccessFlags = 0;
		voxelClearDesc.MiscFlags = 0;
		voxelClearDesc.StructureByteStride = 0;

		std::vector<uint32_t> initVector(VOXEL_ELEMENTS, 0);
		D3D11_SUBRESOURCE_DATA voxinitdata;
		voxinitdata.pSysMem = &initVector[0];
		voxinitdata.SysMemPitch = VOXEL_ELEMENTS * sizeof(uint32_t);
		voxinitdata.SysMemSlicePitch = 0;

		CHECKDX(device->CreateBuffer(&voxelClearDesc, &voxinitdata, &voxelClearBuffer));

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = VOXEL_ELEMENTS;
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

		CHECKDX(device->CreateUnorderedAccessView(voxelStructure, &uavDesc, &voxelUAV));

		D3D11_SHADER_RESOURCE_VIEW_DESC voxelstructsdesc;
		voxelstructsdesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		voxelstructsdesc.BufferEx.FirstElement = 0;
		voxelstructsdesc.BufferEx.NumElements = VOXEL_ELEMENTS;
		voxelstructsdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		voxelstructsdesc.Format = DXGI_FORMAT_R32_TYPELESS;
		
		CHECKDX(device->CreateShaderResourceView(voxelStructure, &voxelstructsdesc, &voxelSRV));

		D3D11_BUFFER_DESC voxelStagingDesc;
		voxelStagingDesc.ByteWidth = VOXEL_ELEMENTS * sizeof(uint32_t);
		voxelStagingDesc.Usage = D3D11_USAGE_STAGING;
		voxelStagingDesc.BindFlags = 0;
		voxelStagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		voxelStagingDesc.MiscFlags = 0;
		voxelStagingDesc.StructureByteStride = 0;

		CHECKDX(device->CreateBuffer(&voxelStagingDesc, nullptr, &voxelStagingStructure));

		voxelVoidRT.CreateRenderTarget(VOXELSIZE, VOXELSIZE, DXGI_FORMAT_R8G8_UNORM);

		D3D11_BLEND_DESC blendDesc = { 0 };

		CHECKDX(device->CreateBlendState(&blendDesc, &voxelBlendState));

	}

	//Set up ray marching sample kernel
	{
		int32_t kernelSizeX = 5;
		int32_t kernelSizeZ = 5;

		for (int32_t x = 0; x < kernelSizeX; ++x)
		{
			for (int32_t z = 0; z < kernelSizeZ; ++z)
			{
				sampleKernel[x + kernelSizeX * z] = Vector4(((x - kernelSizeX / 2) * 0.5f), ((z - kernelSizeZ / 2)* 0.5f), 1.0f, 0.0f);
				sampleKernel[x + kernelSizeX * z].Normalize();
			}
		}

		D3D11_BUFFER_DESC buffdesc;
		buffdesc.ByteWidth = 25 * sizeof(Vector4);
		buffdesc.Usage = D3D11_USAGE_IMMUTABLE;
		buffdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		buffdesc.CPUAccessFlags = 0;
		buffdesc.MiscFlags = 0;
		buffdesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = &sampleKernel[0];
		data.SysMemPitch = 25 * sizeof(Vector4);
		data.SysMemSlicePitch = 0;

		CHECKDX(device->CreateBuffer(&buffdesc, &data, &voxelSampleKernel));
		
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

		//Voxelize
		ID3D11RenderTargetView* RT = voxelVoidRT.GetRenderTargetView();
		context->OMSetRenderTargetsAndUnorderedAccessViews(1, &RT, 0, 1, 1, &voxelUAV, nullptr);
		context->OMSetBlendState(voxelBlendState, nullptr, 0xffffffff);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//context->CopyResource(voxelStructure, voxelClearBuffer);

		D3D11_VIEWPORT voxviewPort;
		ZeroMemory(&voxviewPort, sizeof(D3D11_VIEWPORT));

		voxviewPort.TopLeftX = 0;
		voxviewPort.TopLeftY = 0;
		voxviewPort.MinDepth = 0.0f;
		voxviewPort.MaxDepth = 1.0f;
		voxviewPort.Width = VOXELSIZE;
		voxviewPort.Height = VOXELSIZE;

		context->RSSetViewports(1, &voxviewPort);

		context->VSSetShader(voxelVS, 0, 0);
		context->GSSetShader(voxelGS, 0, 0);
		context->PSSetShader(voxelPS, 0, 0);
		context->IASetInputLayout(voxelLayout);
		context->GSSetConstantBuffers(0, 1, &voxelMatBuffer);
		context->RSSetState(RSStateSolid);

		
		mesh->Apply();
		mesh->Draw();

		

#ifdef DEBUG_DRAW
		context->CopyResource(voxelStagingStructure, voxelStructure);

		D3D11_MAPPED_SUBRESOURCE mapData;
		if (context->Map(voxelStagingStructure, 0, D3D11_MAP_READ, 0, &mapData) == S_OK)
		{
			memcpy(voxelCPUstructure, mapData.pData, VOXEL_ELEMENTS * sizeof(uint32_t));
			context->Unmap(voxelStagingStructure, 0);
		}
#endif

		//Clear RTs before drawing
		colorRT.Clear();
		normalRT.Clear();
		positionRT.Clear();
		context->ClearRenderTargetView(backBuffer, clearColor);
		context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		//Set Gbuffer RTs
		ID3D11RenderTargetView* RTs[] = { colorRT.GetRenderTargetView(), normalRT.GetRenderTargetView(), positionRT.GetRenderTargetView() };
		ID3D11RenderTargetView* RTzero[] = { 0, 0, 0 };
		context->OMSetRenderTargets(3, RTs, depthStencilView);
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);


		//Set shaders
		D3D11_VIEWPORT viewPort;
		ZeroMemory(&viewPort, sizeof(D3D11_VIEWPORT));

		viewPort.TopLeftX = 0;
		viewPort.TopLeftY = 0;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;
		viewPort.Width = WINDOW_WIDTH;
		viewPort.Height = WINDOW_HEIGHT;

		context->RSSetViewports(1, &viewPort);

		context->VSSetShader(VS, 0, 0);
		context->GSSetShader(nullptr, 0, 0);
		context->PSSetShader(PS, 0, 0);
		context->IASetInputLayout(layout);
		context->PSSetSamplers(0, 1, &linearSamplerState);
		context->VSSetConstantBuffers(0, 1, &camConstBuffer);
		context->RSSetState(RSStateSolid);


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

		//->OMSetRenderTargetsAndUnorderedAccessViews(1, &backBuffer, 0, 1, 1, &voxelUAV, nullptr);

		context->OMSetRenderTargets(1, &backBuffer, 0);
		//Draw to backbuffer
		context->VSSetShader(fsqVS, 0, 0);
		context->PSSetShader(fsqPS, 0, 0);
		context->IASetInputLayout(fsqlayout);
		colorRT.PSSetSRV(0);
		normalRT.PSSetSRV(1);
		positionRT.PSSetSRV(2);
		context->PSSetShaderResources(3, 1, &depthBufferView); 
		context->PSSetShaderResources(4, 1, &voxelSRV);
		context->PSSetConstantBuffers(0, 1, &voxelSampleKernel);
		context->RSSetState(RSStateSolid);

		timer->Start();
		context->DrawInstanced(3, 1, 0, 0);
		timer->Stop();

		double gpuTime = timer->GetTime();

		std::string outstr = "LA: " + std::to_string(interval) + "Voxelization time: " + std::to_string(gpuTime);
		SDL_SetWindowTitle(window, outstr.c_str());

		ID3D11ShaderResourceView* SRVzero[] = { 0 };
		context->PSSetShaderResources(3, 1, SRVzero);

		////////DRAW LINES//////
		//Set depth
		context->OMSetRenderTargets(1, &backBuffer, depthStencilView);

		basicEffect->SetView(XMLoadFloat4x4(&camera.GetCamData().viewMat));
		basicEffect->Apply(context);
		context->IASetInputLayout(basicInputLayout);

#ifdef DEBUG_DRAW
		float chunkSize = VOXELWIDTH;

		primitiveBatch->Begin();

		uint32_t voxelCounter = 0;
		for (uint32_t i = 0; i <  VOXEL_ELEMENTS; ++i)
		{
			int32_t x = i % VOXELSIZE;
			int32_t y = (i / VOXELSIZE) % VOXELSIZE;
			int32_t z = (i / (VOXELSIZE * VOXELSIZE)) * 32;
			uint32_t voxelTile = voxelCPUstructure[i];

			
			for(int32_t j = 0; j < 32; ++j)
			{

				if ( (voxelTile & 0x01) == 1)
				{
					if (voxelCounter % 1000 == 0) //Max primitive batch size is 2^16 indices, splitting up to handle more
					{
						primitiveBatch->End();
						primitiveBatch->Begin();
					}
					++voxelCounter;

					Vector3 ntl = Vector3((float)(x - VOXELSIZE/2) * VOXELWIDTH, (float)(y - VOXELSIZE/2) * VOXELWIDTH+VOXELWIDTH, (float)((z + j) - VOXELSIZE/2) * VOXELWIDTH);
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl, Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(0, 0, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl, Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, 0), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl, Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, 0), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, 0), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, 0), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(0, 0, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(0, 0, chunkSize), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(0, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)));
					primitiveBatch->DrawLine(DirectX::VertexPositionColor(ntl + Vector3(chunkSize, -chunkSize, chunkSize), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(ntl + Vector3(chunkSize, 0, chunkSize), Vector4(1, 0, 0, 0)));
				}
				voxelTile = voxelTile >> 1;
			}
		}

		std::cout << voxelCounter << std::endl;
		primitiveBatch->End();
#endif

		primitiveBatch->Begin();
		
		Vector3 n = Vector3(-0.2f, 0.5f, -1.0f);
		n.Normalize();
		Vector3 b1;
		Vector3 b2;

		if (n.z < -0.9999999f) // Handle the singularity
		{
			b1 = Vector3(0.0f, -1.0f, 0.0f);
			b2 = Vector3(-1.0f, 0.0f, 0.0f);
		}
		else
		{
			const float a = 1.0f / (1.0f + n.z);
			const float b = -n.x*n.y*a;
			b1 = Vector3(1.0f - n.x*n.x*a, b, -n.x);
			b2 = Vector3(b, 1.0f - n.y*n.y*a, -n.y);
		}
		Matrix orthoBasis = Matrix(b1, b2, n);

		primitiveBatch->DrawLine(DirectX::VertexPositionColor(Vector3(0, 0, 0), Vector4(1, 0, 0, 0)), DirectX::VertexPositionColor(n*10, Vector4(1, 0, 0, 0)));
		primitiveBatch->DrawLine(DirectX::VertexPositionColor(Vector3(0, 0, 0), Vector4(0, 1, 0, 0)), DirectX::VertexPositionColor(b1*10, Vector4(0, 1, 0, 0)));
		primitiveBatch->DrawLine(DirectX::VertexPositionColor(Vector3(0, 0, 0), Vector4(0, 0, 1, 0)), DirectX::VertexPositionColor(b2*10, Vector4(0, 0, 1, 0)));

		for (int32_t i = 0; i < 25; i++)
		{
			Vector3 tempNorm =  Vector3::Transform(Vector3(sampleKernel[i]), orthoBasis);

			primitiveBatch->DrawLine(DirectX::VertexPositionColor(Vector3(0, 0, 0), Vector4(0, 1, 1, 0)), DirectX::VertexPositionColor(Vector3(tempNorm * 9), Vector4(0, 1, 1, 0)));
		}

		primitiveBatch->End();

		swapChain->Present(0, 0);

		context->PSSetShaderResources(0, 1, SRVzero);
		context->PSSetShaderResources(1, 1, SRVzero);
		context->PSSetShaderResources(2, 1, SRVzero);
		context->PSSetShaderResources(3, 1, SRVzero);
		context->PSSetShaderResources(4, 1, SRVzero);

		
	}

	std::cout << "Exiting program!" << std::endl;

	delete primitiveBatch;
	delete basicEffect;
	delete timer;
	delete[] voxelCPUstructure;

	depthStencilView->Release();
	RSStateSolid->Release();
	RSStateWireframe->Release();
	voxelBlendState->Release();
	voxelStagingStructure->Release();
	voxelSampleKernel->Release();
	voxelClearBuffer->Release();
	voxelSRV->Release();
	fsqVS->Release();
	fsqPS->Release();
	camConstBuffer->Release();
	depthBufferView->Release();
	layout->Release();
	fsqlayout->Release();
	voxelLayout->Release();
	linearSamplerState->Release();
	VS->Release();
	PS->Release();
	voxelVS->Release();
	voxelGS->Release();
	voxelPS->Release();
	voxelStructure->Release();
	voxelUAV->Release();
	voxelMatBuffer->Release();
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
	positionRT.CreateRenderTarget(WINDOW_WIDTH, WINDOW_HEIGHT, DXGI_FORMAT_R32G32B32A32_FLOAT);

	
}