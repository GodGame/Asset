#include "FBXParser.h"

ostream & operator<<(ostream & os, FbxDouble4 & vt)
{
	os << vt.mData[0] << ", " << vt.mData[1] << ", " << vt.mData[2] << ", " << vt.mData[3];
	return os;
}

ostream& operator<<(ostream& os, FbxVector4 & vt)
{
	os << vt.mData[0] << ", " << vt.mData[1] << ", " << vt.mData[2] << ", " << vt.mData[3];
	return os;
}

ostream& operator<<(ostream& os, XMFLOAT3 & vt)
{
	os << vt.x << ", " << vt.y << ", " << vt.z;
	return os;
}

ostream& operator<<(ostream& os, XMFLOAT2 & vt)
{
	os << vt.x << ", " << vt.y;
	return os;
}


FBXParser::FBXParser()
{
	m_pMgr      = nullptr;
	m_pScene    = nullptr;
	m_pRootNode = nullptr;

	m_pImporter = nullptr;
	m_pAnimLayer = nullptr;
	//m_ppControlPos = nullptr;
	//m_ppPos = nullptr;
	//m_ppNormal = nullptr;

	//m_nObjects  = 0;

//	m_pvcVertex = nullptr;
}


FBXParser::~FBXParser()
{
	DESTROY(m_pMgr);
//	DESTROY(m_pScene);
//	DESTROY(m_pImporter);

	//if (m_ppControlPos)
	//{
	//	for (int i = 0; i < m_nObjects; ++i)
	//		if (m_ppControlPos[i]) delete [] m_ppControlPos[i];
	//	delete [] m_ppControlPos;
	//}
	//if (m_ppPos)
	//{
	//	for (int i = 0; i < m_nObjects; ++i)
	//		if (m_ppPos[i]) delete[] m_ppPos[i];
	//	delete[] m_ppPos;
	//}
	//if (m_ppNormal)
	//{
	//	for (int i = 0; i < m_nObjects; ++i)
	//		if (m_ppNormal[i]) delete[] m_ppNormal[i];
	//	delete[] m_ppNormal;
	//}

	for (auto it = m_vcAnimData.begin(); it != m_vcAnimData.end(); ++it)
	{
		delete *it;
	}
	m_vcAnimData.clear();
}

bool FBXParser::Initialize(const char * pstr)
{
	m_stName = pstr;

	m_pMgr = FbxManager::Create();
	FbxIOSettings * pfbxIOSettings = FbxIOSettings::Create(m_pMgr, IOSROOT);
	m_pMgr->SetIOSettings(pfbxIOSettings);

	pfbxIOSettings->SetBoolProp(IMP_FBX_CHARACTER, true);
	pfbxIOSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
	pfbxIOSettings->SetBoolProp(IMP_FBX_TEXTURE, true);

	m_pImporter = FbxImporter::Create(m_pMgr, "");
	const bool lImportStatus = m_pImporter->Initialize(pstr, -1, m_pMgr->GetIOSettings());
	if (!m_pImporter->IsFBX() || !lImportStatus)
		return false;

	m_pScene = FbxScene::Create(m_pMgr, "My Scene");
	//pfbxMgr->GetIOSettings()->SetBoolProp(IMP_FBX_CHARACTER, true);

	bool bCheck = m_pImporter->Import(m_pScene);
	if (!bCheck) return false;

	return true;
}

void FBXParser::Run()
{
	Setting();

	//VertexRead();
	for (int i = 0; i < m_obj.m_pFbxMeshes.size(); ++i)
		MeshRead(i);

	TextureRead();

	SetAnimation();
	LoadAnimation();

	FileOutObject();
}

void FBXParser::Setting()
{
	FbxAxisSystem SceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
	//FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
	//if (SceneAxisSystem != OurAxisSystem)
	//{
	//	OurAxisSystem.ConvertScene(m_pScene);
	//}
	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(m_pMgr);
	lGeomConverter.Triangulate(m_pScene, true);

	// split meshes per material, so that we only have one material per mesh (for VBO support)
	lGeomConverter.SplitMeshesPerMaterial(m_pScene, true);

	m_pRootNode = m_pScene->GetRootNode();
	int nObjects = m_pRootNode->GetSrcObjectCount();

	for (int i = 0; i < nObjects; ++i)
	{
		FbxObject * pfbxObject = m_pRootNode->GetSrcObject(i);
		cout << "NAME : " << pfbxObject->GetName() << endl;

		FbxNode * pNode = m_pRootNode->GetChild(i);
		FbxNodeAttribute::EType type = (pNode->GetNodeAttribute())->GetAttributeType();

		if (type == FbxNodeAttribute::eMesh)
		{
			m_obj.m_pFbxMeshes.push_back(pNode->GetMesh());
			m_obj.m_vcMeshes.push_back(Mesh());
		}
	}
}

void FBXParser::VertexRead()
{
	for (int i  = 0; i < m_obj.m_pFbxMeshes.size(); ++i)
	{
		FbxMesh * pMesh = m_obj.m_pFbxMeshes[i];

		UINT vertexCounter = 0;
		UINT mTriangleCount = pMesh->GetPolygonCount();
		m_obj.m_vcMeshes[i].nTriangles = mTriangleCount;

		FbxVector4 * lControlPoints = pMesh->GetControlPoints();
		Vertex TempVertex;

		for (int j = 0; j < mTriangleCount; ++j)
		{
			for (int k = 0; k < 3; ++k)
			{
				ZeroMemory(&TempVertex, sizeof(TempVertex));

				int ctrlPointIndex = pMesh->GetPolygonVertex(j, k);
				FbxVector4 ctrlPoint = lControlPoints[ctrlPointIndex];

				NormalRead(pMesh, ctrlPointIndex, vertexCounter, TempVertex.xmf3Normal, TempVertex.xmf3Tangent);
				UVRead(pMesh, ctrlPointIndex, pMesh->GetTextureUVIndex(j, k), TempVertex.xmf2TexCoord);
				//pMesh->GetTextureUV()

				TempVertex.xmf3Pos = XMFLOAT3(ctrlPoint.mData[0], ctrlPoint.mData[1], ctrlPoint.mData[2]);


				m_obj.m_vcMeshes[i].m_vcVertexes.push_back(TempVertex);

				//cout << TempVertex.xmf3Pos << " // " << TempVertex.xmf3Normal << " // " << TempVertex.xmf2TexCoord << endl;
				++vertexCounter;
			}
			//m_ppPos[i][j].x = lControlPoints[j].mData[0];
			//m_ppPos[i][j].y = lControlPoints[j].mData[1];
			//m_ppPos[i][j].z = lControlPoints[j].mData[2];
			//m_ppControlPos[i][j].w = lControlPoints[j].mData[3];

			//cout << m_ppPos[i][j] << endl;
		}
	}
}
void FBXParser::TextureRead()
{
	const int lTextureCount = m_pScene->GetTextureCount();
	wstring * wstrNameArrays = new wstring[lTextureCount];
	UINT nExtraIndex = 0;

	for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
	{
		FbxTexture * lTexture = m_pScene->GetTexture(lTextureIndex);
		FbxFileTexture * lFileTexture = FbxCast<FbxFileTexture>(lTexture);
		if (lFileTexture && !lFileTexture->GetUserDataPtr())
		{
			LPCSTR FileName = lFileTexture->GetFileName();
			char localFilePath[256];
			ZeroMemory(localFilePath, 256);
			//strncpy(localFilePath, "Texture/", 8);

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
				localFilePath[0 + len - i] = temp[i];

			wchar_t szTexFilename[256];
			//CharToWChar(localFilePath, szTexFilename);
			mbstowcs(szTexFilename, localFilePath, strlen(localFilePath) + 1);
			wstring wstrName = szTexFilename;

			// 디퓨즈, 노말, 스펙큘러 텍스쳐 이미지가 아니면 3번부터 박는다.
			eTextureType eType = CheckTextureType(wstrName);
			if (eType == eTextureType::NONE)
			{
				wstrNameArrays[static_cast<int>(eTextureType::SPECULAR) + ++nExtraIndex] = wstrName;
			}
			else
			{
				wstrNameArrays[static_cast<int>(eType)] = wstrName;
			}
			
		}
	}
	for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
	{
		m_obj.m_vcTextureNames.push_back(wstrNameArrays[lTextureIndex]);
	}
	delete[] wstrNameArrays;
}


