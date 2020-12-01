#include <Windows.h>

#define INJ_86 TEXT("GH Injector - x86.exe")
#define INJ_64 TEXT("GH Injector - x64.exe")

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	if (strlen(lpCmdLine))
	{
		int pid = atoi(lpCmdLine);

		HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		
		if (hProc)
		{
			DWORD ExitCode = STILL_ACTIVE;
			while (GetExitCodeProcess(hProc, &ExitCode) && ExitCode == STILL_ACTIVE)
			{
				Sleep(10);
			}

			CloseHandle(hProc);
		}

	}

	BOOL WoW64 = FALSE;
	IsWow64Process(GetCurrentProcess(), &WoW64);

	const TCHAR * szExe = WoW64 ? INJ_64 : INJ_86;

	STARTUPINFO si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	TCHAR FullPath[MAX_PATH]{ 0 };
	GetFullPathName(szExe, MAX_PATH, FullPath, nullptr);

	CreateProcess(FullPath, nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	GetFullPathName(TEXT("OLD.exe"), MAX_PATH, FullPath, nullptr);
	DeleteFile(FullPath);

	return ERROR_SUCCESS;
}