/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

const inline auto GH_INJ_GUI_VERSIONA = std::string("4.8");
const inline auto GH_INJ_GUI_VERSIONW = std::wstring(L"4.8");

const inline auto GH_INJ_EXE_LAUNCHA = std::string("GH Injector.exe");
const inline auto GH_INJ_EXE_LAUNCHW = std::wstring(L"GH Injector.exe");

const inline auto GH_INJ_EXE_NAME64A = std::string("GH Injector - x64.exe");
const inline auto GH_INJ_EXE_NAME64W = std::wstring(L"GH Injector - x64.exe");

const inline auto GH_INJ_EXE_NAME86A = std::string("GH Injector - x86.exe");
const inline auto GH_INJ_EXE_NAME86W = std::wstring(L"GH Injector - x86.exe");

const inline auto GH_INJ_MOD_NAME64A = std::string("GH Injector - x64.dll");
const inline auto GH_INJ_MOD_NAME64W = std::wstring(L"GH Injector - x64.dll");

const inline auto GH_INJ_MOD_NAME86A = std::string("GH Injector - x86.dll");
const inline auto GH_INJ_MOD_NAME86W = std::wstring(L"GH Injector - x86.dll");

const inline auto GH_INJ_SM_NAME64A = std::string("GH Injector SM - x64.exe");
const inline auto GH_INJ_SM_NAME64W = std::wstring(L"GH Injector SM - x64.exe");

const inline auto GH_INJ_SM_NAME86A = std::string("GH Injector SM - x86.exe");
const inline auto GH_INJ_SM_NAME86W = std::wstring(L"GH Injector SM - x86.exe");

const inline auto GH_INJ_DNP_NAME64A = std::string("GH Injector DNP - x64.dll");
const inline auto GH_INJ_DNP_NAME64W = std::wstring(L"GH Injector DNP - x64.dll");

const inline auto GH_INJ_DNP_NAME86A = std::string("GH Injector DNP - x86.dll");
const inline auto GH_INJ_DNP_NAME86W = std::wstring(L"GH Injector DNP - x86.dll");

const inline auto GH_INJ_DOTNET_PARSERA = std::string("GH .NET Parser.exe");
const inline auto GH_INJ_DOTNET_PARSERW = std::wstring(L"GH .NET Parser.exe");

const inline auto GH_INJ_ZIPA = std::string("GH Injector.zip");
const inline auto GH_INJ_ZIPW = std::wstring(L"GH Injector.zip");

const inline auto GH_INJ_LOGA = std::string("GH_Inj_Log.txt");
const inline auto GH_INJ_LOGW = std::wstring(L"GH_Inj_Log.txt");

const inline auto GH_HELP_URLA = std::string("https://guidedhacking.com/threads/guidedhacking-dll-injector.8417/");
const inline auto GH_HELP_URLW = std::wstring(L"https://guidedhacking.com/threads/guidedhacking-dll-injector.8417/");

const inline auto GH_DOWNLOAD_PREFIXA = std::string("https://guidedhacking.com/gh/inj/V"); //"http://localhost/GH/V"); epic debugging
const inline auto GH_DOWNLOAD_PREFIXW = std::wstring(L"https://guidedhacking.com/gh/inj/V"); //std::wstring(L"http://localhost/GH/V"); 

const inline auto GH_DOWNLOAD_SUFFIXA = std::string("/GH Injector.zip");
const inline auto GH_DOWNLOAD_SUFFIXW = std::wstring(L"/GH Injector.zip");

const inline auto GH_VERSION_URLA = std::string("https://guidedhacking.com/gh/inj/latestver.txt"); //"http://localhost/GH/latestver.txt");
const inline auto GH_VERSION_URLW = std::wstring(L"https://guidedhacking.com/gh/inj/latestver.txt"); //std::wstring(L"http://localhost/GH/latestver.txt");

const inline auto GH_SETTINGS_INIA = std::string("Settings.ini");
const inline auto GH_SETTINGS_INIW = std::wstring(L"Settings.ini");