eTextureType FBXParser::CheckTextureType(const wstring & wstrName)
{
	auto ItCheckPoint = wstrName.end() - 1;
	UINT offset = 0;
	while (*ItCheckPoint != _T('_'))
	{
		--ItCheckPoint;
		//++offset;
	}

//	wstrName.find_last_of

	const wstring wstrDiffuse  = _T("_diff");
	const wstring wstrNormal   = _T("_norm");
	const wstring wstrSpecular = _T("_spec");
	//const wstring wstrGlow	   = _T("_glow");

	bool bHash[3] = { true, true, true };

	auto it = ItCheckPoint;
	for (int i = 0; i < 5; ++i, ++it, ++offset)//*it != _T('.'); ++it, ++offset)
	{
		if (bHash[eTextureType::DIFFUSE]  && *it != *(wstrDiffuse.begin()  + offset)) bHash[eTextureType::DIFFUSE]  = false;
		if (bHash[eTextureType::NORMAL]   && *it != *(wstrNormal.begin()   + offset)) bHash[eTextureType::NORMAL]   = false;
		if (bHash[eTextureType::SPECULAR] && *it != *(wstrSpecular.begin() + offset)) bHash[eTextureType::SPECULAR] = false;
//		if (bHash[eTextureType::GLOW]     && *it != *(wstrGlow.begin()     + offset)) bHash[eTextureType::GLOW]    = false;
	}

	for (int i = 0; i < 3; ++i)
		if (bHash[i]) 
			return eTextureType(i);

	return eTextureType::NONE;
}
//
//void FBXParser::ControlPointRead()
//{
//	m_ppPos = new XMFLOAT3*[m_nObjects];
//
//	for (int i = 0; i < m_nObjects; ++i)
//	{
//		m_ppControlPos[i] = nullptr;
//
//		FbxNode * pNode = m_pRootNode->GetChild(i);
//		FbxNodeAttribute::EType type = (pNode->GetNodeAttribute())->GetAttributeType();
//
//		if (type == FbxNodeAttribute::eMesh)
//		{
//			FbxMesh* pMesh              = pNode->GetMesh();
//			FbxVector4 * lControlPoints = pMesh->GetControlPoints();
//			UINT nControlPoints         = pMesh->GetControlPointsCount();
//			m_ppControlPos[i]           = new XMFLOAT3[nControlPoints];
//
//			for (int j = 0; j < nControlPoints; ++j)
//			{
//				m_ppControlPos[i][j].x = lControlPoints[j].mData[0];
//				m_ppControlPos[i][j].y = lControlPoints[j].mData[1];
//				m_ppControlPos[i][j].z = lControlPoints[j].mData[2];
//				//m_ppControlPos[i][j].w = lControlPoints[j].mData[3];
//
//				cout << m_ppControlPos[i][j] << " // ";
// 				cout << lControlPoints[j] << endl;
//			}
//
//			FbxVector4 lCurrentVertex;
//			FbxVector4 lCurrentNormal;
//			FbxVector2 lCurrentUV;
//
//		}
//	}
//}

void FBXParser::FileOut()
{
	wstring FileName;

	for (auto it = m_stName.begin(); it != m_stName.end() - 4; ++it)
	{
		FileName.push_back(*it);
	}

	{
		wstring infoName;
		//sprintf(fileName, "%ws.info", filename);
		infoName = FileName + _T(".info");
		wofstream out(infoName, ios::out);

		UINT nTextures = m_obj.m_vcTextureNames.size();

		out << "Textures : " << nTextures << endl;
		for (int i = 0; i < nTextures; ++i)
		{
			for (auto it = m_obj.m_vcTextureNames.begin(); it != m_obj.m_vcTextureNames.end(); ++it)
			{
				for (auto wcit = it->begin(); wcit != it->end(); ++wcit)
					out << *wcit;
			}
			out << endl;
		}
		out << endl << endl;

		UINT nMeshes = m_obj.m_vcMeshes.size();

		out << "All Objects : " << nMeshes << endl;

		for (int i = 0; i < nMeshes; ++i)
		{
			out << "Object " << i << " : " << m_obj.m_vcMeshes[i].m_vcVertexes.size() << endl;

			for (auto it = m_obj.m_vcMeshes[i].m_vcVertexes.begin(); it != m_obj.m_vcMeshes[i].m_vcVertexes.end(); ++it)
			{
				out << "Pos : " << it->xmf3Pos.x << ", " << it->xmf3Pos.y << it->xmf3Pos.z << "\t";
				out << "Tex : " << it->xmf2TexCoord.x << ", " << it->xmf2TexCoord.y << "\t";
				out << "Normal : " << it->xmf3Normal.x << ", " << it->xmf3Normal.y << ", " << it->xmf3Normal.z << "\t";
				out << "Tangent : " << it->xmf3Tangent.x << ", " << it->xmf3Tangent.y << ", " << it->xmf3Tangent.z << "\t";
			}
			out << endl << endl;
		}
		out.close();
	}
	{
		wstring binaryName = FileName + _T(".fbxcjh");
		char name[100];
		int index = 0;
		for (auto it = binaryName.begin(); it != binaryName.end(); ++it)
		{
			name[index++] = (*it);
		}
		name[index] = '\0';

		FILE * bin;
		bin = fopen(name, "wb");

		UINT nMeshes = m_obj.m_vcMeshes.size();

		fwrite(&nMeshes, sizeof(UINT), 1, bin);

		for (int i = 0; i < nMeshes; ++i)
		{
			UINT sz = m_obj.m_vcMeshes[i].m_vcVertexes.size();
			fwrite(&sz, sizeof(UINT), 1, bin);
		}

		for (int i = 0; i < nMeshes; ++i)
		{
		//	UINT sz = m_pvcVertex[i].size();
		//	fwrite(&sz, sizeof(UINT), 1, bin);

			for (auto it = m_obj.m_vcMeshes[i].m_vcVertexes.begin(); it != m_obj.m_vcMeshes[i].m_vcVertexes.end(); ++it)
			{
				fwrite(&it->xmf3Pos, sizeof(XMFLOAT3), 1, bin);
				fwrite(&it->xmf2TexCoord, sizeof(XMFLOAT2), 1, bin);
				fwrite(&it->xmf3Normal, sizeof(XMFLOAT3), 1, bin);
				fwrite(&it->xmf3Tangent, sizeof(XMFLOAT3), 1, bin);
			}
		}



		//sz = indices.size();
		//fwrite(&sz, sizeof(UINT), 1, bin);
		////bin << indices.size();

		//for (auto it = indices.begin(); it != indices.end(); ++it)
		//{
		//	fwrite(&(*it), sizeof(UINT), 1, bin);
		//	//bin << (*it);
		//}
		fclose(bin);
	}
}

