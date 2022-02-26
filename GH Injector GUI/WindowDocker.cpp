#include "pch.h"

#include "WindowDocker.h"

#define DOCK_COLOR "rgb(32, 245, 80)"

const QString WindowDocker::Border_Highlight[5] =
{
	"#windowFrame{border-width: 1px; border-style: solid; border-color: palette(highlight) " DOCK_COLOR " palette(highlight) palette(highlight); border-radius: 5px 5px 5px 5px; background-color: palette(Window);}",
	"#windowFrame{border-width: 1px; border-style: solid; border-color: palette(highlight) palette(highlight) palette(highlight) " DOCK_COLOR "; border-radius: 5px 5px 5px 5px; background-color: palette(Window);}",
	"#windowFrame{border-width: 1px; border-style: solid; border-color: " DOCK_COLOR " palette(highlight) palette(highlight) palette(highlight); border-radius: 5px 5px 5px 5px; background-color: palette(Window);}",
	"#windowFrame{border-width: 1px; border-style: solid; border-color: palette(highlight) palette(highlight) " DOCK_COLOR " palette(highlight); border-radius: 5px 5px 5px 5px; background-color: palette(Window); }",
	"#windowFrame{border-width: 1px; border-style: solid; border-color: palette(highlight) palette(highlight) palette(highlight) palette(highlight);border-radius: 5px 5px 5px 5px; background-color: palette(Window);}"
};

WindowDocker::WindowDocker(FramelessWindow * Master, FramelessWindow * Slave)
{
	m_Master = Master;
	m_Slave = Slave;

	m_Master->installEventFilter(this);
	m_Slave->installEventFilter(this);

	connect(m_Slave, SIGNAL(dockButton_clicked()), this, SLOT(on_dock_button_clicked()));
	connect(m_Slave, SIGNAL(windowTitlebar_clicked()), this, SLOT(on_titlebar_clicked()));
	connect(m_Slave, SIGNAL(windowTitlebar_released()), this, SLOT(on_titlebar_released()));

	m_bDocking[0] = false;
	m_bDocking[1] = false;
	m_bDocking[2] = false;
	m_bDocking[3] = false;

	m_DockIndex		= -1;
	m_OldDockIndex	= -1;

	m_bResizeV	= false;
	m_bResizeH	= false;

	m_WidthH	= 450;
	m_HeightH	= 250;
	m_WidthV	= 250;
	m_HeightV	= 350;

	m_bOnResize = false;
	m_bOnMove	= false;
	m_bMoveOnly = false;

	m_bTitlebarClicked	= false;
	m_SnapOption_Master = -1;
	m_SnapOption_Slave	= -1;

	m_d	= { 0 };
	s_d	= { 0 };
}

void WindowDocker::SetDocking(bool right, bool left, bool top, bool bottom)
{
	m_bDocking[0] = right;
	m_bDocking[1] = left;
	m_bDocking[2] = top;
	m_bDocking[3] = bottom;
}

int WindowDocker::GetDockIndex()
{
	return m_DockIndex;
}

int WindowDocker::GetOldDockIndex()
{
	return m_OldDockIndex;
}

void WindowDocker::SetResizing(bool vertical, bool horizontal)
{
	m_bResizeV = vertical;
	m_bResizeH = horizontal;
}

void WindowDocker::SetDefaultSize(QSize VerticalSize, QSize HorizontalSize)
{
	m_WidthH	= HorizontalSize.width();
	m_HeightH	= HorizontalSize.height();

	m_WidthV	= VerticalSize.width();
	m_HeightV	= VerticalSize.height();
}

