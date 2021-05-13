#pragma once

#ifndef NT_FAIL
#define NT_FAIL(status) (status < 0)
#endif

#include "pch.h"

#include "DebugConsole.h"

enum class ARCH : int
{
	NONE,
	X64,
	X86
};

struct Process_Struct
{
	DWORD	PID;
	wchar_t	szName[100];
	wchar_t szPath[MAX_PATH];
	ARCH	Arch;
	int     Session;
	HICON	hIcon;

	Process_Struct();
	~Process_Struct();
};

enum class SORT_SENSE : int
{
	SS_PID_LO,
	SS_PID_HI,
	SS_NAME_LO,
	SS_NAME_HI,
	SS_ARCH_LO,
	SS_ARCH_HI
};

enum _PROCESSINFOCLASS
{
	ProcessSessionInformation = 24
};
typedef _PROCESSINFOCLASS PROCESSINFOCLASS;

struct PROCESS_SESSION_INFORMATION
{
	ULONG SessionId;
};

using f_NtQueryInformationProcess = NTSTATUS(__stdcall *)
(
	HANDLE					hTargetProc,
	PROCESSINFOCLASS		PIC,
	void * pBuffer,
	ULONG					BufferSize,
	ULONG * SizeOut
);

ARCH getFileArchA(const char	* szDllFile);
ARCH getFileArchW(const wchar_t * szDllFile);
ARCH getProcArch(const int PID);

ARCH StrToArchA(const char * szStr);
ARCH StrToArchW(const wchar_t * szStr);

std::string ArchToStrA(ARCH arch);
std::wstring ArchToStrW(ARCH arch);

int getProcSession(const int PID);
bool getProcFullPathA(char		* szFullPath, DWORD BufferSize, int PID);
bool getProcFullPathW(wchar_t	* szfullPath, DWORD BufferSize, int PID);

Process_Struct getProcessByNameA(const char		* szExeName);
Process_Struct getProcessByNameW(const wchar_t	* szExeName);
Process_Struct getProcessByPID(const int PID);

bool getProcessList(std::vector<Process_Struct*> & list, bool get_icon = false);
bool sortProcessList(std::vector<Process_Struct*> & pl, SORT_SENSE sort);

bool SetDebugPrivilege(bool Enable);
bool IsNativeProcess(const int PID);

bool FileExistsW(const wchar_t * szFile);
bool FileExistsA(const char * szFile);

int strcicmpA(const char * a, const char * b);
int strcicmpW(const wchar_t * a, const wchar_t * b);