void FBXParser::FileOutObject()
{
	wstring FileName;
	bool bUseTangent = false;

	for (auto it = m_stName.begin(); it != m_stName.end() - 4; ++it)
	{
		FileName.push_back(*it);
	}

	if (m_obj.m_vcMeshes.size() > 0)
	{
		if (m_obj.m_vcMeshes[0].m_vcVertexes[0].xmf3Tangent.x != 0 && m_obj.m_vcMeshes[0].m_vcVertexes[0].xmf3Tangent.y != 0 && m_obj.m_vcMeshes[0].m_vcVertexes[0].xmf3Tangent.z != 0)
		{
			bUseTangent = true;
		}
	}

	{
		wstring infoName;
		//sprintf(fileName, "%ws.info", filename);
		infoName = FileName + _T(".info");
		wofstream out(infoName, ios::out);

		UINT nTextures = m_obj.m_vcTextureNames.size();

		out << "Textures : " << nTextures << endl;
		for (auto it = m_obj.m_vcTextureNames.begin(); it != m_obj.m_vcTextureNames.end(); ++it)
		{
			for (auto wcit = it->begin(); wcit != it->end(); ++wcit)
				out << *wcit;
			out << endl;
		}
		out << endl << endl;

		
		UINT nMeshes = m_obj.m_vcMeshes.size();

		out << "All Objects : " << nMeshes << endl;

		for (int i = 0; i < nMeshes; ++i)
		{
			out << "Object " << i << " : " << m_obj.m_vcMeshes[i].m_vcVertexes.size() << endl;

			for (auto it = m_obj.m_vcMeshes[i].m_vcVertexes.begin(); it != m_obj.m_vcMeshes[i].m_vcVertexes.end(); ++it)
			{
				out << "Pos : " << it->xmf3Pos.x << ", " << it->xmf3Pos.y << it->xmf3Pos.z << "\t";
				out << "Tex : " << it->xmf2TexCoord.x << ", " << it->xmf2TexCoord.y << "\t";
				out << "Normal : " << it->xmf3Normal.x << ", " << it->xmf3Normal.y << ", " << it->xmf3Normal.z << "\t";
				if (bUseTangent) out << "Tangent : " << it->xmf3Tangent.x << ", " << it->xmf3Tangent.y << ", " << it->xmf3Tangent.z << "\t";
				out << endl;
			}
			out << endl;

			out << "Index 총 " << m_obj.m_vcMeshes[i].m_stIndices.size() << " 개" << endl;
			for (auto it = m_obj.m_vcMeshes[i].m_stIndices.begin(); it != m_obj.m_vcMeshes[i].m_stIndices.end(); ++it)
			{
				out << *it << " ";
			}
			out << endl << endl;
		}

		out.close();
	}
	{
		wstring binaryName = FileName + _T(".fbxcjh");
		char name[100];
		int index = 0;
		for (auto it = binaryName.begin(); it != binaryName.end(); ++it)
		{
			name[index++] = (*it);
		}
		name[index] = '\0';

		FILE * bin;
		bin = fopen(name, "wb");

		UINT nTextures = m_obj.m_vcTextureNames.size();
		fwrite(&nTextures, sizeof(UINT), 1, bin);

		for (int i = 0; i < nTextures; ++i)
		{
			UINT sz = m_obj.m_vcTextureNames[i].size();
			fwrite(&sz, sizeof(UINT), 1, bin);
			fwrite(&(m_obj.m_vcTextureNames[i][0]), sizeof(wchar_t), m_obj.m_vcTextureNames[i].size(), bin);
		}

		UINT nMeshes = m_obj.m_vcMeshes.size();
		fwrite(&nMeshes, sizeof(UINT), 1, bin);

		for (int i = 0; i < nMeshes; ++i)
		{
			UINT sz = m_obj.m_vcMeshes[i].m_vcVertexes.size();
			fwrite(&sz, sizeof(UINT), 1, bin);
		}

		for (int i = 0; i < nMeshes; ++i)
		{
			for (auto it = m_obj.m_vcMeshes[i].m_vcVertexes.begin(); it != m_obj.m_vcMeshes[i].m_vcVertexes.end(); ++it)
			{
				fwrite(&it->xmf3Pos, sizeof(XMFLOAT3), 1, bin);
				fwrite(&it->xmf2TexCoord, sizeof(XMFLOAT2), 1, bin);
				fwrite(&it->xmf3Normal, sizeof(XMFLOAT3), 1, bin);
				fwrite(&it->xmf3Tangent, sizeof(XMFLOAT3), 1, bin);
			}
		}
		fclose(bin);
	}
}

void FBXParser::LoadAnimation()
{

	for (FbxTime nTime = m_tStart; nTime <= m_tStop; nTime += m_tFrameTime)
	{
		cout << "Time : " << nTime.GetMilliSeconds() << endl;
		cout << "Child Num : " << m_pRootNode->GetSrcObjectCount() << endl;

		for (int i = 0; i < m_pRootNode->GetSrcObjectCount(); ++i)
		{
			FbxObject * pfbxObject = m_pRootNode->GetSrcObject(i);
			FbxNode * pThisNode = m_pRootNode->GetChild(i);

			GetChildAnimation(pThisNode, nTime);
		}
		//CreateVBBuffer(pd3dDevice);
	}

	//Init(pd3dDevice);

	//printf("%d번째 캐릭터 애니메이션 총 개수 = %d 정점 개수 = %d 인덱스 개수 = %d\n", test++, GetTotalAnimationCount(), GetTotalVertCount(), GetTotalIndexCount());


	//for (auto i = 0; i < GetMeshSize(); ++i)
	//{
	//	for (int j = 0; j < GetVBOMesh(i)->mSubMeshes.GetCount(); ++j)
	//		GetVBOMesh(i)->mSubMeshes.RemoveIt(GetVBOMesh(i)->mSubMeshes[j]);

	//	GetVBOMesh(i)->mSubMeshes.Clear();

	//	GetVBOMesh(i)->mNormals.clear();
	//	GetVBOMesh(i)->mTexCoords.clear();
	//}

	//FbxArrayDelete(mAnimStackNameArray);
	//FbxArrayDelete(mPoseArray);
	//if (mCurrentAnimLayer)
	//{
	//	mCurrentAnimLayer->Clear();
	//	mCurrentAnimLayer->Destroy();
	//}
}

void FBXParser::GetChildAnimation(FbxNode * pParentNode, FbxTime nTime)
{
	if (pParentNode == nullptr) return;

	FbxAMatrix lDummyGlobalPosition[20];
	vector<FbxNode*> pNode;

	FbxAMatrix lGeometryOffset[20];
	FbxAMatrix lGlobalOffPosition[20];
	FbxPose * lPose[20];
	for (UINT i = 0; i < 20; ++i)
	{
		lPose[i] = NULL;
	}

	int nObjects = pParentNode->GetSrcObjectCount();
	cout << "자식의 개수 : " << nObjects << endl;

	for (int i = 0; i < nObjects; ++i)
	{
		FbxObject * pfbxObject = pParentNode->GetSrcObject(i);
		FbxNode * pChildNode = pParentNode->GetChild(i);
		if (pChildNode == nullptr) continue;

		GetChildAnimation(pChildNode, nTime);

		FbxMesh * pMesh = nullptr;//pChildNode->GetMesh();

		//			VBOMesh* pMesh = GetVBOMesh(i);
		pNode.push_back(pChildNode);

		lPose[i] = NULL;
		FbxAMatrix lGlobalPosition = GetGlobalPosition(pNode[i], nTime, lPose[i], &lDummyGlobalPosition[i]);

		FbxNodeAttribute* lNodeAttribute = pNode[i]->GetNodeAttribute();

		if (lNodeAttribute)
		{
			// Geometry offset.
			// it is not inherited by the children.
			lGeometryOffset[i] = GetGeometry(pNode[i]);
			lGlobalOffPosition[i] = lGlobalPosition * lGeometryOffset[i];

			AnimateNode(pMesh, pNode[i], nTime, /*mCurrentAnimLayer*/m_pAnimLayer, lDummyGlobalPosition[i], lGlobalOffPosition[i], lPose[i]);
		}
	}

	for (auto it = pNode.begin(); it != pNode.end(); ++it)
	{
		(*it)->Destroy();
	}

	for (UINT i = 0; i < 20; ++i)
	{
		if (lPose[i] != NULL) lPose[i]->Destroy();
	}
	
}

