#include <fbxsdk.h>
#include "FBXParser.h"

int main()
{
	bool bCheck = false;
	FBXParser parser;
	bCheck = parser.Initialize("Demon_T_Wiezzorek.fbx");
	if (!bCheck)
	{
		cout << "������ �ֽ��ϴ�. " << endl;
		return 0;
	}
	
	parser.Run();
}