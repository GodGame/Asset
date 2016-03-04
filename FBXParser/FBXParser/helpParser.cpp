#include "stdafx.h"
#include "FbxParser.h"

#ifdef IOS_REF
#undef  IOS_REF
#define IOS_REF (*(pManager->GetIOSettings()))
#endif

void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene)
{
	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
	pManager = FbxManager::Create();
	if (!pManager)
	{
#ifdef _DEBUG
		FBXSDK_printf("Error: Unable to create FBX Manager!\n");
#endif
		exit(1);
	}
	else {
#ifdef _DEBUG
		FBXSDK_printf("Autodesk FBX SDK version %s\n", pManager->GetVersion());
#endif
	}
	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(pManager, IOSROOT);
	pManager->SetIOSettings(ios);

	//Load plugins from the executable directory (optional)
	FbxString lPath = FbxGetApplicationDirectory();
	pManager->LoadPluginsDirectory(lPath.Buffer());

	//Create an FBX scene. This object holds most objects imported/exported from/to files.
	pScene = FbxScene::Create(pManager, "My Scene");
	if (!pScene)
	{
		FBXSDK_printf("Error: Unable to create FBX scene!\n");
		exit(1);
	}
}

void DestroySdkObjects(FbxManager* pManager, bool pExitStatus)
{
	//Delete the FBX Manager. All the objects that have been allocated using the FBX Manager and that haven't been explicitly destroyed are also automatically destroyed.
	if (pManager) pManager->Destroy();
	if (pExitStatus) FBXSDK_printf("Program Success!\n");
}

bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat, bool pEmbedMedia)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create an exporter.
	FbxExporter* lExporter = FbxExporter::Create(pManager, "");

	if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format in less no ASCII format found
		pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		//Try to export in ASCII if possible
		int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

		for (lFormatIndex = 0; lFormatIndex<lFormatCount; lFormatIndex++)
		{
			if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
			{
				FbxString lDesc = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
				const char *lASCII = "ascii";
				if (lDesc.Find(lASCII) >= 0)
				{
					pFileFormat = lFormatIndex;
					break;
				}
			}
		}
	}

	// Set the export states. By default, the export states are always set to 
	// true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
	// shows how to change these states.
	IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
	IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
	IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
	IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
	IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
	IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
	IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	// Initialize the exporter by providing a filename.
	if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
	{
		FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
		return false;
	}

	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	//FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

	// Export the scene.
	lStatus = lExporter->Export(pScene);

	// Destroy the exporter.
	lExporter->Destroy();
	return lStatus;
}

bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename)
{
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor, lSDKMinor, lSDKRevision;
	//int lFileFormat = -1;
	int  lAnimStackCount;
	bool lStatus;
	char lPassword[1024];

	// Get the file version number generate by the FBX SDK.
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(pManager, "");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize(pFilename, -1, pManager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if (!lImportStatus)
	{
		FbxString error = lImporter->GetStatus().GetErrorString();
		FBXSDK_printf("Call to FbxImporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", error.Buffer());

		if (lImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
		{
			FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);
			FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);
		}

		return false;
	}

	//FBXSDK_printf("FBX file format version for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		//FBXSDK_printf("FBX file format version for file '%s' is %d.%d.%d\n\n", pFilename, lFileMajor, lFileMinor, lFileRevision);

		// From this point, it is possible to access animation stack information without
		// the expense of loading the entire file.

		//FBXSDK_printf("Animation Stack Information\n");

		lAnimStackCount = lImporter->GetAnimStackCount();

		//FBXSDK_printf("    Number of Animation Stacks: %d\n", lAnimStackCount);
		//FBXSDK_printf("    Current Animation Stack: \"%s\"\n", lImporter->GetActiveAnimStackName().Buffer());
		//FBXSDK_printf("\n");

		//for (i = 0; i < lAnimStackCount; i++)
		//{
		//	FbxTakeInfo* lTakeInfo = lImporter->GetTakeInfo(i);

		//	FBXSDK_printf("    Animation Stack %d\n", i);
		//	FBXSDK_printf("         Name: \"%s\"\n", lTakeInfo->mName.Buffer());
		//	FBXSDK_printf("         Description: \"%s\"\n", lTakeInfo->mDescription.Buffer());

		//	// Change the value of the import name if the animation stack should be imported 
		//	// under a different name.
		//	FBXSDK_printf("         Import Name: \"%s\"\n", lTakeInfo->mImportName.Buffer());

		//	// Set the value of the import state to false if the animation stack should be not
		//	// be imported. 
		//	FBXSDK_printf("         Import State: %s\n", lTakeInfo->mSelect ? "true" : "false");
		//	FBXSDK_printf("\n");
		//}

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		IOS_REF.SetBoolProp(IMP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(IMP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(IMP_FBX_LINK, true);
		IOS_REF.SetBoolProp(IMP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(IMP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(IMP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	lStatus = lImporter->Import(pScene);

	if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
	{
		FBXSDK_printf("Please enter password: ");

		lPassword[0] = '\0';

		FBXSDK_CRT_SECURE_NO_WARNING_BEGIN
			scanf("%s", lPassword);
		FBXSDK_CRT_SECURE_NO_WARNING_END

			FbxString lString(lPassword);

		IOS_REF.SetStringProp(IMP_FBX_PASSWORD, lString);
		IOS_REF.SetBoolProp(IMP_FBX_PASSWORD_ENABLE, true);

		lStatus = lImporter->Import(pScene);

		if (lStatus == false && lImporter->GetStatus().GetCode() == FbxStatus::ePasswordError)
		{
			FBXSDK_printf("\nPassword is wrong, import aborted.\n");
		}
	}

	// Destroy the importer.
	lImporter->Destroy();

	return lStatus;
}

//---------------------------------------------------------------------------------------------------------------------------------

CFbxParser::CFbxParser() :
	m_pFbxManager(NULL)
{
}

CFbxParser::~CFbxParser(void)
{
	//DestroySdkObjects(m_pFbxManager, true);
}

bool CFbxParser::Init()
{
	mScene = NULL;

	return true;
}

bool CFbxParser::Load(Character* pObject, const char* strFilename, const char* strTexname, ID3D11Device* pd3dDevice)
{
	if (!Init())
		return false;

	bool bResult = true;

	InitializeSdkObjects(m_pFbxManager, mScene);

	if (!m_pFbxManager)
		return false;

	bResult = LoadScene(m_pFbxManager, mScene, strFilename);
	if (!bResult) return false;

	pObject->mFrameTime.SetTime(0, 0, 0, 1, 0, mScene->GetGlobalSettings().GetTimeMode());

	FbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(mScene);
	}

	FbxSystemUnit SceneSystemUnit = mScene->GetGlobalSettings().GetSystemUnit();
	if (SceneSystemUnit.GetScaleFactor() != 1.0)
	{
		FbxSystemUnit::cm.ConvertScene(mScene);
	}

	// Get the list of all the animation stack.
	mScene->FillAnimStackNameArray(pObject->mAnimStackNameArray);

	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(m_pFbxManager);
	lGeomConverter.Triangulate(mScene, true);

	// split meshes per material, so that we only have one material per mesh (for VBO support)
	lGeomConverter.SplitMeshesPerMaterial(mScene, true);

	pObject->LoadCacheRecursive(pd3dDevice, mScene->GetRootNode());

	pObject->CreateIBBuffer(pd3dDevice);

	FillPoseArray(pObject);
	SetAnimation(pObject);

	const int lTextureCount = mScene->GetTextureCount();
	for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
	{
		FbxTexture * lTexture = mScene->GetTexture(lTextureIndex);
		FbxFileTexture * lFileTexture = FbxCast<FbxFileTexture>(lTexture);
		if (lFileTexture && !lFileTexture->GetUserDataPtr())
		{
			LPCSTR FileName = lFileTexture->GetFileName();
			char localFilePath[256];
			ZeroMemory(localFilePath, 256);
			strncpy(localFilePath, "Texture/", 8);

			char temp[256];
			ZeroMemory(temp, 256);

			int count = 0;
			for (int i = strlen(FileName) - 1; i >= 0; --i)
			{
				if (FileName[i] == '\\') break;

				temp[count++] = FileName[i];
			}

			int len = strlen(temp) - 1;

			for (int i = len; i >= 0; --i)
				localFilePath[8 + len - i] = temp[i];

			wchar_t szTexFilename[256];
			//CharToWChar(localFilePath, szTexFilename);
			mbstowcs(szTexFilename, localFilePath, strlen(localFilePath) + 1);

			WCHAR	wstr[512];
			MultiByteToWideChar(CP_ACP, 0, strTexname, strlen(strTexname) + 1, wstr, sizeof(wstr) / sizeof(wstr[0]));


			ID3D11ShaderResourceView* pSRV = NULL;
			D3DX11CreateShaderResourceViewFromFile(pd3dDevice, wstr,
				0, 0, &pSRV, 0);

			if (pSRV != NULL)
			{
				pObject->PushSRV(pSRV);
				pSRV = NULL;
			}
		}
	}

	UnloadCacheRecursive(mScene->GetRootNode());

	return bResult;
}

// Unload the cache and release the memory under this node recursively.
void CFbxParser::UnloadCacheRecursive(FbxNode * pNode)
{
	// Unload the material cache
	const int lMaterialCount = pNode->GetMaterialCount();
	for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
	{
		FbxSurfaceMaterial * lMaterial = pNode->GetMaterial(lMaterialIndex);
		if (lMaterial && lMaterial->GetUserDataPtr())
		{
			MaterialCache * lMaterialCache = static_cast<MaterialCache *>(lMaterial->GetUserDataPtr());
			lMaterial->SetUserDataPtr(NULL);
			delete lMaterialCache;
		}
	}

	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
	if (lNodeAttribute)
	{
		// Unload the mesh cache
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			FbxMesh * lMesh = pNode->GetMesh();
			if (lMesh && lMesh->GetUserDataPtr())
			{
				VBOMesh * lMeshCache = static_cast<VBOMesh *>(lMesh->GetUserDataPtr());
				lMesh->SetUserDataPtr(NULL);
				delete lMeshCache;
			}
		}
		// Unload the light cache
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eLight)
		{
			FbxLight * lLight = pNode->GetLight();
			if (lLight && lLight->GetUserDataPtr())
			{
				LightCache * lLightCache = static_cast<LightCache *>(lLight->GetUserDataPtr());
				lLight->SetUserDataPtr(NULL);
				delete lLightCache;
			}
		}
	}

	const int lChildCount = pNode->GetChildCount();
	for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
	{
		UnloadCacheRecursive(pNode->GetChild(lChildIndex));
	}
}

void CFbxParser::SetAnimation(Character* pObject)
{
	// If one node is selected, draw it and its children.
	FbxAMatrix lDummyGlobalPosition;
	FbxNode* pNode = NULL;

	for (auto i = 0; i < pObject->GetMeshSize(); ++i)
	{
		pNode = pObject->GetVBOMesh(i)->GetMesh()->GetNode();

		int lCurrentAnimStackIndex = 0;

		// Add the animation stack names.	애니메이션 스택 네임 추가
		for (int lPoseIndex = 0; lPoseIndex < pObject->mAnimStackNameArray.GetCount(); ++lPoseIndex)
		{
			// Track the current animation stack index.		현재 애니메이션 스택 인덱스를 추적해서 해당하면 인덱스설정
			if (pObject->mAnimStackNameArray[lPoseIndex]->Compare(mScene->ActiveAnimStackName.Get()) == 0)
			{
				lCurrentAnimStackIndex = lPoseIndex;
			}
		}

		SetCurrentAnimStack(pObject, i, lCurrentAnimStackIndex);

		//FbxPose * lPose = NULL;
		//if (mPoseIndex != -1)
		//{
		//	lPose = mScene->GetPose(mPoseIndex);
		//}
	}
}

// 현재 애니메이션스택을 설정
void CFbxParser::SetCurrentAnimStack(Character* pObject, int count, int pIndex)
{
	const int lAnimStackCount = pObject->mAnimStackNameArray.GetCount();
	if (!lAnimStackCount || pIndex >= lAnimStackCount)
		return;

	// select the base layer from the animation stack
	// 애니메이션 스택에서 베이스 층을 선택한다.

	// FbxAnimStack : 애니메이션 스택은 애니메이션 레이어의 모음입니다. 
	// FBX 문서는 하나 이상의 애니메이션 스택을 가질 수있다. 각 스택은 하나가 FBX SDK의 이전 버전에서 "take"로 볼 수 있습니다.
	// "스택"용어는 물체가소 정의 속성에 대한 결과적인 애니메이션을 만들어 내기 위해 블렌딩 모드에 따라 평가되는
	// N애니메이션 레이어 1이 포함되어 있다는 사실에서 나온다.
	FbxAnimStack* lCurrentAnimationStack = mScene->FindMember<FbxAnimStack>(pObject->mAnimStackNameArray[pIndex]->Buffer());
	if (lCurrentAnimationStack == NULL)	       // this is a problem. The anim stack should be found in the scene!
		return;

	// we assume that the first animation layer connected to the animation stack is the base layer
	// (this is the assumption made in the FBXSDK)
	// 우리는 애니메이션 스택에 연결된 첫번째 애니메이션 층이 베이스 층(이것은 FBXSDK 이루어진 가정이다) 인 것으로 가정한다.
	pObject->mCurrentAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();
	// 애니메이션 평가자에 의해 사용되는 현재의 애니메이션 스택 내용을 설정합니다.
	mScene->SetCurrentAnimationStack(lCurrentAnimationStack);

	FbxTakeInfo* lCurrentTakeInfo = mScene->GetTakeInfo(*(pObject->mAnimStackNameArray[pIndex]));
	if (lCurrentTakeInfo)
	{
		pObject->GetVBOMesh(count)->mStart = lCurrentTakeInfo->mLocalTimeSpan.GetStart();
		pObject->GetVBOMesh(count)->mStop = lCurrentTakeInfo->mLocalTimeSpan.GetStop();
	}
	else
	{
		// Take the time line value
		FbxTimeSpan lTimeLineTimeSpan;
		mScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(lTimeLineTimeSpan);

		pObject->GetVBOMesh(count)->mStart = lTimeLineTimeSpan.GetStart();
		pObject->GetVBOMesh(count)->mStop = lTimeLineTimeSpan.GetStop();
	}

	// check for smallest start with cache start
	if (pObject->GetVBOMesh(count)->mCache_Start < pObject->GetVBOMesh(count)->mStart)
		pObject->GetVBOMesh(count)->mStart = pObject->GetVBOMesh(count)->mCache_Start;

	// check for biggest stop with cache stop
	if (pObject->GetVBOMesh(count)->mCache_Stop > pObject->GetVBOMesh(count)->mStop)
		pObject->GetVBOMesh(count)->mStop = pObject->GetVBOMesh(count)->mCache_Stop;

	//// move to beginning
	//mCurrentTime = mStart;
}

//
void CFbxParser::ReadVertexCacheData(FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray)
{
	FbxVertexCacheDeformer*  lDeformer = static_cast<FbxVertexCacheDeformer*>(pMesh->GetDeformer(0, FbxDeformer::eVertexCache));
	FbxCache*                lCache = lDeformer->GetCache();
	int                      lChannelIndex = -1;
	unsigned int             lVertexCount = (unsigned int)pMesh->GetControlPointsCount();
	bool                     lReadSucceed = false;
	double*                  lReadBuf = new double[3 * lVertexCount];

	if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
	{
		if ((lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get())) > -1)
		{
			lReadSucceed = lCache->Read(lChannelIndex, pTime, lReadBuf, lVertexCount);
		}
	}
	else // eMaxPointCacheV2
	{
		lReadSucceed = lCache->Read((unsigned int)pTime.GetFrameCount(), lReadBuf, lVertexCount);
	}

	if (lReadSucceed)
	{
		unsigned int lReadBufIndex = 0;

		while (lReadBufIndex < 3 * lVertexCount)
		{
			// In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
			// on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
			// So separate them.
			pVertexArray[lReadBufIndex / 3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
		}
	}

	delete[] lReadBuf;
}

// Find all poses in this scene.		씬에서 모든 포즈를 찾아서 배열에 추가함
void CFbxParser::FillPoseArray(Character* pObject)
{
	const int lPoseCount = mScene->GetPoseCount();

	for (int i = 0; i < lPoseCount; ++i)
	{
		pObject->mPoseArray.Add(mScene->GetPose(i));
	}
}

//--------------------------------------------------------------------------------------------------------------------


void MatrixScale(FbxAMatrix& pMatrix, double pValue)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pMatrix[i][j] *= pValue;
		}
	}
}

