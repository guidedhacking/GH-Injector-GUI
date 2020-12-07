#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <functional>
#include "process.h"
#include <codecvt>

f_NtQueryInformationProcess p_NtQueryInformationProcess = nullptr;

enum ARCH getFileArch(const wchar_t* szDllFile)
{
    BYTE* pSrcData = nullptr;
    IMAGE_NT_HEADERS* pOldNtHeader = nullptr;
    IMAGE_FILE_HEADER* pOldFileHeader = nullptr;

    if (!GetFileAttributesW(szDllFile))
    {
        printf("File doesn't exist\n");
        return NONE;
    }

    std::ifstream File(szDllFile, std::ios::binary | std::ios::ate);

    if (File.fail())
    {
        printf("Opening the file failed: %X\n", (DWORD)File.rdstate());
        File.close();
        return NONE;
    }

    auto FileSize = File.tellg();
    if (FileSize < 0x1000)
    {
        printf("Filesize is invalid.\n");
        File.close();
        return NONE;
    }

    pSrcData = new BYTE[static_cast<UINT_PTR>(FileSize)];
    if (!pSrcData)
    {
        printf("Memory allocating failed\n");
        File.close();
        return NONE;
    }

    File.seekg(0, std::ios::beg);
    File.read(reinterpret_cast<char*>(pSrcData), FileSize);
    File.close();

    if (reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_magic != 0x5A4D) //"MZ"
    {
        printf("Invalid file\n");
        delete[] pSrcData;
        return NONE;
    }

    pOldNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(pSrcData + reinterpret_cast<IMAGE_DOS_HEADER*>(pSrcData)->e_lfanew);
    pOldFileHeader = &pOldNtHeader->FileHeader;

    if (!(pOldFileHeader->Characteristics & IMAGE_FILE_DLL))
    {
        printf("Invalid file\n");
        delete[] pSrcData;
        return NONE;
    }

    if (pOldFileHeader->Machine == IMAGE_FILE_MACHINE_AMD64)
    {
        delete[] pSrcData;
        return X64;
    }
    else if (pOldFileHeader->Machine == IMAGE_FILE_MACHINE_I386)
    {
        delete[] pSrcData;
        return X86;
    }

    delete[] pSrcData;
    return NONE;
}

ARCH getFileArchA(const char * szDllFile)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring ws(conv.from_bytes(szDllFile));
    return getFileArch(ws.c_str());
}

enum ARCH getProcArch(const int pid)
{
    HANDLE hOpenProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, NULL, pid);
    if (hOpenProc != NULL)
    {
        BOOL tempWow64 = FALSE;

        BOOL bIsWow = IsWow64Process(hOpenProc, &tempWow64);
        if (bIsWow != 0)
        {
            if (tempWow64 == TRUE)
            {
                return X86;
            }
            else
            {
                return X64;
            }
            return NONE;
        }
        CloseHandle(hOpenProc);
    }

    return NONE;
}

int getProcSession(const int pid)
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
	
    HANDLE hTargetProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
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

bool getProcFullPath(char* fullPath, int strSize, int pid)
{
    bool ok = false;
    HANDLE hOpenProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, NULL, pid);
    if (hOpenProc != NULL)
    {

        DWORD writtenBytes = strSize;
        ok = QueryFullProcessImageNameA(hOpenProc, 0, fullPath, &writtenBytes);

        CloseHandle(hOpenProc);
    }

    return ok;
}

Process_Struct getProcessByName(const char* proc)
{
    PROCESSENTRY32 procEntry = { 0 };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    char name[250]; 
    Process_Struct ps;
    memset(&ps, 0, sizeof(Process_Struct));

    if (hSnapshot)
    {
        procEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &procEntry))
        {
            do
            {
                int ret = wcstombs(name, procEntry.szExeFile, sizeof(name));
                if (!strcmp(name, proc))
                {
                    ps.arch = getProcArch(procEntry.th32ProcessID);
                    if (ps.arch != ARCH::NONE)
                    {
#ifndef _WIN64
                        if (ps.arch != ARCH::X86)
                        {
                            continue;
                        }
#endif
                        
                        ps.pid = procEntry.th32ProcessID;
                        strcpy(ps.name, name);
                        getProcFullPath(ps.fullName, sizeof(ps.fullName), ps.pid);
                    	
                        break;
                    }
                }

            } while (Process32Next(hSnapshot, &procEntry));
        }
        CloseHandle(hSnapshot);
    }
    return ps;
}

