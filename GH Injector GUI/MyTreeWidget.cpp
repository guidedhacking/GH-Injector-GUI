#include "pch.h"

#include "MyTreeWidget.h"

// https://www.qtcentre.org/threads/6884-Sorting-in-QTreeWidget

MyTreeWidget::MyTreeWidget(QWidget * parent)
	: QTreeWidget(parent)
{

	headerItem()->setText(0, QString());
	new QTreeWidgetItem(this);
	setObjectName(QString::fromUtf8("tree_process"));
	header()->setMinimumSectionSize(40);
	header()->setDefaultSectionSize(40);
	setSortingEnabled(false);				// disable built-in sorting	
	header()->setSortIndicatorShown(true);	// use our own sorting method instead
	header()->setSectionsClickable(true);		// use our own sorting method instead
	header()->setStretchLastSection(true);

	connect(header(), SIGNAL(sectionClicked(int)), this, SLOT(customSortByColumn(int)));
	customSortByColumn(header()->sortIndicatorSection());
}


//bool QTreeWidgetItem::operator<(const QTreeWidgetItem& other) const
//{
//	int sortCol = treeWidget()->sortColumn();
//	if (sortCol == 1)
//	{
//		int myNumber = text(sortCol).toInt();
//		int otherNumber = other.text(sortCol).toInt();
//		return myNumber < otherNumber;
//	}
//	else
//	{
//		if (text(sortCol) >= other.text(sortCol))
//			return false;
//
//		return true;
//	}
//}

bool compare(QTreeWidgetItem * a, QTreeWidgetItem * b)
{
	if ((*a).text(1).toInt() > (*b).text(1).toInt())
		return  true;
	return false;
}


void MyTreeWidget::customSortByColumn(int column)
{
	// here you can get the order
	Qt::SortOrder order = header()->sortIndicatorOrder();

	return;

	// default
	if (column != 1)
	{
		// and sort the items++----++---
		sortItems(column, order);
		return;
	}


	// bubble, replace this 
	int n = topLevelItemCount();
	for (int i = 0; i < n - 1; i++)
	{
		for (int j = 0; j < n - i - 1; j++)
		{
			if (order == 0 && compare(topLevelItem(j), topLevelItem(j + 1)))
			{
				QTreeWidgetItem * swap = takeTopLevelItem(j);
				insertTopLevelItem(j + 1, swap);
			}

			if (order == 1 && !compare(topLevelItem(j), topLevelItem(j + 1)))
			{
				QTreeWidgetItem * swap = takeTopLevelItem(j);
				insertTopLevelItem(j + 1, swap);
			}
		}
	}

}