#ifdef _WIN64
const inline auto & GH_INJ_MOD_NAMEA	= GH_INJ_MOD_NAME64A;
const inline auto & GH_INJ_MOD_NAMEW	= GH_INJ_MOD_NAME64W;
const inline auto & GH_INJ_EXE_NAMEA	= GH_INJ_EXE_NAME64A;
const inline auto & GH_INJ_EXE_NAMEW	= GH_INJ_EXE_NAME64W;
const inline auto & GH_INJ_SM_NAMEA		= GH_INJ_SM_NAME64A;
const inline auto & GH_INJ_SM_NAMEW		= GH_INJ_SM_NAME64W;
const inline auto & GH_INJ_DNP_NAMEA	= GH_INJ_DNP_NAME64A;
const inline auto & GH_INJ_DNP_NAMEW	= GH_INJ_DNP_NAME64W;
#else
const inline auto & GH_INJ_MOD_NAMEA	= GH_INJ_MOD_NAME86A;
const inline auto & GH_INJ_MOD_NAMEW	= GH_INJ_MOD_NAME86W;
const inline auto & GH_INJ_EXE_NAMEA	= GH_INJ_EXE_NAME86A;
const inline auto & GH_INJ_EXE_NAMEW	= GH_INJ_EXE_NAME86W;
const inline auto & GH_INJ_SM_NAMEA		= GH_INJ_SM_NAME86A;
const inline auto & GH_INJ_SM_NAMEW		= GH_INJ_SM_NAME86W;
const inline auto & GH_INJ_DNP_NAMEA	= GH_INJ_DNP_NAME86A;
const inline auto & GH_INJ_DNP_NAMEW	= GH_INJ_DNP_NAME86W;
#endif

#ifdef UNICODE
const inline auto & GH_INJ_GUI_VERSION	= GH_INJ_GUI_VERSIONW;
const inline auto & GH_INJ_MOD_NAME		= GH_INJ_MOD_NAMEW;
const inline auto & GH_INJ_LAUNCHER_EXE	= GH_INJ_EXE_LAUNCHW;
const inline auto & GH_INJ_EXE_NAME		= GH_INJ_EXE_NAMEW;
const inline auto & GH_INJ_SM_NAME		= GH_INJ_SM_NAMEW;
const inline auto & GH_INJ_DNP_NAME		= GH_INJ_DNP_NAMEW;
const inline auto & GH_INJ_ZIP			= GH_INJ_ZIPW;
const inline auto & GH_INJ_LOG			= GH_INJ_LOGW;
const inline auto & GH_HELP_URL			= GH_HELP_URLW;
const inline auto & GH_DOWNLOAD_PREFIX	= GH_DOWNLOAD_PREFIXW;
const inline auto & GH_DOWNLOAD_SUFFIX	= GH_DOWNLOAD_SUFFIXW;
const inline auto & GH_VERSION_URL		= GH_VERSION_URLW;
const inline auto & GH_SETTINGS_INI		= GH_SETTINGS_INIW;

const inline auto & GH_INJ_MOD_NAME64		= GH_INJ_MOD_NAME64W;
const inline auto & GH_INJ_MOD_NAME86		= GH_INJ_MOD_NAME86W;
const inline auto & GH_INJ_EXE_NAME64		= GH_INJ_EXE_NAME64W;
const inline auto & GH_INJ_EXE_NAME86		= GH_INJ_EXE_NAME86W;
const inline auto & GH_INJ_SM_NAME64		= GH_INJ_SM_NAME64W;
const inline auto & GH_INJ_SM_NAME86		= GH_INJ_SM_NAME86W;
const inline auto & GH_INJ_DNP_NAME64		= GH_INJ_DNP_NAME64W;
const inline auto & GH_INJ_DNP_NAME86		= GH_INJ_DNP_NAME86W;
const inline auto & GH_INJ_DOTNET_PARSER	= GH_INJ_DOTNET_PARSERW;
#else
const inline auto & GH_INJ_GUI_VERSION	= GH_INJ_GUI_VERSIONA;
const inline auto & GH_INJ_MOD_NAME		= GH_INJ_MOD_NAMEA;
const inline auto & GH_INJ_LAUNCHER_EXE	= GH_INJ_EXE_LAUNCHA;
const inline auto & GH_INJ_EXE_NAME		= GH_INJ_EXE_NAMEA;
const inline auto & GH_INJ_SM_NAME		= GH_INJ_SM_NAMEA;
const inline auto & GH_INJ_DNP_NAME		= GH_INJ_DNP_NAMEA;
const inline auto & GH_INJ_ZIP			= GH_INJ_ZIPA;
const inline auto & GH_INJ_LOG			= GH_INJ_LOGA;
const inline auto & GH_HELP_URL			= GH_HELP_URLA;
const inline auto & GH_DOWNLOAD_PREFIX	= GH_DOWNLOAD_PREFIXA;
const inline auto & GH_DOWNLOAD_SUFFIX	= GH_DOWNLOAD_SUFFIXA;
const inline auto & GH_VERSION_URL		= GH_VERSION_URLA;
const inline auto & GH_SETTINGS_INI		= GH_SETTINGS_INIA;

