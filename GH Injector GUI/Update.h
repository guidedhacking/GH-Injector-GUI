#pragma once

#include "pch.h"

#include "DebugConsole.h"
#include "DownloadProgress.h"
#include "DownloadProgressWindow.h"
#include "StatusBox.h"
#include "Zip.h"

std::wstring get_newest_version();
bool update_injector(std::wstring newest_version, bool & ignore);