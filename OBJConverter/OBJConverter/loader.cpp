
#include "objloader.h"


int main()
{
	ObjModel obj;

	wchar_t input[200];

	cout << "OBJ ���� �̸��� �Է��ϼ���. >> ";
	scanf("%ws", input);
	obj.LoadObjModel(input, false, false);
}