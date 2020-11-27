//https://www.codeproject.com/Articles/280650/Zip-Unzip-using-Windows-Shell
//#include <ShellAPI.h>
#include <Windows.h>

class Compress
{
public:
	Compress();
	~Compress();

	void zip();
	void unzip();
	void unzip_GH(WCHAR* source, WCHAR* dest);
};