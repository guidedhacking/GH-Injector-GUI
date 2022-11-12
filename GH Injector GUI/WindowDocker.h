#pragma once

class FramelessWindow;

#include "pch.h"

#include "framelesswindow/framelesswindow.h"

#define DOCK_NONE	-1
#define DOCK_RIGHT	0
#define DOCK_LEFT	1
#define DOCK_TOP	2
#define DOCK_BOTTOM 3
#define DOCK_MAX	4
//support n-sided windows in case no boring ass rectangle

class WindowDocker : public QWidget
{
	Q_OBJECT

private:
	const static int DockDistance = 5;
	const static int SnapDistance = 2500; //squared

	const static QString Border_Highlight[DOCK_MAX + 1];

	FramelessWindow * m_Master;
	FramelessWindow * m_Slave;

	bool m_bDocking[DOCK_MAX];

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

	int find_closest(int & a, int & b) const;
	int check_snap();
	void stop_snap();
	void update_dock_button();

	void to_front();

	void update_data();

private slots:
	void on_dock_button_clicked();
	void on_titlebar_clicked();
	void on_titlebar_released();

public:
	WindowDocker(FramelessWindow * Master, FramelessWindow * Slave);

	void SetDocking(bool right, bool left, bool top, bool bottom);
	void SetResizing(bool vertical, bool horizontal);
	void SetDefaultSize(QSize VerticalSize, QSize HorizontalSize);

	bool IsDocked() const;
	void Dock();
	void Dock(int direction);

	int GetDockIndex() const;
	int GetOldDockIndex() const;

	virtual bool eventFilter(QObject * obj, QEvent * event);

};