void WindowDocker::move_to_pos(int direction)
{
	auto m_r = m_Master->childrenRect();
	auto m_p = m_Master->mapToGlobal(m_r.topLeft());
	auto m_x = m_p.x();
	auto m_y = m_p.y();
	auto m_w = m_r.width();
	auto m_h = m_r.height();
	
	auto new_size	= QSize(0, 0);
	auto new_pos	= QPoint(0, 0);

	auto d_x = m_Slave->size().width()	- m_Slave->childrenRect().width();
	auto d_y = m_Slave->size().height() - m_Slave->childrenRect().height();

	if (m_bMoveOnly)
	{
		new_size = { m_Slave->size().width(), m_Slave->size().height() };
	}
	else
	{
		if (direction <= 1)
		{
			new_size = { m_Slave->minimumSizeHint().width(), m_h + d_y };
		}
		else
		{
			new_size = { m_w + d_x, m_Slave->minimumSizeHint().height() };
		}
	}	

	switch (direction)
	{
		case 0:
			new_pos = QPoint(m_x + m_w - d_x / 2 + DockDistance, m_y - d_y / 2);
			break;

		case 1:
			new_pos = QPoint(m_x - new_size.width() + DockDistance + 5, m_y - d_y / 2); // additional 5 pixel because of border?
			break;

		case 2:
			new_pos = QPoint(m_x - d_x / 2, m_y - new_size.height() + DockDistance + 5); // additional 5 pixel because of border?
			break;

		default:
			new_pos = QPoint(m_x - d_x / 2, m_y + m_h + DockDistance - d_y / 2);
	}

	m_bOnResize = true;
	m_Slave->resize(new_size);
	m_bOnResize = false;

	m_bOnMove = true;
	m_Slave->move(new_pos);
	m_bOnMove = false;
}

void WindowDocker::stop_dock()
{
	m_Slave->setDockButton(true, false, m_DockIndex);
	m_OldDockIndex = m_DockIndex;
	m_DockIndex = -1;
}

int WindowDocker::find_closest(int & a, int & b)
{
	int ret = INT_MAX;

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int d = (m_d.p[i].x - s_d.p[j].x) * (m_d.p[i].x - s_d.p[j].x) + (m_d.p[i].y - s_d.p[j].y) * (m_d.p[i].y - s_d.p[j].y);

			if (d < ret)
			{
				ret = d;

				a = i;
				b = j;
			}
		}
	}

	return ret;
}

int WindowDocker::check_snap()
{
	if (m_Master->isMinimized() || m_Master->isHidden())
	{
		return 0;
	}

	auto s_r = m_Slave->childrenRect();
	auto s_p = m_Slave->mapToGlobal(s_r.topLeft());

	s_d.x = s_p.x();
	s_d.y = s_p.y();
	s_d.w = s_r.width();
	s_d.h = s_r.height();
	
	s_d.p[0] = { s_d.x + s_d.w,		s_d.y + s_d.h / 2	};
	s_d.p[1] = { s_d.x,				s_d.y + s_d.h / 2	};
	s_d.p[2] = { s_d.x + s_d.w / 2, s_d.y				};
	s_d.p[3] = { s_d.x + s_d.w / 2, s_d.y + s_d.h		};

	int s_index = -1;
	int m_index = -1;

	auto distance = find_closest(m_index, s_index);

	if (distance < SnapDistance)
	{
		if (m_SnapOption_Master != m_index)
		{
			m_Master->setBorderStyle(Border_Highlight[m_index]);
			m_SnapOption_Master = m_index;
		}

		if (m_SnapOption_Slave != s_index)
		{
			m_Slave->setBorderStyle(Border_Highlight[s_index]);
			m_SnapOption_Slave = s_index;
		}
	}
	else
	{
		if (m_SnapOption_Master != -1)
		{
			m_Master->setBorderStyle(Border_Highlight[4]);
			m_SnapOption_Master = -1;
		}

		if (m_SnapOption_Slave != -1)
		{
			m_Slave->setBorderStyle(Border_Highlight[4]);
			m_SnapOption_Slave = -1;
		}
	}

	return 0;
}

void WindowDocker::stop_snap()
{
	if (m_SnapOption_Master != -1)
	{
		Dock(m_SnapOption_Master);

		m_Master->setBorderStyle(Border_Highlight[4]);
		m_Slave->setBorderStyle(Border_Highlight[4]);

		m_SnapOption_Master		= -1;
		m_SnapOption_Slave		= -1;
	}
}

