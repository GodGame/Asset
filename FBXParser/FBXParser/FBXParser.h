#pragma once

#include "stdafx.h"

ostream& operator<<(ostream& os, FbxVector4 & vt);
ostream& operator<<(ostream& os, XMFLOAT3 & vt);
ostream& operator<<(ostream& os, XMFLOAT2 & vt);


struct Vertex
{
	XMFLOAT3 xmf3Pos;
	XMFLOAT3 xmf3Normal;
	XMFLOAT3 xmf3BiNoraml;
	XMFLOAT2 xmf2TexCoord;

	Vertex()
	{
		ZeroMemory(&xmf3Pos, sizeof(xmf3Pos));
		ZeroMemory(&xmf3Normal, sizeof(xmf3Normal));
		ZeroMemory(&xmf3BiNoraml, sizeof(xmf3BiNoraml));
		ZeroMemory(&xmf2TexCoord, sizeof(xmf2TexCoord));
	}
};

class FBXParser
{
	string        m_stName;
	FbxManager  * m_pMgr;
	FbxScene    * m_pScene;
	FbxNode     * m_pRootNode;

	FbxImporter * m_pImporter;

	XMFLOAT3 ** m_ppControlPos;
	XMFLOAT3 ** m_ppPos;
	XMFLOAT3 ** m_ppNormal;

	int m_nObjects;

	vector<Vertex> * m_pvcVertex;
public:
	FBXParser();
	~FBXParser();

public:
	bool Initialize(const char * pstr);
	void Run();

	void Setting();
	void VertexRead();
	void ControlPointRead();

	void FileOut();

public:
	void NormalRead(FbxMesh * pMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
	void UVRead(FbxMesh * pMesh, int inCtrlPointIndex, int inUVIndex, XMFLOAT2 & outUV);
};

#define DESTROY(x) if(x) x->Destroy();

