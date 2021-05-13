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

#ifndef FRAMELESSWINDOW_H
#define FRAMELESSWINDOW_H

#include "..\pch.h"
#include "ui_framelesswindow.h"

namespace Ui
{
	class FramelessWindow;
}

class FramelessWindow : public QWidget
{
	Q_OBJECT

private:
	QWidget * content;
	QWidget * m_dock_parent;

	bool resize_left;
	bool resize_right;
	bool resize_top;
	bool resize_bottom;

public:
	explicit FramelessWindow(QWidget * parent = Q_NULLPTR);
	virtual ~FramelessWindow();
	void setContent(QWidget * w);
	void setTitleBar(bool active);
	void setMinimizeButton(bool active);
	void setDockButton(bool active);

	void setResizeLeft	(bool enabled = false);
	void setResizeRight	(bool enabled = false);
	void setResizeTop	(bool enabled = false);
	void setResizeBottom(bool enabled = false);

	void setResizeHorizontal(bool enabled = false);
	void setResizeVertical	(bool enabled = false);

	void setResize(bool enabled = false);

	void dock(bool docked = false, QWidget * dock_parent = nullptr);

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
	void on_windowTitlebar_doubleClicked();

	void update_docked_pos();

protected:
	virtual void changeEvent(QEvent * event);
	virtual void mouseDoubleClickEvent(QMouseEvent * event);
	virtual void checkBorderDragging(QMouseEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual bool eventFilter(QObject * obj, QEvent * event);

private:
	Ui::FramelessWindow * ui;
	QRect m_StartGeometry;
	const quint8 CONST_DRAG_BORDER_SIZE = 15;
	bool m_bMousePressed;
	bool m_bDragTop;
	bool m_bDragLeft;
	bool m_bDragRight;
	bool m_bDragBottom;
	bool m_bDocked;
	bool m_bUpdateDockPos;
};

#endif  // FRAMELESSWINDOW_H
