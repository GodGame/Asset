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
	m_ppControlPos = nullptr;
	m_ppPos = nullptr;

	m_nObjects  = 0;

	m_pvcVertex = nullptr;
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
	if (m_ppPos)
	{
		for (int i = 0; i < m_nObjects; ++i)
			if (m_ppPos[i]) delete[] m_ppPos[i];
		delete[] m_ppPos;
	}
	if (m_ppNormal)
	{
		for (int i = 0; i < m_nObjects; ++i)
			if (m_ppNormal[i]) delete[] m_ppNormal[i];
		delete[] m_ppNormal;
	}

	if (m_pvcVertex) delete[] m_pvcVertex;
}

bool FBXParser::Initialize(const char * pstr)
{
	m_stName = pstr;

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
	//m_ppControlPos = new XMFLOAT3*[m_nObjects];
	//m_ppPos = new XMFLOAT3*[m_nObjects];
	//m_ppNormal = new XMFLOAT3*[m_nObjects];
	m_pvcVertex = new vector<Vertex>[m_nObjects];

	//for (auto it = m_wstName.begin(); it != m_wstName.end(); ++it)
	//	cout << *it;
	//cout << endl;

	for (int i = 0; i < m_nObjects; ++i)
	{
		//m_ppPos[i] = nullptr;
		//m_ppNormal[i] = nullptr;
		//m_ppControlPos[i] = nullptr;

		FbxNode * pNode = m_pRootNode->GetChild(i);
		FbxNodeAttribute::EType type = (pNode->GetNodeAttribute())->GetAttributeType();

		if (type == FbxNodeAttribute::eMesh)
		{
			FbxMesh* pMesh = pNode->GetMesh();
			UINT vertexCounter = 0;
			UINT mTriangleCount = pMesh->GetPolygonCount();

			//m_ppPos[i] = new XMFLOAT3[mTriangleCount];
			//m_ppNormal[i] = new XMFLOAT3[mTriangleCount];
			m_pvcVertex[i].reserve(mTriangleCount * 3);

			FbxVector4 * lControlPoints = pMesh->GetControlPoints();
			Vertex TempVertex;

			for (int j = 0; j < mTriangleCount; ++j)
			{
				for (int k = 0; k < 3; ++k)
				{
					ZeroMemory(&TempVertex, sizeof(TempVertex));

					int ctrlPointIndex = pMesh->GetPolygonVertex(j, k);
					FbxVector4 ctrlPoint = lControlPoints[ctrlPointIndex];

					NormalRead(pMesh, ctrlPointIndex, vertexCounter, TempVertex.xmf3Normal);
					UVRead(pMesh, ctrlPointIndex, pMesh->GetTextureUVIndex(j, k), TempVertex.xmf2TexCoord);
					//pMesh->GetTextureUV()

					TempVertex.xmf3Pos = XMFLOAT3(ctrlPoint.mData[0], ctrlPoint.mData[1], ctrlPoint.mData[2]);

					cout << TempVertex.xmf3Pos << " // " << TempVertex.xmf3Normal << " // " << TempVertex.xmf2TexCoord << endl;
					++vertexCounter;
				}
				//m_ppPos[i][j].x = lControlPoints[j].mData[0];
				//m_ppPos[i][j].y = lControlPoints[j].mData[1];
				//m_ppPos[i][j].z = lControlPoints[j].mData[2];
				//m_ppControlPos[i][j].w = lControlPoints[j].mData[3];

				//cout << m_ppPos[i][j] << endl;
			}

			//FbxVector4 lCurrentVertex;
			//FbxVector4 lCurrentNormal;
			//FbxVector2 lCurrentUV;

			//for(auto it = lControlPoints.b)
		}
	}
}

void FBXParser::ControlPointRead()
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

