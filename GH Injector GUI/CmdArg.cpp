#include <algorithm>
#include <string>
#include <vector>

#include "CmdArg.hpp"

#include <thread>

#include "Injection.h"
#include "Process.h"
#include "cxxopts.hpp"
#include "InjectionLib.hpp"

#define INJ_KEEP_HEADER 0x0000



int CmdArg(int argc, char* argv[])
{
	int orig_argc = argc;
	
	if (orig_argc < 2)
		return  err::none;
	
	cxxopts::Options options("injector.exe -p csgo.exe -f mogeln.dll", "A brief description");

	options.add_options()
		("p,pid", "Process name or PID", cxxopts::value<std::string>())
		("f,file", "File path", cxxopts::value<std::string>())
		("d,delay", "Delay [ms]", cxxopts::value<int >()->default_value("0"))
		("w,wait", "Wait for process", cxxopts::value<bool>()->default_value("false"))

		("l,load", "Load method [ loadlib | ldr | ldrp | manual]", cxxopts::value<std::string>()->default_value("load"))
		("s,start", "Launch method [ create | hijack | hook | apc ]", cxxopts::value<std::string>()->default_value("create"))
		("j,hijack", "Hijack handle", cxxopts::value<bool>()->default_value("false"))
		("o,cloak", "Cloak thread", cxxopts::value<bool>()->default_value("false"))

		("e,peh", "PEH [ keep | erase | fake ]", cxxopts::value<std::string>()->default_value("keep"))
		("r,randomize", "Randomize file name", cxxopts::value<bool>()->default_value("false"))
		("u,unlink", "Unlink from PEB", cxxopts::value<bool>()->default_value("false"))
		("c,copy", "Load DLL copy", cxxopts::value<bool>()->default_value("false"))
		("m,mapping", "Manual mapping flags (MM_DEFAULT)", cxxopts::value<int>()->default_value("0x01fc0000"))
	
		("v,version", "Print version", cxxopts::value<bool>()->default_value("false"))
		("y,style", "Performance style", cxxopts::value<bool>()->default_value("false"))
		("h,help", "Print usage")
		;

	auto result = options.parse(argc, argv);

	if (result.count("style"))
		return none_performance;

	
//#ifdef _DEBUG
	int iAttach = AttachConsole(ATTACH_PARENT_PROCESS);
	int iAttachErr = GetLastError();
	if(iAttachErr == ERROR_INVALID_HANDLE)
	{
		int iAlloc = AllocConsole();
		int iAllocErr = GetLastError();
		if(!iAlloc)
		{
			return no_console_attach;
		}
	}
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
	//std::cout << iAttach << iAttachErr << std::endl;
//#endif
	
	
	if (result.count("version"))
	{
		std::cout << GH_INJ_VERSIONA << std::endl;
		return err::version;
	}
	
	if(result.count("help") || orig_argc == 2)
	{
		std::cout << options.help() << std::endl;
		return err::help;
	}

	InjectionLib injectionLib;
	bool bLib = injectionLib.Init();
	if(bLib == false)

	//HINSTANCE hInjectionMod = LoadLibrary(GH_INJ_MOD_NAME);
	//if (hInjectionMod == nullptr)
	{
		std::cout <<  "dll not found" << std::endl;
		return err::no_lib;
	}

	
	//auto InjectA = (f_InjectA)GetProcAddress(hInjectionMod, "InjectA");
	//if (InjectA == nullptr)
	//{
	//	std::cout << "InjectA " << " not found" << std::endl;
	//	return err::no_func;
	//}

	INJECTIONDATAA data{ 0 };

	
	bool bDebug = SetDebugPrivilege(true);
	
	bool bPlatform = isCorrectPlatform();

	
	// Dll
	if (!result.count("file"))
	{
		std::cout << "File arg missing" << std::endl;
		return no_lib_arg;
	}
	
	std::string dll = result["file"].as<std::string>();	
	const ARCH fileArch = getFileArchA(dll.c_str());
	if(fileArch == ARCH::NONE)
	{
		std::cout << "File not found" << std::endl;
		return no_file;
	}

	const int fileLength = GetFullPathNameA(dll.c_str(), MAX_PATH * 2, data.szDllPath, nullptr);
	if (fileLength == 0)
	{
		std::cout << "Full path not found" << std::endl;
		return err::file_path;
	}

	
	// process
	if (!result.count("pid"))
	{
		std::cout << "Process arg missing" << std::endl;
		return no_process_arg;
	}

	std::string proc = result["pid"].as<std::string>();
	Process_Struct procStruct{ 0 };


	do
	{
		if (is_number(proc))
			procStruct = getProcessByPID(std::atoi(proc.c_str()));
		else
			procStruct = getProcessByName(proc.c_str());

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		
	} while (procStruct.pid == 0 && result.count("wait"));
	

	
	if (procStruct.arch == NONE)
	{
		std::cout << "Process not found" << std::endl;
		return err::no_process;
	}

	// delay
	if (result.count("delay"))
		std::this_thread::sleep_for(std::chrono::milliseconds(result["delay"].as<int>()));
		

	if (procStruct.arch != fileArch)
	{
		std::cout << "File and process have different architecture" << std::endl;
		return err::different_arch;
	}
	
	data.ProcessID = procStruct.pid;

	
	// inject
	if (result.count("load"))
		data.Mode = getInjMode(result["load"].as<std::string>());

	
	// launch
	if (result.count("start"))
		data.Method = getLaunchMethod(result["start"].as<std::string>());


	if (result.count("hijack")) data.Flags |= INJ_HIJACK_HANDLE;
	if (result.count("cloak"))  data.Flags |= INJ_THREAD_CREATE_CLOAKED;

	
	if (result.count("peh"))
	{
		
		std::string peh = result["peh"].as<std::string>();
		
		if (peh == "erase")
			data.Flags |= INJ_ERASE_HEADER;
		else if (peh == "fake")
			data.Flags |= INJ_FAKE_HEADER;
		else
			data.Flags |= INJ_KEEP_HEADER;
	}

	
	if (result.count("randomize")) data.Flags |= INJ_SCRAMBLE_DLL_NAME;
	if (result.count("unlink"))    data.Flags |= INJ_UNLINK_FROM_PEB;
	if (result.count("copy"))      data.Flags |= INJ_LOAD_DLL_COPY;

	
	// flags
	if (result.count("manual"))
		data.Flags |= result["manual"].as<int>();
	else
		if (data.Mode == INJECTION_MODE::IM_ManualMap)
			data.Flags |= MM_DEFAULT;

	
	data.GenerateErrorLog = true;

	data.Timeout = 2000;

	
	int iInject = injectionLib.InjectFuncA(&data);
	//int iInject = InjectA(&data);
	if (iInject != 0)
	{
		std::cout << "InjectA failed with " << iInject << std::endl;
		return err::inject_fail;
	}
	
	int i = 42;
	std::cout << "Success" << std::endl;
	return  ok;
}