// Add a value to all the elements in the diagonal of the matrix.
void MatrixAddToDiagonal(FbxAMatrix& pMatrix, double pValue)
{
	pMatrix[0][0] += pValue;
	pMatrix[1][1] += pValue;
	pMatrix[2][2] += pValue;
	pMatrix[3][3] += pValue;
}

// Sum two matrices element by element.
void MatrixAdd(FbxAMatrix& pDstMatrix, FbxAMatrix& pSrcMatrix)
{
	int i, j;

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			pDstMatrix[i][j] += pSrcMatrix[i][j];
		}
	}
}

// Deform the vertex array in classic linear way.
void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

	int lVertexCount = pMesh->GetControlPointsCount();
	FbxAMatrix* lClusterDeformation = new FbxAMatrix[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount * sizeof(FbxAMatrix));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));

	if (lClusterMode == FbxCluster::eAdditive)
	{
		for (int i = 0; i < lVertexCount; ++i)
		{
			lClusterDeformation[i].SetIdentity();
		}
	}

	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int lSkinIndex = 0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);

		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];

				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;

				double lWeight = lCluster->GetControlPointWeights()[k];

				if (lWeight == 0.0)
				{
					continue;
				}

				// Compute the influence of the link on the vertex.
				FbxAMatrix lInfluence = lVertexTransformMatrix;
				MatrixScale(lInfluence, lWeight);

				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Multiply with the product of the deformations on the vertex.
					MatrixAddToDiagonal(lInfluence, 1.0 - lWeight);
					lClusterDeformation[lIndex] = lInfluence * lClusterDeformation[lIndex];

					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					// Add to the sum of the deformations on the vertex.
					MatrixAdd(lClusterDeformation[lIndex], lInfluence);

					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex			
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeight = lClusterWeight[i];

		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeight != 0.0)
		{
			lDstVertex = lClusterDeformation[i].MultT(lSrcVertex);
			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links. 
				lDstVertex /= lWeight;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links. 
				lSrcVertex *= (1.0 - lWeight);
				lDstVertex += lSrcVertex;
			}
		}
	}

	delete[] lClusterDeformation;
	delete[] lClusterWeight;
}