const inline auto & GH_INJ_MOD_NAME64		= GH_INJ_MOD_NAME64A;
const inline auto & GH_INJ_MOD_NAME86		= GH_INJ_MOD_NAME86A;
const inline auto & GH_INJ_EXE_NAME64		= GH_INJ_EXE_NAME64A;
const inline auto & GH_INJ_EXE_NAME86		= GH_INJ_EXE_NAME86A;
const inline auto & GH_INJ_SM_NAME64		= GH_INJ_SM_NAME64A;
const inline auto & GH_INJ_SM_NAME86		= GH_INJ_SM_NAME86A;
const inline auto & GH_INJ_DNP_NAME64		= GH_INJ_DNP_NAME64A;
const inline auto & GH_INJ_DNP_NAME86		= GH_INJ_DNP_NAME86A;
const inline auto & GH_INJ_DOTNET_PARSER	= GH_INJ_DOTNET_PARSERA;
#endif

enum class INJECTION_MODE
{
	IM_LoadLibraryExW,
	IM_LdrLoadDll,
	IM_LdrpLoadDll,
	IM_LdrpLoadDllInternal,
	IM_ManualMap
};

enum class LAUNCH_METHOD
{
	LM_NtCreateThreadEx,
	LM_HijackThread,
	LM_SetWindowsHookEx,
	LM_QueueUserAPC,
	LM_KernelCallback,
	LM_FakeVEH
};

//ansi version of the info structure:
struct INJECTIONDATAA
{
	char			szDllPath[MAX_PATH * 2];	//fullpath to the dll to inject
	DWORD			ProcessID;					//process identifier of the target process
	INJECTION_MODE	Mode;						//injection mode
	LAUNCH_METHOD	Method;						//method to execute the remote shellcode
	DWORD			Flags;						//combination of the flags defined above
	DWORD			Timeout;					//timeout for DllMain return in milliseconds
	DWORD			hHandleValue;				//optional value to identify a handle in a process
	HINSTANCE		hDllOut;					//returned image base of the injection
	bool			GenerateErrorLog;			//if true error data is generated and stored in GH_Inj_Log.txt
};

//unicode version of the info structure (documentation above)
struct INJECTIONDATAW
{
	wchar_t			szDllPath[MAX_PATH * 2];
	DWORD			ProcessID;
	INJECTION_MODE	Mode;
	LAUNCH_METHOD	Method;
	DWORD			Flags;
	DWORD			Timeout;
	DWORD			hHandleValue;
	HINSTANCE		hDllOut;
	bool			GenerateErrorLog;
};

#ifdef UNICODE
#define INJECTIONDATA INJECTIONDATAW
#else
#define INJECTIONDATA INJECTIONDATAA
#endif

//ansi version of the .NET info structure:
struct DOTNET_INJECTIONDATAA
{
	char			szDllPath[MAX_PATH * 2];
	DWORD			ProcessID;
	INJECTION_MODE	Mode;
	LAUNCH_METHOD	Method;
	DWORD			Flags;
	DWORD			Timeout;
	DWORD			hHandleValue;
	HINSTANCE		hDllOut;
	bool			GenerateErrorLog;

