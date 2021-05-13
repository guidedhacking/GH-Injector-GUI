#include "..\pch.h"

/*
###############################################################################
#                                                                             #
# The MIT License                                                             #
#                                                                             #
# Copyright (C) 2017 by Juergen Skrotzky (JorgenVikingGod@gmail.com)          #
#               >> https://github.com/Jorgen-VikingGod                        #
#                                                                             #
# Sources: https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle  #
#                                                                             #
###############################################################################
*/

#include "framelesswindow.h"
#include "ui_framelesswindow.h"

FramelessWindow::FramelessWindow(QWidget * parent)
	: QWidget(parent),
	ui(new Ui::FramelessWindow),
	m_bMousePressed(false),
	m_bDragTop(false),
	m_bDragLeft(false),
	m_bDragRight(false),
	m_bDragBottom(false),
	m_bDocked(false),
	m_bUpdateDockPos(false)
{

	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
	// append minimize button flag in case of windows,
	// for correct windows native handling of minimize function
#if defined(Q_OS_WIN)
	setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
#endif
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground);

	ui->setupUi(this);

	resize_left		= false;
	resize_right	= false;
	resize_top		= false;
	resize_bottom	= false;

	content			= Q_NULLPTR;
	m_dock_parent	= Q_NULLPTR;

	// shadow under window title text
	QGraphicsDropShadowEffect * textShadow = new QGraphicsDropShadowEffect;
	textShadow->setBlurRadius(4.0);
	textShadow->setColor(QColor(0, 0, 0));
	textShadow->setOffset(0.0);
	//ui->titleText->setGraphicsEffect(textShadow);

	// window shadow
	QGraphicsDropShadowEffect * windowShadow = new QGraphicsDropShadowEffect;
	windowShadow->setBlurRadius(9.0);
	windowShadow->setColor(palette().color(QPalette::Highlight));
	windowShadow->setOffset(0.0);
	ui->windowFrame->setGraphicsEffect(windowShadow);

	QObject::connect(qApp, &QGuiApplication::applicationStateChanged, this,
		&FramelessWindow::on_applicationStateChanged);
	setMouseTracking(true);

	auto al = ui->titleText->alignment();
	al.setFlag(Qt::AlignmentFlag::AlignVCenter);
	ui->titleText->setAlignment(al);

	auto x = ui->titleText->x();
	auto txt_height = ui->titleText->sizeHint().height();
	auto bar_height = ui->windowTitlebar->height();
	ui->titleText->move(x, bar_height / 2 - txt_height / 2);

	ui->dockButton->setIcon(QIcon(":/images/icon_dock.png"));

	// important to watch mouse move from all child widgets
	QApplication::instance()->installEventFilter(this);
}

FramelessWindow::~FramelessWindow()
{
	delete ui;
}

void FramelessWindow::changeEvent(QEvent * event)
{
	if (event->type() == QEvent::WindowStateChange)
	{
		if (windowState().testFlag(Qt::WindowNoState))
		{
			styleWindow(true, true);
			event->ignore();
		}
		else if (windowState().testFlag(Qt::WindowMaximized))
		{
			styleWindow(true, false);
			event->ignore();
		}
	}
	event->accept();
}

void FramelessWindow::setContent(QWidget * w)
{
	content = w;

	ui->windowContent->layout()->addWidget(w);
}

void FramelessWindow::setTitleBar(bool active)
{
	if (active)
	{
		ui->windowTitlebar->setHidden(false);
	}
	else
	{
		ui->windowTitlebar->setHidden(true);
	}
}

void FramelessWindow::setMinimizeButton(bool active)
{
	if (active)
	{
		ui->minimizeButton->setDisabled(false);
		ui->minimizeButton->setHidden(false);
	}
	else
	{
		ui->minimizeButton->setHidden(true);
		ui->minimizeButton->setDisabled(true);
	}
}

void FramelessWindow::setDockButton(bool active)
{
	if (active)
	{
		ui->dockButton->setDisabled(false);
		ui->dockButton->setHidden(false);

		ui->dockButton->setIcon(QIcon(":/images/icon_undock.png"));

		m_bDocked = true;
	}
	else
	{
		ui->dockButton->setHidden(true);
		ui->dockButton->setDisabled(true);

		m_bDocked = false;
	}
}

void FramelessWindow::setResizeLeft(bool enabled)
{
	resize_left = enabled;

	updateSizePolicy();
}

void FramelessWindow::setResizeRight(bool enabled)
{
	resize_right = enabled;

	updateSizePolicy();
}

