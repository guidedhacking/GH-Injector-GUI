#pragma once

#include "pch.h"

#include "DebugConsole.h"
#include "DownloadProgress.h"
#include "DownloadProgressWindow.h"
#include "Process.h"
#include "StatusBox.h"
#include "Zip.h"

std::wstring get_newest_version();
bool update_injector(const std::wstring & newest_version, bool & ignore, InjectionLib * Lib);

#define UPDATE_ERR_SUCCESS				0
#define UPDATE_ERR_DOWNLOAD_FAIL		-1
#define UPDATE_ERR_DELETE_FAIL			-2
#define UPDATE_ERR_RENAME_FAIL			-3
#define UPDATE_ERR_UNZIP_FAIL			-4
#define UPDATE_ERR_INCOMPLETE			-5
#define UPDATE_ERR_LAUNCH_FAIL			-6
#define UPDATE_ERR_INTERRUPTED			-7