// Deform the vertex array in Dual Quaternion Skinning way.
void ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray,
	FbxPose* pPose)
{
	// All the links must have the same link mode.
	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

	int lVertexCount = pMesh->GetControlPointsCount();
	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);

	FbxDualQuaternion* lDQClusterDeformation = new FbxDualQuaternion[lVertexCount];
	memset(lDQClusterDeformation, 0, lVertexCount * sizeof(FbxDualQuaternion));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount * sizeof(double));

	// For all skins and all clusters, accumulate their deformation and weight
	// on each vertices and store them in lClusterDeformation and lClusterWeight.
	for (int lSkinIndex = 0; lSkinIndex<lSkinCount; ++lSkinIndex)
	{
		FbxSkin * lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);
		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex<lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			FbxQuaternion lQ = lVertexTransformMatrix.GetQ();
			FbxVector4 lT = lVertexTransformMatrix.GetT();
			FbxDualQuaternion lDualQuaternion(lQ, lT);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];

				// Sometimes, the mesh can have less points than at the time of the skinning
				// because a smooth operator was active when skinning but has been deactivated during export.
				if (lIndex >= lVertexCount)
					continue;

				double lWeight = lCluster->GetControlPointWeights()[k];

				if (lWeight == 0.0)
					continue;

				// Compute the influence of the link on the vertex.
				FbxDualQuaternion lInfluence = lDualQuaternion * lWeight;
				if (lClusterMode == FbxCluster::eAdditive)
				{
					// Simply influenced by the dual quaternion.
					lDQClusterDeformation[lIndex] = lInfluence;

					// Set the link to 1.0 just to know this vertex is influenced by a link.
					lClusterWeight[lIndex] = 1.0;
				}
				else // lLinkMode == FbxCluster::eNormalize || lLinkMode == FbxCluster::eTotalOne
				{
					if (lClusterIndex == 0)
					{
						lDQClusterDeformation[lIndex] = lInfluence;
					}
					else
					{
						// Add to the sum of the deformations on the vertex.
						// Make sure the deformation is accumulated in the same rotation direction. 
						// Use dot product to judge the sign.
						double lSign = lDQClusterDeformation[lIndex].GetFirstQuaternion().DotProduct(lDualQuaternion.GetFirstQuaternion());
						if (lSign >= 0.0)
						{
							lDQClusterDeformation[lIndex] += lInfluence;
						}
						else
						{
							lDQClusterDeformation[lIndex] -= lInfluence;
						}
					}
					// Add to the sum of weights to either normalize or complete the vertex.
					lClusterWeight[lIndex] += lWeight;
				}
			}//For each vertex
		}//lClusterCount
	}

	//Actually deform each vertices here by information stored in lClusterDeformation and lClusterWeight
	for (int i = 0; i < lVertexCount; i++)
	{
		FbxVector4 lSrcVertex = pVertexArray[i];
		FbxVector4& lDstVertex = pVertexArray[i];
		double lWeightSum = lClusterWeight[i];

		// Deform the vertex if there was at least a link with an influence on the vertex,
		if (lWeightSum != 0.0)
		{
			lDQClusterDeformation[i].Normalize();
			lDstVertex = lDQClusterDeformation[i].Deform(lDstVertex);

			if (lClusterMode == FbxCluster::eNormalize)
			{
				// In the normalized link mode, a vertex is always totally influenced by the links. 
				lDstVertex /= lWeightSum;
			}
			else if (lClusterMode == FbxCluster::eTotalOne)
			{
				// In the total 1 link mode, a vertex can be partially influenced by the links. 
				lSrcVertex *= (1.0 - lWeightSum);
				lDstVertex += lSrcVertex;
			}
		}
	}

	delete[] lDQClusterDeformation;
	delete[] lClusterWeight;
}

FbxAMatrix GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition)
{
	FbxAMatrix lGlobalPosition;
	bool        lPositionFound = false;

	if (pPose)
	{
		int lNodeIndex = pPose->Find(pNode);

		if (lNodeIndex > -1)
		{
			// The bind pose is always a global matrix.
			// If we have a rest pose, we need to check if it is
			// stored in global or local space.
			if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
			{
				lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
			}
			else
			{
				// We have a local matrix, we need to convert it to
				// a global space matrix.
				FbxAMatrix lParentGlobalPosition;

				if (pParentGlobalPosition)
				{
					lParentGlobalPosition = *pParentGlobalPosition;
				}
				else
				{
					if (pNode->GetParent())
					{
						lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose);
					}
				}

				FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
				lGlobalPosition = lParentGlobalPosition * lLocalPosition;
			}

			lPositionFound = true;
		}
	}

	if (!lPositionFound)
	{
		// There is no pose entry for that node, get the current global position instead.

		// Ideally this would use parent global position and local position to compute the global position.
		// Unfortunately the equation 
		//    lGlobalPosition = pParentGlobalPosition * lLocalPosition
		// does not hold when inheritance type is other than "Parent" (RSrs).
		// To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
		lGlobalPosition = pNode->EvaluateGlobalTransform(pTime);
	}

	return lGlobalPosition;
}

FbxAMatrix GetPoseMatrix(FbxPose* pPose, int pNodeIndex)
{
	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

	return lPoseMatrix;
}

