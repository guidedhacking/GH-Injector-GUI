/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

#include "DebugConsole.h"

class DragDropWindow
{
	HWND	m_hMainWnd			= NULL;
	HWND	m_hDropWnd			= NULL;
	HDC		m_hDeviceContext	= NULL;
	HICON	m_hDropIcon			= NULL;

	int m_x = 0;
	int m_y = 0;
	int m_size = 0;

	const static TCHAR m_szClassName[];
	static int m_ClassRefCount;

	std::function<void(const QString &)> m_pCallback;

	void DrawIcon();

public:
	DragDropWindow();
	~DragDropWindow();

	HWND CreateDragDropWindow(HWND hParent, int Size = 30);
	void Close();

	HWND GetHWND() const;
	HWND GetParent() const;
	void GetPosition(int & x, int & y) const;

	void SetPosition(int x, int y, bool hide, bool active);

	void HandleDrop(HDROP hDrop);
	void SetCallback(const decltype(m_pCallback) & callback);

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};