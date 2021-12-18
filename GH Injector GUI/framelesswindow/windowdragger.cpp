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

#include "windowdragger.h"

WindowDragger::WindowDragger(QWidget * parent) : QWidget(parent)
{
	mousePressed = false;
}

void WindowDragger::mousePressEvent(QMouseEvent * event)
{
	mousePressed	= true;
	mousePos		= event->globalPos();

	QWidget * parent = parentWidget();
	if (parent)
	{
		parent = parent->parentWidget();
		wndPos = parent->pos();
	}
}

void WindowDragger::mouseMoveEvent(QMouseEvent * event)
{
	QWidget * parent = parentWidget();
	if (parent)
	{
		parent = parent->parentWidget();
	}

	if (parent && mousePressed)
	{
		parent->move(wndPos + (event->globalPos() - mousePos));
	}		
}

void WindowDragger::mouseReleaseEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	mousePressed = false;
}

void WindowDragger::paintEvent(QPaintEvent * event)
{
	Q_UNUSED(event);

	QStyleOption styleOption;
	styleOption.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);
}

void WindowDragger::mouseDoubleClickEvent(QMouseEvent * event)
{
	Q_UNUSED(event);

	emit doubleClicked();
}