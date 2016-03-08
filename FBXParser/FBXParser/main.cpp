#include <fbxsdk.h>
#include "FBXParser.h"

int main()
{
	char fileName[256];
	while (true)
	{
		cout << endl << "파일 이름을 입력해주세요 : ";
		cin >> fileName;

		bool bCheck = false;
		FBXParser parser;
		bCheck = parser.Initialize(fileName);
		if (!bCheck)
		{
			cout << "오류가 있습니다. " << endl;
			break;
		}

		parser.Run();
		cout << endl;
	}
}