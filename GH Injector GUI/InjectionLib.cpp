#include "pch.h"

#include "InjectionLib.h"

InjectionLib::InjectionLib()
	: hInjectionMod(nullptr), InjectA(nullptr), InjectW(nullptr), ValidateFunc(nullptr), RestoreFunc(nullptr)
{

}

InjectionLib::~InjectionLib()
{
	FreeLibrary(hInjectionMod);
}

bool InjectionLib::Init()
{
	hInjectionMod = LoadLibrary(GH_INJ_MOD_NAME);
	if (hInjectionMod == nullptr)
	{
		return false;
	}

	InjectA = reinterpret_cast<f_InjectA>(GetProcAddress(hInjectionMod, "InjectA"));
	InjectW = reinterpret_cast<f_InjectW>(GetProcAddress(hInjectionMod, "InjectW"));

	ValidateFunc = reinterpret_cast<f_ValidateInjectionFunctions>(GetProcAddress(hInjectionMod, "ValidateInjectionFunctions"));
	RestoreFunc = reinterpret_cast<f_RestoreInjectionFunctions>(GetProcAddress(hInjectionMod, "RestoreInjectionFunctions"));

	GetVersionA = reinterpret_cast<f_GetVersionA>(GetProcAddress(hInjectionMod, "GetVersionA"));
	GetVersionW = reinterpret_cast<f_GetVersionW>(GetProcAddress(hInjectionMod, "GetVersionW"));

	GetSymbolState = reinterpret_cast<f_GetSymbolState>(GetProcAddress(hInjectionMod, "GetSymbolState"));

	GetDownloadProgress = reinterpret_cast<f_GetDownloadProgress>(GetProcAddress(hInjectionMod, "GetDownloadProgress"));

	return LoadingStatus();
}

bool InjectionLib::LoadingStatus()
{
	if (hInjectionMod == nullptr)
	{
		return false;
	}

	if (InjectA == nullptr || InjectW == nullptr || ValidateFunc == nullptr || RestoreFunc == nullptr || GetVersionA == nullptr || GetVersionW == nullptr || GetSymbolState == nullptr || GetDownloadProgress == nullptr)
	{
		return false;
	}

	return true;
}

void InjectionLib::Unload()
{
	FreeLibrary(hInjectionMod);
}

DWORD InjectionLib::InjectFuncA(INJECTIONDATAA * pData)
{
	if (!LoadingStatus())
	{
		(DWORD)-1;
	}

	return InjectA(pData);
}

DWORD InjectionLib::InjectFuncW(INJECTIONDATAW * pData)
{
	if (!LoadingStatus())
	{
		(DWORD)-1;
	}

	return InjectW(pData);
}

int InjectionLib::ScanHook(int pid, std::vector<std::string> & hList)
{
	if (!LoadingStatus())
	{
		(DWORD)-1;
	}

	memset(info, 0, sizeof(info));
	targetPid = pid;

	DWORD err1, err2;
	auto val_ret = ValidateFunc(targetPid, err1, err2, info, 30, &CountOut);

	if (!val_ret)
	{
		return (DWORD)-2;
	}

	for (UINT i = 0; i != CountOut; ++i)
	{
		if (info[i].ChangeCount && !info[i].ErrorCode)
		{
			hList.push_back(std::string(info[i].ModuleName) + "->" + std::string(info[i].FunctionName));

			++Changed;
		}
	}

	return 0;
}

int InjectionLib::RestoreHook(std::vector<int> & IndexList)
{
	if (!LoadingStatus())
	{
		(DWORD)-1;
	}

	if (!IndexList.size())
	{
		(DWORD)-2;
	}

	HookInfo infoRestore[30]{ 0 };
	int counter = 0;

	for (auto i : IndexList)
	{
		infoRestore[counter++] = info[i];
		printf("Restoring (%d) %s\n", i, info[i].FunctionName);
	}

	auto res_ret = RestoreFunc(targetPid, err1, err2, infoRestore, CountOut, &CountOut);

	if (!res_ret)
	{
		(DWORD)-3;
	}

	return 0;
}

bool InjectionLib::SymbolStatus()
{
	return (GetSymbolState() == 0);
}

float InjectionLib::DownloadProgress(bool bWow64)
{
	return GetDownloadProgress(bWow64);
}