#pragma once
#include "stdafx.h"

class Triangle
{
	XMFLOAT3 Vertex[3];
};

class FBXExporter
{
	int mTriangleCount;
	vector<Triangle> mTriangles;

public:
	FBXExporter();
	~FBXExporter();

	void ProcessMesh(FbxNode* inNode);
	void ProcessControlPoints(FbxNode* inNode);
	void ReadNormal(FbxMesh* inMesh, int inCtrlPointIndex, int inVertexCounter, XMFLOAT3& outNormal);
};

