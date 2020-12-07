#include "InjectionLib.hpp"


InjectionLib::InjectionLib()
    : hInjectionMod(nullptr), InjectA(nullptr), InjectW(nullptr),ValidateFunc(nullptr),RestoreFunc(nullptr)
{
    //Init();
}


InjectionLib::~InjectionLib()
{
    FreeLibrary(hInjectionMod);
}

bool InjectionLib::Init()
{
    hInjectionMod = LoadLibrary(GH_INJ_MOD_NAME);
    if (hInjectionMod == nullptr)
        return false;

    InjectA = reinterpret_cast<f_InjectA>(GetProcAddress(hInjectionMod, "InjectA"));
    InjectW = reinterpret_cast<f_InjectW>(GetProcAddress(hInjectionMod, "InjectW"));
    ValidateFunc = reinterpret_cast<f_ValidateInjectionFunctions>(GetProcAddress(hInjectionMod, "ValidateInjectionFunctions"));
    RestoreFunc = reinterpret_cast<f_RestoreInjectionFunctions>(GetProcAddress(hInjectionMod, "RestoreInjectionFunctions"));
	GetVersionA = reinterpret_cast<f_GetVersionA>(GetProcAddress(hInjectionMod, "GetVersionA"));
	GetVersionW = reinterpret_cast<f_GetVersionW>(GetProcAddress(hInjectionMod, "GetVersionW"));
	GetSymbolState = reinterpret_cast<f_GetSymbolState>(GetProcAddress(hInjectionMod, "GetSymbolState"));

    if (InjectA == nullptr || InjectW == nullptr || ValidateFunc == nullptr || RestoreFunc == nullptr || GetVersionA == nullptr || GetVersionW == nullptr || GetSymbolState == nullptr)
        return false;

    return true;
}

bool InjectionLib::LoadingStatus()
{
    if (hInjectionMod == nullptr)
        return false;
	
    if (InjectA == nullptr || InjectW == nullptr || ValidateFunc == nullptr || RestoreFunc == nullptr)
        return false;

    return true;
}

void InjectionLib::Unload()
{
	FreeLibrary(hInjectionMod);
}

DWORD InjectionLib::InjectFuncA(INJECTIONDATAA* pData)
{
    if (!LoadingStatus())
        return 42;
	
	return InjectA(pData);	
}

DWORD InjectionLib::InjectFuncW(INJECTIONDATAW* pData)
{
    if (!LoadingStatus())
        return 42;
	
    return InjectW(pData);
}

int InjectionLib::ScanHook(int pid, std::vector<std::string>& hList)
{
	if (!LoadingStatus())
		return 42;

    memset(info, 0, sizeof(info));
	targetPid = pid;

    DWORD err1, err2;
    auto val_ret = ValidateFunc(targetPid, err1, err2, info, 30, &CountOut);

	if (!val_ret)
	{
		//printf("ValidateInjectionFunctions failed:\n\t%08X\n\t%08X\n", err1, err2);
		return 43;
	}

    //printf("Injection functions validated\n");

	
	for (UINT i = 0; i != CountOut; ++i)
	{
		if (info[i].ChangeCount && !info[i].ErrorCode)
		{
			//printf("Hook detected: %s->%s (%d)\n", info[i].ModuleName, info[i].FunctionName, info[i].ChangeCount);
			hList.push_back(std::string(info[i].ModuleName) + "->" + std::string(info[i].FunctionName) /*+ info[i].ChangeCount */);
			++Changed;
		}
	}
    
    return 0;
    //ToDo: return Error
}

int InjectionLib::RestoreHook(std::vector<std::string>& hList)
{
	if (!LoadingStatus())
		return 42;

	HookInfo infoRestore[30];
	memset(infoRestore, 0, sizeof(infoRestore));
	int counter = 0;

	// far too complicated
	for (UINT i = 0; i != CountOut; ++i)
	{
		if (info[i].ChangeCount && !info[i].ErrorCode)
		{
			std::string guiString = std::string(info[i].ModuleName) + "->" + std::string(info[i].FunctionName) /*+ info[i].ChangeCount */;
			if (std::find(hList.begin(), hList.end(), guiString) != hList.end())
			{
				memcpy(&infoRestore[counter], &info[i], sizeof(HookInfo));
				counter++;
			}
		}
	}

	if (counter)
	{
		//printf("Restoring hooks\n");

		auto res_ret = RestoreFunc(targetPid, err1, err2, infoRestore, CountOut, &CountOut);

		if (!res_ret)
		{
			//printf("RestoreInjectionFunctions failed:\n\t%08X\n\t%08X\n", err1, err2);

			return 43;
		}

		//printf("Hooks restored\n");
	}
	else
	{
		//printf("No hooks found\n");
	}

	return 0;
}

bool InjectionLib::SymbolStatus()
{
	return (GetSymbolState() == 0);
}