void FBXParser::SetAnimation()
{
	m_tFrameTime.SetTime(0, 0, 0, 1, 0, m_pScene->GetGlobalSettings().GetTimeMode());

	m_pScene->FillAnimStackNameArray(m_stNameArray);

	cout << m_stNameArray.Size() << endl;
	for (int i = 0; i < m_stNameArray.Size(); ++i)
	{
		if (m_stNameArray[i]->Compare(m_pScene->ActiveAnimStackName.Get()) == 0)
		{
			// select the base layer from the animation stack
			// 애니메이션 스택에서 베이스 층을 선택한다.

			// FbxAnimStack : 애니메이션 스택은 애니메이션 레이어의 모음입니다. 
			// FBX 문서는 하나 이상의 애니메이션 스택을 가질 수있다. 각 스택은 하나가 FBX SDK의 이전 버전에서 "take"로 볼 수 있습니다.
			// "스택"용어는 물체가소 정의 속성에 대한 결과적인 애니메이션을 만들어 내기 위해 블렌딩 모드에 따라 평가되는
			// N애니메이션 레이어 1이 포함되어 있다는 사실에서 나온다.
			FbxAnimStack * lCurrentAnimationStack = m_pScene->FindMember<FbxAnimStack>(m_stNameArray[i]->Buffer());
			m_pAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();
			// we assume that the first animation layer connected to the animation stack is the base layer
			// (this is the assumption made in the FBXSDK)
			// 우리는 애니메이션 스택에 연결된 첫번째 애니메이션 층이 베이스 층(이것은 FBXSDK 이루어진 가정이다) 인 것으로 가정한다.
			m_pScene->SetCurrentAnimationStack(lCurrentAnimationStack);

			FbxTakeInfo* lCurrentTakeInfo = m_pScene->GetTakeInfo(*(m_stNameArray[i]));
			//FbxTime mStart, mStop;
			if (lCurrentTakeInfo)
			{
				m_tStart = lCurrentTakeInfo->mLocalTimeSpan.GetStart();
				m_tStop = lCurrentTakeInfo->mLocalTimeSpan.GetStop();
			}
			else
			{
				// Take the time line value
				FbxTimeSpan lTimeLineTimeSpan;
				m_pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(lTimeLineTimeSpan);

				m_tStart = lTimeLineTimeSpan.GetStart();
				m_tStop = lTimeLineTimeSpan.GetStop();
			}

			cout << "St : " << m_tStart.GetMilliSeconds() << ", End : " << m_tStop.GetMilliSeconds() << endl;

			// check for smallest start with cache start
	//		if (pObject->GetVBOMesh(count)->mCache_Start < pObject->GetVBOMesh(count)->mStart)
	//			pObject->GetVBOMesh(count)->mStart = pObject->GetVBOMesh(count)->mCache_Start;

			// check for biggest stop with cache stop
		//	if (pObject->GetVBOMesh(count)->mCache_Stop > pObject->GetVBOMesh(count)->mStop)
		//		pObject->GetVBOMesh(count)->mStop = pObject->GetVBOMesh(count)->mCache_Stop;
		}
		cout << *m_stNameArray[i] << endl;
	}
	FbxArray<FbxPose*> PoseArray;
	const int lPoseCount = m_pScene->GetPoseCount();

	for (int i = 0; i < lPoseCount; ++i)
	{
		FbxPose * pPose = m_pScene->GetPose(i);
		cout << "Count : " << pPose->GetCount() << endl;
		cout << "Name : " << pPose->GetName() << endl;

		PoseArray.Add(pPose);
	}

}

void FBXParser::MeshRead(int MeshIndex)
{
	FbxMesh * pMesh = m_obj.m_pFbxMeshes[MeshIndex];
	Mesh * pDataMesh = &m_obj.m_vcMeshes[MeshIndex];

	const int lPolygonCount = pMesh->GetPolygonCount();

	bool mHasNormal, mHasUV, mAllByControlPoint;
	mHasNormal = mHasUV = mAllByControlPoint = false;

	// Count the polygon count of each material
	FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
	FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;

	if (pMesh->GetElementMaterial())
	{
		lMaterialIndice = &pMesh->GetElementMaterial()->GetIndexArray();
		lMaterialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
			if (lMaterialIndice->GetCount() == lPolygonCount)
			{
				pDataMesh->m_stIndices.resize(lPolygonCount, 0);

				// Count the faces of each material
				for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
				{
					const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
					//if (mSubMeshes.GetCount() < lMaterialIndex + 1)
					//{
					//	mSubMeshes.Resize(lMaterialIndex + 1);
					//}
					//if (mSubMeshes[lMaterialIndex] == NULL)
					//{
					//	mSubMeshes[lMaterialIndex] = new SubMesh;
					//}
					UINT sz = pDataMesh->m_vcSubMeshes.size();
					if (sz < lMaterialIndex + 1)
					{
						pDataMesh->m_vcSubMeshes.resize(lMaterialIndex + 1, Mesh::SubMesh());
					}
					pDataMesh->m_vcSubMeshes[lMaterialIndex].uTriangleCount++;

					//mSubMeshes[lMaterialIndex]->TriangleCount += 1;
				}

				// Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
				// if, in the loop above, we resized the mSubMeshes by more than one slot.
				//for (int i = 0; i < pDataMesh->m_vcSubMeshes.size(); i++)
				//{
				//	if (mSubMeshes[i] == NULL)
				//		mSubMeshes[i] = new SubMesh;
				//}

				// Record the offset (how many vertex)
				const int lMaterialCount = pDataMesh->m_vcSubMeshes.size();
				int lOffset = 0;
				for (int lIndex = 0; lIndex < lMaterialCount; ++lIndex)
				{
					pDataMesh->m_vcSubMeshes[lIndex].IndexOffset = lOffset;
					lOffset += pDataMesh->m_vcSubMeshes[lIndex].uTriangleCount * 3;
					// This will be used as counter in the following procedures, reset to zero
					pDataMesh->m_vcSubMeshes[lIndex].uTriangleCount = 0;
				}
				FBX_ASSERT(lOffset == lPolygonCount * 3);
			}
		}
	}

	// All faces will use the same material.
	//if (mSubMeshes.GetCount() == 0)
	//{
	//	mSubMeshes.Resize(1);
	//	mSubMeshes[0] = new SubMesh();
	//}

	// Congregate all the data of a mesh to be cached in VBOs.
	// If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
	mHasNormal = pMesh->GetElementNormalCount() > 0;
	mHasUV = pMesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
	if (mHasNormal)
	{
		lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
		if (lNormalMappingMode == FbxGeometryElement::eNone)
		{
			mHasNormal = false;
		}
		if (mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}
	if (mHasUV)
	{
		lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
		if (lUVMappingMode == FbxGeometryElement::eNone)
		{
			mHasUV = false;
		}
		if (mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}

	// Allocate the array memory, by control point or by polygon vertex.
	int lPolygonVertexCount = pMesh->GetControlPointsCount();
	if (!mAllByControlPoint)
	{
		lPolygonVertexCount = lPolygonCount * 3;
	}
	
	pDataMesh->m_stIndices.reserve(lPolygonCount * 3);
	//mIndices.resize(lPolygonCount * 3);

	float * lUVs = NULL;
	FbxStringList lUVNames;
	pMesh->GetUVSetNames(lUVNames);
	const char * lUVName = NULL;
	if (mHasUV && lUVNames.GetCount())
	{
		lUVName = lUVNames[0];
	}
	pDataMesh->m_vcVertexes.reserve(lPolygonVertexCount + 1);

	//mVertex.resize(lPolygonVertexCount);
	//mTexCoords.resize(lPolygonVertexCount);
	//mNormals.resize(lPolygonVertexCount);

	// Populate the array with vertex attribute, if by control point.
	const FbxVector4 * lControlPoints = pMesh->GetControlPoints();
	FbxVector4 lCurrentVertex;
	FbxVector4 lCurrentNormal;
	FbxVector4 lCurrentTangent;
	FbxVector2 lCurrentUV;
	BoundingBox			m_BoundingBox;

	XMFLOAT3 vMinf3(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	XMFLOAT3 vMaxf3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	if (mAllByControlPoint)
	{
		const FbxGeometryElementTangent * lTangentElement = NULL;
		const FbxGeometryElementNormal  * lNormalElement  = NULL;
		const FbxGeometryElementUV      * lUVElement      = NULL;
		if (mHasNormal)
		{
			lNormalElement = pMesh->GetElementNormal(0);
		}
		if (mHasUV)
		{
			lUVElement = pMesh->GetElementUV(0);
		}

		for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
		{
			// Save the vertex position.
			Vertex tempVertex;

			lCurrentVertex = lControlPoints[lIndex];

			SetFbxFloatToXmFloat(tempVertex.xmf3Pos, lCurrentVertex);
//			tempVertex.xmf3Pos.x = static_cast<float>(lCurrentVertex[0]);
//			tempVertex.xmf3Pos.z = static_cast<float>(lCurrentVertex[1]);
//			tempVertex.xmf3Pos.y = static_cast<float>(lCurrentVertex[2]);

			// Save the normal.
			if (mHasNormal)
			{
				int lNormalIndex = lIndex;
				if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);

				SetFbxFloatToXmFloat(tempVertex.xmf3Normal, lCurrentNormal);

				//tempVertex.xmf3Normal.x = static_cast<float>(lCurrentNormal[0]);
				//tempVertex.xmf3Normal.z = static_cast<float>(lCurrentNormal[1]);
				//tempVertex.xmf3Normal.y = static_cast<float>(lCurrentNormal[2]);
				//mNormals[lIndex].x = mVertex[lIndex].normal.x;
				//mNormals[lIndex].z = mVertex[lIndex].normal.z;
				//mNormals[lIndex].y = mVertex[lIndex].normal.y;

				int lTangentIndex = lIndex;
				if (lTangentElement->GetReferenceMode() == FbxLayerElement::EReferenceMode::eIndexToDirect)
				{
					lTangentIndex = lTangentElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentTangent = lTangentElement->GetDirectArray().GetAt(lTangentIndex);

				SetFbxFloatToXmFloat(tempVertex.xmf3Tangent, lCurrentTangent);

				//tempVertex.xmf3Tangent.x = static_cast<float>(lCurrentTangent[0]);
				//tempVertex.xmf3Tangent.z = static_cast<float>(lCurrentTangent[1]);
				//tempVertex.xmf3Tangent.y = static_cast<float>(lCurrentTangent[2]);
			}

			// Save the UV.
			if (mHasUV)
			{
				int lUVIndex = lIndex;
				if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);

				SetFbxFloatToXmFloat(tempVertex.xmf2TexCoord, lCurrentUV);

				//tempVertex.xmf2TexCoord.x = static_cast<float>(lCurrentUV[0]);
				//tempVertex.xmf2TexCoord.y = static_cast<float>(1.0f - lCurrentUV[1]);
				//mTexCoords[lIndex].x = mVertex[lIndex].texcoord.x;
				//mTexCoords[lIndex].y = mVertex[lIndex].texcoord.y;
			}

			pDataMesh->m_vcVertexes.push_back(tempVertex);
		}

	}
	else
	{
		pDataMesh->m_vcVertexes.resize(lPolygonVertexCount, Vertex());
	}


	int lVertexCount = 0;
	if (pDataMesh->m_vcSubMeshes.size() == 0)
	{
		pDataMesh->m_vcSubMeshes.resize(lPolygonCount, Mesh::SubMesh());
		pDataMesh->m_stIndices.resize(lPolygonCount * 3, 0);
	}


	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	{
		// The material for current face.
		int lMaterialIndex = 0;
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
		}

		// Where should I save the vertex attribute index, according to the material
		const int lIndexOffset = pDataMesh->m_vcSubMeshes[lMaterialIndex].IndexOffset +
			pDataMesh->m_vcSubMeshes[lMaterialIndex].uTriangleCount * 3;
		for (int lVerticeIndex = 0; lVerticeIndex < 3; ++lVerticeIndex)
		{
			const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);

			if (lIndexOffset + lVerticeIndex + 1 > pDataMesh->m_stIndices.size())
			{
				pDataMesh->m_stIndices.resize(lIndexOffset + lVerticeIndex + 1, 0);
			}
			if (lVertexCount > pDataMesh->m_vcVertexes.size())
			{
				pDataMesh->m_vcVertexes.resize(lVertexCount + 1, Vertex());
			}

			if (mAllByControlPoint)
			{
				pDataMesh->m_stIndices[lIndexOffset + lVerticeIndex] = static_cast<USHORT>(lControlPointIndex);
				//mIndices[lIndexOffset + lVerticeIndex] = static_cast<USHORT>(lControlPointIndex);
			}
			// Populate the array with vertex attribute, if by polygon vertex.
			else
			{
				pDataMesh->m_stIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

				lCurrentVertex = lControlPoints[lControlPointIndex];

				SetFbxFloatToXmFloat(pDataMesh->m_vcVertexes[lVertexCount].xmf3Pos, lCurrentVertex);

				if (mHasNormal)
				{
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
					SetFbxFloatToXmFloat(pDataMesh->m_vcVertexes[lVertexCount].xmf3Normal, lCurrentNormal);
					
					//mNormals[lVertexCount].x = mVertex[lVertexCount].normal.x;
					//mNormals[lVertexCount].z = mVertex[lVertexCount].normal.z;
					//mNormals[lVertexCount].y = mVertex[lVertexCount].normal.y;
				}

				if (mHasUV)
				{
					bool lUnmappedUV;
					pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
					SetFbxFloatToXmFloat(pDataMesh->m_vcVertexes[lVertexCount].xmf2TexCoord, lCurrentUV);

					//pDataMesh->m_vcVertexes[lVertexCount].xmf2TexCoord.x = static_cast<float>(lCurrentUV[0]);
					//pDataMesh->m_vcVertexes[lVertexCount].xmf2TexCoord.y = static_cast<float>(1.0f - lCurrentUV[1]);
					//mTexCoords[lVertexCount].x = mVertex[lVertexCount].texcoord.x;
					//mTexCoords[lVertexCount].y = mVertex[lVertexCount].texcoord.y;
				}
			}

			++lVertexCount;


		}
		pDataMesh->m_vcSubMeshes[lMaterialIndex].uTriangleCount++;
//		mSubMeshes[lMaterialIndex]->TriangleCount += 1;
	}

