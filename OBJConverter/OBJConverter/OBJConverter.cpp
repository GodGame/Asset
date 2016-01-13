#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>
using namespace std;

bool gbIsIndexStartZero = false;
UINT guExportState = 0;
#define QUAD_FACE 4

struct FLOAT2
{
	float val[2];
};

struct FLOAT3
{
	float val[3];
};

struct INT3
{
	int val[3];
};

struct VT
{
	FLOAT3 vertex;
	FLOAT2 tex;
};

struct VTN
{
	FLOAT3 vertex;
	FLOAT2 tex;
	FLOAT3 normal;
};

ostream& operator<<(ostream& os, const FLOAT3 & v)
{
	os << v.val[0] << ", " << v.val[1] << ", " << v.val[2];
	return os;
}

ostream& operator<<(ostream& os, const INT3 & v)
{
	os << v.val[0] << "/" << v.val[1] << "/" << v.val[2];
	return os;
}

ostream& operator<<(ostream& os, const FLOAT2 & vt)
{
	os << vt.val[0] << ", " << vt.val[1];
	return os;
}

size_t FindNextLine(FILE * file);
size_t FindVertex(FILE * file, char * info);
INT3 ChangeForFace(FILE * file, char * err);

float ChangeToFloat(FILE * file, char * info);
float ChangeToFloatPoint6(FILE * file);
FLOAT3 NextFloat3(FILE * file);
bool WordRead(FILE * file, char out[]);

class Obj
{
	char objName[100];
	vector<FLOAT3> v3Vertex, v3Normal;
	vector<FLOAT2> v2Texture;
	vector<INT3> i3Face;
	vector<FLOAT3> v3Result;

public:
	Obj(char * name)
	{
		strcpy(objName, name);
		cout << objName;
	}
	void InsertVertex(FLOAT3 & v) { v3Vertex.push_back(v); }
	void InsertNormal(FLOAT3 & vn) { v3Normal.push_back(vn); }
	void InsertTexture(FLOAT2 & vt) { v2Texture.push_back(vt); }
	void InsertFace(INT3 & f, int vtnum) 
	{ 
		i3Face.push_back(f); 
		if (vtnum == 4)
		{
			int lastindex = i3Face.size();
			INT3 Temp = i3Face[lastindex - 1];
			i3Face[lastindex - 1] = i3Face[lastindex - 2];
			i3Face[lastindex - 2] = Temp;
		}
	}

	void PrintInfo()
	{
		cout << "ObjName : " << objName << endl;

		cout << "VertexCount : " << v3Vertex.size() << endl;
		for (auto it = v3Vertex.begin(); it != v3Vertex.end(); ++it)
			cout << (*it) << endl;

		cout << "NormalCount : " << v3Normal.size() << endl;
		for (auto it = v3Normal.begin(); it != v3Normal.end(); ++it)
			cout << (*it) << endl;

		cout << "TextureUV Count : " << v2Texture.size() << endl;
		for (auto it = v2Texture.begin(); it != v2Texture.end(); ++it)
			cout << (*it) << endl;

		cout << "Face Count : " << i3Face.size() << endl;
		for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
			cout << (*it) << endl;
	}
	void WriteToTextInfo()
	{
		char fileName[150];
		sprintf(fileName, "%s.info", objName);
		ofstream out(fileName, ios::out);

		FLOAT3 vertex;
		FLOAT2 uv;
		out << "vertex count : " << v3Vertex.size() << "\n";
		for (auto it = v3Vertex.begin(); it != v3Vertex.end(); ++it)
		{
			out << (*it) << "\t";
		}
		out << "\n\n";
		out << "normal count : " <<  v3Normal.size() << "\n";
		for (auto it = v3Normal.begin(); it != v3Normal.end(); ++it)
		{
			out << (*it) << "\t";
		}
		out << "\n\n";
		out << "TextrueUV Count : " << v2Texture.size() << "\n";
		for (auto it = v2Texture.begin(); it != v2Texture.end(); ++it)
		{
			out << (*it) << "\t";
		}
		out << "\n\n";
		out << "face Count : " << i3Face.size() << "\n";
		for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
		{
			out << (*it) << "\t";
		}
		out << "\n\n\n\n" << "--Data List--" << "\n";


		int deltaIndex = 1;
//		gbIsIndexStartZero ? deltaIndex = 0 : deltaIndex = 1;
		
			if (v3Normal.size() == 0)
		{
			for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
			{
				out << v3Vertex[it->val[0] - deltaIndex] << "\t\t" << v2Texture[it->val[1] - deltaIndex] << "\n";
			}
		}
		else
		{
			for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
			{
				out << v3Vertex[it->val[0] - deltaIndex] << "\t\t" << v2Texture[it->val[1] - deltaIndex] << "\t\t" << v3Normal[it->val[2] - deltaIndex] << "\n";
			}
		}
	
		out.close();
	}
	void WrtieToBinaryInfo()
	{
		char fileName[150];
		sprintf(fileName, "%s.chae", objName);
		FILE * file;
		file = fopen(fileName, "wb");

		size_t sz = i3Face.size();
		fwrite(&sz, sizeof(size_t), 1, file);

		int deltaIndex = 1;
//		gbIsIndexStartZero ? deltaIndex = 0 : deltaIndex = 1;


		if (v3Normal.size() == 0)
		{
			VT Temp;

			for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
			{
				Temp.vertex = v3Vertex[it->val[0] - deltaIndex];
				Temp.tex = v2Texture[it->val[1] - deltaIndex];
				fwrite(&Temp, sizeof(VT), 1, file);
			}
		}
		else
		{
			VTN Temp;
			for (auto it = i3Face.begin(); it != i3Face.end(); ++it)
			{
				Temp.vertex = v3Vertex[it->val[0] - deltaIndex];
				Temp.tex = v2Texture[it->val[1] - deltaIndex];
				Temp.normal = v3Normal[it->val[2] - deltaIndex];
				fwrite(&Temp, sizeof(VTN), 1, file);
			}
		}
		fclose(file);

	}