FbxAMatrix GetGeometry(FbxNode* pNode)
{
	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void ReadVertexCacheData(FbxMesh* pMesh,
	FbxTime& pTime,
	FbxVector4* pVertexArray)
{
	FbxVertexCacheDeformer*  lDeformer = static_cast<FbxVertexCacheDeformer*>(pMesh->GetDeformer(0, FbxDeformer::eVertexCache));
	FbxCache*                lCache = lDeformer->GetCache();
	int                      lChannelIndex = -1;
	unsigned int             lVertexCount = (unsigned int)pMesh->GetControlPointsCount();
	bool                     lReadSucceed = false;
	double*                  lReadBuf = new double[3 * lVertexCount];

	if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
	{
		if ((lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get())) > -1)
		{
			lReadSucceed = lCache->Read(lChannelIndex, pTime, lReadBuf, lVertexCount);
		}
	}
	else // eMaxPointCacheV2
	{
		lReadSucceed = lCache->Read((unsigned int)pTime.GetFrameCount(), lReadBuf, lVertexCount);
	}

	if (lReadSucceed)
	{
		unsigned int lReadBufIndex = 0;

		while (lReadBufIndex < 3 * lVertexCount)
		{
			// In statements like "pVertexArray[lReadBufIndex/3].SetAt(2, lReadBuf[lReadBufIndex++])", 
			// on Mac platform, "lReadBufIndex++" is evaluated before "lReadBufIndex/3". 
			// So separate them.
			pVertexArray[lReadBufIndex / 3].mData[0] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[1] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
			pVertexArray[lReadBufIndex / 3].mData[2] = lReadBuf[lReadBufIndex]; lReadBufIndex++;
		}
	}

	delete[] lReadBuf;
}

void AnimateNode(ID3D11Device* pd3dDevice, VBOMesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
	FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose)
{
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

	if (lNodeAttribute)
	{
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			AnimateMesh(pd3dDevice, pMesh, pNode, pTime, pAnimLayer, pGlobalPosition, pPose);
		}
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			AnimateSkeleton(pNode, pParentGlobalPosition, pGlobalPosition);
		}
	}
}