bool is_number(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

INJECTION_MODE getInjMode(std::string str)
{
	for (auto c : str)
		std::transform(str.begin(), str.end(), str.begin(), 
			[](unsigned char c) {return std::tolower(c); });

	
	if (str == "loadlib")
		return  INJECTION_MODE::IM_LoadLibraryExW;

	
	if (str == "ldr")
		return INJECTION_MODE::IM_LdrLoadDll;

	
	if (str == "ldrp")
		return INJECTION_MODE::IM_LdrpLoadDll;

	
	if (str == "manual")
		return  INJECTION_MODE::IM_LdrLoadDll;

	
	return INJECTION_MODE::IM_LoadLibraryExW;	
}


LAUNCH_METHOD getLaunchMethod(std::string str)
{
	for (auto c : str)
		std::transform(str.begin(), str.end(), str.begin(),
			[](unsigned char c) {return std::tolower(c); });


	if (str == "create")
		return  LAUNCH_METHOD::LM_NtCreateThreadEx;


	if (str == "hijack")
		return LAUNCH_METHOD::LM_HijackThread;


	if (str == "hook")
		return LAUNCH_METHOD::LM_SetWindowsHookEx;


	if (str == "apc")
		return  LAUNCH_METHOD::LM_QueueUserAPC;


	return LAUNCH_METHOD::LM_NtCreateThreadEx;
}
