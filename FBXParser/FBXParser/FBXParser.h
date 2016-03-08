#pragma once

#include "stdafx.h"

ostream& operator<<(ostream& os, FbxVector4 & vt);
ostream& operator<<(ostream& os, XMFLOAT3 & vt);
ostream& operator<<(ostream& os, XMFLOAT2 & vt);


struct Vertex
{
	XMFLOAT3 xmf3Pos;
	XMFLOAT2 xmf2TexCoord;
	XMFLOAT3 xmf3Normal;
	XMFLOAT3 xmf3Tangent;

	Vertex()
	{
		ZeroMemory(&xmf3Pos, sizeof(xmf3Pos));
		ZeroMemory(&xmf2TexCoord, sizeof(xmf2TexCoord));
		ZeroMemory(&xmf3Normal, sizeof(xmf3Normal));
		ZeroMemory(&xmf3Tangent, sizeof(xmf3Tangent));
	}
};

struct Mesh
{
	struct SubMesh
	{
		UINT uTriangleCount;
		UINT IndexOffset;
		SubMesh() { IndexOffset = uTriangleCount = 0; }
	};

	UINT nTriangles;
	UINT nIndices;

	vector<Vertex> m_vcVertexes;
	vector<USHORT> m_stIndices;
	vector<SubMesh> m_vcSubMeshes;

	Mesh()
	{
		nTriangles = 0;
		nIndices = 0;
	}
};

struct Object
{
	vector<wstring> m_vcTextureNames;
	vector<Mesh> m_vcMeshes;
	vector<FbxMesh*> m_pFbxMeshes;
};

enum eTextureType
{
	NONE = -1,
	DIFFUSE,
	NORMAL,
	SPECULAR,
	GLOW
};

class FBXParser
{
	string        m_stName;
	FbxManager  * m_pMgr;
	FbxScene    * m_pScene;
	FbxNode     * m_pRootNode;

	FbxImporter * m_pImporter;

	//XMFLOAT3    ** m_ppControlPos;
	//XMFLOAT3    ** m_ppPos;
	//XMFLOAT3    ** m_ppNormal;

//	int m_nObjects;

	//vector<Vertex> * m_pvcVertex;
	Object m_obj;

public:
	FBXParser();
	~FBXParser();

public:
	bool Initialize(const char * pstr);
	void Run();

	void Setting();
	void VertexRead();
	void TextureRead();
	eTextureType CheckTextureType(const wstring & wstrName);

	void FileOut();
	void FileOutObject();


public:
	void MeshRead(int MeshIndex);
	void NormalRead(FbxMesh * pMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal, XMFLOAT3& outTangent);
	void UVRead(FbxMesh * pMesh, int inCtrlPointIndex, int inUVIndex, XMFLOAT2 & outUV);

	void SetFbxFloatToXmFloat(XMFLOAT3 & data, FbxVector4 & fbxdata);
	void SetFbxFloatToXmFloat(XMFLOAT2 & data, FbxVector2 & fbxdata);

	//void ControlPointRead();
};

#define DESTROY(x) if(x) x->Destroy();

