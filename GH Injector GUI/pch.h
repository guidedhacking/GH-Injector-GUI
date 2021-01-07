#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <algorithm>
#include <codecvt>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <functional>
#include <guiddef.h>
#include <iostream>
#include <Shlobj.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <sstream>
#include <string>
#include <thread>
#include <TlHelp32.h>
#include <Urlmon.h>
#include <vector>
#include <Windows.h>
#include <WinInet.h>

#pragma warning(disable: 6011)  //qt not checking pointers I guess
#pragma warning(disable: 26451) //qt overflow
#pragma warning(disable: 26495) //qt uninitialized membervariables
#pragma warning(disable: 26498) //qt constexpr and compiler optimizations
#pragma warning(disable: 26812) //qt enums are not enum class

#include <QApplication>
#include <QtWidgets>

#pragma warning(default: 6011)
#pragma warning(default: 26451)
#pragma warning(default: 26495)
#pragma warning(default: 26498)
#pragma warning(default: 26812)