/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "Injection.h"
#include "DebugConsole.h"

#define DECLARE_INIT_INJECTION_FUNCTION(name) f_##name m_##name = nullptr

class InjectionLib
{
	static const UINT m_HookCount = 50;

	HookInfo m_HookInfo[m_HookCount];

	DWORD m_Err			= 0;
	DWORD m_Win32Err	= 0;

	UINT	m_CountOut	= 0;
	UINT	m_Changed	= 0;

	DWORD	m_TargetPID = 0;

	HINSTANCE m_hInjectionMod = NULL;

	DECLARE_INIT_INJECTION_FUNCTION(InjectA);
	DECLARE_INIT_INJECTION_FUNCTION(InjectW);
	DECLARE_INIT_INJECTION_FUNCTION(DotNet_InjectA);
	DECLARE_INIT_INJECTION_FUNCTION(DotNet_InjectW);
	DECLARE_INIT_INJECTION_FUNCTION(Memory_Inject);
	DECLARE_INIT_INJECTION_FUNCTION(ValidateInjectionFunctions);
	DECLARE_INIT_INJECTION_FUNCTION(RestoreInjectionFunctions);
	DECLARE_INIT_INJECTION_FUNCTION(GetVersionA);
	DECLARE_INIT_INJECTION_FUNCTION(GetVersionW);
	DECLARE_INIT_INJECTION_FUNCTION(GetSymbolState);
	DECLARE_INIT_INJECTION_FUNCTION(GetImportState);
	DECLARE_INIT_INJECTION_FUNCTION(GetDownloadProgressEx);
	DECLARE_INIT_INJECTION_FUNCTION(StartDownload);
	DECLARE_INIT_INJECTION_FUNCTION(InterruptDownload);
	DECLARE_INIT_INJECTION_FUNCTION(InterruptInjection);
	DECLARE_INIT_INJECTION_FUNCTION(SetRawPrintCallback);

public:
	InjectionLib();
	~InjectionLib();

	bool Init();
	bool LoadingStatus() const;

	void Unload();

	DWORD InjectA(INJECTIONDATAA * pData) const;
	DWORD InjectW(INJECTIONDATAW * pData) const;
	DWORD DotNet_InjectA(DOTNET_INJECTIONDATAA * pData) const;
	DWORD DotNet_InjectW(DOTNET_INJECTIONDATAW * pData) const;
	DWORD Memory_Inject(MEMORY_INJECTIONDATA * pData) const;
	bool ValidateInjectionFunctions(int PID, std::vector<std::wstring> & hList);
	bool RestoreInjectionFunctions(std::vector<int> & hList);
	std::string GetVersionA() const;
	std::wstring GetVersionW() const;
	DWORD GetSymbolState() const;
	DWORD GetImportState() const;
	float GetDownloadProgressEx(int index, bool bWow64) const;
	void StartDownload() const;
	void InterruptDownload() const;
	bool InterruptInjection() const;
	DWORD SetRawPrintCallback(f_raw_print_callback callback) const;
};