void FramelessWindow::setResizeTop(bool enabled)
{
	resize_top = enabled;

	updateSizePolicy();
}

void FramelessWindow::setResizeBottom(bool enabled)
{
	resize_bottom = enabled;

	updateSizePolicy();
}

void FramelessWindow::setResizeHorizontal(bool enabled)
{
	resize_left		= enabled;
	resize_right	= enabled;

	updateSizePolicy();
}

void FramelessWindow::setResizeVertical(bool enabled)
{
	resize_top		= enabled;
	resize_bottom	= enabled;

	updateSizePolicy();
}

void FramelessWindow::setResize(bool enabled)
{
	resize_left		= enabled;
	resize_right	= enabled;
	resize_top		= enabled;
	resize_bottom	= enabled;

	updateSizePolicy();
}

void FramelessWindow::dock(bool docked, QWidget * dock_parent)
{
	m_dock_parent = dock_parent;

	if (m_dock_parent)
	{
		dock_parent->installEventFilter(this);
	}

	m_bDocked = docked;
	
	if (m_bDocked)
	{
		ui->dockButton->setIcon(QIcon(":/images/icon_undock.png"));
		ui->dockButton->setToolTip("Undock");
		update_docked_pos();
	}
	else
	{
		ui->dockButton->setIcon(QIcon(":/images/icon_dock.png"));
		ui->dockButton->setToolTip("Dock");
	}
}

void FramelessWindow::setWindowTitle(const QString & text)
{
	ui->titleText->setText(text);
}

void FramelessWindow::setWindowIcon(const QIcon & ico)
{
	ui->icon->setPixmap(ico.pixmap(16, 16));
}

void FramelessWindow::styleWindow(bool bActive, bool bNoState)
{
	if (bActive)
	{
		if (bNoState)
		{
			layout()->setMargin(15);
			ui->windowTitlebar->setStyleSheet(QStringLiteral(
				"#windowTitlebar{border: 0px none palette(shadow); "
				"border-top-left-radius:5px; border-top-right-radius:5px; "
				"background-color:palette(shadow); height:20px;}"));
			ui->windowFrame->setStyleSheet(QStringLiteral(
				"#windowFrame{border:1px solid palette(highlight); border-radius:5px "
				"5px 5px 5px; background-color:palette(Window);}"));
			QGraphicsEffect * oldShadow = ui->windowFrame->graphicsEffect();
			if (oldShadow) delete oldShadow;
			QGraphicsDropShadowEffect * windowShadow = new QGraphicsDropShadowEffect;
			windowShadow->setBlurRadius(9.0);
			windowShadow->setColor(palette().color(QPalette::Highlight));
			windowShadow->setOffset(0.0);
			ui->windowFrame->setGraphicsEffect(windowShadow);
		}
		else
		{
			layout()->setMargin(0);
			ui->windowTitlebar->setStyleSheet(QStringLiteral(
				"#windowTitlebar{border: 0px none palette(shadow); "
				"border-top-left-radius:0px; border-top-right-radius:0px; "
				"background-color:palette(shadow); height:20px;}"));
			ui->windowFrame->setStyleSheet(QStringLiteral(
				"#windowFrame{border:1px solid palette(dark); border-radius:0px 0px "
				"0px 0px; background-color:palette(Window);}"));
			QGraphicsEffect * oldShadow = ui->windowFrame->graphicsEffect();
			if (oldShadow) delete oldShadow;
			ui->windowFrame->setGraphicsEffect(nullptr);
		}  // if (bNoState) else maximize
	}
	else
	{
		if (bNoState)
		{
			layout()->setMargin(15);
			ui->windowTitlebar->setStyleSheet(QStringLiteral(
				"#windowTitlebar{border: 0px none palette(shadow); "
				"border-top-left-radius:5px; border-top-right-radius:5px; "
				"background-color:palette(dark); height:20px;}"));
			ui->windowFrame->setStyleSheet(QStringLiteral(
				"#windowFrame{border:1px solid #000000; border-radius:5px 5px 5px "
				"5px; background-color:palette(Window);}"));
			QGraphicsEffect * oldShadow = ui->windowFrame->graphicsEffect();
			if (oldShadow) delete oldShadow;
			QGraphicsDropShadowEffect * windowShadow = new QGraphicsDropShadowEffect;
			windowShadow->setBlurRadius(9.0);
			windowShadow->setColor(palette().color(QPalette::Shadow));
			windowShadow->setOffset(0.0);
			ui->windowFrame->setGraphicsEffect(windowShadow);
		}
		else
		{
			layout()->setMargin(0);
			ui->windowTitlebar->setStyleSheet(QStringLiteral(
				"#titlebarWidget{border: 0px none palette(shadow); "
				"border-top-left-radius:0px; border-top-right-radius:0px; "
				"background-color:palette(dark); height:20px;}"));
			ui->windowFrame->setStyleSheet(QStringLiteral(
				"#windowFrame{border:1px solid palette(shadow); border-radius:0px "
				"0px 0px 0px; background-color:palette(Window);}"));
			QGraphicsEffect * oldShadow = ui->windowFrame->graphicsEffect();
			if (oldShadow) delete oldShadow;
			ui->windowFrame->setGraphicsEffect(nullptr);
		}  // if (bNoState) { else maximize
	}    // if (bActive) { else no focus
}

