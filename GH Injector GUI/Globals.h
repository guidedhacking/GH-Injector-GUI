#pragma once

constexpr auto GH_INJECTOR_ZIP = "GH Injector.zip";
constexpr auto GH_INJECTOR_FOLDER = "GH_Injector\\";
constexpr auto GH_INJECTOR_EXE_X86 = "GH_Injector_x86.exe";
constexpr auto GH_INJECTOR_EXE_X64 = "GH_Injector_x64.exe";

constexpr auto GH_INJECTOR_SM_X86 = "GH Injector SM - x86.exe";
constexpr auto GH_INJECTOR_SM_X64 = "GH Injector SM - x64.exe";

#define GH_INJ_EXE_NAME64A "GH Injector - x64.exe"
#define GH_INJ_EXE_NAME86A "GH Injector - x86.exe"


constexpr auto GH_HELP_URL = "https://guidedhacking.com/resources/guided-hacking-dll-injector.4/";
constexpr auto GH_LOG_URL = "https://pastebin.com/eN7KPX3x";


#ifdef _DEBUG
constexpr auto GH_DOWNLOAD_PREFIX = "http://nas:80/V";
constexpr auto GH_DOWNLOAD_SUFFIX = "/GH Injector.zip";
constexpr auto GH_VERSION_URL = "https://guidedhacking.com/gh/inj/";
#else
constexpr auto GH_DOWNLOAD_PREFIX = "https://guidedhacking.com/gh/inj/V";
constexpr auto GH_DOWNLOAD_SUFFIX = "/GH Injector.zip";
constexpr auto GH_VERSION_URL = "https://guidedhacking.com/gh/inj/";
#endif
