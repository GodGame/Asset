#include "objloader.h"
/*
class Cube_POS_LIST{
private:
public:
int Node_BX;
int Node_BZ;
int Node_BY;
Cube_POS_LIST(){};
Cube_POS_LIST(int BX, int BZ, int BY){
Node_BX = BX;
Node_BZ = BZ;
Node_BY = BY;
}
};

std::istream & operator>>(std::istream &is, Cube_POS_LIST &a)
{
is >> a.Node_BX >> a.Node_BZ >> a.Node_BY;
return is;
}




static Cube_POS_LIST UsingList;
static std::list<Cube_POS_LIST> Cube_Pos;
static Cube_POS_LIST UsingList_IRON;
static std::list<Cube_POS_LIST> Cube_Pos_IRON;
*/




std::istream & operator>>(std::istream &is, Cube_POS_LIST &a)
{
	is >> a.Node_BX >> a.Node_BZ >> a.Node_BY;
	return is;
}



static Cube_POS_LIST UsingList;
static Cube_POS_LIST UsingList_IRON;

void cullAABB(ID3D11DeviceContext* pd3dDeviceContext, std::vector<XMFLOAT4>& frustumPlanes, int& numObjToDraw,
	int& numObj, std::vector<XMFLOAT3> objAABB, std::list<InstanceData*> instanceData, ID3D11Buffer* instanceBuff)
{
	numObjToDraw = 0;

	std::vector<InstanceData> tempTreeInstDat(numObj);

	bool cull = false;

	for (auto inst : instanceData)
	{
		cull = false;
		for (int planeID = 0; planeID < 6; ++planeID)
		{
			XMVECTOR planeNormal = XMVectorSet(frustumPlanes[planeID].x, frustumPlanes[planeID].y, frustumPlanes[planeID].z, 0.0f);
			float planeConstant = frustumPlanes[planeID].w;

			XMFLOAT3 axisVert;

			// x-axis
			if (frustumPlanes[planeID].x < 0.0f)
				axisVert.x = objAABB[0].x + inst->pos.x;
			else
				axisVert.x = objAABB[1].x + inst->pos.x;

			// y-axis
			if (frustumPlanes[planeID].y < 0.0f)
				axisVert.y = objAABB[0].y + inst->pos.y;
			else
				axisVert.y = objAABB[1].y + inst->pos.y;

			// z-axis
			if (frustumPlanes[planeID].z < 0.0f)
				axisVert.z = objAABB[0].z + inst->pos.z;
			else
				axisVert.z = objAABB[1].z + inst->pos.z;

			// 보이지않게 설정한거 컬링
			if (XMVectorGetX(XMVector3Dot(planeNormal, XMLoadFloat3(&axisVert))) + planeConstant < 0.0f || inst->disenable == true)
			{
				cull = true;
				break;
			}
		}

		if (!cull)
		{
			tempTreeInstDat[numObjToDraw].pos = inst->pos;

			numObjToDraw++;
		}
	}

	if (numObj)
		pd3dDeviceContext->UpdateSubresource(instanceBuff, 0, NULL, &tempTreeInstDat[0], 0, 0);
}


//-----------------------------------------------------------------------------------------------------------------------------------

ObjModel::ObjModel() : m_pd3dDevice(nullptr), m_HP(100), m_Enable(false)
{
	m_Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pd3dDevice = nullptr;
	vertLayout = NULL;
	cbPerObjectBuffer = NULL;

	m_VS = NULL;
	m_PS = NULL;
	m_VS_Buffer = NULL;
	m_PS_Buffer = NULL;

	meshVertBuff = NULL;
	meshIndexBuff = NULL;
	RSCullNone = NULL;
	CubesTexSamplerState = NULL;
	Transparency = NULL;
	effect_Transparency = NULL;
	for (int i = 0; i < 25; ++i) {
		effectsprite[i] = NULL;
	}
	for (int i = 0; i < meshSRV.size(); ++i)
		meshSRV[i] = NULL;
}

ObjModel::~ObjModel()
{

}

