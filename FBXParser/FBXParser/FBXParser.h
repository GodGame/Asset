#pragma once

#include "stdafx.h"

ostream& operator<<(ostream& os, FbxVector4 & vt);
ostream& operator<<(ostream& os, XMFLOAT3 & vt);

class FBXParser
{
	FbxManager  * m_pMgr;
	FbxScene    * m_pScene;
	FbxNode     * m_pRootNode;

	FbxImporter * m_pImporter;

	XMFLOAT3 ** m_ppControlPos;
	XMFLOAT3 ** m_ppNormal;

	int m_nObjects;

public:
	FBXParser();
	~FBXParser();

	bool Initialize(const char * pstr);
	void Run();

	void Setting();
	void VertexRead();
	void NormalRead();
};

#define DESTROY(x) if(x) x->Destroy();

