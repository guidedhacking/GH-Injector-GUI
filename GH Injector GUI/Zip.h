#pragma once
#include <Windows.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include "Globals.h"


int RemoveFile(const char* szSrcPath);
int RemoveFolder(const char* szSrcPath);

int Zip(const char* szSrcPath, const char* szDstPath);
int Unzip(const char* szSrcPath, const char* szDstPath);

int GetZipFilePath(char* szPath, int size);
int GetZipFolderPath(char* szPath, int size);
int GetZipFolderExePath(char* szPath, int size);

void replaceAll(std::string& str, const std::string& from, const std::string& to);