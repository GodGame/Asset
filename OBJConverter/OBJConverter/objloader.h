#pragma once
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include <Windows.h>
#include <fstream>
#include <istream>
#include <sstream>
#include <random>
#include <ctime>
#include <functional>
#include <list>
#include <DirectXMath.h>
using namespace DirectX;
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <d3d11.h>
#include <d3dcompiler.h>  	//쉐이더 컴파일 함수를 사용하기 위한 헤더 파일
#include <Mmsystem.h>
#include <vector>
using namespace std;

struct cbPerObject
{
	XMMATRIX  WVP;
	XMMATRIX World;

	XMFLOAT4 difColor;
	bool hasTexture;
	BOOL hasNormMap;

	BOOL isInstance;
};

struct InstanceData
{
	XMFLOAT3 pos;
	int hp;
	bool disenable;
	int cube_num;
};

struct SurfaceMaterial
{
	std::wstring matName;
	XMFLOAT4 difColor;
	int texArrayIndex;
	int normMapTexArrayIndex;
	bool hasNormMap;
	bool hasTexture;
	bool transparent;
};

struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z,
		float u, float v)
		: pos(x, y, z), texCoord(u, v) {}
	Vertex(float x, float y, float z,
		float u, float v,
		float nx, float ny, float nz,
		float tx, float ty, float tz)
		: pos(x, y, z), texCoord(u, v), normal(nx, ny, nz),
		tangent(tx, ty, tz) {}

	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT3 biTangent;
};

class Cube_POS_LIST {
private:
public:
	int Node_BX;
	int Node_BZ;
	int Node_BY;
	Cube_POS_LIST() {};
	Cube_POS_LIST(int BX, int BZ, int BY) {
		Node_BX = BX;
		Node_BZ = BZ;
		Node_BY = BY;
	}
};

class ObjModel
{
protected:
	ID3D11BlendState* Transparency;
	ID3D11BlendState* effect_Transparency;
	ID3D11Buffer* meshVertBuff;
	ID3D11Buffer* meshIndexBuff;

	XMFLOAT3	 		m_Up;
	XMFLOAT3 		m_Look;
	XMFLOAT3			m_Right;
	int meshSubsets = 0;


	//std::vector<ID3D11ShaderResourceView*> meshSRV;


	//std::vector<SurfaceMaterial> material;

	//
	ID3D11Device* m_pd3dDevice;

	ID3D11SamplerState* CubesTexSamplerState;
	ID3D11Buffer* cbPerObjectBuffer;

	ID3D11InputLayout* vertLayout;
	ID3D11VertexShader* m_VS;
	ID3D11PixelShader* m_PS;
	ID3D10Blob* m_VS_Buffer;
	ID3D10Blob* m_PS_Buffer;

	ID3D11RasterizerState* RSCullNone;

	//
	std::vector<XMFLOAT3> objAABB;

	int m_HP;						// 체력
	bool m_Enable;					// 활성화상태

public:
	ID3D11ShaderResourceView* effectsprite[25];
	XMFLOAT4X4 meshWorld;
	std::vector<std::wstring> textureNameArray;
	std::vector<SurfaceMaterial> material;
	std::vector<int> meshSubsetIndexStart;
	std::vector<int> meshSubsetTexture;
	std::vector<ID3D11ShaderResourceView*> meshSRV;
	ObjModel();
	virtual ~ObjModel();
	bool LoadObjModel(std::wstring filename,			//.obj filename
		bool isRHCoordSys,							//true if model was created in right hand coord system
		bool computeNormals);						//true to compute the normals, false to use the files normals
	XMFLOAT4X4	GetWorld() { return meshWorld; }
	XMFLOAT3		GetRight() { return m_Right; }
	XMFLOAT3		GetUp() { return m_Up; }
	XMFLOAT3    GetLook() { return m_Look; }
	int GetHP() { return m_HP; }
	bool GetEnable() { return m_Enable; }

	void		SetWorld(XMFLOAT4X4 world) { meshWorld = world; }
	void		SetRight(XMFLOAT3 right) { m_Right = right; }
	void		SetUp(XMFLOAT3 up) { m_Up = up; }
	void    SetLook(XMFLOAT3 look) { m_Look = look; }
};