void FramelessWindow::updateSizePolicy()
{
	QSizePolicy policy;

	if (resize_left || resize_right)
	{
		policy.setHorizontalPolicy(QSizePolicy::Policy::MinimumExpanding);
	}
	else
	{
		policy.setHorizontalPolicy(QSizePolicy::Policy::Fixed);
	}

	if (resize_top || resize_bottom)
	{
		policy.setVerticalPolicy(QSizePolicy::Policy::MinimumExpanding);
	}
	else
	{
		policy.setVerticalPolicy(QSizePolicy::Policy::Fixed);
	}

	this->setSizePolicy(policy);
	this->ui->windowFrame->setSizePolicy(policy);
	this->ui->windowContent->setSizePolicy(policy);

	if (content)
	{
		this->content->setSizePolicy(policy);
	}
}

void FramelessWindow::on_applicationStateChanged(Qt::ApplicationState state)
{
	if (windowState().testFlag(Qt::WindowNoState))
	{
		if (state == Qt::ApplicationActive)
		{
			styleWindow(true, true);
		}
		else
		{
			styleWindow(false, true);
		}
	}
	else if (windowState().testFlag(Qt::WindowFullScreen))
	{
		if (state == Qt::ApplicationActive)
		{
			styleWindow(true, false);
		}
		else
		{
			styleWindow(false, false);
		}
	}
}

void FramelessWindow::on_dockButton_clicked()
{
	if (m_bDocked)
	{
		ui->dockButton->setIcon(QIcon(":/images/icon_dock.png"));
		ui->dockButton->setToolTip("Dock");
		m_bDocked = false;
	}
	else
	{
		ui->dockButton->setIcon(QIcon(":/images/icon_undock.png"));
		ui->dockButton->setToolTip("Undock");
		update_docked_pos();
		m_bDocked = true;
	}
}

void FramelessWindow::on_minimizeButton_clicked()
{
	setWindowState(Qt::WindowMinimized);
}

void FramelessWindow::on_closeButton_clicked()
{
	close();

	if (content)
	{
		content->close();
	}
}

void FramelessWindow::on_windowTitlebar_doubleClicked()
{

}

void FramelessWindow::update_docked_pos()
{
	if (!m_dock_parent)
	{
		return;
	}

	m_bUpdateDockPos = true;

	auto r			= m_dock_parent->childrenRect();	
	auto p_pos		= m_dock_parent->mapToGlobal(r.topLeft());
	auto dy			= size().height() - childrenRect().height();
	auto new_pos	= QPoint(p_pos.x() + r.width() - 10, p_pos.y() - dy / 2);
	auto new_size	= QSize(size().width(), r.height() + dy);

	this->move(new_pos);
	this->resize(new_size);

	m_bUpdateDockPos = false;
}

void FramelessWindow::mouseDoubleClickEvent(QMouseEvent * event)
{
	Q_UNUSED(event);
}

