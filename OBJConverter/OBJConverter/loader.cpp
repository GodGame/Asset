
#include "objloader.h"


int main()
{
	ObjModel obj;
	
	wchar_t input[200];
	int iCheck = 0;

	cout << "OBJ 파일 이름을 입력하세요. >> ";
	scanf("%ws", input);

	cout << "Normal과 Tangent의 계산이 필요합니까? (yes:1 / no:0) >> ";
	cin >> iCheck;

	obj.LoadObjModel(input, false, iCheck);
}