//	cout << "Tangent Before : " << pMesh->GetElementTangentCount() << "\t";
//	pMesh->InitTangents();
//	pMesh->GenerateTangentsDataForAllUVSets(true);
////	pMesh->GenerateTangentsData()
//
//
//	const FbxGeometryElementTangent * lTangentElement = nullptr;
//	lTangentElement = pMesh->GetElementTangent();
//	
//	//FbxVector4 * pArray = lTangentElement->GetDirectArray();
//
//	cout << "Tangent After : " <<  pMesh->GetElementTangentCount() << "/" << pMesh->GetLayerCount() << endl;
//
//	for (int i = 0; i < pMesh->GetLayerCount(); ++i)
//	{
//	//	pDataMesh->m_vcVertexes[i].xmf3Tangent = lTangentElement->GetDirectArray().GetAt(i);
//		SetFbxFloatToXmFloat(pDataMesh->m_vcVertexes[i].xmf3Tangent, lTangentElement->GetDirectArray().GetAt(i));
//	}

	float vecX, vecY, vecZ, tcU1, tcU2, tcV1, tcV2;
	XMVECTOR edge1, edge2;
	XMFLOAT3 tangent[3], normal[3];

	int CalIndex[] = { 1, 2, 1, 2, 1, 2 }; //{ +1, +2, +1, -1, -2, -1 };
	UINT offsetLeft3 = 0, offset = 0;

	UINT nVertices = pDataMesh->m_vcVertexes.size();
	for (int i = 0; i < nVertices; ++i)
	{
		offset = (i % 3);
		offsetLeft3 = offset * 2;
		tangent[offset] = { 0, 0, 0 };

		Vertex * pNearVertexes[3] = { &pDataMesh->m_vcVertexes[i], 
			&pDataMesh->m_vcVertexes[(i + CalIndex[offsetLeft3 + 1]) % nVertices], 
			&pDataMesh->m_vcVertexes[(i + CalIndex[offsetLeft3 + 2]) % nVertices] };

		//Get the vector describing one edge of our triangle (edge 0,2)
		vecX = pNearVertexes[0]->xmf3Pos.x - pNearVertexes[2]->xmf3Pos.x;
		vecY = pNearVertexes[0]->xmf3Pos.y - pNearVertexes[2]->xmf3Pos.y;
		vecZ = pNearVertexes[0]->xmf3Pos.z - pNearVertexes[2]->xmf3Pos.z;
		edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our first edge

		//Get the vector describing another edge of our triangle (edge 2,1)
		vecX = pNearVertexes[2]->xmf3Pos.x - pNearVertexes[1]->xmf3Pos.x;
		vecY = pNearVertexes[2]->xmf3Pos.y - pNearVertexes[1]->xmf3Pos.y;
		vecZ = pNearVertexes[2]->xmf3Pos.z - pNearVertexes[1]->xmf3Pos.z;
		edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our second edge

		//Cross multiply the two edge vectors to get the un-normalized face normal
		XMStoreFloat3(&normal[offset], -XMVector3Cross(edge1, edge2));
		//pDataMesh->m_vcVertexes[i].xmf3Normal = normal[offset];

		//Find first texture coordinate edge 2d vector
		tcU1 = pNearVertexes[0]->xmf2TexCoord.x - pNearVertexes[2]->xmf2TexCoord.x;
		tcV1 = pNearVertexes[0]->xmf2TexCoord.y - pNearVertexes[2]->xmf2TexCoord.y;

		//Find second texture coordinate edge 2d vector
		tcU2 = pNearVertexes[2]->xmf2TexCoord.x - pNearVertexes[1]->xmf2TexCoord.x;
		tcV2 = pNearVertexes[2]->xmf2TexCoord.y - pNearVertexes[1]->xmf2TexCoord.y;

		//Find tangent using both tex coord edges and position edges
		double fVal = (double)(tcU1 * tcV2 - tcU2 * tcV1);
		if (fVal == 0.0) fVal += 0.00001;
		tangent[offset].x += (tcV1 * XMVectorGetX(edge1) - tcV2 * XMVectorGetX(edge2)) * 1 / fVal;
		tangent[offset].y += (tcV1 * XMVectorGetY(edge1) - tcV2 * XMVectorGetY(edge2)) * 1 / fVal;
		tangent[offset].z += (tcV1 * XMVectorGetZ(edge1) - tcV2 * XMVectorGetZ(edge2)) * 1 / fVal;
		//pDataMesh->m_vcVertexes[i].xmf3Tangent = tangent[offset];

//		XMStoreFloat3(&tangent, XMVector3Normalize(XMLoadFloat3(&tangent)));

		if (i % 3 == 2)
		{
			XMVECTOR tangentSum = XMLoadFloat3(&tangent[offset]);
			tangentSum += XMLoadFloat3(&tangent[offset - 1]);
			tangentSum += XMLoadFloat3(&tangent[offset - 2]);
			tangentSum  = XMVector3Normalize(tangentSum);

			XMStoreFloat3(&pDataMesh->m_vcVertexes[i].xmf3Tangent, tangentSum);
			XMStoreFloat3(&pDataMesh->m_vcVertexes[i - 1].xmf3Tangent, tangentSum);
			XMStoreFloat3(&pDataMesh->m_vcVertexes[i - 2].xmf3Tangent, tangentSum);

			//XMVECTOR normalSum = XMLoadFloat3(&normal[offset]);
			//normalSum += XMLoadFloat3(&normal[offset - 1]);
			//normalSum += XMLoadFloat3(&normal[offset - 2]);
			//normalSum = XMVector3Normalize(normalSum);

			//XMStoreFloat3(&pDataMesh->m_vcVertexes[i].xmf3Normal, normalSum);
			//XMStoreFloat3(&pDataMesh->m_vcVertexes[i - 1].xmf3Normal, normalSum);
			//XMStoreFloat3(&pDataMesh->m_vcVertexes[i - 2].xmf3Normal, normalSum);
		}

		//tempTangent.push_back(tangent);
	}
}

