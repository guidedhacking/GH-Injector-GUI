/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#include "pch.h"

#include "Process.h"

#ifdef UNICODE
#undef PROCESSENTRY32
#undef Process32First
#undef Process32Next
#endif

f_NtQueryInformationProcess ProcessData::m_pNtQueryInformationProcess = nullptr;

#ifndef _WIN64
const std::wstring ProcessData::system32	= L"\\system32\\";
const std::wstring ProcessData::sysnative	= L"\\sysnative\\";
const std::wstring ProcessData::syswow64	= L"\\syswow64\\";
#endif

ARCHITECTURE::ARCHITECTURE()
{

}

ARCHITECTURE::ARCHITECTURE(UINT in)
{
	arch = ARCH(in % 3);
}

ARCHITECTURE::ARCHITECTURE(ARCH in)
{
	arch = in;
}

std::wstring ARCHITECTURE::ToStdWString()
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

std::string ARCHITECTURE::ToStdString()
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

bool ARCHITECTURE::operator==(const ARCH & rhs) const
{
	return (arch == rhs);
}

bool ARCHITECTURE::operator==(const ARCHITECTURE & rhs) const
{
	return (arch == rhs.arch);
}

ARCHITECTURE StrToArchA(const std::string & Str)
{
	if (!strcicmpA(Str.c_str(), "x64"))
	{
		return ARCHITECTURE(ARCH::X64);
	}
	else if (!strcicmpA(Str.c_str(), "x86"))
	{
		return ARCHITECTURE(ARCH::X86);
	}

	return ARCHITECTURE(ARCH::NONE);
}

ARCHITECTURE StrToArchW(const std::wstring & Str)
{
	if (!strcicmpW(Str.c_str(), L"x64"))
	{
		return ARCHITECTURE(ARCH::X64);
	}
	else if (!strcicmpW(Str.c_str(), L"x86"))
	{
		return ARCHITECTURE(ARCH::X86);
	}

	return ARCHITECTURE(ARCH::NONE);
}

ProcessData::ProcessData()
{
	if (!m_pNtQueryInformationProcess)
	{
		auto h_NTDLL = GetModuleHandleW(L"ntdll.dll");
		if (!h_NTDLL)
		{
			THROW("Failed to resolve ntdll");
		}

		auto address = GetProcAddress(h_NTDLL, "NtQueryInformationProcess");
		if (!address)
		{
			THROW("Failed to resolve NtQueryInformationProcess");
		}

		m_pNtQueryInformationProcess = reinterpret_cast<f_NtQueryInformationProcess>(address);
	}
}

ProcessData::ProcessData(DWORD PID)
	: ProcessData()
{
	m_PID = PID;

	if (!get_handle())
	{
		return;
	}

	if (!get_path())
	{
		return;
	}

	if (!get_name())
	{
		return;
	}

	if (!load_remaining_data())
	{
		return;
	}

	m_Valid = true;
}

ProcessData::ProcessData(const std::string & ExeName) 
	: ProcessData()
{
	m_ExeName = StdStringToStdWString(ExeName);

	if (!get_pid())
	{
		return;
	}

	if (!get_handle())
	{
		return;
	}

	if (!get_path())
	{
		return;
	}

	if (!load_remaining_data())
	{
		return;
	}

	m_Valid = true;
}

ProcessData::ProcessData(const std::wstring & ExeName)
	: ProcessData()
{
	m_ExeName = ExeName;

	if (!get_pid())
	{
		return;
	}

	if (!get_handle())
	{
		return;
	}

	if (!get_path())
	{
		return;
	}

	if (!load_remaining_data())
	{
		return;
	}

	m_Valid = true;
}

ProcessData::~ProcessData()
{
	clean_up();
}

bool ProcessData::get_pid()
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	PROCESSENTRY32W PE32 = { 0 };
	PE32.dwSize = sizeof(PROCESSENTRY32W);

	BOOL bRet = Process32FirstW(hSnapshot, &PE32);
	while (bRet)
	{
		if (!strcicmpW(PE32.szExeFile, m_ExeName.c_str()))
		{
			m_PID = PE32.th32ProcessID;
			
			break;			
		}

		bRet = Process32NextW(hSnapshot, &PE32);
	}
	
	CloseHandle(hSnapshot);

	return (m_PID != 0);
}

