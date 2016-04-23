#pragma once

#include <SimpleMath.h>
using namespace DirectX::SimpleMath;

struct Vertex
{
	Vertex(){};
	Vertex(Vector3 ppos, Vector3 pnormal, Vector2 ptexc)
	{
		pos		= ppos;
		normal	= pnormal;
		texc	= ptexc;
	}

	Vector3 pos;
	Vector3 normal;
	Vector2 texc;
};

struct VertexPT
{
	VertexPT(){};
	VertexPT(Vector3 ppos)
	{
		pos = ppos;
	}

	Vector3 pos;
};