#include "pch.h"

#include "Zip.h"

int Unzip(const wchar_t * szSrcPath, const wchar_t * szDstPath)
{
	wchar_t szCommand[MAX_PATH * 3];
	swprintf_s(szCommand, sizeof(szCommand) / sizeof(szCommand[0]), L"\"Powershell.exe Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", szSrcPath, szDstPath);

	return _wsystem(szCommand);
}