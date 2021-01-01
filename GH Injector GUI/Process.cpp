#include "pch.h"

#include "Process.h"

#ifdef UNICODE
#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next
#endif

Process_Struct::Process_Struct()
{
	PID		= 0;
	Arch	= ARCH::NONE;
	Session = -1;
	hIcon	= NULL;

	memset(szName, 0, sizeof(szName));
	memset(szPath, 0, sizeof(szPath));
}

Process_Struct::~Process_Struct()
{
	if (hIcon)
	{
		DestroyIcon(hIcon);
	}
}

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

ARCH StrToArchA(const char * szStr)
{
	if (!strcicmpA(szStr, "x64"))
	{
		return ARCH::X64;
	}
	else if (!strcicmpA(szStr, "x86"))
	{
		return ARCH::X86;
	}

	return ARCH::NONE;
}

ARCH StrToArchW(const wchar_t * szStr)
{
	if (!strcicmpW(szStr, L"x64"))
	{
		return ARCH::X64;
	}
	else if (!strcicmpW(szStr, L"x86"))
	{
		return ARCH::X86;
	}

	return ARCH::NONE;
}

std::string ArchToStrA(ARCH arch)
{
	switch (arch)
	{
		case ARCH::X64:
			return "x64";

		case ARCH::X86:
			return "x86";
	}

	return "";
}

std::wstring ArchToStrW(ARCH arch)
{
	switch (arch)
	{
		case ARCH::X64:
			return L"x64";

		case ARCH::X86:
			return L"x86";
	}

	return L"";
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
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);

	if (hProc != NULL)
	{
		BOOL bRet = QueryFullProcessImageNameA(hProc, 0, szfullPath, &BufferSize);

		CloseHandle(hProc);

		return (bRet == TRUE);
	}

	return false;
}

bool getProcFullPathW(wchar_t * szfullPath, DWORD BufferSize, int PID)
{
	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PID);

	if (hProc != NULL)
	{
		BOOL bRet = QueryFullProcessImageNameW(hProc, 0, szfullPath, &BufferSize);

		CloseHandle(hProc);

		return (bRet != FALSE);
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
	Process_Struct ps_item;

	if (!szProcess)
	{
		return ps_item;
	}

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE)
	{
		return ps_item;
	}

	PROCESSENTRY32W PE32 = { 0 };
	PE32.dwSize = sizeof(PROCESSENTRY32W);

	BOOL bRet = Process32FirstW(hSnapshot, &PE32);

	while (bRet)
	{
		if (!strcicmpW(PE32.szExeFile, szProcess))
		{
			ps_item.Arch = getProcArch(PE32.th32ProcessID);
			
			if (ps_item.Arch != ARCH::NONE)
			{
#ifndef _WIN64
				if (ps_item.Arch != ARCH::X86)
				{
					continue;
				}
#endif

				ps_item.PID = PE32.th32ProcessID;
				lstrcpyW(ps_item.szName, PE32.szExeFile);
				getProcFullPathW(ps_item.szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), ps_item.PID);

				break;
			}
		}

		bRet = Process32NextW(hSnapshot, &PE32);
	}

	CloseHandle(hSnapshot);

	return ps_item;
}

Process_Struct getProcessByPID(const int PID)
{
	Process_Struct ps_item;

	if (!PID)
	{
		return ps_item;
	}

	auto arch = getProcArch(PID);

#ifndef _WIN64
	if (arch != ARCH::X86)
	{
		return ps_item;
	}
#endif

	if (arch != ARCH::NONE && getProcFullPathW(ps_item.szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), PID))
	{
		std::wstring ws(ps_item.szPath);
		auto pos = ws.find_last_of('\\');
		if (pos)
		{
			lstrcpyW(ps_item.szName, ps_item.szPath + pos + 1);
			ps_item.PID		= PID;
			ps_item.Arch	= arch;
		}
	}

	return ps_item;
}

