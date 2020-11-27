#pragma once

#include <Windows.h>
#include "GuiMain.h"

HWND CreateDragDropWindow(HWND hMainWnd, GuiMain * pGui);
void CloseDragDropWindow();