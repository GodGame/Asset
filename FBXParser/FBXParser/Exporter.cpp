//#include "FBXExporter.h"
//
//
//
//FBXExporter::FBXExporter()
//{
//}
//
//
//FBXExporter::~FBXExporter()
//{
//}
//
//void FBXExporter::ProcessMesh(FbxNode* inNode)
//{
//	FbxMesh* currMesh = inNode->GetMesh();
//
//	mTriangleCount = currMesh->GetPolygonCount();
//	int vertexCounter = 0;
//	mTriangles.reserve(mTriangleCount);
//
//	for (unsigned int i = 0; i < mTriangleCount; ++i)
//	{
//		XMFLOAT3 normal[3];
//		XMFLOAT3 tangent[3];
//		XMFLOAT3 binormal[3];
//		XMFLOAT2 UV[3][2];
//		Triangle currTriangle;
//		mTriangles.push_back(currTriangle);
//
//		for (unsigned int j = 0; j < 3; ++j)
//		{
//			int ctrlPointIndex = currMesh->GetPolygonVertex(i, j);
//			CtrlPoint* currCtrlPoint = mControlPoints[ctrlPointIndex];
//
//
//			ReadNormal(currMesh, ctrlPointIndex, vertexCounter, normal[j]);
//			// We only have diffuse texture
//			for (int k = 0; k < 1; ++k)
//			{
//				ReadUV(currMesh, ctrlPointIndex, currMesh->GetTextureUVIndex(i, j), k, UV[j][k]);
//			}
//
//
//			PNTIWVertex temp;
//			temp.mPosition = currCtrlPoint->mPosition;
//			temp.mNormal = normal[j];
//			temp.mUV = UV[j][0];
//			// Copy the blending info from each control point
//			for (unsigned int i = 0; i < currCtrlPoint->mBlendingInfo.size(); ++i)
//			{
//				VertexBlendingInfo currBlendingInfo;
//				currBlendingInfo.mBlendingIndex = currCtrlPoint->mBlendingInfo[i].mBlendingIndex;
//				currBlendingInfo.mBlendingWeight = currCtrlPoint->mBlendingInfo[i].mBlendingWeight;
//				temp.mVertexBlendingInfos.push_back(currBlendingInfo);
//			}
//			// Sort the blending info so that later we can remove
//			// duplicated vertices
//			temp.SortBlendingInfoByWeight();
//
//			mVertices.push_back(temp);
//			mTriangles.back().mIndices.push_back(vertexCounter);
//			++vertexCounter;
//		}
//	}
//
//	// Now mControlPoints has served its purpose
//	// We can free its memory
//	for (auto itr = mControlPoints.begin(); itr != mControlPoints.end(); ++itr)
//	{
//		delete itr->second;
//	}
//	mControlPoints.clear();
//}
//
//// inNode is the Node in this FBX Scene that contains the mesh
//// this is why I can use inNode->GetMesh() on it to get the mesh
//void FBXExporter::ProcessControlPoints(FbxNode* inNode)
//{
//	FbxMesh* currMesh = inNode->GetMesh();
//	unsigned int ctrlPointCount = currMesh->GetControlPointsCount();
//	for (unsigned int i = 0; i < ctrlPointCount; ++i)
//	{
//		CtrlPoint* currCtrlPoint = new CtrlPoint();
//		XMFLOAT3 currPosition;
//		currPosition.x = static_cast<float>(currMesh->GetControlPointAt(i).mData[0]);
//		currPosition.y = static_cast<float>(currMesh->GetControlPointAt(i).mData[1]);
//		currPosition.z = static_cast<float>(currMesh->GetControlPointAt(i).mData[2]);
//		currCtrlPoint->mPosition = currPosition;
//		mControlPoints[i] = currCtrlPoint;
//	}
//}
//
//
//
//void FBXExporter::ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal)
//{
//	if (inMesh->GetElementNormalCount() < 1)
//	{
//		throw std::exception("Invalid Normal Number");
//	}
//
//	FbxGeometryElementNormal* vertexNormal = inMesh->GetElementNormal(0);
//	switch (vertexNormal->GetMappingMode())
//	{
//	case FbxGeometryElement::eByControlPoint:
//		switch (vertexNormal->GetReferenceMode())
//		{
//		case FbxGeometryElement::eDirect:
//		{
//			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[0]);
//			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[1]);
//			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inCtrlPointIndex).mData[2]);
//		}
//		break;
//
//		case FbxGeometryElement::eIndexToDirect:
//		{
//			int index = vertexNormal->GetIndexArray().GetAt(inCtrlPointIndex);
//			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
//			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
//			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
//		}
//		break;
//
//		default:
//			throw std::exception("Invalid Reference");
//		}
//		break;
//
//	case FbxGeometryElement::eByPolygonVertex:
//		switch (vertexNormal->GetReferenceMode())
//		{
//		case FbxGeometryElement::eDirect:
//		{
//			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[0]);
//			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[1]);
//			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(inVertexCounter).mData[2]);
//		}
//		break;
//
//		case FbxGeometryElement::eIndexToDirect:
//		{
//			int index = vertexNormal->GetIndexArray().GetAt(inVertexCounter);
//			outNormal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
//			outNormal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
//			outNormal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
//		}
//		break;
//
//		default:
//			throw std::exception("Invalid Reference");
//		}
//		break;
//	}
//}
//
//