void AnimateMesh(ID3D11Device* pd3dDevice, VBOMesh* pMesh, FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
	FbxAMatrix& pGlobalPosition, FbxPose* pPose)
{
	FbxMesh* lMesh = pNode->GetMesh();
	const int lVertexCount = lMesh->GetControlPointsCount();

	if (lVertexCount == 0)
		return;

	const VBOMesh * lMeshCache = static_cast<const VBOMesh *>(lMesh->GetUserDataPtr());

	const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
		(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
	const bool lHasShape = lMesh->GetShapeCount() > 0;
	const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
	const bool lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;

	FbxVector4* lVertexArray = NULL;
	if (!lMeshCache || lHasDeformation)
	{
		lVertexArray = new FbxVector4[lVertexCount];
		memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
	}

	if (lHasDeformation)
	{
		if (lHasVertexCache)
		{
			ReadVertexCacheData(lMesh, pTime, lVertexArray);
		}
		else
		{
			if (lHasShape)
			{
				// Deform the vertex array with the shapes.
				ComputeShapeDeformation(lMesh, pTime, pAnimLayer, lVertexArray);
			}

			// we need to get the number of clusters
			const int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);
			int lClusterCount = 0;
			for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
			{
				lClusterCount += ((FbxSkin *)(lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
			}
			if (lClusterCount)
			{
				ComputeSkinDeformation(pd3dDevice, pMesh, pGlobalPosition, lMesh, pTime, lVertexArray, pPose);
			}
		}
	}

	if (lVertexArray != NULL)
		delete[] lVertexArray;
}

void AnimateSkeleton(FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition)
{
}

// Deform the vertex array with the shapes contained in the mesh.
void ComputeShapeDeformation(FbxMesh* pMesh, FbxTime& pTime, FbxAnimLayer * pAnimLayer, FbxVector4* pVertexArray)
{
	int lVertexCount = pMesh->GetControlPointsCount();

	FbxVector4* lSrcVertexArray = pVertexArray;
	FbxVector4* lDstVertexArray = new FbxVector4[lVertexCount];
	memcpy(lDstVertexArray, pVertexArray, lVertexCount * sizeof(FbxVector4));

	int lBlendShapeDeformerCount = pMesh->GetDeformerCount(FbxDeformer::eBlendShape);
	for (int lBlendShapeIndex = 0; lBlendShapeIndex<lBlendShapeDeformerCount; ++lBlendShapeIndex)
	{
		FbxBlendShape* lBlendShape = (FbxBlendShape*)pMesh->GetDeformer(lBlendShapeIndex, FbxDeformer::eBlendShape);

		int lBlendShapeChannelCount = lBlendShape->GetBlendShapeChannelCount();
		for (int lChannelIndex = 0; lChannelIndex<lBlendShapeChannelCount; ++lChannelIndex)
		{
			FbxBlendShapeChannel* lChannel = lBlendShape->GetBlendShapeChannel(lChannelIndex);
			if (lChannel)
			{
				// Get the percentage of influence on this channel.
				FbxAnimCurve* lFCurve = pMesh->GetShapeChannel(lBlendShapeIndex, lChannelIndex, pAnimLayer);
				if (!lFCurve) continue;
				double lWeight = lFCurve->Evaluate(pTime);

				/*
				If there is only one targetShape on this channel, the influence is easy to calculate:
				influence = (targetShape - baseGeometry) * weight * 0.01
				dstGeometry = baseGeometry + influence

				But if there are more than one targetShapes on this channel, this is an in-between
				blendshape, also called progressive morph. The calculation of influence is different.

				For example, given two in-between targets, the full weight percentage of first target
				is 50, and the full weight percentage of the second target is 100.
				When the weight percentage reach 50, the base geometry is already be fully morphed
				to the first target shape. When the weight go over 50, it begin to morph from the
				first target shape to the second target shape.

				To calculate influence when the weight percentage is 25:
				1. 25 falls in the scope of 0 and 50, the morphing is from base geometry to the first target.
				2. And since 25 is already half way between 0 and 50, so the real weight percentage change to
				the first target is 50.
				influence = (firstTargetShape - baseGeometry) * (25-0)/(50-0) * 100
				dstGeometry = baseGeometry + influence

				To calculate influence when the weight percentage is 75:
				1. 75 falls in the scope of 50 and 100, the morphing is from the first target to the second.
				2. And since 75 is already half way between 50 and 100, so the real weight percentage change
				to the second target is 50.
				influence = (secondTargetShape - firstTargetShape) * (75-50)/(100-50) * 100
				dstGeometry = firstTargetShape + influence
				*/

				// Find the two shape indices for influence calculation according to the weight.
				// Consider index of base geometry as -1.

				int lShapeCount = lChannel->GetTargetShapeCount();
				double* lFullWeights = lChannel->GetTargetShapeFullWeights();

				// Find out which scope the lWeight falls in.
				int lStartIndex = -1;
				int lEndIndex = -1;
				for (int lShapeIndex = 0; lShapeIndex<lShapeCount; ++lShapeIndex)
				{
					if (lWeight > 0 && lWeight <= lFullWeights[0])
					{
						lEndIndex = 0;
						break;
					}
					if (lWeight > lFullWeights[lShapeIndex] && lWeight < lFullWeights[lShapeIndex + 1])
					{
						lStartIndex = lShapeIndex;
						lEndIndex = lShapeIndex + 1;
						break;
					}
				}

				FbxShape* lStartShape = NULL;
				FbxShape* lEndShape = NULL;
				if (lStartIndex > -1)
				{
					lStartShape = lChannel->GetTargetShape(lStartIndex);
				}
				if (lEndIndex > -1)
				{
					lEndShape = lChannel->GetTargetShape(lEndIndex);
				}

				//The weight percentage falls between base geometry and the first target shape.
				if (lStartIndex == -1 && lEndShape)
				{
					double lEndWeight = lFullWeights[0];
					// Calculate the real weight.
					lWeight = (lWeight / lEndWeight) * 100;
					// Initialize the lDstVertexArray with vertex of base geometry.
					memcpy(lDstVertexArray, lSrcVertexArray, lVertexCount * sizeof(FbxVector4));
					for (int j = 0; j < lVertexCount; j++)
					{
						// Add the influence of the shape vertex to the mesh vertex.
						FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lSrcVertexArray[j]) * lWeight * 0.01;

						lDstVertexArray[j] += lInfluence;
					}
				}
				//The weight percentage falls between two target shapes.
				else if (lStartShape && lEndShape)
				{
					double lStartWeight = lFullWeights[lStartIndex];
					double lEndWeight = lFullWeights[lEndIndex];
					// Calculate the real weight.
					lWeight = ((lWeight - lStartWeight) / (lEndWeight - lStartWeight)) * 100;
					// Initialize the lDstVertexArray with vertex of the previous target shape geometry.
					memcpy(lDstVertexArray, lStartShape->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
					for (int j = 0; j < lVertexCount; j++)
					{
						// Add the influence of the shape vertex to the previous shape vertex.
						FbxVector4 lInfluence = (lEndShape->GetControlPoints()[j] - lStartShape->GetControlPoints()[j]) * lWeight * 0.01;
						lDstVertexArray[j] += lInfluence;
					}
				}
			}//If lChannel is valid
		}//For each blend shape channel
	}//For each blend shape deformer

	memcpy(pVertexArray, lDstVertexArray, lVertexCount * sizeof(FbxVector4));

	delete[] lDstVertexArray;
}

void ComputeClusterDeformation(FbxAMatrix& pGlobalPosition,
	FbxMesh* pMesh,
	FbxCluster* pCluster,
	FbxAMatrix& pVertexTransformMatrix,
	FbxTime pTime,
	FbxPose* pPose)
{
	FbxCluster::ELinkMode lClusterMode = pCluster->GetLinkMode();

	FbxAMatrix lReferenceGlobalInitPosition;
	FbxAMatrix lReferenceGlobalCurrentPosition;
	FbxAMatrix lAssociateGlobalInitPosition;
	FbxAMatrix lAssociateGlobalCurrentPosition;
	FbxAMatrix lClusterGlobalInitPosition;
	FbxAMatrix lClusterGlobalCurrentPosition;

	FbxAMatrix lReferenceGeometry;
	FbxAMatrix lAssociateGeometry;
	FbxAMatrix lClusterGeometry;

	FbxAMatrix lClusterRelativeInitPosition;
	FbxAMatrix lClusterRelativeCurrentPositionInverse;

	if (lClusterMode == FbxCluster::eAdditive && pCluster->GetAssociateModel())
	{
		pCluster->GetTransformAssociateModelMatrix(lAssociateGlobalInitPosition);
		// Geometric transform of the model
		lAssociateGeometry = GetGeometry(pCluster->GetAssociateModel());
		lAssociateGlobalInitPosition *= lAssociateGeometry;
		lAssociateGlobalCurrentPosition = GetGlobalPosition(pCluster->GetAssociateModel(), pTime, pPose);

		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;
		lReferenceGlobalCurrentPosition = pGlobalPosition;

		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		// Multiply lClusterGlobalInitPosition by Geometric Transformation
		lClusterGeometry = GetGeometry(pCluster->GetLink());
		lClusterGlobalInitPosition *= lClusterGeometry;
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);

		// Compute the shift of the link relative to the reference.
		//ModelM-1 * AssoM * AssoGX-1 * LinkGX * LinkM-1*ModelM
		pVertexTransformMatrix = lReferenceGlobalInitPosition.Inverse() * lAssociateGlobalInitPosition * lAssociateGlobalCurrentPosition.Inverse() *
			lClusterGlobalCurrentPosition * lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;
	}
	else
	{
		pCluster->GetTransformMatrix(lReferenceGlobalInitPosition);
		lReferenceGlobalCurrentPosition = pGlobalPosition;
		// Multiply lReferenceGlobalInitPosition by Geometric Transformation
		lReferenceGeometry = GetGeometry(pMesh->GetNode());
		lReferenceGlobalInitPosition *= lReferenceGeometry;

		// Get the link initial global position and the link current global position.
		pCluster->GetTransformLinkMatrix(lClusterGlobalInitPosition);
		lClusterGlobalCurrentPosition = GetGlobalPosition(pCluster->GetLink(), pTime, pPose);

		// Compute the initial position of the link relative to the reference.
		lClusterRelativeInitPosition = lClusterGlobalInitPosition.Inverse() * lReferenceGlobalInitPosition;

		// Compute the current position of the link relative to the reference.
		lClusterRelativeCurrentPositionInverse = lReferenceGlobalCurrentPosition.Inverse() * lClusterGlobalCurrentPosition;

		// Compute the shift of the link relative to the reference.
		pVertexTransformMatrix = lClusterRelativeCurrentPositionInverse * lClusterRelativeInitPosition;
	}
}

// Deform the vertex array according to the links contained in the mesh and the skinning type.
void ComputeSkinDeformation(ID3D11Device* pd3dDevice, VBOMesh* pObjectMesh, FbxAMatrix& pGlobalPosition, FbxMesh* pMesh,
	FbxTime& pTime, FbxVector4* pVertexArray, FbxPose* pPose)
{
	FbxSkin* lSkinDeformer = (FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin);
	FbxSkin::EType lSkinningType = lSkinDeformer->GetSkinningType();

	FbxCluster::ELinkMode lClusterMode = ((FbxSkin*)pMesh->GetDeformer(0, FbxDeformer::eSkin))->GetCluster(0)->GetLinkMode();

	int lVertexCount = pMesh->GetControlPointsCount();

	XMMATRIX* lClusterDeformation = new XMMATRIX[lVertexCount];
	memset(lClusterDeformation, 0, lVertexCount*sizeof(XMMATRIX));

	double* lClusterWeight = new double[lVertexCount];
	memset(lClusterWeight, 0, lVertexCount*sizeof(double));

	if (lClusterMode == FbxCluster::eAdditive)
	{
		for (int i = 0; i < lVertexCount; ++i)
			lClusterDeformation[i] = XMMatrixIdentity();
	}

	int lSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
	for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
	{
		FbxSkin* lSkinDeformer = (FbxSkin *)pMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin);

		int lClusterCount = lSkinDeformer->GetClusterCount();
		for (int lClusterIndex = 0; lClusterIndex < lClusterCount; ++lClusterIndex)
		{
			FbxCluster* lCluster = lSkinDeformer->GetCluster(lClusterIndex);
			if (!lCluster->GetLink())
				continue;

			FbxAMatrix lVertexTransformMatrix;
			ComputeClusterDeformation(pGlobalPosition, pMesh, lCluster, lVertexTransformMatrix, pTime, pPose);

			int lVertexIndexCount = lCluster->GetControlPointIndicesCount();
			for (int k = 0; k < lVertexIndexCount; ++k)
			{
				int lIndex = lCluster->GetControlPointIndices()[k];
				if (lIndex >= lVertexCount)
					continue;

				double lWeight = lCluster->GetControlPointWeights()[k];
				if (lWeight == 0.0)
					continue;

				XMFLOAT4X4 xmf4x4Temp;// = lVertexTransformMatrix;

				for (int i = 0; i < 4; ++i)
				{
					for (int j = 0; j < 4; ++j)
					{
						xmf4x4Temp.m[i][j] = lVertexTransformMatrix[i][j] * lWeight;
						//lClusterDeformation[lIndex].m[i][j] += lVertexTransformMatrix[i][j] * lWeight;
					}
				}
				lClusterDeformation[lIndex] += XMLoadFloat4x4(&xmf4x4Temp);
				lClusterWeight[lIndex] += lWeight;
			}
		}
	}

	XMFLOAT3* ppVertexArray = new XMFLOAT3[lVertexCount];
	for (UINT i = 0; i < lVertexCount; ++i)
	{
		ppVertexArray[i].x = pVertexArray[i][0];
		ppVertexArray[i].y = pVertexArray[i][1];
		ppVertexArray[i].z = pVertexArray[i][2];
	}

	//ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
	//ID3D11DeviceContext* pd3dDeviceContext = DXUTGetD3D11DeviceContext();

	for (int i = 0; i < lVertexCount; ++i)
	{
		if (lClusterWeight[i] != 0.0)
		{
			XMFLOAT3 vertexVector;
			vertexVector.x = pVertexArray[i][0];
			vertexVector.y = pVertexArray[i][1];
			vertexVector.z = pVertexArray[i][2];

			XMFLOAT3 tempVector = vertexVector;
			//pVertexArray[i] = lClusterDeformation[i].MultT(pVertexArray[i]);

			//ppVertexArray[i].x = (lClusterDeformation[i].r[0][0] * tempVector.x +
			//	lClusterDeformation[i].r[1][0] * tempVector.y +
			//	lClusterDeformation[i].r[2][0] * tempVector.z +
			//	lClusterDeformation[i].r[3][0]);

			//ppVertexArray[i].y = (lClusterDeformation[i].m[0][1] * tempVector.x +
			//	lClusterDeformation[i].m[1][1] * tempVector.y +
			//	lClusterDeformation[i].m[2][1] * tempVector.z +
			//	lClusterDeformation[i].m[3][1]);

			//ppVertexArray[i].z = (lClusterDeformation[i].m[0][2] * tempVector.x +
			//	lClusterDeformation[i].m[1][2] * tempVector.y +
			//	lClusterDeformation[i].m[2][2] * tempVector.z +
			//	lClusterDeformation[i].m[3][2]);

		}
	}

	delete[] lClusterDeformation;
	delete[] lClusterWeight;

	VERTEX* nVertex = &pObjectMesh->mVertex[0];

	//BoundingBox			m_BoundingBox;
	// Convert to the same sequence with data in GPU.

	//if (pObjectMesh->GetAllByControlPoint())
	//{
	//	lVertexCount = pMesh->GetControlPointsCount();

	//	for (int lIndex = 0; lIndex < lVertexCount; ++lIndex)
	//	{
	//		nVertex[lIndex].position = ppVertexArray[lIndex];
	//		nVertex[lIndex].normal.x = pObjectMesh->mNormals[lIndex].x;
	//		nVertex[lIndex].normal.y = pObjectMesh->mNormals[lIndex].y;
	//		nVertex[lIndex].normal.z = pObjectMesh->mNormals[lIndex].z;
	//		nVertex[lIndex].texcoord.x = pObjectMesh->mTexCoords[lIndex].x;
	//		nVertex[lIndex].texcoord.y = pObjectMesh->mTexCoords[lIndex].y;

	//		if (m_BoundingBox.m_fMinX > nVertex[lIndex].position.x)
	//			m_BoundingBox.m_fMinX = nVertex[lIndex].position.x;
	//		if (m_BoundingBox.m_fMinY > nVertex[lIndex].position.y)
	//			m_BoundingBox.m_fMinY = nVertex[lIndex].position.y;
	//		if (m_BoundingBox.m_fMinZ > nVertex[lIndex].position.z)
	//			m_BoundingBox.m_fMinZ = nVertex[lIndex].position.z;
	//		if (m_BoundingBox.m_fMaxX < nVertex[lIndex].position.x)
	//			m_BoundingBox.m_fMaxX = nVertex[lIndex].position.x;
	//		if (m_BoundingBox.m_fMaxY < nVertex[lIndex].position.y)
	//			m_BoundingBox.m_fMaxY = nVertex[lIndex].position.y;
	//		if (m_BoundingBox.m_fMaxZ < nVertex[lIndex].position.z)
	//			m_BoundingBox.m_fMaxZ = nVertex[lIndex].position.z;

	//	}
	//}
	//else
	//{
	//	const int lPolygonCount = pMesh->GetPolygonCount();
	//	lVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;

	//	lVertexCount = 0;
	//	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	//	{
	//		for (int lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
	//		{
	//			const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);

	//			nVertex[lVertexCount].position = ppVertexArray[lControlPointIndex];
	//			nVertex[lVertexCount].normal.x = pObjectMesh->mNormals[lVertexCount].x;
	//			nVertex[lVertexCount].normal.y = pObjectMesh->mNormals[lVertexCount].y;
	//			nVertex[lVertexCount].normal.z = pObjectMesh->mNormals[lVertexCount].z;
	//			nVertex[lVertexCount].texcoord.x = pObjectMesh->mTexCoords[lVertexCount].x;
	//			nVertex[lVertexCount].texcoord.y = pObjectMesh->mTexCoords[lVertexCount].y;
	//			if (m_BoundingBox.m_fMinX > nVertex[lVertexCount].position.x)
	//				m_BoundingBox.m_fMinX = nVertex[lVertexCount].position.x;
	//			if (m_BoundingBox.m_fMinY > nVertex[lVertexCount].position.y)
	//				m_BoundingBox.m_fMinY = nVertex[lVertexCount].position.y;
	//			if (m_BoundingBox.m_fMinZ > nVertex[lVertexCount].position.z)
	//				m_BoundingBox.m_fMinZ = nVertex[lVertexCount].position.z;
	//			if (m_BoundingBox.m_fMaxX < nVertex[lVertexCount].position.x)
	//				m_BoundingBox.m_fMaxX = nVertex[lVertexCount].position.x;
	//			if (m_BoundingBox.m_fMaxY < nVertex[lVertexCount].position.y)
	//				m_BoundingBox.m_fMaxY = nVertex[lVertexCount].position.y;
	//			if (m_BoundingBox.m_fMaxZ < nVertex[lVertexCount].position.z)
	//				m_BoundingBox.m_fMaxZ = nVertex[lVertexCount].position.z;
	//			++lVertexCount;
	//		}
	//	}
	//}

	delete[] ppVertexArray;

	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));

	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(VERTEX) * lVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	ZeroMemory(&vinitData, sizeof(vinitData));

	vinitData.pSysMem = &nVertex[0];

	ID3D11Buffer* pVertexBuffer;
	pd3dDevice->CreateBuffer(&vbd, &vinitData, &pVertexBuffer);

	pObjectMesh->PushBuffer(pVertexBuffer);
	pVertexBuffer = NULL;

	VERTEX boundingBoxVertices[8];

	//boundingBoxVertices[0].position = XMFLOAT3(m_BoundingBox.m_fMinX, m_BoundingBox.m_fMaxY, m_BoundingBox.m_fMinZ);
	//boundingBoxVertices[1].position = XMFLOAT3(m_BoundingBox.m_fMaxX, m_BoundingBox.m_fMaxY, m_BoundingBox.m_fMinZ);
	//boundingBoxVertices[2].position = XMFLOAT3(m_BoundingBox.m_fMaxX, m_BoundingBox.m_fMaxY, m_BoundingBox.m_fMaxZ);
	//boundingBoxVertices[3].position = XMFLOAT3(m_BoundingBox.m_fMinX, m_BoundingBox.m_fMaxY, m_BoundingBox.m_fMaxZ);
	//boundingBoxVertices[4].position = XMFLOAT3(m_BoundingBox.m_fMinX, m_BoundingBox.m_fMinY, m_BoundingBox.m_fMinZ);
	//boundingBoxVertices[5].position = XMFLOAT3(m_BoundingBox.m_fMaxX, m_BoundingBox.m_fMinY, m_BoundingBox.m_fMinZ);
	//boundingBoxVertices[6].position = XMFLOAT3(m_BoundingBox.m_fMaxX, m_BoundingBox.m_fMinY, m_BoundingBox.m_fMaxZ);
	//boundingBoxVertices[7].position = XMFLOAT3(m_BoundingBox.m_fMinX, m_BoundingBox.m_fMinY, m_BoundingBox.m_fMaxZ);


	D3D11_BUFFER_DESC d3dBufferDesc;
	ZeroMemory(&d3dBufferDesc, sizeof(D3D11_BUFFER_DESC));
	d3dBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	d3dBufferDesc.ByteWidth = sizeof(VERTEX) * 8;
	d3dBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	d3dBufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA d3dBufferData;
	ZeroMemory(&d3dBufferData, sizeof(D3D11_SUBRESOURCE_DATA));
	d3dBufferData.pSysMem = &boundingBoxVertices[0];

	ID3D11Buffer* boundingBuffer = NULL;
	pd3dDevice->CreateBuffer(&d3dBufferDesc, &d3dBufferData, &boundingBuffer);
	pObjectMesh->PushBoundingBoxBuffer(boundingBuffer);
	boundingBuffer = NULL;
}