bool ObjModel::LoadObjModel(std::wstring filename,
	bool isRHCoordSys,
	bool computeNormals)
{
	HRESULT hr = 0;

	std::wifstream fileIn(filename.c_str());	//Open file
	std::wstring meshMatLib;					//String to hold our obj material library filename

												//Arrays to store our model's information
	std::vector<DWORD> mtlIndex;
	std::vector<DWORD> indices;
	std::vector<XMFLOAT3> vertPos;
	std::vector<XMFLOAT3> vertNorm;
	std::vector<XMFLOAT2> vertTexCoord;
	std::vector<std::wstring> meshMaterials;
	std::vector<std::wstring> fileNames;

	//Vertex definition indices
	std::vector<int> vertPosIndex;
	std::vector<int> vertNormIndex;
	std::vector<int> vertTCIndex;

	//Make sure we have a default if no tex coords or normals are defined
	bool hasTexCoord = false;
	bool hasNorm = false;

	//Temp variables to store into vectors
	std::wstring meshMaterialsTemp;
	int vertPosIndexTemp;
	int vertNormIndexTemp;
	int vertTCIndexTemp;

	wchar_t checkChar;		//The variable we will use to store one char from file at a time
	std::wstring face;		//Holds the string containing our face vertices
	int vIndex = 0;			//Keep track of our vertex index count
	int triangleCount = 0;	//Total Triangles
	int totalVerts = 0;
	int meshTriangles = 0;

	//Check to see if the file was opened
	if (fileIn)
	{
		while (fileIn)
		{
			checkChar = fileIn.get();	//Get next char

			switch (checkChar)
			{
			case '#':
				checkChar = fileIn.get();
				while (checkChar != '\n')
					checkChar = fileIn.get();
				break;
			case 'v':	//Get Vertex Descriptions
				checkChar = fileIn.get();
				if (checkChar == ' ')	//v - vert position
				{
					float vz, vy, vx;
					fileIn >> vx >> vy >> vz;	//Store the next three types

					if (isRHCoordSys)	//If model is from an RH Coord System
						vertPos.push_back(XMFLOAT3(vx, vy, vz * -1.0f));	//Invert the Z axis
					else
						vertPos.push_back(XMFLOAT3(vx, vy, vz));
				}
				if (checkChar == 't')	//vt - vert tex coords
				{
					float vtcu, vtcv;
					fileIn >> vtcu >> vtcv;		//Store next two types

					if (isRHCoordSys)	//If model is from an RH Coord System
						vertTexCoord.push_back(XMFLOAT2(vtcu, 1.0f - vtcv));	//Reverse the "v" axis
					else
						vertTexCoord.push_back(XMFLOAT2(vtcu, vtcv));

					hasTexCoord = true;	//We know the model uses texture coords
				}
				//Since we compute the normals later, we don't need to check for normals
				//In the file, but i'll do it here anyway
				if (checkChar == 'n')	//vn - vert normal
				{
					float vnx, vny, vnz;
					fileIn >> vnx >> vny >> vnz;	//Store next three types

					if (isRHCoordSys)	//If model is from an RH Coord System
						vertNorm.push_back(XMFLOAT3(vnx, vny, vnz * -1.0f));	//Invert the Z axis
					else
						vertNorm.push_back(XMFLOAT3(vnx, vny, vnz));

					hasNorm = true;	//We know the model defines normals
				}
				break;

				//New group (Subset)
			case 'g':	//g - defines a group
				checkChar = fileIn.get();
				if (checkChar == ' ')
				{
					meshSubsetIndexStart.push_back(vIndex);		//Start index for this subset
					meshSubsets++;
				}
				break;

				//Get Face Index
			case 'f':	//f - defines the faces
				checkChar = fileIn.get();
				if (checkChar == ' ')
				{
					face = L"";
					std::wstring VertDef;	//Holds one vertex definition at a time
					triangleCount = 0;

					checkChar = fileIn.get();
					while (checkChar != '\n')
					{
						face += checkChar;			//Add the char to our face string
						checkChar = fileIn.get();	//Get the next Character
						if (checkChar == ' ')		//If its a space...
							triangleCount++;		//Increase our triangle count
					}

					//Check for space at the end of our face string
					if (face[face.length() - 1] == ' ')
						triangleCount--;	//Each space adds to our triangle count

					triangleCount -= 1;		//Ever vertex in the face AFTER the first two are new faces

					std::wstringstream ss(face);

					if (face.length() > 0)
					{
						int firstVIndex, lastVIndex;	//Holds the first and last vertice's index

						for (int i = 0; i < 3; ++i)		//First three vertices (first triangle)
						{
							ss >> VertDef;	//Get vertex definition (vPos/vTexCoord/vNorm)

							std::wstring vertPart;
							int whichPart = 0;		//(vPos, vTexCoord, or vNorm)

													//Parse this string
							for (int j = 0; j < VertDef.length(); ++j)
							{
								if (VertDef[j] != '/')	//If there is no divider "/", add a char to our vertPart
									vertPart += VertDef[j];

								//If the current char is a divider "/", or its the last character in the string
								if (VertDef[j] == '/' || j == VertDef.length() - 1)
								{
									std::wistringstream wstringToInt(vertPart);	//Used to convert wstring to int

									if (whichPart == 0)	//If vPos
									{
										wstringToInt >> vertPosIndexTemp;
										vertPosIndexTemp -= 1;		//subtract one since c++ arrays start with 0, and obj start with 1

																	//Check to see if the vert pos was the only thing specified
										if (j == VertDef.length() - 1)
										{
											vertNormIndexTemp = 0;
											vertTCIndexTemp = 0;
										}
									}

									else if (whichPart == 1)	//If vTexCoord
									{
										if (vertPart != L"")	//Check to see if there even is a tex coord
										{
											wstringToInt >> vertTCIndexTemp;
											vertTCIndexTemp -= 1;	//subtract one since c++ arrays start with 0, and obj start with 1
										}
										else	//If there is no tex coord, make a default
											vertTCIndexTemp = 0;

										//If the cur. char is the second to last in the string, then
										//there must be no normal, so set a default normal
										if (j == VertDef.length() - 1)
											vertNormIndexTemp = 0;

									}
									else if (whichPart == 2)	//If vNorm
									{
										std::wistringstream wstringToInt(vertPart);

										wstringToInt >> vertNormIndexTemp;
										vertNormIndexTemp -= 1;		//subtract one since c++ arrays start with 0, and obj start with 1
									}

									vertPart = L"";	//Get ready for next vertex part
									whichPart++;	//Move on to next vertex part					
								}
							}

							//Check to make sure there is at least one subset
							if (meshSubsets == 0)
							{
								meshSubsetIndexStart.push_back(vIndex);		//Start index for this subset
								meshSubsets++;
							}

							//Avoid duplicate vertices
							bool vertAlreadyExists = false;
							if (totalVerts >= 3)	//Make sure we at least have one triangle to check
							{
								//Loop through all the vertices
								for (int iCheck = 0; iCheck < totalVerts; ++iCheck)
								{
									//If the vertex position and texture coordinate in memory are the same
									//As the vertex position and texture coordinate we just now got out
									//of the obj file, we will set this faces vertex index to the vertex's
									//index value in memory. This makes sure we don't create duplicate vertices
									if (vertPosIndexTemp == vertPosIndex[iCheck] && !vertAlreadyExists)
									{
										if (vertTCIndexTemp == vertTCIndex[iCheck])
										{
											indices.push_back(iCheck);		//Set index for this vertex
											vertAlreadyExists = true;		//If we've made it here, the vertex already exists
										}
									}
								}
							}

							//If this vertex is not already in our vertex arrays, put it there
							if (!vertAlreadyExists)
							{
								vertPosIndex.push_back(vertPosIndexTemp);
								vertTCIndex.push_back(vertTCIndexTemp);
								vertNormIndex.push_back(vertNormIndexTemp);
								totalVerts++;	//We created a new vertex
								indices.push_back(totalVerts - 1);	//Set index for this vertex
							}

							//If this is the very first vertex in the face, we need to
							//make sure the rest of the triangles use this vertex
							if (i == 0)
							{
								firstVIndex = indices[vIndex];	//The first vertex index of this FACE

							}

							//If this was the last vertex in the first triangle, we will make sure
							//the next triangle uses this one (eg. tri1(1,2,3) tri2(1,3,4) tri3(1,4,5))
							if (i == 2)
							{
								lastVIndex = indices[vIndex];	//The last vertex index of this TRIANGLE
							}
							vIndex++;	//Increment index count
						}

						meshTriangles++;	//One triangle down

											//If there are more than three vertices in the face definition, we need to make sure
											//we convert the face to triangles. We created our first triangle above, now we will
											//create a new triangle for every new vertex in the face, using the very first vertex
											//of the face, and the last vertex from the triangle before the current triangle
						for (int l = 0; l < triangleCount - 1; ++l)	//Loop through the next vertices to create new triangles
						{
							//First vertex of this triangle (the very first vertex of the face too)
							indices.push_back(firstVIndex);			//Set index for this vertex
							vIndex++;

							//Second Vertex of this triangle (the last vertex used in the tri before this one)
							indices.push_back(lastVIndex);			//Set index for this vertex
							vIndex++;

							//Get the third vertex for this triangle
							ss >> VertDef;

							std::wstring vertPart;
							int whichPart = 0;

							//Parse this string (same as above)
							for (int j = 0; j < VertDef.length(); ++j)
							{
								if (VertDef[j] != '/')
									vertPart += VertDef[j];
								if (VertDef[j] == '/' || j == VertDef.length() - 1)
								{
									std::wistringstream wstringToInt(vertPart);

									if (whichPart == 0)
									{
										wstringToInt >> vertPosIndexTemp;
										vertPosIndexTemp -= 1;

										//Check to see if the vert pos was the only thing specified
										if (j == VertDef.length() - 1)
										{
											vertTCIndexTemp = 0;
											vertNormIndexTemp = 0;
										}
									}
									else if (whichPart == 1)
									{
										if (vertPart != L"")
										{
											wstringToInt >> vertTCIndexTemp;
											vertTCIndexTemp -= 1;
										}
										else
											vertTCIndexTemp = 0;
										if (j == VertDef.length() - 1)
											vertNormIndexTemp = 0;

									}
									else if (whichPart == 2)
									{
										std::wistringstream wstringToInt(vertPart);

										wstringToInt >> vertNormIndexTemp;
										vertNormIndexTemp -= 1;
									}

									vertPart = L"";
									whichPart++;
								}
							}

							//Check for duplicate vertices
							bool vertAlreadyExists = false;
							if (totalVerts >= 3)	//Make sure we at least have one triangle to check
							{
								for (int iCheck = 0; iCheck < totalVerts; ++iCheck)
								{
									if (vertPosIndexTemp == vertPosIndex[iCheck] && !vertAlreadyExists)
									{
										if (vertTCIndexTemp == vertTCIndex[iCheck])
										{
											indices.push_back(iCheck);			//Set index for this vertex
											vertAlreadyExists = true;		//If we've made it here, the vertex already exists
										}
									}
								}
							}

							if (!vertAlreadyExists)
							{
								vertPosIndex.push_back(vertPosIndexTemp);
								vertTCIndex.push_back(vertTCIndexTemp);
								vertNormIndex.push_back(vertNormIndexTemp);
								totalVerts++;					//New vertex created, add to total verts
								indices.push_back(totalVerts - 1);		//Set index for this vertex
							}

							//Set the second vertex for the next triangle to the last vertex we got		
							lastVIndex = indices[vIndex];	//The last vertex index of this TRIANGLE

							meshTriangles++;	//New triangle defined
							vIndex++;
						}
					}
				}
				break;

			case 'm':	//mtllib - material library filename
				checkChar = fileIn.get();
				if (checkChar == 't')
				{
					checkChar = fileIn.get();
					if (checkChar == 'l')
					{
						checkChar = fileIn.get();
						if (checkChar == 'l')
						{
							checkChar = fileIn.get();
							if (checkChar == 'i')
							{
								checkChar = fileIn.get();
								if (checkChar == 'b')
								{
									checkChar = fileIn.get();
									if (checkChar == ' ')
									{
										//Store the material libraries file name
										fileIn >> meshMatLib;
									}
								}
							}
						}
					}
				}

				break;

			case 'u':	//usemtl - which material to use
				checkChar = fileIn.get();
				if (checkChar == 's')
				{
					checkChar = fileIn.get();
					if (checkChar == 'e')
					{
						checkChar = fileIn.get();
						if (checkChar == 'm')
						{
							checkChar = fileIn.get();
							if (checkChar == 't')
							{
								checkChar = fileIn.get();
								if (checkChar == 'l')
								{
									checkChar = fileIn.get();
									if (checkChar == ' ')
									{
										meshMaterialsTemp = L"";	//Make sure this is cleared

										fileIn >> meshMaterialsTemp; //Get next type (string)

										meshMaterials.push_back(meshMaterialsTemp);
										mtlIndex.push_back(indices.size());
									}
								}
							}
						}
					}
				}
				break;

			default:
				break;
			}
		}
	}
	else	//If we could not open the file
	{
		//SwapChain->SetFullscreenState(false, NULL);	//Make sure we are out of fullscreen

		//create message
		std::wstring message = L"Could not open: ";
		message += filename;

		MessageBox(0, message.c_str(),	//display message
			L"Error", MB_OK);

		//return false;
	}

	meshSubsetIndexStart.push_back(vIndex); //There won't be another index start after our last subset, so set it here

											//sometimes "g" is defined at the very top of the file, then again before the first group of faces.
											//This makes sure the first subset does not conatain "0" indices.
	if (meshSubsetIndexStart[1] == 0)
	{
		meshSubsetIndexStart.erase(meshSubsetIndexStart.begin() + 1);
		meshSubsets--;
	}

	//Make sure we have a default for the tex coord and normal
	//if one or both are not specified
	if (!hasNorm)
		vertNorm.push_back(XMFLOAT3(0.0f, 0.0f, 0.0f));
	if (!hasTexCoord)
		vertTexCoord.push_back(XMFLOAT2(0.0f, 0.0f));

	//Close the obj file, and open the mtl file
	fileIn.close();
	fileIn.open(meshMatLib.c_str());

	std::wstring lastStringRead;
	int matCount = material.size();	//total materials

									//kdset - If our diffuse color was not set, we can use the ambient color (which is usually the same)
									//If the diffuse color WAS set, then we don't need to set our diffuse color to ambient
	bool kdset = false;

	if (fileIn)
	{
		while (fileIn)
		{
			checkChar = fileIn.get();	//Get next char

			switch (checkChar)
			{
				//Check for comment
			case '#':
				checkChar = fileIn.get();
				while (checkChar != '\n')
					checkChar = fileIn.get();
				break;

				//Set diffuse color
			case 'K':
				checkChar = fileIn.get();
				if (checkChar == 'd')	//Diffuse Color
				{
					checkChar = fileIn.get();	//remove space

					fileIn >> material[matCount - 1].difColor.x;
					fileIn >> material[matCount - 1].difColor.y;
					fileIn >> material[matCount - 1].difColor.z;

					kdset = true;
				}

				//Ambient Color (We'll store it in diffuse if there isn't a diffuse already)
				if (checkChar == 'a')
				{
					checkChar = fileIn.get();	//remove space
					if (!kdset)
					{
						fileIn >> material[matCount - 1].difColor.x;
						fileIn >> material[matCount - 1].difColor.y;
						fileIn >> material[matCount - 1].difColor.z;
					}
				}
				break;

				//Check for transparency
			case 'T':
				checkChar = fileIn.get();
				if (checkChar == 'r')
				{
					checkChar = fileIn.get();	//remove space
					float Transparency;
					fileIn >> Transparency;

					material[matCount - 1].difColor.w = Transparency;

					if (Transparency > 0.0f)
						material[matCount - 1].transparent = true;
				}
				break;

				//Some obj files specify d for transparency
			case 'd':
				checkChar = fileIn.get();
				if (checkChar == ' ')
				{
					float Transparency;
					fileIn >> Transparency;

					//'d' - 0 being most transparent, and 1 being opaque, opposite of Tr
					Transparency = 1.0f - Transparency;

					material[matCount - 1].difColor.w = Transparency;

					if (Transparency > 0.0f)
						material[matCount - 1].transparent = true;
				}
				break;

				//Get the diffuse map (texture)
			case 'm':
				checkChar = fileIn.get();
				if (checkChar == 'a')
				{
					checkChar = fileIn.get();
					if (checkChar == 'p')
					{
						checkChar = fileIn.get();
						if (checkChar == '_')
						{
							//map_Kd - Diffuse map
							checkChar = fileIn.get();
							if (checkChar == 'K')
							{
								checkChar = fileIn.get();
								if (checkChar == 'd')
								{
									std::wstring fileNamePath;

									fileIn.get();	//Remove whitespace between map_Kd and file

													//Get the file path - We read the pathname char by char since
													//pathnames can sometimes contain spaces, so we will read until
													//we find the file extension
									bool texFilePathEnd = false;
									while (!texFilePathEnd)
									{
										checkChar = fileIn.get();

										fileNamePath += checkChar;

										if (checkChar == '.')
										{
											for (int i = 0; i < 3; ++i)
												fileNamePath += fileIn.get();

											texFilePathEnd = true;
										}
									}

									//check if this texture has already been loaded
									bool alreadyLoaded = false;
									for (int i = 0; i < textureNameArray.size(); ++i)
									{
										if (fileNamePath == textureNameArray[i])
										{
											alreadyLoaded = true;
											material[matCount - 1].texArrayIndex = i;
											material[matCount - 1].hasTexture = true;
										}
									}

									//if the texture is not already loaded, load it now
									if (!alreadyLoaded)
									{
										ID3D11ShaderResourceView* tempMeshSRV;
										fileNames.push_back(fileNamePath.c_str());
										//hr = D3DX11CreateShaderResourceViewFromFile(m_pd3dDevice, fileNamePath.c_str(),
										//	NULL, NULL, &tempMeshSRV, NULL);
										if (SUCCEEDED(hr))
										{
											textureNameArray.push_back(fileNamePath.c_str());
											material[matCount - 1].texArrayIndex = meshSRV.size();
											meshSRV.push_back(tempMeshSRV);
											material[matCount - 1].hasTexture = true;
										}
									}
								}
							}
							//map_d - alpha map
							else if (checkChar == 'd')
							{
								//Alpha maps are usually the same as the diffuse map
								//So we will assume that for now by only enabling
								//transparency for this material, as we will already
								//be using the alpha channel in the diffuse map
								material[matCount - 1].transparent = true;
							}

							//map_bump - bump map (we're usinga normal map though)
							else if (checkChar == 'b')
							{
								checkChar = fileIn.get();
								if (checkChar == 'u')
								{
									checkChar = fileIn.get();
									if (checkChar == 'm')
									{
										checkChar = fileIn.get();
										if (checkChar == 'p')
										{
											std::wstring fileNamePath;

											fileIn.get();	//Remove whitespace between map_bump and file

															//Get the file path - We read the pathname char by char since
															//pathnames can sometimes contain spaces, so we will read until
															//we find the file extension
											bool texFilePathEnd = false;
											while (!texFilePathEnd)
											{
												checkChar = fileIn.get();

												fileNamePath += checkChar;

												if (checkChar == '.')
												{
													for (int i = 0; i < 3; ++i)
														fileNamePath += fileIn.get();

													texFilePathEnd = true;
												}
											}

											//check if this texture has already been loaded
											bool alreadyLoaded = false;
											for (int i = 0; i < textureNameArray.size(); ++i)
											{
												if (fileNamePath == textureNameArray[i])
												{
													alreadyLoaded = true;
													material[matCount - 1].normMapTexArrayIndex = i;
													material[matCount - 1].hasNormMap = true;
												}
											}

											//if the texture is not already loaded, load it now
											if (!alreadyLoaded)
											{
												ID3D11ShaderResourceView* tempMeshSRV;
												fileNames.push_back(fileNamePath.c_str());
												//hr = D3DX11CreateShaderResourceViewFromFile(m_pd3dDevice, fileNamePath.c_str(),
												//	NULL, NULL, &tempMeshSRV, NULL);
												if (SUCCEEDED(hr))
												{
													textureNameArray.push_back(fileNamePath.c_str());
													material[matCount - 1].normMapTexArrayIndex = meshSRV.size();
													meshSRV.push_back(tempMeshSRV);
													material[matCount - 1].hasNormMap = true;
												}
											}
										}
									}
								}
							}
						}
					}
				}
				break;

			case 'n':	//newmtl - Declare new material
				checkChar = fileIn.get();
				if (checkChar == 'e')
				{
					checkChar = fileIn.get();
					if (checkChar == 'w')
					{
						checkChar = fileIn.get();
						if (checkChar == 'm')
						{
							checkChar = fileIn.get();
							if (checkChar == 't')
							{
								checkChar = fileIn.get();
								if (checkChar == 'l')
								{
									checkChar = fileIn.get();
									if (checkChar == ' ')
									{
										//New material, set its defaults
										SurfaceMaterial tempMat;
										material.push_back(tempMat);
										fileIn >> material[matCount].matName;
										material[matCount].transparent = false;
										material[matCount].hasTexture = false;
										material[matCount].hasNormMap = false;
										material[matCount].normMapTexArrayIndex = 0;
										material[matCount].texArrayIndex = 0;
										matCount++;
										kdset = false;
									}
								}
							}
						}
					}
				}
				break;

			default:
				break;
			}
		}
	}
	else
	{
		//SwapChain->SetFullscreenState(false, NULL);	//Make sure we are out of fullscreen

		std::wstring message = L"Could not open: ";
		message += meshMatLib;

		MessageBox(0, message.c_str(),
			L"Error", MB_OK);

		return false;
	}

	fileIn.close();


	//Set the subsets material to the index value
	//of the its material in our material array
	for (int i = 0; i < meshSubsets; ++i)
	{
		bool hasMat = false;
		for (int j = 0; j < material.size(); ++j)
		{
			if (meshMaterials[i] == material[j].matName)
			{
				meshSubsetTexture.push_back(j);
				hasMat = true;
			}
		}
		if (!hasMat)
			meshSubsetTexture.push_back(0); //Use first material in array
	}



	std::vector<Vertex> vertices;
	Vertex tempVert;

	//Create our vertices using the information we got 
	//from the file and store them in a vector
	for (int j = 0; j < totalVerts; ++j)
	{
		tempVert.pos = vertPos[vertPosIndex[j]];
		tempVert.normal = vertNorm[vertNormIndex[j]];
		tempVert.texCoord = vertTexCoord[vertTCIndex[j]];

		vertices.push_back(tempVert);
	}







	////////////////////////Compute Normals///////////////////////////
	//If computeNormals was set to true then we will create our own
	//normals, if it was set to false we will use the obj files normals
	if (computeNormals)
	{
		std::vector<XMFLOAT3> tempNormal;

		//normalized and unnormalized normals
		XMFLOAT3 unnormalized = XMFLOAT3(0.0f, 0.0f, 0.0f);

		//tangent stuff
		std::vector<XMFLOAT3> tempTangent;
		XMFLOAT3 tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
		float tcU1, tcV1, tcU2, tcV2;

		//Used to get vectors (sides) from the position of the verts
		float vecX, vecY, vecZ;

		//Two edges of our triangle
		XMVECTOR edge1 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR edge2 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		//Compute face normals
		//And Tangents
		for (int i = 0; i < meshTriangles; ++i)
		{
			//Get the vector describing one edge of our triangle (edge 0,2)
			vecX = vertices[indices[(i * 3)]].pos.x - vertices[indices[(i * 3) + 2]].pos.x;
			vecY = vertices[indices[(i * 3)]].pos.y - vertices[indices[(i * 3) + 2]].pos.y;
			vecZ = vertices[indices[(i * 3)]].pos.z - vertices[indices[(i * 3) + 2]].pos.z;
			edge1 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our first edge
															//Get the vector describing another edge of our triangle (edge 2,1)
			vecX = vertices[indices[(i * 3) + 2]].pos.x - vertices[indices[(i * 3) + 1]].pos.x;
			vecY = vertices[indices[(i * 3) + 2]].pos.y - vertices[indices[(i * 3) + 1]].pos.y;
			vecZ = vertices[indices[(i * 3) + 2]].pos.z - vertices[indices[(i * 3) + 1]].pos.z;
			edge2 = XMVectorSet(vecX, vecY, vecZ, 0.0f);	//Create our second edge
															//Cross multiply the two edge vectors to get the un-normalized face normal
			XMStoreFloat3(&unnormalized, XMVector3Cross(edge1, edge2));
			tempNormal.push_back(unnormalized);

			//Find first texture coordinate edge 2d vector
			tcU1 = vertices[indices[(i * 3)]].texCoord.x - vertices[indices[(i * 3) + 2]].texCoord.x;
			tcV1 = vertices[indices[(i * 3)]].texCoord.y - vertices[indices[(i * 3) + 2]].texCoord.y;

			//Find second texture coordinate edge 2d vector
			tcU2 = vertices[indices[(i * 3) + 2]].texCoord.x - vertices[indices[(i * 3) + 1]].texCoord.x;
			tcV2 = vertices[indices[(i * 3) + 2]].texCoord.y - vertices[indices[(i * 3) + 1]].texCoord.y;

			//Find tangent using both tex coord edges and position edges
			tangent.x = (tcV1 * XMVectorGetX(edge1) - tcV2 * XMVectorGetX(edge2)) * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1));
			tangent.y = (tcV1 * XMVectorGetY(edge1) - tcV2 * XMVectorGetY(edge2)) * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1));
			tangent.z = (tcV1 * XMVectorGetZ(edge1) - tcV2 * XMVectorGetZ(edge2)) * (1.0f / (tcU1 * tcV2 - tcU2 * tcV1));

			tempTangent.push_back(tangent);
		}

		//Compute vertex normals (normal Averaging)
		XMVECTOR normalSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR tangentSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		int facesUsing = 0;
		float tX, tY, tZ;	//temp axis variables

							//Go through each vertex
		for (int i = 0; i < totalVerts; ++i)
		{
			//Check which triangles use this vertex
			for (int j = 0; j < meshTriangles; ++j)
			{
				if (indices[j * 3] == i ||
					indices[(j * 3) + 1] == i ||
					indices[(j * 3) + 2] == i)
				{
					tX = XMVectorGetX(normalSum) + tempNormal[j].x;
					tY = XMVectorGetY(normalSum) + tempNormal[j].y;
					tZ = XMVectorGetZ(normalSum) + tempNormal[j].z;

					normalSum = XMVectorSet(tX, tY, tZ, 0.0f);	//If a face is using the vertex, add the unormalized face normal to the normalSum

																//We can reuse tX, tY, tZ to sum up tangents
					tX = XMVectorGetX(tangentSum) + tempTangent[j].x;
					tY = XMVectorGetY(tangentSum) + tempTangent[j].y;
					tZ = XMVectorGetZ(tangentSum) + tempTangent[j].z;

					tangentSum = XMVectorSet(tX, tY, tZ, 0.0f); //sum up face tangents using this vertex

					facesUsing++;
				}
			}

			//Get the actual normal by dividing the normalSum by the number of faces sharing the vertex
			normalSum  = normalSum / (float)facesUsing;
			tangentSum = tangentSum / (float)facesUsing;

			//Normalize the normalSum vector and tangent
			normalSum  = -XMVector3Normalize(normalSum);
			tangentSum =  XMVector3Normalize(tangentSum);

			//Store the normal and tangent in our current vertex
			vertices[i].normal.x = XMVectorGetX(normalSum);
			vertices[i].normal.y = XMVectorGetY(normalSum);
			vertices[i].normal.z = XMVectorGetZ(normalSum);

			vertices[i].tangent.x = -XMVectorGetX(tangentSum);
			vertices[i].tangent.y =  XMVectorGetY(tangentSum);
			vertices[i].tangent.z =  XMVectorGetZ(tangentSum);

			//cout << vertices[i].tangent.x <<", " << vertices[i].tangent.y <<", " << vertices[i].tangent.z << endl;

			//Clear normalSum, tangentSum and facesUsing for next vertex
			normalSum  = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			tangentSum = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
			facesUsing = 0;

		}
	}

	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(DWORD)* meshTriangles * 3;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = &indices[0];
	//m_pd3dDevice->CreateBuffer(&indexBufferDesc, &iinitData, &meshIndexBuff);

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));

	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(Vertex)* totalVerts;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferData;

	ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
	vertexBufferData.pSysMem = &vertices[0];
	//hr = m_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &meshVertBuff);

	//

	//cout << "vertex : " << endl;
	//for (auto it = vertPos.begin(); it != vertPos.end(); ++it)
	//{
	//	cout << (*it).x << ", " << (*it).y << ", " << (*it).z << endl;
	//}

	//