	char szNamespace[128];	//namespace of the class in the target module
	char szClassName[128];	//name of the class in the target module
	char szMethodName[128];	//name of the method in the target module
	char szArgument[128];	//argument to be send to the method
};

//unicode version of the .NET info structure:
struct DOTNET_INJECTIONDATAW
{
	wchar_t			szDllPath[MAX_PATH * 2];
	DWORD			ProcessID;
	INJECTION_MODE	Mode;
	LAUNCH_METHOD	Method;
	DWORD			Flags;
	DWORD			Timeout;
	DWORD			hHandleValue;
	HINSTANCE		hDllOut;
	bool			GenerateErrorLog;

	wchar_t szNamespace[128];
	wchar_t szClassName[128];
	wchar_t szMethodName[128];
	wchar_t szArgument[128];
};

//use this to load a file from memory (manual mapping only, other methods will be ignored, make sure to set appropriate manual mapping flags)
struct MEMORY_INJECTIONDATA
{
	BYTE *			RawData;	//pointer to raw file data
	DWORD			RawSize;	//size in bytes of RawData
	DWORD			ProcessID;
	INJECTION_MODE	Mode;
	LAUNCH_METHOD	Method;
	DWORD			Flags;
	DWORD			Timeout;
	DWORD			hHandleValue;
	HINSTANCE		hDllOut;
	bool			GenerateErrorLog;
};

//amount of bytes to be scanned by ValidateInjectionFunctions and restored by RestoreInjectionFunctions
#define HOOK_SCAN_BYTE_COUNT 0x10

//ValidateInjectionFunctions fills an std::vector with this info, result can simply be passed to RestoreInjectionFunctions
struct HookInfo
{
	const wchar_t	* ModuleName;
	const char		* FunctionName;

	HINSTANCE		hModuleBase;
	void		*	pFunc;
	UINT			ChangeCount;
	BYTE			OriginalBytes[HOOK_SCAN_BYTE_COUNT];

	DWORD ErrorCode;
};

//Cloaking options:
#define INJ_ERASE_HEADER				0x0001	//replaces the first 0x1000 bytes of the dll with 0's (takes priority over INJ_FAKE_HEADER if both are specified)
#define INJ_FAKE_HEADER					0x0002	//replaces the dlls header with the header of the ntdll.dll (superseded by INJ_ERASE_HEADER if both are specified)
#define INJ_UNLINK_FROM_PEB				0x0004	//unlinks the module from the process enviroment block (1)
#define INJ_THREAD_CREATE_CLOAKED		0x0008	//induces INJ_CTF_FAKE_START_ADDRESS | INJ_CTF_HIDE_FROM_DEBUGGER (2), see thread creation options for more flags
#define INJ_SCRAMBLE_DLL_NAME			0x0010	//randomizes the dll name on disk before injecting it
#define INJ_LOAD_DLL_COPY				0x0020	//loads a copy of the dll from %temp% directory
#define INJ_HIJACK_HANDLE				0x0040	//tries to a hijack a handle from another process instead of using OpenProcess

//Notes:
///(1) ignored when manual mapping
///(2) launch method must be NtCreateThreadEx, ignored otherwise

//Thread creation options:
#define INJ_CTF_FAKE_START_ADDRESS	0x00001000
#define INJ_CTF_HIDE_FROM_DEBUGGER	0x00002000
#define INJ_CTF_SKIP_THREAD_ATTACH	0x00004000
#define INJ_CTF_FAKE_TEB_CLIENT_ID	0x00008000
#define CTF_MASK (INJ_CTF_FAKE_START_ADDRESS | INJ_CTF_HIDE_FROM_DEBUGGER | INJ_CTF_SKIP_THREAD_ATTACH | INJ_CTF_FAKE_TEB_CLIENT_ID)