bool ProcessData::get_name()
{
	auto name_pos = m_ExePath.find_last_of('\\');
	if (name_pos == std::wstring::npos)
	{
		return false;
	}

	m_ExeName = m_ExePath.substr(name_pos + 1);

	return true;
}

bool ProcessData::get_handle()
{
	m_Handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_PID);

	return (m_Handle != NULL);
}

bool ProcessData::load_remaining_data()
{
	if (!get_arch())
	{
		return false;
	}

	if (!get_session())
	{
		return false;
	}

	get_icon();

	return true;
}

bool ProcessData::get_path()
{
	wchar_t szBuffer[MAX_PATH * 2]{ 0 };
	DWORD size = sizeof(szBuffer) / sizeof(wchar_t);
	if (!QueryFullProcessImageNameW(m_Handle, NULL, szBuffer, &size))
	{
		return false;
	}

#ifndef _WIN64
	if (!g_IsNative)
	{
		std::wstring temp_path1 = szBuffer;
		std::wstring temp_path2 = szBuffer;
		StdWStringToLower(temp_path1);

		auto pos = temp_path1.find(system32);
		if (pos != std::wstring::npos)
		{
			temp_path2.replace(pos, system32.length(), L"\\Sysnative\\");
		}

		m_ExePath = temp_path2;
	}
#else
	m_ExePath = szBuffer;
#endif

	return true;
}

bool ProcessData::get_arch()
{
	BOOL bWOW64 = FALSE;
	if (!IsWow64Process(m_Handle, &bWOW64))
	{
		return false;
	}

	if (bWOW64)
	{
		m_Architecture	= ARCHITECTURE(ARCH::X86);
		m_Native		= false;

		return true;
	}

	m_Native = true;

	if (g_IsNative)
	{
#ifdef _WIN64
		m_Architecture = ARCHITECTURE(ARCH::X64);
#else
		m_Architecture = ARCHITECTURE(ARCH::X86);
#endif
	}
	else
	{
		m_Architecture = ARCHITECTURE(ARCH::X64);
	}

	return true;
}

bool ProcessData::get_session()
{
	ULONG SessionID = INVALID_SESSION_ID;

	auto ntRet = m_pNtQueryInformationProcess(m_Handle, PROCESSINFOCLASS::ProcessSessionInformation, &SessionID, sizeof(SessionID), nullptr);
	if (NT_FAIL(ntRet))
	{
		return false;
	}

	m_SessionID = SessionID;

	return true;
}

void ProcessData::get_icon()
{
	auto hr = SHDefExtractIconW(m_ExePath.c_str(), 0, NULL, &m_Icon, nullptr, 32);
	if (hr != S_OK)
	{
		m_Icon = NULL;
	}
}

void ProcessData::clean_up()
{
	m_Valid = false;
	m_PID	= 0;

	if (m_Handle)
	{
		CloseHandle(m_Handle);
		m_Handle = NULL;
	}

	if (m_Icon)
	{
		DestroyIcon(m_Icon);
		m_Icon = NULL;
	}
}

bool ProcessData::IsValid() const
{
	return m_Valid;
}

bool ProcessData::IsRunning()
{
	if (!IsValid())
	{
		return false;
	}

	DWORD ExitCode = STILL_ACTIVE;
	if (!GetExitCodeProcess(m_Handle, &ExitCode))
	{
		clean_up();

		return false;
	}

	if (ExitCode == STILL_ACTIVE)
	{
		auto WaitRet = WaitForSingleObject(m_Handle, 0);
		if (WaitRet == WAIT_TIMEOUT)
		{
			clean_up();

			return false;
		}

		return true;
	}

	clean_up();

	return false;
}

bool ProcessData::GetProcessID(DWORD & PID) const
{
	if (!IsValid())
	{
		return false;
	}

	PID = m_PID;

	return true;
}

bool ProcessData::GetNameA(std::string & ExeName) const
{
	if (!IsValid())
	{
		return false;
	}

	ExeName = StdWStringtoStdString(m_ExeName);

	return true;
}

bool ProcessData::GetNameW(std::wstring & ExeName) const
{
	if (!IsValid())
	{
		return false;
	}

	ExeName = m_ExeName;

	return true;
}

bool ProcessData::GetFullPathA(std::string & ExePath) const
{
	if (!IsValid())
	{
		return false;
	}

	ExePath = StdWStringtoStdString(m_ExePath);

	return true;
}

bool ProcessData::GetFullPathW(std::wstring & ExePath) const
{
	if (!IsValid())
	{
		return false;
	}

	ExePath = m_ExePath;

	return true;
}

