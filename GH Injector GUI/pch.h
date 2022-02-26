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
#include <objidl.h> 
#include <Shlobj.h>
#include <ShlGuid.h>
#include <ShObjIdl.h>
#include <sstream>
#include <string>
#include <thread>
#include <TlHelp32.h>
#include <urlmon.h>
#include <vector>
#include <Windows.h>
#include <wininet.h>

#pragma warning(disable: 26812) //qt enums are not enum class

#pragma warning(push)

#pragma warning(disable: 5054)  //qt doing bad stuff with enums
#pragma warning(disable: 6011)  //qt not checking pointers I guess
#pragma warning(disable: 26451) //qt overflow
#pragma warning(disable: 26495) //qt uninitialized membervariables
#pragma warning(disable: 26498) //qt constexpr and compiler optimizations

#include <QApplication>
#include <QtWidgets>

#pragma warning(pop)

#if (QT_VERSION_CHECK(5,15,8) < QT_VERSION)
#error Invalid Qt version. Only Qt5.15.x is supported.
#endif

#if (QT_VERSION_CHECK(5,15,0) > QT_VERSION)
#error Invalid Qt version. Only Qt5.15.x is supported.
#endif

void THROW(std::string error_msg);

template <typename T>
void SAFE_DELETE(T * & t)
{
	if (t)
	{
		delete t;
		t = nullptr;
	}
}