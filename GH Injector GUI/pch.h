/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include <Windows.h>

#if (NTDDI_VERSION < NTDDI_WIN7)
#error The mininum requirement for this GUI is Windows 7.
#endif

#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <guiddef.h>
#include <iostream>
#include <objidl.h> 
#include <Psapi.h>
#include <Shlobj.h>
#include <ShlGuid.h>
#include <ShObjIdl.h>
#include <string>
#include <thread>
#include <TlHelp32.h>
#include <urlmon.h>
#include <vector>
#include <wininet.h>

#pragma warning(push)

#pragma warning(disable: 5054)  //qt doing bad stuff with enums
#pragma warning(disable: 6011)  //qt not checking pointers I guess
#pragma warning(disable: 26451) //qt overflow
#pragma warning(disable: 26495) //qt uninitialized membervariables
#pragma warning(disable: 26498) //qt constexpr and compiler optimizations
#pragma warning(disable: 26812) //qt enums are not enum class

#include <QApplication>
#include <QtWidgets>

#pragma warning(pop)

#if (QT_VERSION_CHECK(5,15,8) < QT_VERSION)
#error Invalid Qt version. Only Qt5.15.x is supported.
#endif

#if (QT_VERSION_CHECK(5,15,0) > QT_VERSION)
#error Invalid Qt version. Only Qt5.15.x is supported.
#endif

void __declspec(noreturn) THROW(std::string error_msg);

template <typename T>
void SAFE_DELETE(T * & t)
{
	if (t)
	{
		delete t;
		t = nullptr;
	}
}

inline bool			g_IsNative	= false;
inline ULONG		g_SessionID = 0;
inline std::wstring g_RootPath;

int strcicmpA(const char * a, const char * b);
int strcicmpW(const wchar_t * a, const wchar_t * b);

void StdWStringToLower(std::wstring & String);
void StdStringToLower(std::string & String);

std::wstring CharArrayToStdWString(const char * szString);
std::wstring StdStringToStdWString(const std::string & String);

std::string	WCharArrayToStdString(const wchar_t * szString);
std::string StdWStringtoStdString(const std::wstring & String);

//#define DEBUG_CONSOLE_TO_CMD