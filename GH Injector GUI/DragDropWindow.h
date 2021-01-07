#pragma once

#include "pch.h"

#include "DebugConsole.h"

class DragDropWindow
{
	HWND	m_hMainWnd;
	HWND	m_hDropWnd;
	HDC		m_hDeviceContext;
	HICON	m_hDropIcon;

	const static wchar_t m_szClassName[];
	static int m_ClassRefCount;

	std::function<void(const QString &)> m_Callback;

	int m_x;
	int m_y;
	int m_size;

	void DrawIcon();

public:
	DragDropWindow();
	~DragDropWindow();

	HWND CreateDragDropWindow(HWND hParent, int Size = 30);
	void Close();

	HWND GetHWND();
	HWND GetParent();
	void GetPosition(int & x, int & y);

	void SetPosition(int x, int y, bool hide, bool active);

	void HandleDrop(HDROP hDrop);
	void SetCallback(std::function<void(const QString &)> callback);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};