bool ProcessData::GetArchitecture(ARCHITECTURE & Architecture) const
{
	if (!IsValid())
	{
		return false;
	}

	Architecture = m_Architecture;

	return true;
}

bool ProcessData::IsNativeProcess(bool & Native) const
{
	if (!IsValid())
	{
		return false;
	}

	Native = m_Native;

	return true;
}

bool ProcessData::GetSessionID(ULONG & SessionID) const
{
	if (!IsValid())
	{
		return false;
	}

	SessionID = m_SessionID;

	return true;
}

bool ProcessData::GetIcon(HICON & hIcon) const
{
	if (!IsValid())
	{
		hIcon = NULL;

		return false;
	}

	if (!m_Icon)
	{
		hIcon = NULL;

		return false;
	}

	hIcon = m_Icon;

	return true;
}

bool ProcessData::UpdateData(DWORD PID)
{
	if (!m_pNtQueryInformationProcess)
	{
		return false;
	}

	clean_up();

	m_PID = PID;

	if (!get_handle())
	{
		return false;
	}

	if (!get_path())
	{
		return false;
	}

	if (!get_name())
	{
		return false;
	}

	if (!load_remaining_data())
	{
		return false;
	}

	m_Valid = true;

	return true;
}

bool ProcessData::UpdateData(const std::string & ExeName)
{
	auto ExeNameW = StdStringToStdWString(ExeName);

	return UpdateData(ExeNameW);
}

bool ProcessData::UpdateData(const std::wstring & ExeName)
{
	if (!m_pNtQueryInformationProcess)
	{
		return false;
	}

	clean_up();

	m_ExeName = ExeName;

	if (!get_pid())
	{
		return false;
	}

	if (!get_handle())
	{
		return false;
	}

	if (!get_path())
	{
		return false;
	}

	if (!load_remaining_data())
	{
		return false;
	}

	m_Valid = true;

	return true;
}

bool ProcessData::UpdateData(const ProcessData & Data)
{
	return UpdateData(Data.m_PID);
}

bool ProcessData::operator==(const ProcessData & rhs) const
{
	return (m_PID == rhs.m_PID);
}

bool ProcessData::operator==(const DWORD & rhs) const
{
	return (m_PID == rhs);
}

bool ProcessData::operator==(const tagPROCESSENTRY32 & rhs) const
{
	return (m_PID == rhs.th32ProcessID);
}

bool ProcessData::operator==(const tagPROCESSENTRY32W & rhs) const
{
	return (m_PID == rhs.th32ProcessID);
}

bool ProcessData::operator<(const ProcessData & rhs) const
{
	return (m_PID < rhs.m_PID);
}

bool ProcessData::operator>(const ProcessData & rhs) const
{
	return (m_PID > rhs.m_PID);
}

bool FileExistsA(const std::string & FilePath)
{
	return (GetFileAttributesA(FilePath.c_str()) != INVALID_FILE_ATTRIBUTES);
}

bool FileExistsW(const std::wstring & FilePath)
{
	return (GetFileAttributesW(FilePath.c_str()) != INVALID_FILE_ATTRIBUTES);
}

ARCHITECTURE GetFileArchitectureA(const std::string & DllFile, bool & IsDotNet)
{
	auto path = StdStringToStdWString(DllFile);
	
	return GetFileArchitectureW(path, IsDotNet);
}