Process_Struct getProcessByPID(const int pid)
{
    PROCESSENTRY32 procEntry = { 0 };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    char name[250];
    Process_Struct ps;
    memset(&ps, 0, sizeof(Process_Struct));

    if (hSnapshot)
    {
        procEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &procEntry))
        {
            do
            {
                if(procEntry.th32ProcessID == pid)
                {

                    ps.arch = getProcArch(procEntry.th32ProcessID);
                    if (ps.arch != ARCH::NONE)
                    {
#ifndef _WIN64
                        if (ps.arch != ARCH::X86)
                        {
                            continue;
                        }
#endif
                        ps.pid = procEntry.th32ProcessID;
                        int ret = wcstombs(name, procEntry.szExeFile, sizeof(name));
                        strcpy(ps.name, name);
                        getProcFullPath(ps.fullName, sizeof(ps.fullName), ps.pid);
                    	
                        break;
                    }
                }

            } while (Process32Next(hSnapshot, &procEntry));
        }
        CloseHandle(hSnapshot);
    }
    return ps;
}

bool getProcessList(std::vector<Process_Struct>& pl)
{

    PROCESSENTRY32 procEntry = { 0 };
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (hSnapshot)
    {
        procEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &procEntry))
        {
            do
            {
                if (!procEntry.th32ProcessID || procEntry.th32ProcessID == 4)
                {
                    continue;
                }

                Process_Struct ps_item;
                memset(&ps_item, 0, sizeof(Process_Struct));

                ps_item.pid = procEntry.th32ProcessID;
                ps_item.arch = getProcArch(procEntry.th32ProcessID);
                int ret = wcstombs(ps_item.name, procEntry.szExeFile, sizeof(ps_item.name));
                getProcFullPath(ps_item.fullName, sizeof(ps_item.fullName), ps_item.pid);

                pl.push_back(ps_item);

            } while (Process32Next(hSnapshot, &procEntry));
        }
        CloseHandle(hSnapshot);
    }
    return true;
}

bool sortProcessList(std::vector<Process_Struct>& pl, SORT_PS sort)
{
    if (sort == NUM_LOW)
    {
		std::sort(pl.begin(), pl.end(), [](Process_Struct& p1, Process_Struct& p2) {
			if (p1.pid < p2.pid)
				return true;
			return false;
			});
    }
    else if (sort == NUM_HIGH)
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct& p1, Process_Struct& p2) {
			if (p1.pid > p2.pid)
				return true;
			return false;
			});
	}
	else if (sort == ASCI_A)
	{
	    std::sort(pl.begin(), pl.end(), [](Process_Struct& p1, Process_Struct& p2) {
            int res = strcmp(p1.name, p2.name);
            if(res < 0)
			    return true;
		    return false;
		    });
	}
	else
	{
		std::sort(pl.begin(), pl.end(), [](Process_Struct& p1, Process_Struct& p2) {
			int res = strcmp(p1.name, p2.name);
			if (res > 0)
				return true;
			return false;
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
        return FALSE;
    }

    // Get the LUID for the privilege. 
    BOOL bLpv = LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &tkp.Privileges[0].Luid);
    if (!bLpv)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tkp.PrivilegeCount = 1;  // one privilege to set
    if (Enable)
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;


    // Set the privilege for this process. 
    BOOL bAtp = AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, 0);
    if (!bAtp)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

bool isCorrectPlatform()
{
    SYSTEM_INFO stInfo;
    GetSystemInfo(&stInfo);
    int proccessArch = stInfo.wProcessorArchitecture;

    SYSTEM_INFO stInfo2;
    GetNativeSystemInfo(&stInfo2);
    int platformArch = stInfo2.wProcessorArchitecture;

    if (proccessArch == platformArch)
        return true;

    return false;
}

bool FileExistsW(const wchar_t * szFile)
{
    return (GetFileAttributesW(szFile) != INVALID_FILE_ATTRIBUTES);
}