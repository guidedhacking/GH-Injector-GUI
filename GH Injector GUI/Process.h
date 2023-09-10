/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#ifndef NT_FAIL
#define NT_FAIL(status) (status < 0)
#endif

#define INVALID_SESSION_ID (ULONG)-1

#include "pch.h"

#include "DebugConsole.h"

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
	void				*	pBuffer,
	ULONG					BufferSize,
	ULONG				*	SizeOut
);

enum class ARCH : int
{
	NONE,
	X64,
	X86
};

struct ARCHITECTURE
{
	ARCH arch = ARCH::NONE;

	std::wstring ToStdWString();
	std::string ToStdString();

	ARCHITECTURE();
	ARCHITECTURE(UINT in);
	ARCHITECTURE(ARCH in);

	bool operator== (const ARCH & rhs) const;
	bool operator== (const ARCHITECTURE & rhs) const;
};

ARCHITECTURE StrToArchA(const std::string & Str);
ARCHITECTURE StrToArchW(const std::wstring & Str);

class ProcessData
{
	static f_NtQueryInformationProcess m_pNtQueryInformationProcess;

#ifndef _WIN64
	static const std::wstring system32;
	static const std::wstring sysnative;
	static const std::wstring syswow64;
#endif

	std::wstring	m_ExeName;
	std::wstring	m_ExePath;

	DWORD			m_PID			= 0;
	HANDLE			m_Handle		= NULL;
	ARCHITECTURE	m_Architecture	= ARCHITECTURE(ARCH::NONE);
	bool			m_Native		= false;
	ULONG			m_SessionID		= INVALID_SESSION_ID;
	HICON			m_Icon			= NULL;
	bool			m_Valid			= false;

	bool get_pid();
	bool get_name();
	bool get_handle();

	bool load_remaining_data();

	bool get_path();
	bool get_arch();
	bool get_session();
	void get_icon();

	void clean_up();

public:
	ProcessData();
	ProcessData(DWORD PID);
	ProcessData(const std::string & ExeName);
	ProcessData(const std::wstring & ExeName);
	~ProcessData();

	bool IsValid() const;

	bool GetProcessID(DWORD & PID) const;

	bool GetNameA(std::string & ExeName) const;
	bool GetNameW(std::wstring & ExeName) const;

	bool GetFullPathA(std::string & ExePath) const;
	bool GetFullPathW(std::wstring & ExePath) const;

	bool GetArchitecture(ARCHITECTURE & Architecture) const;
	bool IsNativeProcess(bool & Native) const;

	bool GetSessionID(ULONG & SessionID) const;

	bool GetIcon(HICON & hIcon) const;

	bool UpdateData(DWORD PID);
	bool UpdateData(const std::string & ExeName);
	bool UpdateData(const std::wstring & ExeName);
	bool UpdateData(const ProcessData & Data);

	bool IsRunning();

	bool operator== (const ProcessData & rhs) const;
	bool operator== (const DWORD & rhs) const;
	bool operator== (const tagPROCESSENTRY32 & rhs) const;
	bool operator== (const tagPROCESSENTRY32W & rhs) const;

	bool operator< (const ProcessData & rhs) const;
	bool operator> (const ProcessData & rhs) const;
};

bool FileExistsA(const std::string & FilePath);
bool FileExistsW(const std::wstring & FilePath);
ARCHITECTURE GetFileArchitectureA(const std::string & DllFile, bool & IsDotNet);
ARCHITECTURE GetFileArchitectureW(const std::wstring & DllFile, bool & IsDotNet);

#ifdef UNICODE
#define FileExists FileExistsW
#define GetFileArchitecture GetFileArchitectureW
#else
#define FileExists FileExistsA
#define GetFileArchitecture GetFileArchitectureA
#endif

bool GetProcessList(std::vector<ProcessData *> & list);
bool SortProcessList(std::vector<ProcessData *> & list, SORT_SENSE sort);

bool SetDebugPrivilege(bool Enable);