//	char fileName[150];

	wstring FileName = filename;
	for (int i = 1; i <= 4; ++i)
	{
		FileName.pop_back();
	}

	{
		wstring SaveName = FileName + _T(".info");
		//sprintf(fileName, "%ws.info", filename);
		wofstream out(SaveName, ios::out);

		out << "PNT Count : " << vertices.size() << endl;
		for (auto it = vertices.begin(); it != vertices.end(); ++it)
		{
			out << "Pos : " << it->pos.x << ", " << it->pos.y << ", " << it->pos.z << "\t";
			out << "Tex : " << it->texCoord.x << ", " << it->texCoord.y << "\t";
			out << "Nor : " << it->normal.x << ", " << it->normal.y << ", " << it->normal.z << "\t";
			out << "Tan : " << it->tangent.x << ", " << it->tangent.y << ", " << it->tangent.z << endl;
		}
		out << endl << endl;

		out << "index : " << indices.size() << endl;
		for (auto it = indices.begin(); it != indices.end(); ++it)
		{
			out << (*it) << " ";
		}

		out << endl << endl;

		out << "SubMeshes Num : " << meshSubsets << " / UseMtl Num : " << mtlIndex.size() << endl;// " / " << material.size() << " / " << meshSubsetTexture.size() << endl;
		
		for (int i = 0; i < mtlIndex.size(); ++i)
		{
			out << "index Start: " << mtlIndex[i] << "\t\t"; //meshSubsetIndexStart[i] << endl;
		
			//int sz = material.size() * i;
			//wstring temp = material[sz + i].matName;

			//for (int j = 0; j < meshMaterials.size(); ++j) {
				out << "Mat : " << meshMaterials[i] << "\t";//material[sz + j].matName;
				//for (auto it = material[i].matName.begin(); it != material[i].matName.end(); ++it)
				//	out << (char*)(*it);		
				//out << ", Tx : "  << meshSubsetTexture[sz + i] << endl;
			//}
				out << endl;
				//out << "Texture : " << material[i].matName << endl;
				//material[i].
				//out << "Texture Array Index" << material[meshSubsetTexture[i]].texArrayIndex << endl;
		}
/* 
		for (auto it = meshSubsetIndexStart.begin(); it != meshSubsetIndexStart.end(); ++it)
		{
			out << *it << ", ";
		}
		out << endl;
*/
		out.close();
	}
	{
		wstring binaryName = FileName + _T(".chae");
		char name[100];
		int index = 0;
		for ( auto it = binaryName.begin(); it != binaryName.end(); ++ it)
		{
			name[index++] = (*it);
		}
		name[index] = '\0';
		
		FILE * bin;
		bin = fopen(name, "wb");

		//bin << vertices.size();
		UINT sz = vertices.size();
		fwrite(&sz, sizeof(UINT), 1, bin);
		for (auto it = vertices.begin(); it != vertices.end(); ++it)
		{
			fwrite(&it->pos, sizeof(XMFLOAT3), 1, bin);
			fwrite(&it->texCoord, sizeof(XMFLOAT2), 1, bin);
			fwrite(&it->normal, sizeof(XMFLOAT3), 1, bin);
			fwrite(&it->tangent, sizeof(XMFLOAT3), 1, bin);
		}

		sz = indices.size();
		fwrite(&sz, sizeof(UINT), 1, bin);
		//bin << indices.size();

		for (auto it = indices.begin(); it != indices.end(); ++it)
		{
			fwrite(&(*it), sizeof(UINT), 1, bin);
			//bin << (*it);
		}
		fclose(bin);
		//bin.close();
	}
	//

	//cout << "Tex : " << endl;
	//for (auto it = vertTexCoord.begin(); it != vertTexCoord.end(); ++it)
	//{
	//	cout << (*it).x << ", " << (*it).y << ", " << endl;
	//}
	//cout << "Norm : " << endl;
	//for (auto it = vertNorm.begin(); it != vertNorm.end(); ++it)
	//{
	//	cout << (*it).x << ", " << (*it).y << ", " << (*it).z << endl;
	//}

	//cout << "index : " << indices.size() << endl;
	//for (auto it = indices.begin(); it != indices.end(); ++it)
	//{
	//	cout << (*it);
	//}

	return true;
}
