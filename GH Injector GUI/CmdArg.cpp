#include "pch.h"

#include "CmdArg.h"

void help();

int FindArg(int argc, const wchar_t * arg, const wchar_t * const argv[], bool Parameter = false);

template <typename T>
T GetArg(int argc, const wchar_t * arg, const wchar_t * const argv[], T DefaultValue = T());
template <> bool			GetArg<bool>		(int argc, const wchar_t * arg, const wchar_t * const argv[], bool			DefaultValue);
template <> DWORD			GetArg<DWORD>		(int argc, const wchar_t * arg, const wchar_t * const argv[], DWORD			DefaultValue);
template <> UINT			GetArg<UINT>		(int argc, const wchar_t * arg, const wchar_t * const argv[], UINT			DefaultValue);
template <> std::wstring	GetArg<std::wstring>(int argc, const wchar_t * arg, const wchar_t * const argv[], std::wstring	DefaultValue);

int CmdArg(int argc, const wchar_t * const argv[], bool & silent)
{
	//Console output is buggy for non ASCII characters, works fine internally

	silent = GetArg<bool>(argc, L"-silent", argv);
	if (!silent)
	{
		AllocConsole();
		FILE * pFile = nullptr;
		freopen_s(&pFile, "CONOUT$", "w", stdout);
	}

	if (!SetDebugPrivilege(true))
	{
		printf("Failed to enable debug privileges. This might affect the functionality of the injector.\n");
	}

	if (GetArg<bool>(argc, L"-help", argv))
	{
		silent = false;

		help();

		return -1;
	}

	InjectionLib lib;
	if (!lib.Init())
	{
		printf("Failed to initialze injection library.\n");

		return -1;
	}
	else
	{
		printf("Injection library intialized\n");
	}

	lib.StartDownload();

	if (GetArg<bool>(argc, L"-version", argv))
	{
		printf("GH Injector library Version = V%s\n", lib.GetVersionA().c_str());
		printf("GH Injector GUI Version = V%s\n", GH_INJ_GUI_VERSIONA.c_str());

		silent = false;

		return -1;
	}

	auto ProcessID		= GetArg<UINT>(argc, L"-i", argv, 0);
	auto ProcessName	= GetArg<std::wstring>(argc, L"-p", argv);

	if (ProcessName.empty() && !ProcessID)
	{
		printf("No target process specified.\n");

		return -1;
	}
	else
	{
		if (ProcessID)
		{
			printf("Target process = %08X\n", ProcessID);
		}
		else
		{
			printf("Target process = %ls\n", ProcessName.c_str());
		}
	}

	auto DllPath = GetArg<std::wstring>(argc, L"-f", argv);
	if (DllPath.empty())
	{
		printf("No dll to inject specified.\n");

		return -1;
	}
	else
	{
		if (!FileExistsW(DllPath))
		{
			printf("Specified DLL file doesn't exist.\n");

			return -1;
		}

		printf("DLL file = %ls\n", DllPath.c_str());
	}
		
	bool isDotNet	= false;
	bool fromMemory = false;
	INJECTIONDATAW			data		{ 0 };
	DOTNET_INJECTIONDATAW	dotnet_data	{ 0 };
	MEMORY_INJECTIONDATA	memory_data	{ 0 };

	if (DllPath.length() + 1 >= sizeof(data.szDllPath) / sizeof(wchar_t))
	{
		printf("Target DLL path exceeds 520 characters.\n");

		return -1;
	}

	DllPath.copy(data.szDllPath, DllPath.length());

	auto injection_mode = (INJECTION_MODE)GetArg<UINT>(argc, L"-l", argv);
	switch (injection_mode)
	{
		case INJECTION_MODE::IM_LoadLibraryExW:
		case INJECTION_MODE::IM_LdrLoadDll:
		case INJECTION_MODE::IM_LdrpLoadDll:
		case INJECTION_MODE::IM_LdrpLoadDllInternal:
		case INJECTION_MODE::IM_ManualMap:
			data.Mode = injection_mode;
			break;

		default:
			printf("Invalid injection mode specified. Defaulting to LoadLibraryExW.\n");
			data.Mode = INJECTION_MODE::IM_LoadLibraryExW;
	}
	printf("Injection mode = %d\n", injection_mode);

	auto launch_method = (LAUNCH_METHOD)GetArg<UINT>(argc, L"-s", argv);
	switch (launch_method)
	{
		case LAUNCH_METHOD::LM_NtCreateThreadEx:
		case LAUNCH_METHOD::LM_HijackThread:
		case LAUNCH_METHOD::LM_SetWindowsHookEx:
		case LAUNCH_METHOD::LM_QueueUserAPC:
		case LAUNCH_METHOD::LM_KernelCallback:
		case LAUNCH_METHOD::LM_FakeVEH:
			data.Method = launch_method;
			break;

		default:
			printf("Invalid launch method specified. Defaulting to NtCreateThreadEx.\n");
			data.Method = LAUNCH_METHOD::LM_NtCreateThreadEx;
			break;
	}
	printf("Launch method = %d\n", launch_method);

	DWORD Flags = 0;

	auto peh_option = GetArg<UINT>(argc, L"-peh", argv);
	switch (peh_option)
	{
		case 1: Flags |= INJ_ERASE_HEADER; break;
		case 2: Flags |= INJ_FAKE_HEADER; break;
		default: break;
	}

	Flags |= GetArg<bool>(argc, L"-unlink", argv) ? INJ_UNLINK_FROM_PEB		: NULL;
	Flags |= GetArg<bool>(argc, L"-random", argv) ? INJ_SCRAMBLE_DLL_NAME	: NULL;
	Flags |= GetArg<bool>(argc, L"-copy",	argv) ? INJ_LOAD_DLL_COPY		: NULL;
	Flags |= GetArg<bool>(argc, L"-hijack", argv) ? INJ_HIJACK_HANDLE		: NULL;

	if (data.Method == LAUNCH_METHOD::LM_NtCreateThreadEx)
	{
		if (GetArg<bool>(argc, L"-cloak", argv))
		{
			Flags |= INJ_THREAD_CREATE_CLOAKED;
		}

		DWORD cflags = GetArg<DWORD>(argc, L"-cloakflags", argv, NULL);
		Flags |= (cflags & CTF_MASK);
	}

	if (data.Mode == INJECTION_MODE::IM_ManualMap)
	{
		DWORD mmflags = GetArg<DWORD>(argc, L"-mmflags", argv, MM_DEFAULT);
		Flags |= (mmflags & MM_MASK);
	}

	data.Flags = Flags;

	printf("Flags = %08X\n", Flags);

	data.GenerateErrorLog	= GetArg<bool>(argc, L"-log", argv);
	data.Timeout			= GetArg<UINT>(argc, L"-timeout", argv, 2000);

	printf("Timeout = %d\n", data.Timeout);

	isDotNet = GetArg<bool>(argc, L"-dotnet", argv);
	if (isDotNet)
	{
		auto name = GetArg<std::wstring>(argc, L"-namespace", argv);
		if (name.empty())
		{
			printf("Target DLL is .NET but no namespace was provided.\n");

			return -1;
		}
		else
		{
			auto max = (DWORD)(sizeof(dotnet_data.szNamespace) / sizeof(wchar_t));
			if (name.length() >= max)
			{
				printf(".NET namespace name exceeds %d characters.\n", max);

				return -1;
			}

			name.copy(dotnet_data.szNamespace, name.length());
		}

		auto classname = GetArg<std::wstring>(argc, L"-class", argv);
		if (classname.empty())
		{
			printf("Target DLL is .NET but no class was provided.\n");

			return -1;
		}
		else
		{
			auto max = (DWORD)(sizeof(dotnet_data.szClassName) / sizeof(wchar_t));
			if (classname.length() >= max)
			{
				printf(".NET class name name exceeds %d characters.\n", max);

				return -1;
			}

			classname.copy(dotnet_data.szClassName, classname.length());
		}

		auto method = GetArg<std::wstring>(argc, L"-method", argv);
		if (method.empty())
		{
			printf("Target DLL is .NET but no method was provided.\n");

			return -1;
		}
		else
		{
			auto max = (DWORD)(sizeof(dotnet_data.szMethodName) / sizeof(wchar_t));
			if (method.length() >= max)
			{
				printf(".NET method name exceeds %d characters.\n", max);

				return -1;
			}

			method.copy(dotnet_data.szMethodName, method.length());
		}

		auto argument = GetArg<std::wstring>(argc, L"-argument", argv);
		if (!argument.empty())
		{
			auto max = (DWORD)(sizeof(dotnet_data.szArgument) / sizeof(wchar_t));
			if (argument.length() >= max)
			{
				printf(".NET argument exceeds %d characters.\n", max);

				return -1;
			}

			argument.copy(dotnet_data.szArgument, argument.length());
		}

		dotnet_data.Method				= data.Method;
		dotnet_data.Mode				= data.Mode;
		dotnet_data.Flags				= data.Flags;
		dotnet_data.Timeout				= data.Timeout;
		dotnet_data.GenerateErrorLog	= data.GenerateErrorLog;
		memcpy(dotnet_data.szDllPath, data.szDllPath, sizeof(data.szDllPath));
	}
	else if (data.Flags & INJ_MM_MAP_FROM_MEMORY)
	{
		fromMemory = true;

		memory_data.Method				= data.Method;
		memory_data.Mode				= data.Mode;
		memory_data.Flags				= data.Flags;
		memory_data.Timeout				= data.Timeout;
		memory_data.GenerateErrorLog	= data.GenerateErrorLog;

		std::ifstream File(data.szDllPath, std::ios::binary | std::ios::ate);
		if (!File.good())
		{
			printf("Can't open file:\n\t%ls\n", data.szDllPath);

			return -1;
		}

		memory_data.RawSize = static_cast<DWORD>(File.tellg());
		memory_data.RawData = new(std::nothrow) BYTE[memory_data.RawSize];
		if (!memory_data.RawData)
		{
			printf("Memory allocation of %08X bytes failed for:\n\t%ls\n", memory_data.RawSize, data.szDllPath);

			return -1;
		}

		File.seekg(0, std::ios::beg);
		File.read(reinterpret_cast<char *>(memory_data.RawData), memory_data.RawSize);
		File.close();
	}

	printf("Waiting for library to initialize\n");

	auto symbol_state = lib.GetSymbolState();
	while (symbol_state == INJ_ERR_SYMBOL_INIT_NOT_DONE)
	{
		Sleep(10);

		symbol_state = lib.GetSymbolState();
	}

	if (symbol_state != INJ_ERR_SUCCESS)
	{
		printf("Failed to load PDBs: 0x%08X\n", symbol_state);

		return -1;
	}

	printf("Symbol files initialized\n");
	printf("Waiting for imports to be resolved\n");

	auto import_state = lib.GetImportState();
	while (import_state == INJ_ERR_IMPORT_HANDLER_NOT_DONE)
	{
		Sleep(10);

		import_state = lib.GetImportState();
	}

	if (import_state != INJ_ERR_SUCCESS)
	{
		printf("Failed to resolve imports: 0x%08X\n", import_state);

		return -1;
	}

	printf("Imports resolved\n");
	printf("Injection library ready\n");

	auto proc_struct = ProcessData();
	if (ProcessID)
	{
		if (!proc_struct.UpdateData(ProcessID))
		{
			printf("Target process doesn't exist.\n");

			return -1;
		}
	}
	else
	{
		proc_struct.UpdateData(ProcessName);
	}

	if (!proc_struct.IsValid())
	{
		if (!GetArg<bool>(argc, L"-wait", argv))
		{
			printf("Target process doesn't exist.\n");

			return -1;
		}

		printf("Waiting for target process");

		auto current_tick = GetTickCount64();
		UINT dot_count = 0;

		while (!proc_struct.IsValid())
		{
			Sleep(50);
			proc_struct.UpdateData(ProcessName);

			if (current_tick + 500 < GetTickCount64())
			{
				current_tick = GetTickCount64();
				switch (dot_count++)
				{
					case 0:
					case 1:
					case 2:
						printf(".");
					break;

					default:
						dot_count = 0;
						printf("\b\b\b   \b\b\b");
				}
			}
		}

		printf("\n");
	}

	DWORD PID = 0;
	proc_struct.GetProcessID(PID);
	printf("Target process found (pid = %d)\n", PID);

	data.ProcessID			= PID;
	dotnet_data.ProcessID	= PID;
	memory_data.ProcessID	= PID;

	UINT delay = GetArg<UINT>(argc, L"-delay", argv);
	if (delay)
	{
		printf("Executing delay\n");
		Sleep(delay);
	}

	printf("Injecting\n");

	DWORD inj_status = INJ_ERR_SUCCESS;
	if (isDotNet)
	{
		inj_status = lib.DotNet_InjectW(&dotnet_data);
	}
	else if (fromMemory)
	{
		inj_status = lib.Memory_Inject(&memory_data);
	}
	else
	{
		inj_status = lib.InjectW(&data);
	}

	lib.Unload();
	
	if (inj_status != ERROR_SUCCESS)
	{
		printf("Injection failed with code %08X\n", inj_status);

		return -1;
	}
	else
	{
		printf("Injection succeeded. Dll laoded at ");
		if (isDotNet)
		{
			printf("%p\n", dotnet_data.hDllOut);
		}
		else if (fromMemory)
		{
			printf("%p\n", memory_data.hDllOut);
		}
		else
		{
			printf("%p\n", data.hDllOut);
		}		
	}

	return 0;
}

int FindArg(int argc, const wchar_t * arg, const wchar_t * const argv[], bool HasParameter)
{
	for (int i = 0; i < argc; ++i)
	{
		if (!lstrcmpiW(arg, argv[i]))
		{
			if (HasParameter)
			{
				if (i + 1 >= argc)
				{
					return 0;
				}
			}

			return i;
		}
	}

	return 0;
}

template <> bool GetArg<bool>(int argc, const wchar_t * arg, const wchar_t * const argv[], bool DefaultValue)
{
	auto index = FindArg(argc, arg, argv);
	if (!index)
	{
		return DefaultValue;
	}

	return true;
}

template <> DWORD GetArg<DWORD>(int argc, const wchar_t * arg, const wchar_t * const argv[], DWORD DefaultValue)
{
	auto index = FindArg(argc, arg, argv, true);
	if (!index)
	{
		return DefaultValue;
	}

	return (DWORD)std::stoul(argv[index + 1], nullptr, 0x10);
}

template <> UINT GetArg<UINT>(int argc, const wchar_t * arg, const wchar_t * const argv[], UINT DefaultValue)
{
	auto index = FindArg(argc, arg, argv, true);
	if (!index)
	{
		return DefaultValue;
	}

	return (UINT)std::stoul(argv[index + 1], nullptr, 10);
}

template <> std::wstring GetArg<std::wstring>(int argc, const wchar_t * arg, const wchar_t * const argv[], std::wstring DefaultValue)
{
	auto index = FindArg(argc, arg, argv, true);
	if (!index)
	{
		return DefaultValue;
	}

	return std::wstring(argv[index + 1]);
}

void help()
{
	//required
	printf("The following arguments are required:\n\n");

	printf("\t-f [\"path_to_dll\\dll_name.dll\"]\n");
	printf("\t\tThis argument specifies the path to dll to inject. This path can be relative.\n\n");

	//either required
	printf("Either one of the following arguments are required:\n\n");

	printf("\t-p [\"TargetProcess.exe\"]\n");
	printf("\t\tThis argument specifies the target process by name.\n\n");

	printf("\t-i [procdess identifier]\n");
	printf("\t\tThis argument specifies the target process by process id.\n\n");

	//optional
	printf("The following arguments are optional:\n\n");

	printf("\t-l [0,1,2,3]\n");
	printf("\t\tThis argument specifies the injection method:\n");
	printf("\t\t\t 0 = LoadLibraryExW (default)\n");
	printf("\t\t\t 1 = LdrLoadDll\n");
	printf("\t\t\t 2 = LdrpLoadDll\n");
	printf("\t\t\t 3 = LdrpLoadDllInternal (Windows 10+ only)\n");
	printf("\t\t\t 4 = Manual Mapping\n\n");

	printf("\t-s [0,1,2,3,4,5]\n");
	printf("\t\tThis argument specifies the launch method:\n");
	printf("\t\t\t 0 = NtCreateThreadEx (default)\n");
	printf("\t\t\t 1 = Thread hijacking\n");
	printf("\t\t\t 2 = SetWindowsHookEx\n");
	printf("\t\t\t 3 = QueueUserAPC\n");
	printf("\t\t\t 4 = KernelCallbackTable\n");
	printf("\t\t\t 5 = FakeVEH\n\n");

	printf("\t-peh [0,1,2]\n");
	printf("\t\tThis argument specifies the header option:\n");
	printf("\t\t\t 0 = Keep PE header (default)\n");
	printf("\t\t\t 1 = Erase PE header\n");
	printf("\t\t\t 2 = Fake PE header\n\n");

	printf("\t-wait\n");
	printf("\t\tIf specified the injector waits for the target process to start.\n\n");

	printf("\t-log\n");
	printf("\t\tIf specified the injector generates an error log if the injection fails.\n\n");

	printf("\t-silent\n");
	printf("\t\tIf specified the no console will be spawned and thus no debug output generated.\n\n");

	printf("\t-delay [value in ms]\n");
	printf("\t\tIf specified the injector waits for specified amount in milliseconds before the injection.\n");
	printf("\t\tThe default value is 0ms.\n\n");

	printf("\t-timeout [value in ms]\n");
	printf("\t\tThis argument specifies how long the injector waits for the shellcode and DllMain to execute.\n");
	printf("\t\tThe default value is 2000ms.\n\n");

	printf("\t-unlink\n");
	printf("\t\tIf set the injected module will be unlinked from the PEB.\n\n");

	printf("\t-cloak\n");
	printf("\t\tCreates the thread cloaked with INJ_CTF_FAKE_START_ADDRESS and INJ_CTF_HIDE_FROM_DEBUGGER. See -cloakflags for more information.\n\n");

	printf("\t-cloakflags [value (hex)]\n");
	printf("\t\tFlags that specify how the thread will be created (NtCreateThreadEx) in hexadecimal and a bitwise combination of these flags:\n");
	printf("\t\t\t0x00001000 = INJ_CTF_FAKE_START_ADDRESS\n");
	printf("\t\t\t0x00002000 = INJ_CTF_HIDE_FROM_DEBUGGER\n");
	printf("\t\t\t0x00004000 = INJ_CTF_SKIP_THREAD_ATTACH\n");
	printf("\t\t\t0x00008000 = INJ_CTF_FAKE_TEB_CLIENT_ID\n");

	printf("\t-random\n");
	printf("\t\tIf set the dll name will be scrambled before the injection.\n\n");

	printf("\t-copy\n");
	printf("\t\tIf set a copy of the dll will be loaded.\n\n");

	printf("\t-hijack\n");
	printf("\t\tIf set the injector will try to hijack a handle to the target process instead of opening a new one.\n\n");

	printf("\t-mmflags [value (hex)]\n");
	printf("\t\tFlags that specify the manual mapping options in hexadecimal and a bitwise combination of these flags:\n");
	printf("\t\t\t0x00010000 = INJ_MM_CLEAN_DATA_DIR\n");
	printf("\t\t\t0x00020000 = INJ_MM_RESOLVE_IMPORTS\n");
	printf("\t\t\t0x00040000 = INJ_MM_RESOLVE_DELAY_IMPORTS\n");
	printf("\t\t\t0x00080000 = INJ_MM_EXECUTE_TLS\n");
	printf("\t\t\t0x00100000 = INJ_MM_ENABLE_EXCEPTIONS\n");
	printf("\t\t\t0x00200000 = INJ_MM_SET_PAGE_PROTECTIONS\n");
	printf("\t\t\t0x00400000 = INJ_MM_INIT_SECURITY_COOKIE\n");
	printf("\t\t\t0x00800000 = INJ_MM_RUN_DLL_MAIN\n");
	printf("\t\t\t0x01000000 = INJ_MM_RUN_UNDER_LDR_LOCK\n");
	printf("\t\t\t0x02000000 = INJ_MM_SHIFT_MODULE_BASE\n");
	printf("\t\t\t0x04000000 = INJ_MM_MAP_FROM_MEMORY\n");
	printf("\t\tThe default value is MM_DEFAULT (0x01FE0000)\n\n");

	//optional
	printf("The following arguments are .NET related:\n\n");
	printf("\t-dotnet\n");
	printf("\t\tIf set the dll will be forwarded to the .NET injection functions.\n\n");
	printf("\t-namespace [value]\n");
	printf("\t\tUse this argument to specify the namespace of the function to be executed. This parameter is required.\n\n");
	printf("\t-class [value]\n");
	printf("\t\tUse this argument to specify the name of the class of the function to be executed. This parameter is required.\n\n");
	printf("\t-method [value]\n");
	printf("\t\tUse this argument to specify the name of the method to be executed. This parameter is required.\n\n");
	printf("\t-argument [value]\n");
	printf("\t\tUse this argument to specify the argument which will be sent to the specified function. This parameter is optional.\n\n");

	//additional
	printf("Additional commands:\n\n");

	printf("\t-help\n");
	printf("\t\tThis command lists all the commands and arguments (this).\n\n");

	printf("\t-version\n");
	printf("\t\tThis command prints the current version of the injector.\n\n");
}