void FramelessWindow::checkBorderDragging(QMouseEvent * event)
{
	if (isMaximized())
	{
		return;
	}

	QPoint globalMousePos = event->globalPos();
	if (m_bMousePressed)
	{
		QScreen * screen = QGuiApplication::primaryScreen();
		// available geometry excludes taskbar
		QRect availGeometry = screen->availableGeometry();
		int h = availGeometry.height();
		int w = availGeometry.width();
		QList<QScreen *> screenlist = screen->virtualSiblings();
		if (screenlist.contains(screen))
		{
			QSize sz = QApplication::desktop()->size();
			h = sz.height();
			w = sz.width();
		}
		// top right corner
		if (m_bDragTop && m_bDragRight)
		{
			int diff = globalMousePos.x() - (m_StartGeometry.x() + m_StartGeometry.width());
			int neww = m_StartGeometry.width() + diff;
			diff = globalMousePos.y() - m_StartGeometry.y();
			int newy = m_StartGeometry.y() + diff;
			if (neww > 0 && newy > 0 && newy < h - 50)
			{
				QRect newg = m_StartGeometry;
				newg.setWidth(neww);
				newg.setX(m_StartGeometry.x());
				newg.setY(newy);
				setGeometry(newg);
			}
		}
		// top left corner
		else if (m_bDragTop && m_bDragLeft)
		{
			int diff = globalMousePos.y() - m_StartGeometry.y();
			int newy = m_StartGeometry.y() + diff;
			diff = globalMousePos.x() - m_StartGeometry.x();
			int newx = m_StartGeometry.x() + diff;
			if (newy > 0 && newx > 0)
			{
				QRect newg = m_StartGeometry;
				newg.setY(newy);
				newg.setX(newx);
				setGeometry(newg);
			}
		}
		// bottom right corner
		else if (m_bDragBottom && m_bDragLeft)
		{
			int diff = globalMousePos.y() - (m_StartGeometry.y() + m_StartGeometry.height());
			int newh = m_StartGeometry.height() + diff;
			diff = globalMousePos.x() - m_StartGeometry.x();
			int newx = m_StartGeometry.x() + diff;
			if (newh > 0 && newx > 0)
			{
				QRect newg = m_StartGeometry;
				newg.setX(newx);
				newg.setHeight(newh);
				setGeometry(newg);
			}
		}
		else if (m_bDragTop)
		{
			int diff = globalMousePos.y() - m_StartGeometry.y();
			int newh = m_StartGeometry.height() - diff / 2;
			if (newh > 0 && newh < h - 50)
			{
				int old_height = 0;
				if (content)
				{
					old_height = content->height();
				}

				QRect newg = m_StartGeometry;
				newg.setHeight(newh);
				setGeometry(newg);

				int new_height = 0;
				if (content)
				{
					new_height = content->height();
				}

				if (old_height != new_height || !content)
				{
					newg.setY(globalMousePos.y());
					setGeometry(newg);
				}
			}
		}
		else if (m_bDragLeft)
		{
			int diff = globalMousePos.x() - m_StartGeometry.x();
			int newx = m_StartGeometry.x() + diff;
			if (newx > 0 && newx < w - 50)
			{
				QRect newg = m_StartGeometry;
				newg.setX(newx);
				setGeometry(newg);
			}
		}
		else if (m_bDragRight)
		{
			int diff = globalMousePos.x() - (m_StartGeometry.x() + m_StartGeometry.width());
			int neww = m_StartGeometry.width() + diff;
			if (neww > 0)
			{
				QRect newg = m_StartGeometry;
				newg.setWidth(neww);
				newg.setX(m_StartGeometry.x());
				setGeometry(newg);
			}
		}
		else if (m_bDragBottom)
		{
			int diff = globalMousePos.y() - (m_StartGeometry.y() + m_StartGeometry.height());
			int newh = m_StartGeometry.height() + diff;
			if (newh > 0)
			{
				QRect newg = m_StartGeometry;
				newg.setHeight(newh);
				newg.setY(m_StartGeometry.y());
				setGeometry(newg);
			}
		}
	}
	else
	{
		// no mouse pressed
		if (leftBorderHit(globalMousePos) && topBorderHit(globalMousePos))
		{
			setCursor(Qt::SizeFDiagCursor);
		}
		else if (rightBorderHit(globalMousePos) && topBorderHit(globalMousePos))
		{
			setCursor(Qt::SizeBDiagCursor);
		}
		else if (leftBorderHit(globalMousePos) && bottomBorderHit(globalMousePos))
		{
			setCursor(Qt::SizeBDiagCursor);
		}
		else
		{
			if (topBorderHit(globalMousePos))
			{
				setCursor(Qt::SizeVerCursor);
			}
			else if (leftBorderHit(globalMousePos))
			{
				setCursor(Qt::SizeHorCursor);
			}
			else if (rightBorderHit(globalMousePos))
			{
				setCursor(Qt::SizeHorCursor);
			}
			else if (bottomBorderHit(globalMousePos))
			{
				setCursor(Qt::SizeVerCursor);
			}
			else
			{
				m_bDragTop		= false;
				m_bDragLeft		= false;
				m_bDragRight	= false;
				m_bDragBottom	= false;
				setCursor(Qt::ArrowCursor);
			}
		}
	}
}

// pos in global virtual desktop coordinates
bool FramelessWindow::leftBorderHit(const QPoint & pos)
{
	if (resize_left)
	{
		const QRect & rect = this->geometry();
		if (pos.x() >= rect.x() && pos.x() <= rect.x() + CONST_DRAG_BORDER_SIZE)
		{
			return true;
		}
	}

	return false;
}

