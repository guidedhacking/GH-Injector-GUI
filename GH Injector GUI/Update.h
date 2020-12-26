#pragma once

#include "GuiMain.h"

std::wstring get_newest_version();
bool update_injector(std::wstring newest_version, bool & ignore);