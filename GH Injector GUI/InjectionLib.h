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
	DWORD GetSymbolState();
	DWORD GetImportState();
	float GetDownloadProgressEx(int index, bool bWow64);
	void StartDownload();
	void InterruptDownload();
	bool InterruptInjection();
	DWORD SetRawPrintCallback(f_raw_print_callback callback);

private:

	static const UINT m_HookCount = 30;
	
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> m_StringConverter;

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
	DECLARE_INJECTION_FUNCTION(GetImportState);
	DECLARE_INJECTION_FUNCTION(GetDownloadProgressEx);
	DECLARE_INJECTION_FUNCTION(StartDownload);
	DECLARE_INJECTION_FUNCTION(InterruptDownload);
	DECLARE_INJECTION_FUNCTION(InterruptInjection);
	DECLARE_INJECTION_FUNCTION(SetRawPrintCallback);
};