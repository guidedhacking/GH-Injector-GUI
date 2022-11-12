#include "pch.h"
#include "resource.h"

#include "DragDropWindow.h"

const TCHAR DragDropWindow::m_szClassName[]	= TEXT("DragnDropWindow");
int DragDropWindow::m_ClassRefCount			= 0;

DragDropWindow::DragDropWindow()
{

}

DragDropWindow::~DragDropWindow()
{
	Close();
}

HWND DragDropWindow::CreateDragDropWindow(HWND hParent, int Size)
{
	g_print("Creating DragDrop window\n");

	m_hMainWnd	= hParent;
	m_size		= Size;

	HINSTANCE hInstance = GetModuleHandle(nullptr);

	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(WNDCLASSEX);

	if (!GetClassInfoEx(hInstance, m_szClassName, &wc))
	{
		wc.cbSize			= sizeof(WNDCLASSEX);
		wc.style			= 0;
		wc.lpfnWndProc		= DragDropWindow::WndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= hInstance;
		wc.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName		= nullptr;
		wc.lpszClassName	= m_szClassName;
		wc.hIconSm			= LoadIcon(NULL, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
		{
			g_print("RegisterClassEx failed: %08X\n", GetLastError());

			return NULL;
		}
	}

	++m_ClassRefCount;
	
	m_hDropWnd = CreateWindowEx(WS_EX_ACCEPTFILES | WS_EX_TOOLWINDOW, m_szClassName, nullptr, WS_BORDER | WS_POPUP, -1000000, -1000000, m_size, m_size, NULL, NULL, hInstance, this);
	if (m_hDropWnd == NULL)
	{
		g_print("CreateWindowExW failed: %08X\n", GetLastError());

		if (m_ClassRefCount == 1)
		{
			UnregisterClass(m_szClassName, hInstance);

			--m_ClassRefCount;
		}

		return NULL;
	}

	SetWindowLongPtr(m_hDropWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	m_hDropIcon = reinterpret_cast<HICON>(LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, m_size, m_size, NULL));
	if (!m_hDropIcon)
	{
		g_print("LoadImage failed: %08X\n", GetLastError());
	
		DestroyWindow(m_hDropWnd);

		if (m_ClassRefCount == 1)
		{
			UnregisterClass(m_szClassName, hInstance);

			--m_ClassRefCount;
		}

		return NULL;
	}

	m_hDeviceContext = GetDC(m_hDropWnd);
	if (!m_hDeviceContext)
	{
		g_print("GetDC failed: %08X\n", GetLastError());

		DestroyIcon(m_hDropIcon);
		DestroyWindow(m_hDropWnd);

		if (m_ClassRefCount == 1)
		{
			UnregisterClass(m_szClassName, hInstance);

			--m_ClassRefCount;
		}

		return NULL;
	}

	ChangeWindowMessageFilterEx(m_hDropWnd, WM_DROPFILES, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_hDropWnd, WM_COPYDATA, MSGFLT_ALLOW, nullptr);
	ChangeWindowMessageFilterEx(m_hDropWnd, 0x0049, MSGFLT_ALLOW, nullptr);

	g_print(" HWND = %08X\n", (DWORD)(((UINT_PTR)m_hDropWnd) & 0xFFFFFFFF));

	return m_hDropWnd;
}

void DragDropWindow::Close()
{
	g_print("Closing DragDrop window\n");

	if (!m_hDropWnd)
	{
		return;
	}

	CloseWindow(m_hDropWnd);

	ReleaseDC(m_hDropWnd, m_hDeviceContext);
	DestroyIcon(m_hDropIcon);
	DestroyWindow(m_hDropWnd);

	m_hDeviceContext	= NULL;
	m_hDropWnd			= NULL;
	m_hDropIcon			= NULL;

	--m_ClassRefCount;

	if (!m_ClassRefCount)
	{
		UnregisterClass(m_szClassName, GetModuleHandle(nullptr));
	}
}

void DragDropWindow::DrawIcon()
{
	auto ret = DrawIconEx(m_hDeviceContext, 0, 0, m_hDropIcon, m_size, m_size, 0, NULL, DI_NORMAL);
	if (!ret)
	{
		DWORD err = GetLastError();

		g_print("Draw Icon failed: %08X\n", err);
	}
}

HWND DragDropWindow::GetHWND() const
{
	return m_hDropWnd;
}

HWND DragDropWindow::GetParent() const
{
	return m_hMainWnd;
}

void DragDropWindow::GetPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void DragDropWindow::SetPosition(int x, int y, bool hide, bool active)
{
	if (hide)
	{
		SetWindowPos(m_hDropWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_HIDEWINDOW);

		return;
	}

	DWORD flag = SWP_SHOWWINDOW | SWP_NOACTIVATE;
	if (x == -1 || y == -1)
	{
		flag |= SWP_NOMOVE;
	}

	if (active)
	{
		SetWindowPos(m_hDropWnd, HWND_TOPMOST, x, y, m_size, m_size, flag);
	}
	else
	{
		SetWindowPos(m_hDropWnd, m_hMainWnd, x, y, m_size, m_size, flag);
		SetWindowPos(m_hMainWnd, m_hDropWnd, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	if (x != -1 && y != -1)
	{
		m_x = x;
		m_y = y;
	}
}

void DragDropWindow::HandleDrop(HDROP hDrop)
{
	g_print("Dropping files\n");

	if (!hDrop)
	{
		g_print("hDrop = 0\n");

		return;
	}

	auto DropCount = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	if (!DropCount)
	{
		g_print("DropCount = 0\n");
		
		return;
	}

	wchar_t ** Drops = new(std::nothrow) wchar_t * [DropCount]();
	if (!Drops)
	{
		g_print("Can't allocate memory for drops\n");

		return;
	}

	for (UINT i = 0; i < DropCount; ++i)
	{
		auto BufferSize = DragQueryFileW(hDrop, i, nullptr, 0);

		if (!BufferSize)
		{
			g_print("DragQueryFile returned invalid size for drop %d\n", i);

			continue;
		}

		BufferSize++;

		Drops[i] = new(std::nothrow) wchar_t[static_cast<size_t>(BufferSize)]();

		if (!Drops[i])
		{
			g_print("Can't allocate memory for drop %d\n", i);

			continue;
		}

		if (!DragQueryFileW(hDrop, i, Drops[i], BufferSize))
		{
			g_print("DragQueryFile failed for drop %d\n", i);

			delete[] Drops[i];

			continue;
		}

		QString qDrop = QString::fromWCharArray(Drops[i]);
		qDrop.replace("\\", "/");

		m_Callback(qDrop);

		delete[] Drops[i];
	}

	delete[] Drops;
}

void DragDropWindow::SetCallback(std::function<void(const QString &)> Callback)
{
	m_Callback = Callback;
}

LRESULT CALLBACK DragDropWindow::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_DROPFILES || uMsg == WM_PAINT || uMsg == WM_ACTIVATE)
	{
		DragDropWindow * pThis = reinterpret_cast<DragDropWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		if (pThis && pThis->GetHWND() == hWnd)
		{
			switch (uMsg)
			{
				case WM_DROPFILES:
					pThis->HandleDrop(reinterpret_cast<HDROP>(wParam));
					break;

				case WM_PAINT:
					pThis->DrawIcon();
					break;

				case WM_ACTIVATE:
					if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE)
					{
						SetWindowPos(pThis->GetParent(), pThis->GetHWND(), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
					}
					break;
			}
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}