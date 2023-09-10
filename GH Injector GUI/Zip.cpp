/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#include "pch.h"

#include "Zip.h"

int Unzip(const std::wstring & SrcPath, const std::wstring & DestPath)
{
	std::wstring command = L"\"Powershell.exe Expand-Archive -Path '";
	command += SrcPath;
	command += L"' -DestinationPath '";
	command += DestPath;
	command += L"' -Force\"";

	return _wsystem(command.c_str());
}