void FBXParser::NormalRead(FbxMesh * pMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3 & outNormal, XMFLOAT3& outTangent)
{
	if (pMesh->GetElementNormalCount() < 1)
	{
		throw std::exception("Invalid Normal Number");
	}

	FbxGeometryElementNormal* vertexNormal = pMesh->GetElementNormal(0);

	FbxGeometryElementTangent* vertexTangent = pMesh->GetElementTangent(0);

	switch (vertexNormal->GetMappingMode())
	{
	case FbxGeometryElement::eByControlPoint:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
//			vertexTangent->Get
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inCtrlPointIndex);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (vertexNormal->GetReferenceMode())
		{
		case FbxGeometryElement::eDirect:
		{
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
		}
		break;

		case FbxGeometryElement::eIndexToDirect:
		{
			int index = vertexNormal->GetIndexArray().GetAt(inVertexCounter);
			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
		}
		break;

		default:
			throw std::exception("Invalid Reference");
		}
		break;
	}
}

void FBXParser::UVRead(FbxMesh * pMesh, int inCtrlPointIndex, int inUVIndex, XMFLOAT2 & outUV)
{
	//const FbxGeometryElementUV* lUVElement = pMesh->GetElementUV(lUVSetName);
	FbxGeometryElementUV* vertexUV = pMesh->GetElementUV(0);

	const bool lUseIndex = vertexUV->GetMappingMode() != FbxGeometryElement::eDirect;

	FbxVector2 lUVValue;

	if (vertexUV->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		//int lPolyVertIndex = pMesh->GetPolygonVertex(//(lPolyIndex, lVertIndex);
		//int lUVIndex = lUseIndex ? vertexUV->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;
		lUVValue = vertexUV->GetDirectArray().GetAt(inUVIndex);
	}
	else if (vertexUV->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		lUVValue = vertexUV->GetDirectArray().GetAt(inUVIndex);
	}

	outUV = XMFLOAT2(lUVValue.mData[0], lUVValue.mData[1]);

#ifdef ALL_GET_UV
	if (vertexNormal->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
		{
			// build the max index array that we need to pass into MakePoly
			const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
			for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
			{
				FbxVector2 lUVValue;

				//get the index of the current vertex in control points array
				int lPolyVertIndex = pMesh->GetPolygonVertex(lPolyIndex, lVertIndex);

				//the UV index depends on the reference mode
				int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

				lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

				//User TODO:
				//Print out the value of UV(lUVValue) or log it to a file
			}
		}
	}
	else if (vertexNormal->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		int lPolyIndexCounter = 0;
		for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
		{
			// build the max index array that we need to pass into MakePoly
			const int lPolySize = pMesh->GetPolygonSize(lPolyIndex);
			for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
			{
				if (lPolyIndexCounter < lIndexCount)
				{
					FbxVector2 lUVValue;

					//the UV index depends on the reference mode
					int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

					lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

					//User TODO:
					//Print out the value of UV(lUVValue) or log it to a file

					lPolyIndexCounter++;
				}
			}
		}
	}
#endif

}

void FBXParser::SetFbxFloatToXmFloat(XMFLOAT3 & data, FbxVector4 & fbxdata)
{
#ifdef CHANGE_SYSTEM_COORD
	data.x = static_cast<float>(fbxdata[0]);
	data.z = static_cast<float>(fbxdata[1]);
	data.y = static_cast<float>(fbxdata[2]);
#else
	data.x = static_cast<float>(fbxdata[0]);
	data.y = static_cast<float>(fbxdata[1]);
	data.z = static_cast<float>(fbxdata[2]);
#endif
}

void FBXParser::SetFbxFloatToXmFloat(XMFLOAT2 & data, FbxVector2 & fbxdata)
{
#ifdef CHANGE_SYSTEM_COORD
	data.x = static_cast<float>(fbxdata[0]);
	data.y = static_cast<float>(1.0 - fbxdata[1]);
#else
	data.x = static_cast<float>(fbxdata[0]);
	data.y = static_cast<float>(1.0 - fbxdata[1]);
#endif
}

FbxAMatrix FBXParser::GetGlobalPosition(FbxNode * pNode, const FbxTime & pTime, FbxPose * pPose, FbxAMatrix * pParentGlobalPosition)
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
						lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose, nullptr); // nullptr?
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

FbxAMatrix FBXParser::GetPoseMatrix(FbxPose * pPose, int pNodeIndex)
{
	FbxAMatrix lPoseMatrix;
	FbxMatrix lMatrix = pPose->GetMatrix(pNodeIndex);

	memcpy((double*)lPoseMatrix, (double*)lMatrix, sizeof(lMatrix.mData));

	return lPoseMatrix;
}

FbxAMatrix FBXParser::GetGeometry(FbxNode * pNode)
{
	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);

	return FbxAMatrix(lT, lR, lS);
}

void FBXParser::AnimateNode(FbxMesh * pMesh, FbxNode * pNode, FbxTime & pTime, FbxAnimLayer * pAnimLayer, FbxAMatrix & pParentGlobalPosition, FbxAMatrix & pGlobalPosition, FbxPose * pPose)
{
	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

	if (lNodeAttribute)
	{
		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			cout << "이것은 Animate Mesh다!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
			//AnimateMesh(pd3dDevice, pMesh, pNode, pTime, pAnimLayer, pGlobalPosition, pPose);
		}
		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			cout << "이것은 Skeleton이다!!";
			AnimateSkeleton(pMesh, pNode, pParentGlobalPosition, pGlobalPosition);
		}
	}

}

