#include <Windows.h>
#include <string>

#define INJ_86 TEXT("GH Injector - x86.exe")
#define INJ_64 TEXT("GH Injector - x64.exe")

int main()
{
	BOOL WoW64 = FALSE;
	IsWow64Process(GetCurrentProcess(), &WoW64);

	const TCHAR * szExe = WoW64 ? INJ_64 : INJ_86;
	
	if (GetFileAttributes(szExe) == INVALID_FILE_ATTRIBUTES && WoW64)
	{
		szExe = INJ_86;
	}

	if (GetFileAttributes(szExe) == INVALID_FILE_ATTRIBUTES)
	{
		MessageBoxA(NULL, "GH Injector executables are missing. Make sure all files are in the correct directory.", "Error", MB_ICONERROR);

		return ERROR_FILE_NOT_FOUND;
	}

	STARTUPINFO			si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	auto bRet = CreateProcess(szExe, nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);
	if (!bRet)
	{
		DWORD dwRet = GetLastError();

		char error_code[9]{ 0 };
		_ultoa_s(dwRet, error_code, 0x10);

		std::string msg;
		msg += "Failed to launch GH Injector. CreateProcess returned error code 0x";
		msg += error_code;

		MessageBoxA(NULL, msg.c_str(), "Error", MB_ICONERROR);

		return dwRet;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	TCHAR OldPath[MAX_PATH]{ 0 };
	GetFullPathName(TEXT("OLD.exe"), MAX_PATH, OldPath, nullptr);

	if (GetFileAttributes(OldPath) != INVALID_FILE_ATTRIBUTES)
	{
		while (!DeleteFile(OldPath))
		{
			if (GetLastError() != ERROR_ACCESS_DENIED)
			{
				break;
			}

			Sleep(10);
		}
	}

	return ERROR_SUCCESS;
}