ARCHITECTURE GetFileArchitectureW(const std::wstring & DllFile, bool & IsDotNet)
{
	BYTE * pSrcData	= nullptr;

	IMAGE_DOS_HEADER		* pDosHeader	= nullptr;
	IMAGE_NT_HEADERS		* pNtHeaders	= nullptr;
	IMAGE_FILE_HEADER		* pFileHeader	= nullptr;

	if (!FileExistsW(DllFile))
	{
		g_print("File doesn't exist\n");

		return ARCHITECTURE(0);
	}

	std::ifstream File(DllFile, std::ios::binary | std::ios::ate);

	if (File.fail())
	{
		g_print("Opening the file failed: %X\n", (DWORD)File.rdstate());

		File.close();

		return ARCHITECTURE(0);
	}

	auto pe_min_size = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS32);

	auto FileSize = File.tellg();
	if (static_cast<UINT_PTR>(FileSize) < pe_min_size)
	{
		g_print("Filesize is invalid\n");

		File.close();

		return ARCHITECTURE(0);
	}

	pSrcData = new(std::nothrow) BYTE[static_cast<UINT_PTR>(FileSize)];
	if (pSrcData == nullptr)
	{
		g_print("Memory allocation failed\n");

		File.close();

		return ARCHITECTURE(0);
	}

	File.seekg(0, std::ios::beg);
	File.read(reinterpret_cast<char *>(pSrcData), FileSize);

	if (File.fail())
	{
		g_print("Reading the file failed: %X\n", (DWORD)File.rdstate());

		delete[] pSrcData;
		File.close();

		return ARCHITECTURE(0);
	}

	File.close();

	pDosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(pSrcData);

	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		g_print("Invalid DOS signature\n");

		delete[] pSrcData;

		return ARCHITECTURE(0);
	}

	if (!pDosHeader->e_lfanew || pDosHeader->e_lfanew >= FileSize)
	{
		g_print("Invalid DOS header\n");

		delete[] pSrcData;

		return ARCHITECTURE(0);
	}

	pNtHeaders		= reinterpret_cast<IMAGE_NT_HEADERS *>(pSrcData + pDosHeader->e_lfanew);
	pFileHeader		= &pNtHeaders->FileHeader;

	if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
	{
		g_print("Invalid NT signature\n");

		delete[] pSrcData;

		return ARCHITECTURE(0);
	}

	if (!(pFileHeader->Characteristics & IMAGE_FILE_DLL))
	{
		g_print("Invalid PE characteristics\n");

		delete[] pSrcData;

		return ARCHITECTURE(0);
	}

	ARCHITECTURE ret_arch = ARCHITECTURE(0);

	if (pFileHeader->Machine == IMAGE_FILE_MACHINE_AMD64)
	{
		ret_arch.arch = ARCH::X64;

		auto opt_header = &reinterpret_cast<IMAGE_NT_HEADERS64 *>(pNtHeaders)->OptionalHeader;

		if (opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress)
		{
			IsDotNet = true;
		}
		else
		{
			IsDotNet = false;
		}
	}
	else if (pFileHeader->Machine == IMAGE_FILE_MACHINE_I386)
	{
		ret_arch.arch = ARCH::X86;

		auto opt_header = &reinterpret_cast<IMAGE_NT_HEADERS32 *>(pNtHeaders)->OptionalHeader;
		if (opt_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress)
		{
			IsDotNet = true;
		}
		else
		{
			IsDotNet = false;
		}
	}

	delete[] pSrcData;

	return ret_arch;
}

bool GetProcessList(std::vector<ProcessData *> & list)
{
	static auto sort_entries_pid = [](const PROCESSENTRY32W & lhs, const PROCESSENTRY32W & rhs)
	{
		return (lhs.th32ProcessID < rhs.th32ProcessID);
	};

	static auto sort_list_pid = [](const ProcessData * lhs, const ProcessData * rhs)
	{
		return ((*lhs) < (*rhs));
	};

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE)
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
		if (g_IsNative)
		{
			entries.push_back(PE32);
		}
		else
		{
			HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, PE32.th32ProcessID);
			if (!hProc)
			{
				bRet = Process32NextW(hSnapshot, &PE32);

				continue;
			}

			BOOL bWOW64 = FALSE;
			if (!IsWow64Process(hProc, &bWOW64))
			{
				CloseHandle(hProc);

				bRet = Process32NextW(hSnapshot, &PE32);

				continue;
			}

			if (bWOW64)
			{
				entries.push_back(PE32);
			}

			CloseHandle(hProc);
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

	std::sort(entries.begin(), entries.end(), sort_entries_pid);

	for (auto iter = entries.begin(); iter != entries.end(); )
	{
		auto & i = *iter;
		auto search_entries = [&i](const ProcessData * entry) -> bool
		{
			return (i == (*entry));
		};

		auto ret = std::find_if(list.begin(), list.end(), search_entries);
		if (ret != list.end())
		{
			++iter;

			continue;
		}

		ProcessData * new_process_data = new(std::nothrow) ProcessData(i.th32ProcessID);
		if (!new_process_data)
		{
			iter = entries.erase(iter);

			continue;
		}

		if (!new_process_data->IsValid())
		{
			delete new_process_data;

			iter = entries.erase(iter);

			continue;
		}

		list.push_back(new_process_data);

		++iter;
	}

	std::sort(list.begin(), list.end(), sort_list_pid);

	for (UINT i = 0; i < entries.size(); )
	{
		auto & current_entry = *list[i];
		if (current_entry != entries[i])
		{
			delete list[i];
			list.erase(list.begin() + i);
		}
		else
		{
			++i;
		}
	}

	auto size_1 = list.size();
	auto size_2 = entries.size();

	if (size_1 > size_2)
	{
		auto diff = size_1 - size_2;

		for (size_t i = 1; i <= diff; ++i)
		{
			delete list[list.size() - i];
		}

		list.erase(list.end() - diff, list.end());
	}

	return true;
}

