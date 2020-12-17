#include "pch.h"

#include "process.h"

#ifdef UNICODE
#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next
#endif

f_NtQueryInformationProcess p_NtQueryInformationProcess = nullptr;

ARCH getFileArchA(const char * szDllFile)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	std::wstring ws(conv.from_bytes(szDllFile));
	
	return getFileArchW(ws.c_str());
}

ARCH getFileArchW(const wchar_t * szDllFile)
{
	BYTE * pSrcData	= nullptr;

	IMAGE_NT_HEADERS	* pNtHeaders	= nullptr;
	IMAGE_FILE_HEADER	* pFileHeader	= nullptr;

	if (!FileExistsW(szDllFile))
	{
		printf("File doesn't exist\n");

		return ARCH::NONE;
	}

	std::ifstream File(szDllFile, std::ios::binary | std::ios::ate);

	if (File.fail())
	{
		printf("Opening the file failed: %X\n", (DWORD)File.rdstate());
		File.close();

		return ARCH::NONE;
	}

	auto FileSize = File.tellg();
	if (FileSize < 0x1000)
	{
		printf("Filesize is invalid\n");
		File.close();

		return ARCH::NONE;
	}

	pSrcData = new BYTE[static_cast<UINT_PTR>(FileSize)];
	if (!pSrcData)
	{
		printf("Memory allocation failed\n");
		File.close();

		return ARCH::NONE;
	}

	File.seekg(0, std::ios::beg);
	File.read(reinterpret_cast<char *>(pSrcData), FileSize);
	File.close();

	if (reinterpret_cast<IMAGE_DOS_HEADER *>(pSrcData)->e_magic != IMAGE_DOS_SIGNATURE)
	{
		printf("Invalid DOS signature\n");
		delete[] pSrcData;

		return ARCH::NONE;
	}

	pNtHeaders		= reinterpret_cast<IMAGE_NT_HEADERS *>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER *>(pSrcData)->e_lfanew);
	pFileHeader		= &pNtHeaders->FileHeader;

	if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		printf("Invalid NT signature\n");
		delete[] pSrcData;

		return ARCH::NONE;
	}

	if (!(pFileHeader->Characteristics & IMAGE_FILE_DLL))
	{
		printf("Invalid PE characteristics\n");
		delete[] pSrcData;

		return ARCH::NONE;
	}

	if (pFileHeader->Machine == IMAGE_FILE_MACHINE_AMD64)
	{
		delete[] pSrcData;

		return ARCH::X64;
	}
	else if (pFileHeader->Machine == IMAGE_FILE_MACHINE_I386)
	{
		delete[] pSrcData;

		return ARCH::X86;
	}

	delete[] pSrcData;

	return ARCH::NONE;
}

ARCH getProcArch(const int PID)
{
	bool native = IsNativeProcess(PID);

#ifdef _WIN64
	if (!native)
	{
		return ARCH::X86;
	}

	return ARCH::X64;
#else
	return ARCH::X86;
#endif
}

int getProcSession(const int PID)
{
	if (!p_NtQueryInformationProcess)
	{
		auto h_nt_dll = GetModuleHandleA("ntdll.dll");
		if (!h_nt_dll)
		{
			return -1;
		}

		p_NtQueryInformationProcess = reinterpret_cast<f_NtQueryInformationProcess>(GetProcAddress(h_nt_dll, "NtQueryInformationProcess"));
		if (!p_NtQueryInformationProcess)
		{
			return -1;
		}
	}

	HANDLE hTargetProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);
	if (!hTargetProc)
	{
		return -1;
	}

	PROCESS_SESSION_INFORMATION psi{ 0 };
	NTSTATUS ntRet = p_NtQueryInformationProcess(hTargetProc, ProcessSessionInformation, &psi, sizeof(psi), nullptr);

	CloseHandle(hTargetProc);

	if (NT_FAIL(ntRet))
	{
		return -1;
	}

	return (int)psi.SessionId;
}

bool getProcFullPathA(char * szfullPath, DWORD BufferSize, int PID)
{
	HANDLE hOpenProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, NULL, PID);

	if (hOpenProc != NULL)
	{
		BOOL bRet = QueryFullProcessImageNameA(hOpenProc, 0, szfullPath, &BufferSize);

		CloseHandle(hOpenProc);

		return (bRet == TRUE);
	}

	return false;
}

bool getProcFullPathW(wchar_t * szfullPath, DWORD BufferSize, int PID)
{
	HANDLE hOpenProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, NULL, PID);

	if (hOpenProc != NULL)
	{
		BOOL bRet = QueryFullProcessImageNameW(hOpenProc, 0, szfullPath, &BufferSize);

		CloseHandle(hOpenProc);

		return (bRet == TRUE);
	}

	return false;
}

Process_Struct getProcessByNameA(const char * szProcess)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	std::wstring ws(conv.from_bytes(szProcess));

	return getProcessByNameW(ws.c_str());
}

