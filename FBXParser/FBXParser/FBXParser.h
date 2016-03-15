#pragma once

#include "stdafx.h"

ostream& operator<<(ostream& os, FbxDouble4 & vt);
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

struct ClusterID
{
	int data[8];
	ClusterID()
	{
		ZeroMemory(data, sizeof(int) * 8);
	}
};

struct ClusterWeights
{
	float data[8];
	ClusterWeights()
	{
		ZeroMemory(data, sizeof(float) * 8);
	}
};

struct SkinDeformer
{
	vector<FbxSkin *> m_vcSkin;

	//int m_nClusters;
	vector<XMFLOAT4X4> m_vcClusterTransform;
	vector<int> m_vcInfluences;
	vector<ClusterID> m_vcClusterID;
	vector<ClusterWeights> m_vcClusterWeights;
};

struct Object
{
	vector<wstring> m_vcTextureNames;

	int m_nVertices;
	vector<Mesh> m_vcMeshes;
	vector<FbxMesh*> m_pFbxMeshes;
	vector<SkinDeformer*> m_vcSkinDeformer;

	~Object()
	{
		for (auto it = m_vcSkinDeformer.begin(); it != m_vcSkinDeformer.end(); ++it)
			delete *it;
		m_vcSkinDeformer.clear();
	}
};

struct AnimData
{
	FbxString m_stName;
	vector<float> m_vcKeyFrameTime;
	vector<float> m_vcKeyFrameData;
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

	FbxArray<FbxString*> m_stNameArray;
	FbxAnimLayer * m_pAnimLayer;
	//vector<FbxTime> m_tStart;
	FbxTime m_tStart;
	//vector<FbxTime> m_tStop;
	FbxTime m_tStop;
	FbxTime m_tFrameTime;
	FbxTime m_tAnimationLength;

	Object m_obj;
//	SkinDeformer m_SkinDeformer;
	vector<AnimData*> m_vcAnimData;

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

	void SetAnimation();
	void LoadAnimation();

	void GetChildAnimation(FbxNode * pNode, FbxTime nTime);

	void GetFbxSkinData(int index);

public:
	void MeshRead(FbxMesh * pMesh, Mesh * pDataMesh);
	void MeshRead(int MeshIndex);
	void NormalRead(FbxMesh * pMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal, XMFLOAT3& outTangent);
	void UVRead(FbxMesh * pMesh, int inCtrlPointIndex, int inUVIndex, XMFLOAT2 & outUV);

	void SetFbxFloatToXmFloat(XMFLOAT3 & data, FbxVector4 & fbxdata);
	void SetFbxFloatToXmFloat(XMFLOAT2 & data, FbxVector2 & fbxdata);

public: // Ani
	FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition);
	FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex);
	FbxAMatrix GetGeometry(FbxNode* pNode);

	void AnimateNode(FbxMesh * pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose);

	void AnimateMesh(ID3D11Device* pd3dDevice, FbxMesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pGlobalPosition, FbxPose* pPose);

	void AnimateSkeleton(FbxMesh * pMesh, FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition);

public:
	void ParseAnimations();
	void ParseAnimationRecursive(FbxAnimStack* pAnimStack, FbxNode* pNode);
	void ParseAnimationRecursive(FbxAnimLayer* pAnimLayer, FbxNode* pNode, int tabCount);
	void ParseKeyData(const FbxAnimCurve* pCurve, const FbxString& channelName);

};

#define DESTROY(x) if(x) x->Destroy();

