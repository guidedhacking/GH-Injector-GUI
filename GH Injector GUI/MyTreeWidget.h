#pragma once

#include "pch.h"

class MyTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:
	MyTreeWidget(QWidget * parent = 0);

public slots:
	void customSortByColumn(int column);
};