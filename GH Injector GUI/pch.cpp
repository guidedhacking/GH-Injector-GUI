/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#include "pch.h"

#pragma comment (lib, "Dwmapi.lib")
#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Shell32.lib")
#pragma comment (lib, "URLMon.lib")
#pragma comment (lib, "User32.lib")
#pragma comment (lib, "WinInet.lib")

void __declspec(noreturn) THROW(std::string error_msg)
{
	MessageBoxA(NULL, "Error", error_msg.c_str(), NULL);

	throw std::exception(error_msg.c_str());
}

int strcicmpA(const char * a, const char * b)
{
	for (;; a++, b++)
	{
		int c = std::tolower(*a) - std::tolower(*b);
		if (c != 0 || !*a)
		{
			return c;
		}
	}
}

int strcicmpW(const wchar_t * a, const wchar_t * b)
{	
	for (;; a++, b++)
	{
		int c = std::towlower(*a) - std::towlower(*b);
		if (c != 0 || !*a)
		{
			return c;
		}
	}
}

void StdWStringToLower(std::wstring & String)
{
	std::transform(String.begin(), String.end(), String.begin(),
		[](wchar_t c)
		{
			return std::towlower(c);
		}
	);
}

void StdStringToLower(std::string & String)
{
	std::transform(String.begin(), String.end(), String.begin(),
		[](char c)
		{
			return (char)std::tolower((unsigned char)c);
		}
	);
}

std::wstring CharArrayToStdWString(const char * szString)
{
	if (!szString)
	{
		return std::wstring();	
	}

	std::string s(szString);
	std::vector<char> v(s.begin(), s.end());

	return std::wstring(v.begin(), v.end());
}

std::wstring StdStringToStdWString(const std::string & String)
{
	std::vector<char> v(String.begin(), String.end());

	return std::wstring(v.begin(), v.end());
}

std::string WCharArrayToStdString(const wchar_t * szString)
{
	if (!szString)
	{
		return std::string();
	}

	std::wstring s(szString);
	std::string ret;
	ret.resize(s.length());

	std::transform(s.begin(), s.end(), ret.begin(), //Stolen from https://stackoverflow.com/a/12097772
		[](wchar_t c)
		{
			return (char)(LOBYTE(c));
		}
	);

	return ret;
}

std::string StdWStringtoStdString(const std::wstring & String)
{
	std::string ret;
	ret.resize(String.length());

	std::transform(String.begin(), String.end(), ret.begin(), //Stolen from https://stackoverflow.com/a/12097772
		[](wchar_t c)
		{
			return (char)(LOBYTE(c));
		}
	);

	return ret;
}