void WindowDocker::to_front()
{
	m_Slave->show();
	SetWindowPos((HWND)m_Slave->winId(), (HWND)m_Master->winId(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void WindowDocker::on_dock_button_clicked()
{
	if (m_Master->isMinimized())
	{
		return;
	}

	if (m_DockIndex == -1)
	{
		if (m_OldDockIndex == -1)
		{
			Dock(0);
		}
		else
		{
			Dock(m_OldDockIndex);
		}
	}
	else
	{
		stop_dock();
	}
}

void WindowDocker::on_titlebar_clicked()
{
	m_bTitlebarClicked = true;

	auto m_r = m_Master->childrenRect();
	auto m_p = m_Master->mapToGlobal(m_r.topLeft());
	m_d.x = m_p.x();
	m_d.y = m_p.y();
	m_d.w = m_r.width();
	m_d.h = m_r.height();

	m_d.p[0] = { m_d.x + m_d.w,		m_d.y + m_d.h / 2	};
	m_d.p[1] = { m_d.x,				m_d.y + m_d.h / 2	};
	m_d.p[2] = { m_d.x + m_d.w / 2, m_d.y				};
	m_d.p[3] = { m_d.x + m_d.w / 2, m_d.y + m_d.h		};
}

void WindowDocker::on_titlebar_released()
{
	stop_snap();

	m_bTitlebarClicked = false;
}

bool WindowDocker::IsDocked()
{
	return (m_DockIndex != -1);
}

void WindowDocker::Dock()
{
	if (m_DockIndex == -1)
	{
		return;
	}

	move_to_pos(m_DockIndex);

	m_Slave->setDockButton(true, true, m_DockIndex);
}

void WindowDocker::Dock(int direction)
{	
	if (direction < 0)
	{
		return;
	}

	direction %= 4;

	if (!m_bDocking[direction])
	{
		return;
	}

	m_OldDockIndex	= m_DockIndex;
	m_DockIndex		= direction;

	move_to_pos(m_DockIndex);

	m_Slave->setDockButton(true, true, m_DockIndex);

	m_Master->setWindowState(m_Master->windowState() | Qt::WindowActive);
	m_Master->activateWindow();
}

bool WindowDocker::eventFilter(QObject * obj, QEvent * event)
{
	if (obj == m_Master)
	{
		if (event->type() == QEvent::Move)
		{
			if (m_DockIndex != -1)
			{
				m_bMoveOnly = true;
				move_to_pos(m_DockIndex);
				m_bMoveOnly = false;
			}
		}
		else if (m_DockIndex != -1)
		{
			if (event->type() == QEvent::WindowStateChange)
			{
				if (m_Master->isMinimized())
				{
					m_Slave->hide();
				}
				else
				{
					to_front();
				}
			}
			else if (event->type() == QEvent::WindowActivate)
			{
				to_front();
			}
			else if (event->type() == QEvent::ApplicationStateChange)
			{
				auto * ascEvent = static_cast<QApplicationStateChangeEvent *>(event);
				if (ascEvent->applicationState() == Qt::ApplicationState::ApplicationActive)
				{
					to_front();
				}
				else
				{
					m_Slave->hide();
				}
			}
		}
	}
	else if (obj == m_Slave)
	{
		if (event->type() == QEvent::Move)
		{
			if (!m_bOnMove && m_DockIndex != -1)
			{
				auto * moveEvent = static_cast<QMoveEvent *>(event);
				if (moveEvent->oldPos() != moveEvent->pos())
				{
					stop_dock();
				}
			}
			else if (m_bTitlebarClicked)
			{
				auto * moveEvent = static_cast<QMoveEvent *>(event);
				if (moveEvent->oldPos() != moveEvent->pos())
				{
					check_snap();
				}
			}
		}
		else if (event->type() == QEvent::Resize && !m_bOnResize && m_DockIndex != -1)
		{
			auto * resizeEvent = static_cast<QResizeEvent *>(event);
			
			auto & old_s = resizeEvent->oldSize();
			auto & new_s = resizeEvent->size();

			if (m_DockIndex == 2 && new_s.height() != old_s.height())
			{
				stop_dock();
			}
			else if (m_DockIndex == 1 && new_s.width() != old_s.width())
			{
				stop_dock();
			}
		}
		else if (event->type() == QEvent::Hide || event->type() == QEvent::Close)
		{

			if (m_DockIndex != -1 && !m_Master->isMinimized())
			{
				stop_dock();
			}
		}
	}

	return QObject::eventFilter(obj, event);
}