bool getProcessList(std::vector<Process_Struct*> & list, bool get_icon)
{
	bool native = IsNativeProcess(GetCurrentProcessId());

	static auto sort_entries = [](const PROCESSENTRY32W & lhs, const PROCESSENTRY32W & rhs)
	{
		return (lhs.th32ProcessID < rhs.th32ProcessID);
	};

	static auto sort_list = [](const Process_Struct * lhs, const Process_Struct * rhs)
	{
		return (lhs->PID < rhs->PID);
	};

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (!hSnapshot || hSnapshot == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	std::vector<PROCESSENTRY32W> entries;

	PROCESSENTRY32W PE32 = { 0 };
	PE32.dwSize = sizeof(PROCESSENTRY32W);

	BOOL bRet = Process32FirstW(hSnapshot, &PE32);

	while (bRet)
	{
		if (!PE32.th32ProcessID || PE32.th32ProcessID == 4 || PE32.th32ProcessID == GetCurrentProcessId()) //dumb processes
		{
			bRet = Process32NextW(hSnapshot, &PE32);

			continue;
		}

#ifndef _WIN64
		if (native || !native && !IsNativeProcess(PE32.th32ProcessID))
		{
			entries.push_back(PE32);
		}
#else	
		entries.push_back(PE32);
#endif
		
		bRet = Process32NextW(hSnapshot, &PE32);
	}

	CloseHandle(hSnapshot);

	if (!entries.size())
	{
		return false;
	}

	std::sort(entries.begin(), entries.end(), sort_entries);

	for (const auto & i : entries)
	{
		auto search_entries = [pid = i.th32ProcessID](const Process_Struct * entry) -> bool
		{
			return (pid == entry->PID);
		};

		if (list.size())
		{
			auto ret = std::find_if(list.begin(), list.end(), search_entries);
			if (ret != list.end() || list.back()->PID == i.th32ProcessID)
			{
				continue;
			}
		}

		Process_Struct * ps_item = new Process_Struct();

		ps_item->PID		= i.th32ProcessID;
		ps_item->Arch		= getProcArch(i.th32ProcessID);
		ps_item->Session	= getProcSession(i.th32ProcessID);

		lstrcpyW(ps_item->szName, i.szExeFile);

		bool path_valid = getProcFullPathW(ps_item->szPath, sizeof(Process_Struct::szPath) / sizeof(Process_Struct::szPath[0]), ps_item->PID);

		if (get_icon && path_valid)
		{
			SHDefExtractIconW(ps_item->szPath, 0, NULL, &ps_item->hIcon, nullptr, 32);
		}

		list.push_back(ps_item);
	}

	std::sort(list.begin(), list.end(), sort_list);

	for (int i = 0; i < list.size(); )
	{
		if (list[i]->PID != entries[i].th32ProcessID)
		{
			delete list[i];

			list.erase(list.begin() + i);
		}
		else
		{
			++i;
		}
	}

	return true;
}

bool sortProcessList(std::vector<Process_Struct*> & pl, SORT_SENSE sort)
{
	if (sort == SORT_SENSE::SS_PID_LO)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				return (lhs->PID < rhs->PID);
			});
	}
	else if (sort == SORT_SENSE::SS_PID_HI)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				return (lhs->PID > rhs->PID);
			});
	}
	else if (sort == SORT_SENSE::SS_NAME_LO)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				auto cmp = strcicmpW(lhs->szName, rhs->szName);

				if (cmp == 0)
				{
					return (lhs->PID < rhs->PID);
				}

				return (cmp < 0);
			});
	}
	else if (sort == SORT_SENSE::SS_NAME_HI)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				auto cmp = strcicmpW(lhs->szName, rhs->szName);

				if (cmp == 0)
				{
					return (lhs->PID > rhs->PID);
				}

				return (cmp > 0);
			});
	}
	else if (sort == SORT_SENSE::SS_ARCH_LO)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				if (lhs->Arch == rhs->Arch)
				{
					auto cmp = strcicmpW(lhs->szName, rhs->szName);

					if (cmp == 0)
					{
						return (lhs->PID < rhs->PID);
					}

					return (cmp < 0);
				}

				return ((int)lhs->Arch > (int)rhs->Arch);
			});
	}
	else if (sort == SORT_SENSE::SS_ARCH_HI)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct * lhs, Process_Struct * rhs)
			{
				if (lhs->Arch == rhs->Arch)
				{
					auto cmp = strcicmpW(lhs->szName, rhs->szName);

					if (cmp == 0)
					{
						return (lhs->PID < rhs->PID);
					}

					return (cmp < 0);
				}
				
				return ((int)lhs->Arch < (int)rhs->Arch);
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