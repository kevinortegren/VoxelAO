#include "Mesh.h"
#include <iostream>

extern ID3D11Device* device;
extern ID3D11DeviceContext* context;

Mesh::Mesh(VertexIndexData* vert_ind_data, std::vector<MeshGroup> pmeshGroups, std::map<std::string, Material> pmaterialMap)
{
	meshGroups = pmeshGroups;
	materialMap = pmaterialMap;

	D3D11_SUBRESOURCE_DATA initData;
	D3D11_BUFFER_DESC bufferDesc;

	numVertices = vert_ind_data->vertexData.size();
	numIndices = vert_ind_data->indexData.size();

	stride = sizeof(Vertex);
	offset = 0;

	HRESULT result;

	//Vertex buffer descriptor
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(Vertex) * numVertices;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	//Vertex init data
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = &vert_ind_data->vertexData[0];

	result = device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);
	if (result != S_OK)
		std::cerr << "Vertex buffer creation failed!" << std::endl;

	//Vertex buffer descriptor
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.ByteWidth = sizeof(unsigned) * numIndices;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	//Vertex init data
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = &vert_ind_data->indexData[0];

	result = device->CreateBuffer(&bufferDesc, &initData, &indexBuffer);
	if (result != S_OK)
		std::cerr << "Index buffer creation failed!" << std::endl;
}

Mesh::~Mesh()
{
	indexBuffer->Release();
	vertexBuffer->Release();
	
	for (auto mat : materialMap)
	{
		delete mat.second.diffuse;
	}
}

void Mesh::Apply()
{
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void Mesh::DrawIndexed()
{
	for (auto mg : meshGroups)
	{
		Material mat = materialMap[mg.material];
		if (mat.diffuse != nullptr)
			mat.diffuse->PSSetSRV(0);
		
		context->DrawIndexed(mg.numIndices, mg.startIndex, 0);
	}
}

void Mesh::Draw()
{
	context->DrawIndexed(numIndices, 0, 0);
}
