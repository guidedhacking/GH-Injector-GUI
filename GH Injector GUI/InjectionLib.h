#pragma once

#include "Injection.h"

class InjectionLib
{
public:
	InjectionLib();
	~InjectionLib();

	bool Init();
	bool LoadingStatus();

	void Unload();

	DWORD InjectFuncA(INJECTIONDATAA * pData);
	DWORD InjectFuncW(INJECTIONDATAW * pData);
	int ScanHook(int pid, std::vector<std::string> & hList);
	int RestoreHook(std::vector<int> & hList);
	bool SymbolStatus();
	float DownloadProgress(bool bWow64);

private:
	HookInfo info[30];
	DWORD err1, err2;

	UINT CountOut = 0;
	UINT Changed = 0;
	DWORD targetPid = 0;

	HINSTANCE hInjectionMod;

	f_InjectA InjectA;
	f_InjectW InjectW;

	f_ValidateInjectionFunctions ValidateFunc;
	f_RestoreInjectionFunctions RestoreFunc;

	f_GetVersionA GetVersionA;
	f_GetVersionW GetVersionW;

	f_GetSymbolState GetSymbolState;

	f_GetDownloadProgress GetDownloadProgress;
};