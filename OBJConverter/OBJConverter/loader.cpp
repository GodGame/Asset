
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
		cout << "OBJ 파일 이름을 입력하세요. >> ";
		gets_s(input, 200);

		string sttemp = input;

		for (auto it = sttemp.begin(); it != sttemp.end(); ++it)
		{
			data.push_back(*it);
		}
		//gets(stdin);

		wcout << data << "의 파일을 읽겠습니다. " << endl;
		//scanf("%ws", input);

		cout << "Normal과 Tangent의 계산이 필요합니까? (yes:1 / no:0) >> ";
		cin >> iCheck;

		obj.LoadObjModel(data, false, iCheck);
		cin.ignore();
	}
}