bool FramelessWindow::rightBorderHit(const QPoint & pos)
{
	if (resize_right)
	{
		const QRect & rect = this->geometry();
		int tmp = rect.x() + rect.width();
		if (pos.x() <= tmp && pos.x() >= (tmp - CONST_DRAG_BORDER_SIZE))
		{
			return true;
		}
	}

	return false;
}

bool FramelessWindow::topBorderHit(const QPoint & pos)
{
	if (resize_top)
	{
		const QRect & rect = this->geometry();
		if (pos.y() >= rect.y() && pos.y() <= rect.y() + CONST_DRAG_BORDER_SIZE)
		{
			return true;
		}
	}

	return false;
}

bool FramelessWindow::bottomBorderHit(const QPoint & pos)
{
	if (resize_bottom)
	{
		const QRect & rect = this->geometry();
		int tmp = rect.y() + rect.height();
		if (pos.y() <= tmp && pos.y() >= (tmp - CONST_DRAG_BORDER_SIZE))
		{
			return true;
		}
	}

	return false;
}

void FramelessWindow::mousePressEvent(QMouseEvent * event)
{
	if (isMaximized())
	{
		return;
	}

	m_bMousePressed = true;
	m_StartGeometry = this->geometry();

	QPoint globalMousePos = mapToGlobal(QPoint(event->x(), event->y()));

	if (leftBorderHit(globalMousePos) && topBorderHit(globalMousePos))
	{
		m_bDragTop = true;
		m_bDragLeft = true;
		setCursor(Qt::SizeFDiagCursor);
	}
	else if (rightBorderHit(globalMousePos) && topBorderHit(globalMousePos))
	{
		m_bDragRight = true;
		m_bDragTop = true;
		setCursor(Qt::SizeBDiagCursor);
	}
	else if (leftBorderHit(globalMousePos) && bottomBorderHit(globalMousePos))
	{
		m_bDragLeft = true;
		m_bDragBottom = true;
		setCursor(Qt::SizeBDiagCursor);
	}
	else
	{
		if (topBorderHit(globalMousePos))
		{
			m_bDragTop = true;
			setCursor(Qt::SizeVerCursor);
		}
		else if (leftBorderHit(globalMousePos))
		{
			m_bDragLeft = true;
			setCursor(Qt::SizeHorCursor);
		}
		else if (rightBorderHit(globalMousePos))
		{
			m_bDragRight = true;
			setCursor(Qt::SizeHorCursor);
		}
		else if (bottomBorderHit(globalMousePos))
		{
			m_bDragBottom = true;
			setCursor(Qt::SizeVerCursor);
		}
	}
}

void FramelessWindow::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	if (isMaximized())
	{
		return;
	}

	m_bMousePressed = false;
	bool bSwitchBackCursorNeeded = m_bDragTop || m_bDragLeft || m_bDragRight || m_bDragBottom;

	m_bDragTop		= false;
	m_bDragLeft		= false;
	m_bDragRight	= false;
	m_bDragBottom	= false;

	if (bSwitchBackCursorNeeded)
	{
		setCursor(Qt::ArrowCursor);
	}
}

bool FramelessWindow::eventFilter(QObject * obj, QEvent * event)
{

	if (event->type() == QEvent::MouseMove)
	{
		QMouseEvent * pMouse = dynamic_cast<QMouseEvent *>(event);
		if (pMouse)
		{
			checkBorderDragging(pMouse);
		}
	}
	// press is triggered only on frame window
	else if (event->type() == QEvent::MouseButtonPress && obj == this)
	{
		QMouseEvent * pMouse = dynamic_cast<QMouseEvent *>(event);
		if (pMouse)
		{
			mousePressEvent(pMouse);
		}
	}
	else if (event->type() == QEvent::MouseButtonRelease)
	{
		if (m_bMousePressed)
		{
			QMouseEvent * pMouse = dynamic_cast<QMouseEvent *>(event);
			if (pMouse)
			{
				mouseReleaseEvent(pMouse);
			}
		}
	}
	else if (event->type() == QEvent::Move && obj == m_dock_parent && m_bDocked)
	{		
		update_docked_pos();
	}
	else if (event->type() == QEvent::Resize && obj == m_dock_parent && m_bDocked)
	{
		update_docked_pos();
	}
	else if (event->type() == QEvent::Move && obj == this && m_bDocked && !m_bUpdateDockPos && !m_bDragRight)
	{
		on_dockButton_clicked();
	}

	return QWidget::eventFilter(obj, event);
}