void FBXParser::FileOut()
{
	wstring FileName;

	for (auto it = m_stName.begin(); it != m_stName.end() - 4; ++it)
	{
		FileName.push_back(*it);
	}

	{
		//sprintf(fileName, "%ws.info", filename);
		FileName = FileName + _T(".info");
		wofstream out(FileName, ios::out);

		for (int i = 0; i < m_nObjects; ++i)
		{
			out << "Object " << i << " : " << m_pvcVertex[i].size() << endl;

			for (auto it = m_pvcVertex[i].begin(); it != m_pvcVertex[i].end(); ++it)
			{
				out << "Pos : " << it->xmf3Pos << endl;
				out << "Tex : " << it->xmf2TexCoord << endl;
				out << "Normal : " << it->xmf3Normal << endl;
			}
		}


		//for (auto it = vertices.begin(); it != vertices.end(); ++it)
		//{
		//	out << "Pos : " << it->pos.x << ", " << it->pos.y << ", " << it->pos.z << "\t";
		//	out << "Tex : " << it->texCoord.x << ", " << it->texCoord.y << "\t";
		//	out << "Nor : " << it->normal.x << ", " << it->normal.y << ", " << it->normal.z << "\t";
		//	out << "Tan : " << it->tangent.x << ", " << it->tangent.y << ", " << it->tangent.z << endl;
		//}
		//out << endl << endl;

		//out << "index : " << indices.size() << endl;
		//for (auto it = indices.begin(); it != indices.end(); ++it)
		//{
		//	out << (*it) << " ";
		//}

		//out << endl << endl;

		//out << "SubMeshes Num : " << meshSubsets << " / UseMtl Num : " << mtlIndex.size() << endl;// " / " << material.size() << " / " << meshSubsetTexture.size() << endl;

		//for (int i = 0; i < mtlIndex.size(); ++i)
		//{
		//	out << "index Start: " << mtlIndex[i] << "\t\t"; //meshSubsetIndexStart[i] << endl;

		//													 //int sz = material.size() * i;
		//													 //wstring temp = material[sz + i].matName;

		//													 //for (int j = 0; j < meshMaterials.size(); ++j) {
		//	out << "Mat : " << meshMaterials[i] << "\t";//material[sz + j].matName;
		//												//for (auto it = material[i].matName.begin(); it != material[i].matName.end(); ++it)
		//												//	out << (char*)(*it);		
		//												//out << ", Tx : "  << meshSubsetTexture[sz + i] << endl;
		//												//}
		//	out << endl;
		//	//out << "Texture : " << material[i].matName << endl;
		//	//material[i].
		//	//out << "Texture Array Index" << material[meshSubsetTexture[i]].texArrayIndex << endl;
		//}
		/*
		for (auto it = meshSubsetIndexStart.begin(); it != meshSubsetIndexStart.end(); ++it)
		{
		out << *it << ", ";
		}
		out << endl;
		*/
		out.close();
	}
	//{
	//	wstring binaryName = FileName + _T(".chae");
	//	char name[100];
	//	int index = 0;
	//	for (auto it = binaryName.begin(); it != binaryName.end(); ++it)
	//	{
	//		name[index++] = (*it);
	//	}
	//	name[index] = '\0';

	//	FILE * bin;
	//	bin = fopen(name, "wb");

	//	//bin << vertices.size();
	//	UINT sz = vertices.size();
	//	fwrite(&sz, sizeof(UINT), 1, bin);
	//	for (auto it = vertices.begin(); it != vertices.end(); ++it)
	//	{
	//		fwrite(&it->pos, sizeof(XMFLOAT3), 1, bin);
	//		fwrite(&it->texCoord, sizeof(XMFLOAT2), 1, bin);
	//		fwrite(&it->normal, sizeof(XMFLOAT3), 1, bin);
	//		fwrite(&it->tangent, sizeof(XMFLOAT3), 1, bin);
	//	}

	//	sz = indices.size();
	//	fwrite(&sz, sizeof(UINT), 1, bin);
	//	//bin << indices.size();

	//	for (auto it = indices.begin(); it != indices.end(); ++it)
	//	{
	//		fwrite(&(*it), sizeof(UINT), 1, bin);
	//		//bin << (*it);
	//	}
	//	fclose(bin);
	//}
}

void FBXParser::NormalRead(FbxMesh * pMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3 & outNormal)
{
	if (pMesh->GetElementNormalCount() < 1)
	{
		throw std::exception("Invalid Normal Number");
	}

	FbxGeometryElementNormal* vertexNormal = pMesh->GetElementNormal(0);
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