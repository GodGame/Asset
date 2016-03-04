#pragma once

#include <fbxsdk.h>

#include "stdafx.h"
#include "FBXObj.h"

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
void DestroySdkObjects(FbxManager* pManager, bool pExitStatus);
void CreateAndFillIOSettings(FbxManager* pManager);

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false);
bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);

struct GameDevice {
	ID3D11Device* m_Device;
	ID3D11Device* GetDevice() { return m_Device; }
	void SetDevice(ID3D11Device* &m_pd3dDevice) { m_Device = m_pd3dDevice; }
};

//-------------------------------------------------------------------------------------------------------------------------------

class CFbxParser
{
public:
	CFbxParser();
	~CFbxParser(void);

	bool Init();
	bool Load(Character* pObject, const char* strFilename, const char* strTexname, ID3D11Device* pd3dDevice);
	void SetAnimation(Character* pObject);
	void SetCurrentAnimStack(Character* pObject, int count, int pIndex);
	bool MeshSet(const FbxMesh* pMesh);

	void UnloadCacheRecursive(FbxNode * pNode);
	void ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray);

	void FillPoseArray(Character* pObject);
	//
	int GetNodeCount() { return mScene->GetNodeCount(); }
	void SetDevice(ID3D11Device* m_pd3dDevice) { m_GameDevice.SetDevice(m_pd3dDevice); }

private:
	FbxManager*				m_pFbxManager;
	FbxScene*				mScene;
	GameDevice				m_GameDevice;	// 디바이스
};

//
//-------------------------------------------------------------------------------------------------------------------------------

FbxAMatrix GetGlobalPosition(FbxNode* pNode,
	const FbxTime& pTime,
	FbxPose* pPose = NULL,
	FbxAMatrix* pParentGlobalPosition = NULL);
FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex);
FbxAMatrix GetGeometry(FbxNode* pNode);
void AnimateNode(ID3D11Device* pd3dDevice, VBOMesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
	FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose);
void AnimateMesh(ID3D11Device* pd3dDevice, VBOMesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
	FbxAMatrix& pGlobalPosition, FbxPose* pPose);
void AnimateSkeleton(FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition);
void FillPoseArray(Character* pObject);
void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer * pAnimLayer, FbxVector4* pVertexArray);
void ComputeSkinDeformation(ID3D11Device* pd3dDevice, VBOMesh* pObjectMesh, FbxAMatrix& pGlobalPosition, FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray, FbxPose* pPose);
void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxCluster* pCluster,
	FbxAMatrix& pVertexTransformMatrix,
	FbxTime pTime,
	FbxPose* pPose);
void ReadVertexCacheData(FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray);
void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose);
void ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose);
void MatrixScale(FbxAMatrix& pMatrix, double pValue);
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue);
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix);