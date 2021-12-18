#include "pch.h"

#include "CmdArg.h"

int FindArg(int argc, const wchar_t * arg, wchar_t * argv[], bool Parameter = false);
void help();

int CmdArg(int argc, wchar_t * argv[])
{
	//Console output is buggy for non ASCII characters, works fine internally
	
	AllocConsole();
	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);

	if (!SetDebugPrivilege(true))
	{
		printf("Failed to enable debug privileges. This might affect the functionality of the injector.\n");
	}

	if (argc < 2)
	{
		printf("Invalid argument count.\n");

		help();

		return -1;
	}

	if (FindArg(argc, L"-help", argv))
	{
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

	if (FindArg(argc, L"-version", argv))
	{
		printf("GH Injector library Version = V%s\n", lib.GetVersionA().c_str());
		printf("GH Injector GUI Version = V%s\n", GH_INJ_GUI_VERSIONA);

		return -1;
	}

	const wchar_t * szProcessName = nullptr;
	int process_index = FindArg(argc, L"-p", argv, true);
	if (!process_index)
	{
		printf("No target process specified.\n");

		return -1;
	}
	else
	{
		szProcessName = argv[process_index + 1];

		printf("target process = %ls\n", szProcessName);
	}

	std::wcout << szProcessName << std::endl;

	const wchar_t * szDllName = nullptr;
	int dll_index = FindArg(argc, L"-f", argv, true);
	if (!dll_index)
	{
		printf("No dll to inject specified.\n");

		return -1;
	}
	else
	{
		szDllName = argv[dll_index + 1];

		if (!FileExistsW(szDllName))
		{
			printf("Specified dll file doesn't exist.\n");

			return -1;
		}

		printf("dll = %ls\n", szDllName);
	}

	auto proc_struct = getProcessByNameW(szProcessName);
	if (!proc_struct.PID)
	{
		if (!FindArg(argc, L"-wait", argv))
		{
			printf("Target process doesn't exist.\n");

			return -1;
		}

		printf("Waiting for target process.\n");

		while (!proc_struct.PID)
		{
			Sleep(50);
			proc_struct = getProcessByNameW(szProcessName);
		}
	}

	printf("Target process found (pid = %d)\n", proc_struct.PID);

	INJECTIONDATAW data{ 0 };
	data.ProcessID = proc_struct.PID;
	lstrcpyW(data.szDllPath, szDllName);

	int injection_mode = 0;
	int inj_mode_index = FindArg(argc, L"-l", argv, true);
	if (inj_mode_index)
	{
		injection_mode = std::stoi(argv[inj_mode_index + 1]);
		if (injection_mode < 0 || injection_mode > 4)
		{
			injection_mode = 0;
		}
	}
	data.Mode = (INJECTION_MODE)injection_mode;
	printf("Injection mode = %d\n", injection_mode);

	int launch_method = 0;
	int inj_method_index = FindArg(argc, L"-s", argv, true);
	if (inj_method_index)
	{
		launch_method = std::stoi(argv[inj_method_index + 1]);
		if (launch_method < 0 || launch_method > 5)
		{
			launch_method = 0;
		}
	}
	data.Method = (LAUNCH_METHOD)launch_method;
	printf("Launch method = %d\n", launch_method);

	DWORD Flags = 0;

	int peh_index = FindArg(argc, L"-peh", argv, true);
	if (peh_index)
	{
		int peh_option = std::stoi(argv[peh_index + 1]);

		switch (peh_option)
		{
		case 1: Flags |= INJ_ERASE_HEADER; break;
		case 2: Flags |= INJ_FAKE_HEADER; break;
		default: break;
		}
	}

	if (FindArg(argc, L"-unlink", argv))
	{
		Flags |= INJ_UNLINK_FROM_PEB;
	}

	if (FindArg(argc, L"-random", argv))
	{
		Flags |= INJ_SCRAMBLE_DLL_NAME;
	}

	if (FindArg(argc, L"-copy", argv))
	{
		Flags |= INJ_LOAD_DLL_COPY;
	}

	if (FindArg(argc, L"-hijack", argv))
	{
		Flags |= INJ_HIJACK_HANDLE;
	}

	if (data.Method == LAUNCH_METHOD::LM_NtCreateThreadEx)
	{
		if (FindArg(argc, L"-cloak", argv))
		{
			Flags |= INJ_THREAD_CREATE_CLOAKED;
		}

		DWORD cflags = 0;
		int cflags_index = FindArg(argc, L"-cloakflags", argv, true);
		if (cflags_index)
		{
			wchar_t * szMMflags = argv[cflags_index + 1];
			cflags = std::stol(szMMflags, nullptr, 0x10);

			DWORD cflags_mask = INJ_CTF_FAKE_START_ADDRESS | INJ_CTF_HIDE_FROM_DEBUGGER | INJ_CTF_SKIP_THREAD_ATTACH | INJ_CTF_FAKE_TEB_CLIENT_ID;
			cflags &= cflags_mask;
		}

		Flags |= cflags;
	}

	if (data.Mode == INJECTION_MODE::IM_ManualMap)
	{
		DWORD mmflags = MM_DEFAULT;

		int mmflags_index = FindArg(argc, L"-mmflags", argv, true);
		if (mmflags_index)
		{
			wchar_t * szMMflags = argv[mmflags_index + 1];
			mmflags = std::stol(szMMflags, nullptr, 0x10);

			DWORD mmflags_mask = MM_DEFAULT | INJ_MM_CLEAN_DATA_DIR | INJ_MM_SHIFT_MODULE_BASE;
			mmflags &= mmflags_mask;
		}

		Flags |= mmflags;
	}

	data.Flags = Flags;

	printf("Flags = %08X\n", Flags);

	if (FindArg(argc, L"-log", argv))
	{
		data.GenerateErrorLog = true;
	}

	int timeout_index = FindArg(argc, L"-timeout", argv, true);
	if (timeout_index)
	{
		data.Timeout = std::stoi(argv[timeout_index + 1]);
	}
	else
	{
		data.Timeout = 2000;
	}

	printf("Timeout = %d\n", data.Timeout);

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

	int delay = 0;
	int delay_index = FindArg(argc, L"-delay", argv, true);
	if (delay_index)
	{
		delay = std::stoi(argv[delay_index + 1]);
		printf("Delay = %d\n", delay);
	}

	if (delay)
	{
		printf("Executing delay\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	printf("Injecting\n");

	DWORD inj_status = lib.InjectW(&data);

	lib.Unload();
	
	if (inj_status != ERROR_SUCCESS)
	{
		printf("Injection failed with code %08X\n", inj_status);

		return -1;
	}
	else
	{
		printf("Injection succeeded. Dll laoded at %p\n", data.hDllOut);
	}

	return 0;
}

int FindArg(int argc, const wchar_t * arg, wchar_t * argv[], bool HasParameter)
{
	for (int i = 0; i < argc; ++i)
	{
		if (!lstrcmpiW(arg, argv[i]))
		{
			if (HasParameter)
			{
				if (i + i == argc)
				{
					return 0;
				}
			}

			return i;
		}
	}

	return 0;
}

void help()
{
	//required
	printf("The following arguments are required:\n\n");

	printf("\t-p [\"TargetProcess.exe\"]\n");
	printf("\t\tThis argument specifies the target process by name.\n\n");

	printf("\t-f [\"path_to_dll\\dll_name.dll\"]\n");
	printf("\t\tThis argument specifies the path to dll to inject.\n\n");

	//optional
	printf("The following arguments are optional:\n\n");

	printf("\t-l [0,1,2,3]\n");
	printf("\t\tThis argument specifies the injection method:\n");
	printf("\t\t\t 0 = LoadLibraryExW (default)\n");
	printf("\t\t\t 1 = LdrLoadDll\n");
	printf("\t\t\t 2 = LdrpLoadDll\n");
	printf("\t\t\t 3 = LdrpLoadDllInternal\n");
	printf("\t\t\t 4 = Manual Mapping\n\n");

	printf("\t-s [0,1,2,3]\n");
	printf("\t\tThis argument specifies the launch method:\n");
	printf("\t\t\t 0 = NtCreateThreadEx (default)\n");
	printf("\t\t\t 1 = Thread hijacking\n");
	printf("\t\t\t 2 = SetWindowsHookEx\n");
	printf("\t\t\t 3 = QueueUserAPC\n\n");

	printf("\t-peh [0,1,2]\n");
	printf("\t\tThis argument specifies the header option:\n");
	printf("\t\t\t 0 = Keep PE header (default)\n");
	printf("\t\t\t 1 = Erase PE header\n");
	printf("\t\t\t 2 = Fake PE header\n\n");

	printf("\t-wait\n");
	printf("\t\tIf specified the injector waits for the target process to start.\n\n");

	printf("\t-log\n");
	printf("\t\tIf specified the injector generates an error log if the injection fails.\n\n");

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
	printf("\t\tThe default is MM_DEFAULT (0x01FE0000)\n\n");

	//additional
	printf("Additional commands:\n\n");

	printf("\t-help\n");
	printf("\t\tThis command lists all the commands and arguments (this).\n\n");

	printf("\t-version\n");
	printf("\t\tThis command prints the current version of the injector.\n\n");
}