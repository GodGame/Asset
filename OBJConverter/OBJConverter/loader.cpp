
#include "objloader.h"
#include <stdlib.h>

int main()
{
	while (true)
	{
		ObjModel obj;
	
		char input[200];
		int iCheck = 0;

		wstring data;
		cout << "OBJ ���� �̸��� �Է��ϼ���. >> ";
		gets_s(input, 200);

		string sttemp = input;

		for (auto it = sttemp.begin(); it != sttemp.end(); ++it)
		{
			data.push_back(*it);
		}
		//gets(stdin);

		wcout << data << "�� ������ �аڽ��ϴ�. " << endl;
		//scanf("%ws", input);

		cout << "Normal�� Tangent�� ����� �ʿ��մϱ�? (yes:1 / no:0) >> ";
		cin >> iCheck;

		obj.LoadObjModel(data, false, iCheck);
		cin.ignore();
	}
}