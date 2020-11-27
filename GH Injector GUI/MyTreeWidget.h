#pragma once
#include <qstandarditemmodel.h>
#include <qtreewidget.h>
#include <qtreeview.h>
#include <qheaderview.h>


class MyTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    MyTreeWidget(QWidget* parent = 0);

public slots:
    void customSortByColumn(int column);

};
