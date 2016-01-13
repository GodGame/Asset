
#include "objloader.h"


int main()
{
	ObjModel obj;

	wchar_t input[200];

	cout << "OBJ 파일 이름을 입력하세요. >> ";
	scanf("%ws", input);
	obj.LoadObjModel(input, false, false);
}