#include "pch.h"

#include "Zip.h"

int RemoveFile(const char * szSrcPath)
{
	size_t numConverted;
	wchar_t wszSrcPath[MAX_PATH]{ 0 };

	mbstowcs_s(&numConverted, wszSrcPath, sizeof(wszSrcPath) / sizeof(wchar_t),
		szSrcPath, strlen(szSrcPath));

	return std::filesystem::remove(std::filesystem::path(wszSrcPath));
}

int RemoveFolder(const char * szSrcPath)
{
	size_t numConverted;
	wchar_t wszSrcPath[MAX_PATH];

	mbstowcs_s(&numConverted, wszSrcPath, sizeof(wszSrcPath) / sizeof(wchar_t),
		szSrcPath, strlen(szSrcPath));

	return std::filesystem::remove_all(std::filesystem::path(wszSrcPath));
}

int Zip(const char * szSrcPath, const char * szDstPath)
{
	char szCommand[MAX_PATH];
	sprintf_s(szCommand, sizeof(szCommand),
		"Powershell.exe Compress-Archive -DestinationPath '%s' -Force -Path '%s'", szDstPath, szSrcPath);

	return system(szCommand);
}

int Unzip(const char * szSrcPath, const char * szDstPath)
{
	char szCommand[MAX_PATH * 10];
	sprintf_s(szCommand, sizeof(szCommand),
		"\"Powershell.exe Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"", szSrcPath, szDstPath);

	return system(szCommand);
}

int GetZipFilePath(char * szPath, int size)
{
	GetCurrentDirectoryA(size, szPath);
	strcat_s(szPath, size, "\\");
	return strcat_s(szPath, size, GH_INJECTOR_ZIP);
}

int GetZipFolderPath(char * szPath, int size)
{
	GetCurrentDirectoryA(size, szPath);
	strcat_s(szPath, size, "\\");
	return strcat_s(szPath, size, GH_INJECTOR_FOLDER);
}

int GetZipFolderExePath(char * szPath, int size)
{
	GetZipFolderPath(szPath, size);
	return strcat_s(szPath, size, GH_INJECTOR_EXE_X64);
}

void replaceAll(std::string & str, const std::string & from, const std::string & to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

void renameFile(const char * szSrcPath, const char * szDesPath)
{
	size_t numConverted;
	wchar_t wszSrcPath[MAX_PATH]{ 0 };
	wchar_t wszDesPath[MAX_PATH]{ 0 };

	mbstowcs_s(&numConverted, wszSrcPath, sizeof(wszSrcPath) / sizeof(wchar_t),
		szSrcPath, strlen(szSrcPath));

	mbstowcs_s(&numConverted, wszDesPath, sizeof(wszDesPath) / sizeof(wchar_t),
		szDesPath, strlen(szDesPath));

	return std::filesystem::rename(wszSrcPath, wszDesPath);
}