	void WriteToFile()
	{
		WriteToTextInfo();
		WrtieToBinaryInfo();
	}
};

int main()
{
	char p;

	FILE * file;
	
	cout << "OBJ 파일을 입력하세요>> ";
	char fileName[100]; 
	cin >> fileName;
//	fgets(fileName, sizeof(fileName), stdin);

	cout << "obj 종류를 선택하세요. 1 >> 같은 파일에 다른 정점 // 2 >> 같은 버텍스 // 4 >> 사각형 face";
	cin >> guExportState;


	file = fopen(fileName, "rb");
	size_t sz;

	sz = 0;//fread(&p, sizeof(p), 2, file);
	FLOAT3 v3Temp;
	FLOAT2 v2Temp;
	INT3 i3Temp;

	ZeroMemory(&i3Temp, sizeof(i3Temp));
	ZeroMemory(&v2Temp, sizeof(v2Temp));
	ZeroMemory(&v3Temp, sizeof(v3Temp));

	//FindVertex(file, &p);
	fread(&p, sizeof(p), 1, file); // 공백 제거

	vector<Obj*> objList;
	Obj * objptr = new Obj(fileName);
	objList.push_back(objptr);


	char tmp[100];
	while (file)
	{
		if (WordRead(file, tmp)) break;
		cout << tmp << " : " ;

		if (feof(file)) break;
		if (guExportState == 1 && (!strcmp(tmp, "o") || !strcmp(tmp, "g")))
		{
			WordRead(file, tmp);
			cout << "ObjName : ";// << tmp << endl;
			fseek(file, -1, SEEK_CUR);
	
			objptr = new Obj(tmp);
			objList.push_back(objptr);

			//fread(&p, sizeof(char), 1, file);
			//FindNextLine(file);
		}
		if (!strcmp(tmp, "v"))
		{
			v3Temp = NextFloat3(file);
			//cout << v3Temp << endl;
			objptr->InsertVertex(v3Temp);
		}
		else if (!strcmp(tmp, "vt"))
		{
			//fread(&p, sizeof(p), 1, file); // 공백 제거
			v3Temp = NextFloat3(file);
			//cout << v3Temp << endl;
			v2Temp.val[0] = v3Temp.val[0];
			v2Temp.val[1] = v3Temp.val[1];
			
			objptr->InsertTexture(v2Temp);
		}
		else if (!strcmp(tmp, "vn"))
		{
			v3Temp = NextFloat3(file);
			//cout << v3Temp << endl;
			objptr->InsertNormal(v3Temp);
		}
		else if (!strcmp(tmp, "f"))
		{
			//fread(&p, sizeof(p), 1, file); // 공백 제거
			int count = 1;
			while (true)
			{
				char tpc;
				i3Temp = ChangeForFace(file, &tpc);
				if (i3Temp.val[0] == 0 && i3Temp.val[1] == 0 && i3Temp.val[2] == 0)
					break;
				
				cout << i3Temp << " ";
				objptr->InsertFace(i3Temp, count++);
				if (tpc == '\n')
					break;
			}
			if (count == 4)
			{
				
			}
			cout << endl;
		}
		else
			FindNextLine(file);
	}

	fclose(file);
	
	for (auto it = objList.begin(); it != objList.end(); ++it)
	{
		(*it)->PrintInfo();
		(*it)->WriteToFile();
		delete (*it);
	}
}

