/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

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

#include "..\pch.h"

class WindowDocker;

#include "ui_framelesswindow.h"
#include "..\WindowDocker.h"

namespace Ui
{
	class FramelessWindow;
}

class FramelessWindow : public QWidget
{
	Q_OBJECT

	const quint8 CONST_DRAG_BORDER_SIZE = 15;
	const static int dock_range = 25;

private:
	QWidget * content = Q_NULLPTR;

	bool m_bMousePressed = false;

	bool resize_left	= false;
	bool resize_right	= false;
	bool resize_top		= false;
	bool resize_bottom	= false;

	bool m_bDockButton	= false;
	bool m_bDragTop		= false;
	bool m_bDragLeft	= false;
	bool m_bDragRight	= false;
	bool m_bDragBottom	= false;
	bool m_bDragged		= false;

	Ui::FramelessWindow * ui = Q_NULLPTR;

	QRect m_StartGeometry = QRect{ 0, 0, 0, 0 };

public:
	explicit FramelessWindow(QWidget * parent = Q_NULLPTR);
	virtual ~FramelessWindow();
	void setContent(QWidget * w);
	void setTitleBar(bool active);
	void setMinimizeButton(bool active);
	void setDockButton(bool active, bool docked, int direction);

	void setResizeLeft	(bool enabled = false);
	void setResizeRight	(bool enabled = false);
	void setResizeTop	(bool enabled = false);
	void setResizeBottom(bool enabled = false);
	void setResizeHorizontal(bool enabled = false);
	void setResizeVertical	(bool enabled = false);
	void setResize(bool enabled = false);
	void setBorderStyle(QString style);

	int getFullButtonWidth() const;
	int getFullButtonHeight() const;
	int getButtonCount() const;

private:
	bool leftBorderHit(const QPoint & pos);
	bool rightBorderHit(const QPoint & pos);
	bool topBorderHit(const QPoint & pos);
	bool bottomBorderHit(const QPoint & pos);
	void styleWindow(bool bActive, bool bNoState);
	void updateSizePolicy();

public slots:
	void setWindowTitle(const QString & text);
	void setWindowIcon(const QIcon & ico);
	
	void on_applicationStateChanged(Qt::ApplicationState state);
	void on_dockButton_clicked();
	void on_minimizeButton_clicked();
	void on_closeButton_clicked();

signals:
	void closeButton_clicked();
	void dockButton_clicked();
	void windowTitlebar_clicked();
	void windowTitlebar_released();

protected:
	virtual void changeEvent(QEvent * event);
	virtual void checkBorderDragging(QMouseEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual bool eventFilter(QObject * obj, QEvent * event);
};