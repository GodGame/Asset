
#include "objloader.h"


int main()
{
	ObjModel obj;
	
	wchar_t input[200];
	int iCheck = 0;

	cout << "OBJ ���� �̸��� �Է��ϼ���. >> ";
	scanf("%ws", input);

	cout << "Normal�� Tangent�� ����� �ʿ��մϱ�? (yes:1 / no:0) >> ";
	cin >> iCheck;

	obj.LoadObjModel(input, false, iCheck);
}