#include "pch.h"

#pragma comment (lib, "Dwmapi.lib")
#pragma comment (lib, "Shell32.lib")
#pragma comment (lib, "URLMon.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "WinInet.lib")

void __declspec(noreturn) THROW(std::string error_msg)
{
	MessageBoxA(NULL, "Error", error_msg.c_str(), NULL);

	throw std::exception(error_msg.c_str());
}