#pragma once

class FramelessWindow;

#include "pch.h"

#include "framelesswindow/framelesswindow.h"

class WindowDocker : public QWidget
{
	Q_OBJECT

private:
	const static int DockDistance = 5;
	const static int SnapDistance = 2500; //squared

	const static QString Border_Highlight[5];

	FramelessWindow * m_Master;
	FramelessWindow * m_Slave;

	bool m_bDocking[4];

	int m_DockIndex;
	int m_OldDockIndex;

	bool m_bResizeV;
	bool m_bResizeH;

	int m_WidthH;
	int m_HeightH;
	int m_WidthV;
	int m_HeightV;

	bool m_bOnResize;
	bool m_bOnMove;
	bool m_bMoveOnly;

	bool m_bTitlebarClicked;
	int m_SnapOption_Master;
	int m_SnapOption_Slave;

	struct wnd_data
	{
		int x, y, w, h;
		POINT p[4];
	};

	wnd_data m_d;
	wnd_data s_d;

	void move_to_pos(int direction);
	void stop_dock();

	int find_closest(int & a, int & b);
	int check_snap();
	void stop_snap();

	void to_front();

private slots:
	void on_dock_button_clicked();
	void on_titlebar_clicked();
	void on_titlebar_released();

public:
	WindowDocker(FramelessWindow * Master, FramelessWindow * Slave);

	void SetDocking(bool right, bool left, bool top, bool bottom);
	void SetResizing(bool vertical, bool horizontal);
	void SetDefaultSize(QSize VerticalSize, QSize HorizontalSize);

	bool IsDocked();
	void Dock(int direction);

	int GetDockIndex();
	int GetOldDockIndex();

	virtual bool eventFilter(QObject * obj, QEvent * event);

};