size_t FindNextLine(FILE * file)
{
	size_t sz = 0;
	char p = '0';

	cout << "/* ";
	while (p != '\n')
	{
		sz += fread(&p, sizeof(p), 1, file);
		cout << p;
	}
	cout << " */" << endl;
	return sz;
}
size_t FindVertex(FILE * file, char * info)
{
	char p = '1';
	size_t sz = 0;

	while (true) {
		if (p != 'o')
		{
			while (p != '\n')
			{
				sz += fread(&p, sizeof(p), 1, file);
				cout << p;
			}
			cout << endl;
		}
		else // p == 'o'
		{
			while (p != '\n')
			{
				sz += fread(&p, sizeof(p), 1, file);
				cout << p;
			}
			cout << endl;
		}
		sz += fread(&p, sizeof(p), 1, file);

		if (p == 'v')
		{
			*info = p;
			return sz;
		}
	}
}

INT3 ChangeForFace(FILE * file, char * err)

{
	char value[10];

	int rindex = 0, index = 0;
	char p;
	INT3 result{ 0, 0, 0 };

	while (true)
	{
		::fread(&p, sizeof(char), 1, file);
		if (p == ' ' || p == '\n') break;
		else if (p == '/')
		{
			if (index == 0) break;

			value[index] = '\0';
			result.val[rindex++] = atoi(value);
			index = 0;
		}
		else
			value[index++] = p;
	}
	value[index] = '\0';
	result.val[rindex] = atoi(value);
	if (result.val[rindex] == 0) gbIsIndexStartZero = true;
	++rindex;

	*err = p;
	return result;
}

float ChangeToFloat(FILE * file, char * info)
{
	char value[15];

	int index = 0;
	char p = '1';

	while (true)
	{
		fread(&p, sizeof(char), 1, file);

		if (p == ' ' || p == '\0' || p == '\n')
		{
			*info = p;
			break;
		}
		value[index++] = p;
	}

	value[index] = '\0';
	return atof(value);
}

float ChangeToFloatPoint6(FILE * file)
{
	char p[9];

	fread(&p, sizeof(char), 8, file);
	p[8] = '\0';
	if (p[0] == '-')
	{
		float val = atof(p);
		char p2[2];
		fread(&p2, sizeof(char), 1, file);
		p2[1] = '\0';
		return -val + (0.000001 * atoi(p2));
	}

	return atof(p);
}

FLOAT3 NextFloat3(FILE * file)
{
	FLOAT3 result{ 0,0,0 };
	char info = '-1';

	for (int i = 0; i < 3; ++i)
	{
		result.val[i] = ChangeToFloat(file, &info);
		if (info == '\n')
			break;
	}
	return result;
}


bool WordRead(FILE * file, char out[])
{
	char p = '0';
	long sz = 0;

	while (true)
	{
		fread(&p, sizeof(char), 1, file);
		if (p == ' ' || p == '\0' || p == '\n')
		{
			out[sz] = '\0';

			int index = 0;
			while ((p == ' ' || p == '\0' || p == '\n'))
			{
				fread(&p, sizeof(char), 1, file);
				index++;
				if (feof(file))
					return true;
			}
			//cout << "end : " << p << endl;
			fseek(file, -1, SEEK_CUR);
			break;
		}
		if (feof(file))
			return true;

		out[sz++] = p;
	}

	return false;
}


//	if (p == 'o' || p == '#')
//		FindNextLine(file);

//	else if (p == 'u')
//	{
//		while (p != '\n')
//		{
//			fread(&p, sizeof(p), 1, file); // 공백 제거
//			cout << p;
//		}
//		cout << endl;
//	}
//	else if (p == 'v')
//	{
//		fread(&p, sizeof(p), 1, file);
//		if (p == 't')
//		{
//			cout << "vt : ";
//			fread(&p, sizeof(p), 1, file); // 공백 제거

//			for (int i = 0; i < 2; ++i)
//				v2Temp.val[i] = ChangeToFloat(file);

//			cout << v2Temp << endl;

//		}
//		else if (p == 'n')
//		{
//			cout << "vn : ";
//			fread(&p, sizeof(p), 1, file); // 공백 제거

//			for (int i = 0; i < 3; ++i)
//				v3Temp.val[i] = ChangeToFloat(file);

//			cout << v3Temp << endl;
//		}
//		else if (p == ' ')
//		{
//			cout << "v : ";
//			char fVal[12];
//			for (int i = 0; i < 3; ++i)
//				v3Temp.val[i] = ChangeToFloat(file);

//			cout << v3Temp << endl;

//		}
//	}
//	else if (p == 'f')
//	{
//		fread(&p, sizeof(p), 1, file); // 공백 제거
//		cout << "f : ";

//		while (true)
//		{
//			char tpc;
//			cout << ChangeForFace(file, &tpc) << " ";
//			if (tpc == '\n')
//				break;
//		}
//		cout << endl;
//	}
//	else if (p == ' ')
//		void(0);

//	fread(&p, sizeof(p), 1, file);
//}