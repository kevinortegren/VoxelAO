#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <string>
#include <SimpleMath.h>

OBJLoader::OBJLoader()
{

}

OBJLoader::~OBJLoader()
{

}

Mesh* OBJLoader::LoadOBJ(const char* file)
{
	std::fstream objFile(file);

	std::vector<Vector3> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;

	std::vector<Vertex> vertexList;
	std::vector<unsigned> indexList;

	std::vector<MeshGroup> meshGroups;

	std::map<std::string, Material> materialMap;


	if (objFile)
	{
		std::string line;
		std::string prefix;

		while (objFile.eof() == false)
		{
			prefix = "NULL";
			std::stringstream lineStream;

			getline(objFile, line);
			lineStream << line;
			lineStream >> prefix;

			
			if (prefix == "v")
			{
				Vector3 pos;
				lineStream >> pos.x >> pos.y >> pos.z;
				positions.push_back(pos);
			}
			else if (prefix == "vt")
			{
				Vector2 uv;
				lineStream >> uv.x >> uv.y;
				uv.y = 1 - uv.y;
				texcoords.push_back(uv);
			}
			else if (prefix == "vn")
			{
				Vector3 normal;
				lineStream >> normal.x >> normal.y >> normal.z;
				normals.push_back(normal);
			}
			else if (prefix == "g")
			{
				
			}
			else if (prefix == "usemtl")
			{
				if (meshGroups.size() != 0)
				{
					meshGroups[meshGroups.size() - 1].numIndices = indexList.size() - meshGroups[meshGroups.size() - 1].startIndex;
				}

				MeshGroup mg;
				mg.startIndex = indexList.size();
				meshGroups.push_back(mg);

				lineStream >> meshGroups[meshGroups.size() - 1].material;
			}
			else if (prefix == "f")
			{
				Vertex vert;
				char slash;

				int indexPos;
				int texPos;
				int normPos;

				//Loop through vertex points in face
				for (unsigned i = 0; i < 3; ++i)
				{
					lineStream >> indexPos >> slash >> texPos >> slash >> normPos;

					vert.pos = positions[indexPos - 1];
					vert.texc = texcoords[texPos - 1];
					vert.normal = normals[normPos - 1];

					indexList.push_back(vertexList.size());
					vertexList.push_back(vert);
				}
				
				//Check if there is a fourth vertex in the face. If there is it means that is a quad face.
				indexPos = -1;
				lineStream >> indexPos >> slash >> texPos >> slash >> normPos;
				if (indexPos != -1)
				{
					vert.pos = positions[indexPos - 1];
					vert.texc = texcoords[texPos - 1];
					vert.normal = normals[normPos - 1];

					unsigned vertexListSize = vertexList.size();
					indexList.push_back(vertexListSize);
					indexList.push_back(vertexListSize - 3);
					indexList.push_back(vertexListSize - 1);
					vertexList.push_back(vert);
				}
			}
			else if (prefix == "mtllib")
			{
				std::string mtlName;
				lineStream >> mtlName;
				std::string mtlPath = "../Assets/Models/" + mtlName;
				LoadMTL(materialMap, mtlPath.c_str());
			}
		}
	}

	if (meshGroups.size() != 0)
	{
		meshGroups[meshGroups.size() - 1].numIndices = indexList.size() - meshGroups[meshGroups.size() - 1].startIndex;
	}

	return new Mesh(&vertexList, &indexList, meshGroups, materialMap);
}

void OBJLoader::LoadMTL(std::map<std::string, Material>& materialMap, const char* file)
{
	std::fstream mtlFile(file);
	std::string currentMaterial;

	if (mtlFile)
	{
		std::string line;
		std::string prefix;

		while (mtlFile.eof() == false)
		{
			prefix = "NULL";
			std::stringstream lineStream;

			getline(mtlFile, line);
			lineStream << line;
			lineStream >> prefix;

			if (prefix == "newmtl")
			{
				lineStream >> currentMaterial;
				Material mat;
				mat.diffuse = new Texture();

				materialMap[currentMaterial] = mat;
			}
			else if (prefix == "map_Kd")
			{
				std::string texName;
				lineStream >> texName;
				std::string filePath = "../Assets/Models/" + texName;
				materialMap[currentMaterial].diffuse->CreateTextureFromFile(filePath.c_str());
			}
			else if (prefix == "map_d")
			{
				
			}
		}
	}
}