bool SortProcessList(std::vector<ProcessData *> & list, SORT_SENSE sort)
{
	switch (sort)
	{
		case SORT_SENSE::SS_PID_LO:
			std::sort(list.begin(), list.end(), 
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					return ((*lhs) < (*rhs));
				}
			);
			break;

		case SORT_SENSE::SS_PID_HI:
			std::sort(list.begin(), list.end(), 
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					return ((*lhs) > (*rhs));
				}
			);
			break;

		case SORT_SENSE::SS_NAME_LO:
			std::sort(list.begin(), list.end(),
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					std::wstring lhs_name;
					std::wstring rhs_name;
					lhs->GetNameW(lhs_name);
					rhs->GetNameW(rhs_name);
					auto cmp = strcicmpW(lhs_name.c_str(), rhs_name.c_str());

					if (cmp == 0)
					{
						return ((*lhs) < (*rhs));
					}

					return (cmp < 0);
				}
			);
			break;

		case SORT_SENSE::SS_NAME_HI:
			std::sort(list.begin(), list.end(), 
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					std::wstring lhs_name;
					std::wstring rhs_name;
					lhs->GetNameW(lhs_name);
					rhs->GetNameW(rhs_name);
					auto cmp = strcicmpW(lhs_name.c_str(), rhs_name.c_str());

					if (cmp == 0)
					{
						return ((*lhs) > (*rhs));
					}

					return (cmp > 0);
				}
			);
			break;

		case SORT_SENSE::SS_ARCH_LO:
			std::sort(list.begin(), list.end(),
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					ARCHITECTURE lhs_arch;
					ARCHITECTURE rhs_arch;
					lhs->GetArchitecture(lhs_arch);
					rhs->GetArchitecture(rhs_arch);

					if (lhs_arch == rhs_arch)
					{
						std::wstring lhs_name;
						std::wstring rhs_name;
						lhs->GetNameW(lhs_name);
						rhs->GetNameW(rhs_name);
						auto cmp = strcicmpW(lhs_name.c_str(), rhs_name.c_str());

						if (cmp == 0)
						{
							return ((*lhs) < (*rhs));
						}

						return (cmp < 0);
					}

					return ((int)lhs_arch.arch > (int)rhs_arch.arch);
				}
			);
			break;

		case SORT_SENSE::SS_ARCH_HI:
			std::sort(list.begin(), list.end(), 
				[](const ProcessData * lhs, const ProcessData * rhs)
				{
					ARCHITECTURE lhs_arch;
					ARCHITECTURE rhs_arch;
					lhs->GetArchitecture(lhs_arch);
					rhs->GetArchitecture(rhs_arch);

					if (lhs_arch == rhs_arch)
					{
						std::wstring lhs_name;
						std::wstring rhs_name;
						lhs->GetNameW(lhs_name);
						rhs->GetNameW(rhs_name);
						auto cmp = strcicmpW(lhs_name.c_str(), rhs_name.c_str());

						if (cmp == 0)
						{
							return ((*lhs) < (*rhs));
						}

						return (cmp < 0);
					}

					return ((int)lhs_arch.arch < (int)rhs_arch.arch);
				}
			);
			break;
	}

	return true;
}

bool SetDebugPrivilege(bool Enable)
{
	HANDLE hToken = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return false;
	}

	TOKEN_PRIVILEGES TokenPrivileges = { 0 };
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &TokenPrivileges.Privileges[0].Luid))
	{
		CloseHandle(hToken);

		return false;
	}

	TokenPrivileges.PrivilegeCount = 1;
	if (Enable)
	{
		TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else
	{
		TokenPrivileges.Privileges[0].Attributes = NULL;
	}
 
	if (!AdjustTokenPrivileges(hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), nullptr, 0))
	{
		CloseHandle(hToken);

		return false;
	}

	CloseHandle(hToken);

	return true;
}