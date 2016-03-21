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

	vector<XMFLOAT3> m_vcPos;
	vector<XMFLOAT3> m_vcNormal;
	vector<XMFLOAT3> m_vcUV;


	vector<USHORT> m_stIndices;
	vector<SubMesh> m_vcSubMeshes;

	FbxTime mStart, mStop, mCurrentTime, mCache_Start, mCache_Stop;

	Mesh()
	{
		mCache_Start = 0;
		mCache_Stop = 0;

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

	void resize(int nSize)
	{
		ClusterID id;
		ClusterWeights wt;

		m_vcInfluences.resize(nSize, 0);
		m_vcClusterID.resize(nSize, id);
		m_vcClusterWeights.resize(nSize, wt);
	}
};

struct Object
{
	vector<wstring> m_vcTextureNames;

	bool mAllByControlPoint;
	bool mHasNormal;
	bool mHasTangent;
	bool mHasUV;

	int m_nVertices;
	vector<Mesh> m_vcMeshes;
	vector<FbxMesh*> m_pFbxMeshes;
	FbxArray<FbxPose*> m_pFbxPose;

	FbxAnimLayer*			mCurrentAnimLayer;
	vector<SkinDeformer*> m_vcSkinDeformer;

	Object()
	{
		mHasUV = true;
		mHasNormal = true;
		mAllByControlPoint = true;
		mHasTangent = false;
	}

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
	int iSettingNum;

	XMFLOAT4X4 xmTransform;
	bool m_bUseAnimatedMesh;
	bool m_bUseSaveTangent;
	bool m_bFixCenter;

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


	vector<Vertex> mVertices;
	Object m_obj;
//	SkinDeformer m_SkinDeformer;
	vector<AnimData*> m_vcAnimData;

public:
	FBXParser();
	~FBXParser();


public:
	void SetOption();
	bool Initialize(const char * pstr);
	void Run();

	void Setting();
	void VertexRead();
	void TextureRead();
	eTextureType CheckTextureType(const wstring & wstrName);

	void FileOutObject();
	void CalculateTangent();
	void TransformVertexes(vector<Vertex>& vcVertexes);
	void FileOutAnimatedMeshes(int iFileNum, int index);

	void SetAnimation();
	void SetCurrentAnimStack(int count, int pIndex);
	void LoadAnimation();

	void GetChildAnimation(FbxNode * pNode, FbxTime nTime);

	void GetFbxSkinData(int index);
	void GetFbxSkinData(FbxMesh * pMesh, int nVertexesSize);

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

	void AnimateNode(Mesh * pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose);

public:
	void AnimateMesh(Mesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
		FbxAMatrix& pGlobalPosition, FbxPose* pPose);

	void ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray);
	void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer * pAnimLayer, FbxVector4* pVertexArray);
	// Deform the vertex array according to the links contained in the mesh and the skinning type.
	void ComputeSkinDeformation(Mesh * pObjectMesh, FbxAMatrix& pGlobalPosition, FbxMesh* pMesh,
		FbxTime& pTime, FbxVector4* pVertexArray, FbxPose* pPose);
	void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
		FbxMesh* pMesh,
		FbxCluster* pCluster,
		FbxAMatrix& pVertexTransformMatrix,
		FbxTime pTime,
		FbxPose* pPose);


public:
	void AnimateSkeleton(FbxMesh * pMesh, FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition);

public:
	void ParseAnimations();
	void ParseAnimationRecursive(FbxAnimStack* pAnimStack, FbxNode* pNode);
	void ParseAnimationRecursive(FbxAnimLayer* pAnimLayer, FbxNode* pNode, int tabCount);
	void ParseKeyData(const FbxAnimCurve* pCurve, const FbxString& channelName);

};

#define DESTROY(x) if(x) x->Destroy();

