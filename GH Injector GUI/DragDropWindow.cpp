#include "DragDropWindow.h"
#include "resource.h"

#include <iostream>

HWND g_hMainWnd		= NULL;
HWND g_hDropWnd		= NULL;
GuiMain * g_GuiMain = NULL;

WNDPROC g_MainWndProc = nullptr;

bool g_Active	= false;
bool g_Hide		= false;

WINDOWPOS g_WinPosOld{ 0 };
WINDOWPOS g_WinPosNew{ 0 };

HANDLE g_hMsgThread		= nullptr;
HANDLE g_hUpdateThread	= nullptr;
HANDLE g_hUpdateNow		= nullptr;

HICON	g_hIcon		= NULL;
HDC		g_hWndDC	= NULL;

const wchar_t g_szClassName[] = L"DragnDropWindow";

void HandleDrops(HDROP hDrop)
{
	if (!hDrop)
	{
		return;
	}

	auto DropCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (!DropCount)
	{
		return;
	}

	wchar_t ** Drops = new wchar_t*[DropCount]();
	for (UINT i = 0; i < DropCount; ++i)
	{
		auto BufferSize = DragQueryFile(hDrop, i, nullptr, 0);

		if (!BufferSize)
		{
			continue;
		}

		BufferSize++;

		Drops[i] = new wchar_t[(UINT_PTR)BufferSize]();

		DragQueryFile(hDrop, i, Drops[i], BufferSize);

		QString qDrop = QString::fromWCharArray(Drops[i]);
		g_GuiMain->add_file_to_list(qDrop, false);

		delete[] Drops[i];
	}

	delete[] Drops;
}