void FBXParser::AnimateMesh(ID3D11Device * pd3dDevice, FbxMesh * pMesh, FbxNode * pNode, FbxTime & pTime, FbxAnimLayer * pAnimLayer, FbxAMatrix & pGlobalPosition, FbxPose * pPose)
{
	//FbxMesh* lMesh = pNode->GetMesh();
	//const int lVertexCount = lMesh->GetControlPointsCount();

	//if (lVertexCount == 0)
	//	return;

	//const VBOMesh * lMeshCache = static_cast<const VBOMesh *>(lMesh->GetUserDataPtr());

	//const bool lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
	//	(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
	//const bool lHasShape = lMesh->GetShapeCount() > 0;
	//const bool lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
	//const bool lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;

	//FbxVector4* lVertexArray = NULL;
	//if (!lMeshCache || lHasDeformation)
	//{
	//	lVertexArray = new FbxVector4[lVertexCount];
	//	memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
	//}

	//if (lHasDeformation)
	//{
	//	if (lHasVertexCache)
	//	{
	//		ReadVertexCacheData(lMesh, pTime, lVertexArray);
	//	}
	//	else
	//	{
	//		if (lHasShape)
	//		{
	//			// Deform the vertex array with the shapes.
	//			ComputeShapeDeformation(lMesh, pTime, pAnimLayer, lVertexArray);
	//		}

	//		// we need to get the number of clusters
	//		const int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);
	//		int lClusterCount = 0;
	//		for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
	//		{
	//			lClusterCount += ((FbxSkin *)(lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
	//		}
	//		if (lClusterCount)
	//		{
	//			ComputeSkinDeformation(pd3dDevice, pMesh, pGlobalPosition, lMesh, pTime, lVertexArray, pPose);
	//		}
	//	}
	//}

	//if (lVertexArray != NULL)
	//	delete[] lVertexArray;

}

void FBXParser::AnimateSkeleton(FbxMesh * pMesh, FbxNode * pNode, FbxAMatrix & pParentGlobalPosition, FbxAMatrix & pGlobalPosition)
{
//	FbxAnimStack* currAnimStack = mFBXScene->GetSrcObject<FbxAnimStack>(0);
//	FbxString animStackName = currAnimStack->GetName();
//	mAnimationName = animStackName.Buffer();
//	FbxTakeInfo* takeInfo = mFBXScene->GetTakeInfo(animStackName);
//	FbxTime start = takeInfo->mLocalTimeSpan.GetStart();
//	FbxTime end = takeInfo->mLocalTimeSpan.GetStop();

	//FbxObject * pObj =  pNode->GetSrcObject();
	//unsigned int numOfDeformers = pObj->Deformer// ->GetDeformerCount();
	//// This geometry transform is something I cannot understand
	//// I think it is from MotionBuilder
	//// If you are using Maya for your models, 99% this is just an
	//// identity matrix
	//// But I am taking it into account anyways......

	//// A deformer is a FBX thing, which contains some clusters
	//// A cluster contains a link, which is basically a joint
	//// Normally, there is only one deformer in a mesh
	//for (unsigned int deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
	//{
	//	// There are many types of deformers in Maya,
	//	// We are using only skins, so we see if this is a skin
	//	FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
	//}


	
	//currAnimStack->Get
//	UINT numOfDeformers = pOb();
	FbxMesh * fbxMesh = pNode->GetMesh();
	int nAnimStacks = m_stNameArray.Size();//m_pRootNode->GetSrcObjectCount<FbxAnimStack>();
	// 애니매이션 스택
	for (int i = 0; i < nAnimStacks; ++i)
	{
		//FbxAnimStack* currAnimStack = m_pScene->GetSrcObject<FbxAnimStack>(i);
		// 애니매이션 레이어
		FbxAnimStack * lCurrentAnimationStack = m_pScene->FindMember<FbxAnimStack>(m_stNameArray[i]->Buffer());
		m_pAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();
		
		int nAnimLayers = lCurrentAnimationStack->GetMemberCount(); // m_pAnimLayer->GetMemberCount(); 
		cout << "총 스택 수 : " << lCurrentAnimationStack->GetMemberCount() << endl;
		cout << "총 레이어 수 : " << m_pAnimLayer->GetMemberCount() << endl;
		
		//FbxAnimLayer * pAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();//currAnimStack->GetMember<FbxAnimLayer>(j);
		for (int j = 0; j < nAnimLayers; ++j)
		{
			FbxAnimLayer * pfbxAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>(j);//m_pAnimLayer->GetMember<FbxAnimLayer>(j);// 
			// 애니매이션 커브(노드 속성)
			FbxString jointName = pNode->LclTranslation.GetParent().GetName();
			cout << "Name : " << jointName << endl;

			FbxAnimCurve * pfbxAnimCurve = nullptr;
			pfbxAnimCurve = pNode->LclTranslation.GetCurve(pfbxAnimLayer, "X");
			FbxNodeAttribute * pfbxAttribute = pNode->GetNodeAttribute();
			ParseKeyData(pfbxAnimCurve, jointName + " X Rot");

			pfbxAnimCurve = pNode->LclTranslation.GetCurve(pfbxAnimLayer, "Y");
			ParseKeyData(pfbxAnimCurve, jointName + " Y Rot");

			pfbxAnimCurve = pNode->LclTranslation.GetCurve(pfbxAnimLayer, "Z");
			ParseKeyData(pfbxAnimCurve, jointName + " Z Rot");

			// 애니매이션 커브 노드FbxAnimCurve
			//FbxProperty fbxProperty = pNode->GetFirstProperty();
			//FbxAnimCurveNode * pfbxCurveNode = fbxProperty.GetCurveNode(pfbxAnimLayer);
			//int nCurveNodes = pfbxCurveNode->GetCurveCount(0);
			//for (int k = 0; k < nCurveNodes; ++k)
			//{
			//	FbxAnimCurve * pfbxAnimCurve = pfbxCurveNode->GetCurve(0, k);
			//}
		}
	}

	//for (int deformerIndex = 0; deformerIndex < numOfDeformers; ++deformerIndex)
	//{
	//	FbxSkin* currSkin = reinterpret_cast<FbxSkin*>(pMesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));

	//	UINT numOfClusters = currSkin->GetClusterCount();

	//}

 	m_tAnimationLength = m_tStop.GetFrameCount(FbxTime::eFrames24) - m_tStart.GetFrameCount(FbxTime::eFrames24) + 1;
//	Keyframe** currAnim = &mSkeleton.mJoints[currJointIndex].mAnimation;

	for (FbxLongLong i = m_tStart.GetFrameCount(FbxTime::eFrames24); i <= m_tStop.GetFrameCount(FbxTime::eFrames24); ++i)
	{
		FbxTime currTime;
		currTime.SetFrame(i, FbxTime::eFrames24);

		cout << "Parent Global : " << endl;
		cout << pParentGlobalPosition.mData[0] << endl;
		cout << pParentGlobalPosition.mData[1] << endl;
		cout << pParentGlobalPosition.mData[2] << endl;
		cout << pParentGlobalPosition.mData[3] << endl << endl;

		cout << "This Global : " << pGlobalPosition.mData[0] << endl;
		cout << pGlobalPosition.mData[1] << endl;
		cout << pGlobalPosition.mData[2] << endl;
		cout << pGlobalPosition.mData[3] << endl;

		//*currAnim = new Keyframe();
		//(*currAnim)->mFrameNum = i;
		FbxAMatrix currentTransformOffset = pNode->EvaluateGlobalTransform(currTime) * pParentGlobalPosition;//geometryTransform;
		//FbxAMatrix Transform = currentTransformOffset.Inverse() * currCluster
	//(*currAnim)->mGlobalTransform = currentTransformOffset.Inverse() * currCluster->GetLink()->EvaluateGlobalTransform(currTime);
//		currAnim = &((*currAnim)->mNext);
	}
}

void FBXParser::ParseAnimations()
{
	//FbxNode* rootNode = m_pScene->GetRootNode();

	//// need this to store the anim data then give it to the motion struct
	//for (int i = 0; i < m_pScene->GetSrcObjectCount(FBX_TYPE(FbxAnimStack)); i++)
	//{
	//	// stacks hold animation layers, so go through the stacks first
	//	FbxAnimStack* pAnimStack = FbxCast<FbxAnimStack>(m_pScene->GetSrcObject(FBX_TYPE(FbxAnimStack), i));
	//	m_pScene->GetAnimationEvaluator()->SetContext(pAnimStack);

	//	FbxTimeSpan timeSpan = pAnimStack->GetLocalTimeSpan();
	//	ParseAnimationRecursive(pAnimStack, rootNode);
	//}
}

