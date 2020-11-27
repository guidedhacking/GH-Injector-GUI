#include "ShortCut.h"
#include "QCoreApplication"
#include "QString"

#include "windows.h"
#include "winnls.h"
#include "shobjidl.h"
#include "objbase.h"
#include "objidl.h"
#include "shlguid.h"

HRESULT CreateLink(LPCWSTR lpszPathObj, LPCSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR args)
{
	HRESULT hres;
	IShellLink* psl;

	CoInitialize(NULL);
	// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
	// has already been called.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description. 
		psl->SetPath(lpszPathObj);
		// psl->SetDescription(lpszDesc);

		psl->SetArguments(args);
		psl->SetWorkingDirectory(lpszDesc);

		// Query IShellLink for the IPersistFile interface, used for saving the 
		// shortcut in persistent storage. 
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode. 
			MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

			// Add code here to check return value from MultiByteWideChar 
			// for success.

			// Save the link by calling IPersistFile::Save. 
			hres = ppf->Save(wsz, TRUE);
			ppf->Release();
		}
		psl->Release();
	}
	return hres;
}


bool CreateLinkWrapper(QString linkName, QString linkArgument)
{
	QString desPath = QCoreApplication::applicationDirPath() + "\\";
	QString InjectorExe = QCoreApplication::applicationFilePath();

	desPath.replace("/", "\\");
	InjectorExe.replace("/", "\\");

	QString completeLinkName = desPath + linkName + QString(".lnk");

	//WCHAR str[MAX_PATH] = L"C:\\git\\QT_GH_Injector\\x64\\Static\\QT_GH_Injector.exe";
	//CHAR str2[MAX_PATH] = "C:\\git\\QT_GH_Injector\\x64\\Static\\QT_GH_Injector_LINK.lnk";
	//WCHAR str3[MAX_PATH] = L"C:\\git\\QT_GH_Injector\\x64\\Static\\";
	//WCHAR args[MAX_PATH] = L"-f C:\\temp\\HelloWorld_x64.dll -p notepad.exe";

	wchar_t str1[1024] = { 0 };
	InjectorExe.toWCharArray(str1);

	//char str2[1024];
	//completeLinkName.toStdString().c_str();

	wchar_t str3[1024] = { 0 };
	desPath.toWCharArray(str3);

	wchar_t str4[1024] = { 0 };
	linkArgument.toWCharArray(str4);

	HRESULT res = CreateLink(str1, completeLinkName.toStdString().c_str(), str3, str4);
	if (res == S_OK)
		return true;

	return false;
}

