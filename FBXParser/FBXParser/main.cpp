#include <fbxsdk.h>
#include "FBXParser.h"

int main()
{
	char fileName[256];
	while (true)
	{
		cout << endl << "���� �̸��� �Է����ּ��� : ";
		cin >> fileName;

		bool bCheck = false;
		FBXParser parser;
		bCheck = parser.Initialize(fileName);
		if (!bCheck)
		{
			cout << "������ �ֽ��ϴ�. " << endl;
			break;
		}

		parser.Run();
		cout << endl;
	}
}