Process_Struct getProcessByNameW(const wchar_t * szProcess)
{
	Process_Struct ps_item{ 0 };

	if (!szProcess)
	{
		return ps_item;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32W procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnapshot, &procEntry))
		{
			do
			{
				if (!strcicmpW(procEntry.szExeFile, szProcess))
				{
					ps_item.Arch = getProcArch(procEntry.th32ProcessID);

					if (ps_item.Arch != ARCH::NONE)
					{
#ifndef _WIN64
						if (ps_item.Arch != ARCH::X86)
						{
							continue;
						}
#endif

						ps_item.PID = procEntry.th32ProcessID;
						lstrcpyW(ps_item.szName, procEntry.szExeFile);
						getProcFullPathW(ps_item.szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), ps_item.PID);

						break;
					}
				}

			} while (Process32NextW(hSnapshot, &procEntry));
		}

		CloseHandle(hSnapshot);
	}

	return ps_item;
}

Process_Struct getProcessByPID(const int PID)
{
	Process_Struct ps_item{ 0 };

	if (!PID)
	{
		return ps_item;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32W procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnapshot, &procEntry))
		{
			do
			{
				if (procEntry.th32ProcessID == PID)
				{
					ps_item.Arch = getProcArch(procEntry.th32ProcessID);
					if (ps_item.Arch != ARCH::NONE)
					{
#ifndef _WIN64
						if (ps_item.Arch != ARCH::X86)
						{
							continue;
						}
#endif
						ps_item.PID		= procEntry.th32ProcessID;
						ps_item.Arch	= getProcArch(procEntry.th32ProcessID);
						lstrcpyW(ps_item.szName, procEntry.szExeFile);
						getProcFullPathW(ps_item.szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), ps_item.PID);

						break;
					}
				}

			} while (Process32NextW(hSnapshot, &procEntry));
		}
		
		CloseHandle(hSnapshot);
	}

	return ps_item;
}

bool getProcessList(std::vector<Process_Struct> & pl)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnapshot && hSnapshot != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32W procEntry = { 0 };
		procEntry.dwSize = sizeof(PROCESSENTRY32W);

		if (Process32FirstW(hSnapshot, &procEntry))
		{
			do
			{
				if (!procEntry.th32ProcessID || procEntry.th32ProcessID == 4) //dumb processes
				{
					continue;
				}

				Process_Struct ps_item{ 0 };

				ps_item.PID		= procEntry.th32ProcessID;
				ps_item.Arch	= getProcArch(procEntry.th32ProcessID);
				lstrcpyW(ps_item.szName, procEntry.szExeFile);
				getProcFullPathW(ps_item.szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), ps_item.PID);

				pl.push_back(ps_item);

			} while (Process32NextW(hSnapshot, &procEntry));
		}

		CloseHandle(hSnapshot);

		return true;
	}

	return false;
}

bool sortProcessList(std::vector<Process_Struct> & pl, SORT_PS sort)
{
	if (sort == SORT_PS::NUM_LOW)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct & p1, Process_Struct & p2)
			{
				return (p1.PID < p2.PID);
			});
	}
	else if (sort == SORT_PS::NUM_HIGH)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct & p1, Process_Struct & p2)
			{
				return (p1.PID > p2.PID);
			});
	}
	else if (sort == SORT_PS::ASCI_A)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct & p1, Process_Struct & p2)
			{
				int res = strcicmpW(p1.szName, p2.szName);
				
				return (res < 0);
			});
	}
	else
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct & p1, Process_Struct & p2)
			{
				int res = strcicmpW(p1.szName, p2.szName);

				return (res > 0);
			});
	}

	return true;
}

bool SetDebugPrivilege(bool Enable)
{
	HANDLE hToken = 0;
	TOKEN_PRIVILEGES tkp = { 0 };

	// Get a token for this process.
	BOOL bOpt = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	if (!bOpt)
	{
		return false;
	}

	// Get the LUID for the privilege. 
	BOOL bLpv = LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &tkp.Privileges[0].Luid);
	if (!bLpv)
	{
		CloseHandle(hToken);

		return false;
	}

	tkp.PrivilegeCount = 1;  // one privilege to set
	if (Enable)
	{
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
	}

	// Set the privilege for this process. 
	BOOL bAtp = AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), nullptr, 0);
	if (!bAtp)
	{
		CloseHandle(hToken);

		return false;
	}

	CloseHandle(hToken);

	return true;
}

bool IsNativeProcess(const int PID)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)PID);
	if (!hProc)
	{
		return false;
	}

	BOOL bWOW64 = FALSE;
	if (!IsWow64Process(hProc, &bWOW64))
	{
		CloseHandle(hProc);

		return false;
	}

	CloseHandle(hProc);

	return (bWOW64 == FALSE);
}

bool FileExistsW(const wchar_t * szFile)
{
	return (GetFileAttributesW(szFile) != INVALID_FILE_ATTRIBUTES);
}

bool FileExistsA(const char * szFile)
{
	return (GetFileAttributesA(szFile) != INVALID_FILE_ATTRIBUTES);
}

int strcicmpA(const char * a, const char * b)
{
	for (;; a++, b++)
	{
		int c = tolower(*a) - tolower(*b);
		if (c != 0 || !*a)
		{
			return c;
		}
	}

	return 0;
}

int strcicmpW(const wchar_t * a, const wchar_t * b)
{
	for (;; a++, b++)
	{
		int c = tolower(*a) - tolower(*b);
		if (c != 0 || !*a)
		{
			return c;
		}
	}

	return 0;
}