void FBXParser::ParseAnimationRecursive(FbxAnimStack* pAnimStack, FbxNode* pNode)
{
	//int nbAnimLayers = pAnimStack->GetMemberCount(FBX_TYPE(FbxAnimLayer));

	//std::string takeName = pAnimStack->GetName();
	//for (int layerNo = 0; layerNo < nbAnimLayers; layerNo++)
	//{
	//	// layers hold animcurves so go through each layers set of curves
	//	FbxAnimLayer* pAnimLayer = pAnimStack->GetMember(FBX_TYPE(KFbxAnimLayer), layerNo);

	//	ParseAnimationRecursive(pAnimLayer, pNode, -2);
	//}
}

void FBXParser::ParseAnimationRecursive(FbxAnimLayer* pAnimLayer, FbxNode* pNode, int tabCount)
{
//	// get the joint name
//	std::string jointName = pNode->LclTranslation.GetParent().GetName();
//	FbxAnimCurve *pCurve = nullptr;
//
//	if (!m_bFoundSbmMetadata)
//	{
//		// if it has this meta data property, then it has all the others,
//		// so parse them out save them
//		FbxProperty prop = pNode->FindProperty("readyTime");
//		if (prop.IsValid())
//		{
//			PrintMetaData(pNode);
//			m_bFoundSbmMetadata = true;
//		}
//	}
//
//	if (pNode->GetNodeAttribute() != NULL && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
//	{
//		memset(m_Buffer, 0, BUFFER_SIZE);
//		for (int i = 0; i < tabCount; i++)
//		{
//			strcat_s(m_Buffer, BUFFER_SIZE, "  ");
//		}
//
//		Print("%s%s: ", m_Buffer, jointName.c_str());
//
//		// transformation data
//		fbxDouble3 data = pNode->LclTranslation.Get();
//		Print("%sLcl Pos: x: %f   y: %f   z: %f", m_Buffer, data.mData[0], data.mData[1], data.mData[2]);
//
//		data = pNode->LclRotation.Get();
//		Print("%sLcl Rot: x: %f   y: %f   z: %f", m_Buffer, data.mData[0], data.mData[1], data.mData[2]);
//
//		data = pNode->PreRotation.Get();
//		Print("%sPre Rot: x: %f   y: %f   z: %f", m_Buffer, data.mData[0], data.mData[1], data.mData[2]);
//
//		// smartbody channel data
//		bool hasSbmX = false, hasSbmY = false, hasSbmZ = false, hasSbmQuat = false;
//		PrintProperty<bool>(pNode, "SbodyPosX", hasSbmX, eBOOL1);
//		PrintProperty<bool>(pNode, "SbodyPosY", hasSbmY, eBOOL1);
//		PrintProperty<bool>(pNode, "SbodyPosZ", hasSbmZ, eBOOL1);
//		PrintProperty<bool>(pNode, "SbodyQuat", hasSbmQuat, eBOOL1);
//
//		if (m_bPrintAnimationData)
//		{
//			if (hasSbmX)
//			{
//				// x pos data
//				pCurve = pNode->LclTranslation.GetCurve<KFbxAnimCurve>(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X FCURVENODE_T_X);
//				ParseKeyData(pCurve, jointName + "Xpos ");
//			}
//
//			if (hasSbmY)
//			{
//				// y pos data
//				pCurve = pNode->LclTranslation.GetCurve<KFbxAnimCurve>(pAnimLayer, KFCURVENODE_T_Y);
//				ParseKeyData(pCurve, jointName + "Ypos ");
//			}
//
//			if (hasSbmZ)
//			{
//				// z pos data
//				pCurve = pNode->LclTranslation.GetCurve<KFbxAnimCurve>(pAnimLayer, KFCURVENODE_T_Z);
//				ParseKeyData(pCurve, jointName + "Zpos ");
//			}
//
//			if (hasSbmQuat)
//			{
//				// x rot data
//				pCurve = pNode->LclRotation.GetCurve<KFbxAnimCurve>(pAnimLayer, KFCURVENODE_R_X);
//				ParseKeyData(pCurve, jointName + "Xrot ");
//
//				// y rot data
//				pCurve = pNode->LclRotation.GetCurve<KFbxAnimCurve>(pAnimLayer, KFCURVENODE_R_Y);
//				ParseKeyData(pCurve, jointName + "Yrot ");
//
//				// z rot data
//				pCurve = pNode->LclRotation.GetCurve<KFbxAnimCurve>(pAnimLayer, KFCURVENODE_R_Z);
//				ParseKeyData(pCurve, jointName + "Zrot ");
//}
//	  }
//
//		Print("\n");
//   }
//
//	int childCount = pNode->GetChildCount();
//	for (int i = 0; i < childCount; i++)
//	{
//		ParseAnimationRecursive(pAnimLayer, pNode->GetChild(i), tabCount + 1);
//	}
}

void FBXParser::ParseKeyData(const FbxAnimCurve* pCurve, const FbxString& channelName)
{
	if (!pCurve)
	{
		return;
	}

	// create a new FBXAnimData and store it
	AnimData* pNewAnimData = new AnimData();
	pNewAnimData->m_stName = channelName;

	int numKeys = pCurve->KeyGetCount();
	cout << "key : ";
	for (int i = 0; i < numKeys; i++)
	{
		FbxAnimCurveKey key = pCurve->KeyGet(i);
		// the time at which the data takes places
		FbxTime time = key.GetTime();
		const FbxLongLong t = time.Get();

		// the key channel transformation data
		float value = key.GetValue();
		//key.Get
		pNewAnimData->m_vcKeyFrameTime.push_back((float)t / 46186158000.0f);
		pNewAnimData->m_vcKeyFrameData.push_back(value);
		cout << value << ", ";
	}
	cout << endl << endl;
	m_vcAnimData.push_back(pNewAnimData);
}


#ifdef ALL_NORMAL_READ
void FBXParser::NormalRead()
{
	m_ppNormal = new XMFLOAT3*[m_nObjects];

	for (int i = 0; i < m_nObjects; ++i)
	{
		m_ppControlPos[i] = nullptr;

		FbxNode * pNode = m_pRootNode->GetChild(i);
		FbxNodeAttribute::EType type = (pNode->GetNodeAttribute())->GetAttributeType();

		if (type == FbxNodeAttribute::eMesh)
		{
			FbxMesh* pMesh = pNode->GetMesh();

			if (pMesh->GetElementNormalCount() < 1) continue;

			UINT nNormals = pMesh->GetElementNormalCount();
			m_ppNormal[i] = new XMFLOAT3[nNormals];

			FbxGeometryElementNormal * vertexNormal = pMesh->GetElementNormal();

			int index = 0;

				switch (vertexNormal->GetMappingMode())
				{
				case FbxGeometryElement::eByControlPoint:
					switch (vertexNormal->GetReferenceMode())
					{
					case FbxGeometryElement::eDirect:
						for (int j = 0; j < nNormals; ++j)
						{
							m_ppNormal[i][j].x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(j).mData[0]);
							m_ppNormal[i][j].y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(j).mData[1]);
							m_ppNormal[i][j].z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(j).mData[2]);
						}
						break;

					case FbxGeometryElement::eIndexToDirect:
						for (int j = 0; j < nNormals; ++j)
						{
							index = vertexNormal->GetIndexArray().GetAt(j);
							m_ppNormal[i][j].x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
							m_ppNormal[i][j].y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
							m_ppNormal[i][j].z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
						}
						break;
					}
					break;

				case FbxGeometryElement::eByPolygonVertex:

					switch (vertexNormal->GetReferenceMode())
					{

					}
					break;
				}

				/*		FbxVector4 * lControlPoints = pMesh->GetControlPoints();
						UINT nControlPoints = pMesh->GetControlPointsCount();
						m_ppControlPos[i] = new XMFLOAT4[nControlPoints];

						for (int j = 0; j < nControlPoints; ++j)
						{
							m_ppControlPos[i][j].x = lControlPoints[j].mData[0];
							m_ppControlPos[i][j].y = lControlPoints[j].mData[1];
							m_ppControlPos[i][j].z = lControlPoints[j].mData[2];
							m_ppControlPos[i][j].w = lControlPoints[j].mData[3];

							cout << m_ppControlPos[i][j] << " // ";
							cout << lControlPoints[j] << endl;
						}

						FbxVector4 lCurrentVertex;
						FbxVector4 lCurrentNormal;
						FbxVector2 lCurrentUV;*/

			
		}
	}
}
#endif