LRESULT CALLBACK LocalWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DROPFILES:
			HandleDrops(reinterpret_cast<HDROP>(wParam));
			break;

		case WM_PAINT:
			DrawIcon(g_hWndDC, 0, 0, g_hIcon);
			break;

		case WM_ACTIVATE:
			if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
			{
				SetWindowPos(g_hMainWnd, g_hDropWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			}
			break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD __stdcall UpdateDragDropWnd_Thread(void * pParam)
{
	UNREFERENCED_PARAMETER(pParam);

	while (true)
	{
		WaitForSingleObject(g_hUpdateNow, INFINITE);
		
		if (g_Hide)
		{
			SetWindowPos(g_hDropWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW);
		}
		else
		{
			float scaling = g_WinPosNew.cx / 1250.0f;
			int y_offset = 200 * scaling;
			int x_offset = 85 * scaling;

			if (g_Active)
			{
				SetWindowPos(g_hDropWnd, HWND_TOPMOST, g_WinPosNew.x + g_WinPosNew.cx - x_offset, g_WinPosNew.y + g_WinPosNew.cy - y_offset, 30, 30, SWP_SHOWWINDOW | SWP_NOACTIVATE);

				g_WinPosOld = g_WinPosNew;
			}
			else
			{					
				SetWindowPos(g_hDropWnd, g_hMainWnd, g_WinPosNew.x + g_WinPosNew.cx - x_offset, g_WinPosNew.y + g_WinPosNew.cy - y_offset, 30, 30, SWP_SHOWWINDOW | SWP_NOACTIVATE);
				SetWindowPos(g_hMainWnd, g_hDropWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

				g_WinPosOld = g_WinPosNew;
			}
		}

		ResetEvent(g_hUpdateNow);
	}
}

LRESULT CALLBACK Proxy_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{	
	if (uMsg == WM_SIZE)
	{
		if (wParam == SIZE_MINIMIZED)
		{
			g_Hide = true;
		}
		else if (wParam == SIZE_RESTORED)
		{
			g_Active	= true;
			g_Hide		= false;
		}

		SetEvent(g_hUpdateNow);
	}
	else if (uMsg == WM_WINDOWPOSCHANGED)
	{
		auto * pos = reinterpret_cast<WINDOWPOS*>(lParam);
		if (g_WinPosOld.cx != pos->cx || g_WinPosOld.cy != pos->cy || g_WinPosOld.x != pos->x || g_WinPosOld.y != pos->y)
		{
			g_Active	= true;
			g_Hide		= false;

			g_WinPosNew = *pos;

			SetEvent(g_hUpdateNow);
		}
	}
	else if (uMsg == WM_ACTIVATE)
	{
		if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
		{
			g_Active	= true;
			g_Hide		= false;
			SetEvent(g_hUpdateNow);
		}
		else if (wParam == WA_INACTIVE && (HWND)lParam != g_hDropWnd)
		{
			g_Active	= false;
			g_Hide		= false;
			SetEvent(g_hUpdateNow);
		}
	}
	else if (uMsg == WM_DISPLAYCHANGE)
	{
		g_GuiMain->slotReboot();
	}

	return g_MainWndProc(hWnd, uMsg, wParam, lParam);
}

DWORD __stdcall MsgLoop(HWND hWnd)
{
	MSG Msg{ 0 };

	while (GetMessage(&Msg, hWnd, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

HWND CreateDragDropWindow(HWND hMainWnd, GuiMain * pGui)
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(WNDCLASSEX); 

	wc.style			= 0;
	wc.lpfnWndProc		= LocalWndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName		= nullptr;
	wc.lpszClassName	= g_szClassName;
	wc.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		printf("RegisterClassEx failed: %08X\n", GetLastError());

		return 0;
	}
	
	g_hMainWnd	= hMainWnd;
	g_GuiMain	= pGui;

	g_MainWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hMainWnd, GWLP_WNDPROC));
	if (!g_MainWndProc)
	{
		printf("GetWindowLongPtr failed: %08X\n", GetLastError());

		return 0;
	}

	if (!SetWindowLongPtr(hMainWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Proxy_WndProc)))
	{
		printf("SetWindowLongPtr failed: %08X\n", GetLastError());

		return 0;
	}

	g_hUpdateNow = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (!g_hUpdateNow)
	{
		printf("CreateEvent failed: %08X\n", GetLastError());

		return 0;
	}

	HWND hWnd = CreateWindowExW(WS_EX_ACCEPTFILES | WS_EX_TOOLWINDOW, g_szClassName, nullptr, WS_BORDER | WS_POPUP, -1000000, -1000000, 30, 30, NULL, NULL, hInstance, nullptr);
	if (hWnd == NULL)
	{
		printf("CreateWindowExW failed: %08X\n", GetLastError());

		return 0;
	}

	g_hDropWnd = hWnd;

	g_hIcon = (HICON)LoadImage(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 40, 40, LR_LOADTRANSPARENT);
	if (!g_hIcon)
	{
		printf("LoadImage failed: %08X\n", GetLastError());

		return 0;
	}

	g_hWndDC = GetDC(hWnd);
	if (!g_hWndDC)
	{
		printf("GetDC failed: %08X\n", GetLastError());

		return 0;
	}

	if (!DrawIcon(g_hWndDC, 0, 0, g_hIcon))
	{
		printf("DrawIcon failed: %08X\n", GetLastError());
	}

	ChangeWindowMessageFilterEx(hWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(hWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(hWnd, 0x0049, MSGFLT_ALLOW, nullptr);

	g_hMsgThread	= CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MsgLoop, (void*)hWnd, NULL, nullptr);
	g_hUpdateThread	= CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)UpdateDragDropWnd_Thread, nullptr, NULL, nullptr);

	return hWnd;
}

void CloseDragDropWindow()
{
	ReleaseDC(g_hDropWnd, g_hWndDC);
	DestroyIcon(g_hIcon);

	DestroyWindow(g_hDropWnd);
	
	TerminateThread(g_hMsgThread, 0);
	TerminateThread(g_hUpdateThread, 0);

	CloseHandle(g_hMsgThread);
	CloseHandle(g_hUpdateThread);
	CloseHandle(g_hUpdateNow);

	UnregisterClass(g_szClassName, GetModuleHandle(nullptr));
}