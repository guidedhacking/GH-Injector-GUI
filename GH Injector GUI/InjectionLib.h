#pragma once

#include "Injection.h"
#include "DebugConsole.h"

#define DECLARE_INJECTION_FUNCTION(name) f_##name m_##name

class InjectionLib
{
public:
	InjectionLib();
	~InjectionLib();

	bool Init();
	bool LoadingStatus();

	void Unload();

	DWORD InjectA(INJECTIONDATAA * pData);
	DWORD InjectW(INJECTIONDATAW * pData);
	bool ValidateInjectionFunctions(int PID, std::vector<std::string> & hList);
	bool RestoreInjectionFunctions(std::vector<int> & hList);
	std::string GetVersionA();
	std::wstring GetVersionW();
	bool GetSymbolState();
	float GetDownloadProgress(bool bWow64);
	DWORD SetRawPrintCallback(f_raw_print_callback callback);

private:

	static const UINT m_HookCount = 30;

	HookInfo m_HookInfo[m_HookCount];
	DWORD m_Err;
	DWORD m_Win32Err;

	UINT	m_CountOut;
	UINT	m_Changed;
	DWORD	m_TargetPID;

	HINSTANCE m_hInjectionMod;

	DECLARE_INJECTION_FUNCTION(InjectA);
	DECLARE_INJECTION_FUNCTION(InjectW);
	DECLARE_INJECTION_FUNCTION(ValidateInjectionFunctions);
	DECLARE_INJECTION_FUNCTION(RestoreInjectionFunctions);
	DECLARE_INJECTION_FUNCTION(GetVersionA);
	DECLARE_INJECTION_FUNCTION(GetVersionW);
	DECLARE_INJECTION_FUNCTION(GetSymbolState);
	DECLARE_INJECTION_FUNCTION(GetDownloadProgress);
	DECLARE_INJECTION_FUNCTION(SetRawPrintCallback);
};