//Manual mapping options:
#define INJ_MM_CLEAN_DATA_DIR			0x00010000	//removes data from the dlls PE header, ignored if INJ_MM_SET_PAGE_PROTECTIONS is set
#define INJ_MM_RESOLVE_IMPORTS			0x00020000	//resolves dll imports
#define INJ_MM_RESOLVE_DELAY_IMPORTS	0x00040000	//resolves delayed imports
#define INJ_MM_EXECUTE_TLS				0x00080000	//executes TLS callbacks and initializes static TLS data
#define INJ_MM_ENABLE_EXCEPTIONS		0x00100000	//enables exception handling
#define INJ_MM_SET_PAGE_PROTECTIONS		0x00200000	//sets page protections based on section characteristics, if set INJ_MM_CLEAN_DATA_DIR will be ignored
#define INJ_MM_INIT_SECURITY_COOKIE		0x00400000	//initializes security cookie for buffer overrun protection
#define INJ_MM_RUN_DLL_MAIN				0x00800000	//executes DllMain
													//this option induces INJ_MM_RESOLVE_IMPORTS
#define INJ_MM_RUN_UNDER_LDR_LOCK		0x01000000	//runs the DllMain under the loader lock
#define INJ_MM_SHIFT_MODULE_BASE		0x02000000	//shifts the module base by a random offset
#define INJ_MM_MAP_FROM_MEMORY			0x04000000	//loads the file from memory instead of from disk (1)
#define INJ_MM_LINK_MODULE				0x08000000	//links the module to the PEB (currently not supported)

//Notes:
///(1) only works with Memory_Inject and is set automatically when that function is used, ignored when passed to (DotNet_)InjectA/W

#define MM_DEFAULT (INJ_MM_RESOLVE_IMPORTS | INJ_MM_RESOLVE_DELAY_IMPORTS | INJ_MM_INIT_SECURITY_COOKIE | INJ_MM_EXECUTE_TLS | INJ_MM_ENABLE_EXCEPTIONS | INJ_MM_RUN_DLL_MAIN | INJ_MM_SET_PAGE_PROTECTIONS | INJ_MM_RUN_UNDER_LDR_LOCK)
#define MM_MASK (MM_DEFAULT | INJ_MM_SHIFT_MODULE_BASE | INJ_MM_CLEAN_DATA_DIR | INJ_MM_MAP_FROM_MEMORY)

#define INJ_ERR_SUCCESS					0x00000000
#define INJ_ERR_SYMBOL_INIT_NOT_DONE	0x0000001C
#define INJ_ERR_IMPORT_HANDLER_NOT_DONE 0x00000037

using f_InjectA = DWORD(__stdcall *)(INJECTIONDATAA * pData);
using f_InjectW = DWORD(__stdcall *)(INJECTIONDATAW * pData);

using f_DotNet_InjectA = DWORD(__stdcall *)(DOTNET_INJECTIONDATAA * pData);
using f_DotNet_InjectW = DWORD(__stdcall *)(DOTNET_INJECTIONDATAW * pData);

using f_Memory_Inject = DWORD(__stdcall *)(MEMORY_INJECTIONDATA * pData);

using f_ValidateInjectionFunctions	= bool(__stdcall *)(DWORD dwTargetProcessId, DWORD & ErrorCode, DWORD & LastWin32Error, HookInfo * HookDataOut, UINT Count, UINT * CountOut);
using f_RestoreInjectionFunctions	= bool(__stdcall *)(DWORD dwTargetProcessId, DWORD & ErrorCode, DWORD & LastWin32Error, HookInfo * HookDataIn, UINT Count, UINT * CountOut);

using f_GetVersionA = HRESULT(__stdcall *)(char		* out, size_t cb_size);
using f_GetVersionW = HRESULT(__stdcall *)(wchar_t	* out, size_t cb_size);

using f_GetSymbolState = DWORD(__stdcall *)();
using f_GetImportState = DWORD(__stdcall *)();

using f_GetDownloadProgressEx	= float(__stdcall *)(int index, bool bWoW64);
using f_StartDownload			= void(__stdcall *)();
using f_InterruptDownload		= void(__stdcall *)();
using f_InterruptInjection		= bool(__stdcall *)(DWORD Timeout);

using f_raw_print_callback	= void(__stdcall *)(const char * szText);
using f_SetRawPrintCallback = DWORD(__stdcall *)(f_raw_print_callback callback);