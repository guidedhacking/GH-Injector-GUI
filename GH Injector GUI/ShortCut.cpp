#include "pch.h"

#include "ShortCut.h"
#include "Injection.h"

HRESULT CreateLink(const std::wstring & TargetObject, const std::wstring & WorkingDirectory, const std::wstring & Description, const std::wstring & LinkPath, const std::wstring & Arguments)
{
	HRESULT	hRet = S_OK;
	IShellLinkW			*	pShellLink		= nullptr;
	IShellLinkDataList	*	pShellLinkData	= nullptr;
	IPersistFile		*	pLinkFile		= nullptr;

	hRet = CoInitialize(NULL);
	if (FAILED(hRet))
	{
		CoUninitialize(); 

		g_print("CoInitialize failed: %08X\n", hRet);

		return hRet;
	}

	hRet = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void **>(&pShellLink));
	if (FAILED(hRet))
	{
		CoUninitialize();

		g_print("CoCreateInstance failed: %08X\n", hRet);

		return hRet;
	}

	pShellLink->SetPath(TargetObject.c_str());
	pShellLink->SetDescription(Description.c_str());
	pShellLink->SetArguments(Arguments.c_str());
	pShellLink->SetWorkingDirectory(WorkingDirectory.c_str());

	hRet = pShellLink->QueryInterface(IID_IShellLinkDataList, reinterpret_cast<void **>(&pShellLinkData));
	if (SUCCEEDED(hRet))
	{
		DWORD flags = NULL;
		pShellLinkData->GetFlags(&flags);
		pShellLinkData->SetFlags(flags | (DWORD)SLDF_RUNAS_USER);

		pShellLinkData->Release();
	}

	hRet = pShellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(&pLinkFile));
	if (FAILED(hRet))
	{
		pShellLink->Release();

		CoUninitialize();
		
		g_print("IShellLinkW::QueryInterface failed: %08X\n", hRet);

		return hRet;
	}
	else
	{
		hRet = pLinkFile->Save(LinkPath.c_str(), TRUE);
		if (FAILED(hRet))
		{
			g_print("IPersistFile::Save failed: %08X\n", hRet);
		}

		pLinkFile->Release();
	}

	pShellLink->Release();

	CoUninitialize();

	return hRet;
}

HRESULT CreateLinkWrapper(const QString & linkName, const QString & linkArgument)
{
	std::wstring ExeTargetPath	= g_RootPath + GH_INJ_EXE_NAME;
	std::wstring WorkingDir		= g_RootPath;
	std::wstring LinkPathName	= g_RootPath + linkName.toStdWString() + L".lnk";
	std::wstring LinkArgument	= linkArgument.toStdWString();
	std::wstring Description	= L"Broihon";

	return CreateLink(ExeTargetPath, WorkingDir, Description, LinkPathName, LinkArgument);
}