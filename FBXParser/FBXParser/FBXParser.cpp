#include "FBXParser.h"

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


FBXParser::FBXParser()
{
	m_pMgr      = nullptr;
	m_pScene    = nullptr;
	m_pRootNode = nullptr;

	m_pImporter = nullptr;
	m_ppControlPos = nullptr;

	m_nObjects  = 0;
}


FBXParser::~FBXParser()
{
	DESTROY(m_pMgr);
	DESTROY(m_pScene);
	DESTROY(m_pImporter);

	if (m_ppControlPos)
	{
		for (int i = 0; i < m_nObjects; ++i)
			if (m_ppControlPos[i]) delete [] m_ppControlPos[i];
		delete [] m_ppControlPos;
	}
}

bool FBXParser::Initialize(const char * pstr)
{
	m_pMgr = FbxManager::Create();
	FbxIOSettings * pfbxIOSettings = FbxIOSettings::Create(m_pMgr, IOSROOT);
	m_pMgr->SetIOSettings(pfbxIOSettings);

	pfbxIOSettings->SetBoolProp(IMP_FBX_CHARACTER, true);
	pfbxIOSettings->SetBoolProp(IMP_FBX_ANIMATION, true);

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

	VertexRead();
}

void FBXParser::Setting()
{
	FbxAxisSystem SceneAxisSystem = m_pScene->GetGlobalSettings().GetAxisSystem();
	FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
	if (SceneAxisSystem != OurAxisSystem)
	{
		OurAxisSystem.ConvertScene(m_pScene);
	}
	// Convert mesh, NURBS and patch into triangle mesh
	FbxGeometryConverter lGeomConverter(m_pMgr);
	lGeomConverter.Triangulate(m_pScene, true);

	// split meshes per material, so that we only have one material per mesh (for VBO support)
	lGeomConverter.SplitMeshesPerMaterial(m_pScene, true);

	m_pRootNode = m_pScene->GetRootNode();
	m_nObjects = m_pRootNode->GetSrcObjectCount();

	for (int i = 0; i < m_nObjects; ++i)
	{
		FbxObject * pfbxObject = m_pRootNode->GetSrcObject(i);
		cout << "NAME : " << pfbxObject->GetName() << endl;
	}
}

void FBXParser::VertexRead()
{
	m_ppControlPos = new XMFLOAT3*[m_nObjects];

	for (int i = 0; i < m_nObjects; ++i)
	{
		m_ppControlPos[i] = nullptr;

		FbxNode * pNode = m_pRootNode->GetChild(i);
		FbxNodeAttribute::EType type = (pNode->GetNodeAttribute())->GetAttributeType();

		if (type == FbxNodeAttribute::eMesh)
		{
			FbxMesh* pMesh              = pNode->GetMesh();
			FbxVector4 * lControlPoints = pMesh->GetControlPoints();
			UINT nControlPoints         = pMesh->GetControlPointsCount();
			m_ppControlPos[i]           = new XMFLOAT3[nControlPoints];

			for (int j = 0; j < nControlPoints; ++j)
			{
				m_ppControlPos[i][j].x = lControlPoints[j].mData[0];
				m_ppControlPos[i][j].y = lControlPoints[j].mData[1];
				m_ppControlPos[i][j].z = lControlPoints[j].mData[2];
				//m_ppControlPos[i][j].w = lControlPoints[j].mData[3];

				cout << m_ppControlPos[i][j] << " // ";
 				cout << lControlPoints[j] << endl;
			}

			FbxVector4 lCurrentVertex;
			FbxVector4 lCurrentNormal;
			FbxVector2 lCurrentUV;

		}
	}
}

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

			FbxGeometryElementNormal * vertexNormal = pMesh->GetElementNormal();

			switch (vertexNormal->GetMappingMode())
			{
			case FbxGeometryElement::eDirect:
			{
				m_ppNormal[i].x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(i).mData[0]);
				m_ppNormal[i].y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(i).mData[1]);
				m_ppNormal[i].z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(i).mData[2]);
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
