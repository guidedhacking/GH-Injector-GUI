/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

#include "DownloadProgressWindow.h"
#include "StatusBox.h"

void ShowPDBDownload(InjectionLib * InjLib);

#define PDB_ERR_SUCCESS				0
#define PEB_ERR_NO_INTERNET			-1
#define PEB_ERR_CONNECTION_BLOCKED	-2
#define PEB_ERR_INTERRUPTED			-3