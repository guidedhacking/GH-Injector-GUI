#include <Windows.h>

#define INJ_86 TEXT("GH Injector - x86.exe")
#define INJ_64 TEXT("GH Injector - x64.exe")

int main()
{
	BOOL WoW64 = FALSE;
	IsWow64Process(GetCurrentProcess(), &WoW64);

	const TCHAR * szExe = WoW64 ? INJ_64 : INJ_86;

	STARTUPINFO si{ 0 };
	PROCESS_INFORMATION pi{ 0 };
	
	if (GetFileAttributes(szExe) == INVALID_FILE_ATTRIBUTES && WoW64)
	{
		szExe = INJ_86;
	}

	if (GetFileAttributes(szExe) == INVALID_FILE_ATTRIBUTES)
	{
		MessageBoxA(NULL, "GH Injector executables are missing. Make sure all files are in the correct directory.", "Error", MB_ICONERROR);

		return ERROR_FILE_NOT_FOUND;
	}

	CreateProcess(szExe, nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

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