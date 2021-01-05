#include "pch.h"

#include "GuiProcess.h"

GuiProcess::GuiProcess(QWidget * parent, FramelessWindow * FramelessParent)
	: QWidget(parent)
{
	ui.setupUi(this);
		
	FramelessParent->setFixedHeight(550);
	setFixedHeight(500);

	frameless_parent = FramelessParent;

	ps	= nullptr;
	pss = nullptr;
	sort_sense = SORT_SENSE::SS_ARCH_LO;

	update_list = new QTimer();

	pxm_generic = QPixmap(":/GuiMain/gh_resource/Generic Icon.png");
	pxm_error	= QPixmap(":/GuiMain/gh_resource/Error Icon.png");

	m_ProcList.clear();

	connect(ui.btn_refresh, SIGNAL(clicked()),						this, SLOT(refresh_process()));
	connect(ui.cmb_arch,	SIGNAL(currentIndexChanged(int)),		this, SLOT(filter_change(int)));
	connect(ui.txt_filter,	SIGNAL(textChanged(const QString &)),	this, SLOT(name_change(const QString &)));
	connect(ui.btn_select,	SIGNAL(clicked()),						this, SLOT(proc_select()));
	connect(ui.cb_session,	SIGNAL(stateChanged(int)),				this, SLOT(session_change()));

	connect(ui.tree_process,			SIGNAL(doubleClicked(const QModelIndex &)),	this, SLOT(double_click_process(const QModelIndex &)));
	connect(ui.tree_process->header(),	SIGNAL(sectionClicked(int)),				this, SLOT(custom_sort(int)));

	connect(update_list, SIGNAL(timeout()), this, SLOT(refresh_process()));
	update_list->start(1000);

	ui.tree_process->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustIgnored);
	ui.tree_process->setColumnWidth(0, 50);
	ui.tree_process->setColumnWidth(1, 50);
	ui.tree_process->setColumnWidth(2, 200);
	ui.tree_process->setColumnWidth(3, 50);
	ui.tree_process->setColumnWidth(4, 0);

	installEventFilter(this);
	ui.tree_process->installEventFilter(this);
	
	own_session = getProcSession(GetCurrentProcessId());
}

GuiProcess::~GuiProcess()
{
	for (auto i : m_ProcList)
	{
		if (i)
		{
			delete i;
		}
	}

	delete[] update_list;
}

bool GuiProcess::eventFilter(QObject * obj, QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if (obj == ui.tree_process)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				proc_select();
			}
		}
		else if (obj == this)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Escape)
			{
				proc_select(true);
			}
		}
	}

	return QObject::eventFilter(obj, event);
}

void GuiProcess::refresh_gui()
{
	ARCH target_arch	= (ARCH)ui.cmb_arch->currentIndex();
	QString filter		= ui.txt_filter->text();
	int proc_count		= 0;

	QTreeWidgetItemIterator it(ui.tree_process);
	for (; *it; ++it)
	{
		ARCH arch = StrToArchW((*it)->text(3).toStdWString().c_str());

		if (target_arch != ARCH::NONE && arch != target_arch)
		{
			(*it)->setHidden(true);

			continue;
		}

		if (ui.cb_session->isChecked())
		{
			int target_session = (*it)->text(4).toInt();
			if (target_session != own_session && own_session != -1)
			{
				(*it)->setHidden(true);

				continue;
			}
		}

		if (!filter.isEmpty())
		{
			bool contains = (*it)->text(2).contains(filter, Qt::CaseInsensitive);
			if (!contains)
			{
				(*it)->setHidden(true);

				continue;
			}
		}

		(*it)->setHidden(false);

		++proc_count;
	}

	frameless_parent->setWindowTitle("Select a process (" + QString::number(proc_count) + ')');
}

void GuiProcess::refresh_process()
{
	if (!getProcessList(m_ProcList, true))
	{
		return;
	}

	if (sort_sense != SORT_SENSE::SS_PID_LO)
	{
		sortProcessList(m_ProcList, sort_sense);
	}

	bool update_tree = false;

	for (int i = 0; i < ui.tree_process->topLevelItemCount(); )
	{
		int pid = ui.tree_process->topLevelItem(i)->text(1).toInt();

		auto search_list = [pid](const Process_Struct * entry) -> bool
		{
			return (pid == entry->PID);
		};

		auto ret = std::find_if(m_ProcList.begin(), m_ProcList.end(), search_list);
		if (ret == m_ProcList.end() && m_ProcList.back()->PID != pid)
		{
			delete ui.tree_process->topLevelItem(i);

			update_tree = true;

			continue;
		}

		++i;
	}

	for (int i = 0; i < m_ProcList.size(); ++i)
	{
		int pid = -1;
		auto * current_item = ui.tree_process->topLevelItem(i);

		if (current_item)
		{
			pid = current_item->text(1).toInt();
		}

		if (m_ProcList[i]->PID != pid)
		{
			TreeWidgetItem * item = new TreeWidgetItem();
			item->setText(1, QString::number(m_ProcList[i]->PID));
			item->setText(2, QString::fromStdWString(m_ProcList[i]->szName));
			item->setText(3, QString::fromStdWString(ArchToStrW(m_ProcList[i]->Arch)));
			item->setText(4, QString::number(m_ProcList[i]->Session));

			QPixmap icon;

			if (m_ProcList[i]->hIcon)
			{
				icon = qt_pixmapFromWinHICON(m_ProcList[i]->hIcon);
			}

			if (icon.isNull())
			{
				if (lstrlenW(m_ProcList[i]->szPath))
				{
					icon = pxm_generic;
				}
				else
				{
					icon = pxm_error;
				}
			}

			item->setIcon(0, icon);

			ui.tree_process->insertTopLevelItem(i, item);

			update_tree = true;
		}
	}

	if (update_tree)
	{
		emit refresh_gui();
	}
}

void GuiProcess::filter_change(int i)
{
	emit refresh_gui();
}

void GuiProcess::session_change()
{
	emit refresh_gui();
}

void GuiProcess::name_change(const QString & str)
{
	emit refresh_gui();
}

void GuiProcess::proc_select(bool ignore)
{
	pss->txtFilter	= ui.txt_filter->text();
	pss->cmbArch	= ui.cmb_arch->currentIndex();
	pss->cbSession	= ui.cb_session->isChecked();

	QTreeWidgetItem * item = ui.tree_process->currentItem();
	if (item)
	{
		ps->PID = item->text(1).toInt();
		ps->Arch = StrToArchW(item->text(3).toStdWString().c_str());
		lstrcpyW(ps->szName, item->text(2).toStdWString().c_str());
	}

	if (ignore)
	{
		ps->PID = 0;
	}

	emit send_to_inj(pss, ps);
}

void GuiProcess::custom_sort(int column)
{
	auto order = ui.tree_process->header()->sortIndicatorOrder();

	switch (order)
	{
		case Qt::DescendingOrder:
			switch (column)
			{
				case 1:
					sort_sense = SORT_SENSE::SS_PID_LO;
					break;

				case 2:
					sort_sense = SORT_SENSE::SS_NAME_LO;
					break;

				case 3:
					sort_sense = SORT_SENSE::SS_ARCH_LO;
					break;
			}
			break;
		
		case Qt::AscendingOrder:
			switch (column)
			{
				case 1:
					sort_sense = SORT_SENSE::SS_PID_HI;
					break;

				case 2:
					sort_sense = SORT_SENSE::SS_NAME_HI;
					break;

				case 3:
					sort_sense = SORT_SENSE::SS_ARCH_HI;
					break;
			}
			break;
	}
}

void GuiProcess::double_click_process(const QModelIndex & index)
{
	proc_select();
}

void GuiProcess::get_from_inj(Process_State_Struct * procStateStruct, Process_Struct * procStruct)
{
	pss = procStateStruct;
	ps	= procStruct;

	ui.txt_filter->setText(pss->txtFilter);
	ui.cmb_arch->setCurrentIndex(pss->cmbArch);
	ui.cb_session->setChecked(pss->cbSession);

	memset(ps, 0, sizeof(Process_Struct));

	ui.tree_process->setFocus();

#ifndef _WIN64
	ui.cmb_arch->setDisabled(true);
	ui.cmb_arch->setCurrentIndex((int)ARCH::X86);
#endif // WIN64

	refresh_process();
}

TreeWidgetItem::TreeWidgetItem(int type)
	: QTreeWidgetItem(type)
{

}

bool TreeWidgetItem::operator<(const QTreeWidgetItem & rhs) const
{
	int column = treeWidget()->sortColumn();
	auto direction = treeWidget()->header()->sortIndicatorOrder();

	switch (column)
	{
		case 1:
			return (text(1).toInt() > rhs.text(1).toInt());

		case 2:
		{
			int cmp = text(2).compare(rhs.text(2), Qt::CaseSensitivity::CaseInsensitive);

			if (cmp == 0)
			{
				return (text(1).toInt() > rhs.text(1).toInt());
			}

			return (cmp > 0);
		}

		case 3:
		{
			int cmp = text(3).compare(rhs.text(3), Qt::CaseSensitivity::CaseInsensitive);

			if (cmp == 0)
			{
				cmp = text(2).compare(rhs.text(2), Qt::CaseSensitivity::CaseInsensitive);

				if (cmp == 0)
				{
					if (direction == Qt::SortOrder::AscendingOrder)
					{
						return  (text(1).toInt() < rhs.text(1).toInt());
					}
					else
					{
						return  (text(1).toInt() > rhs.text(1).toInt());;
					}
				}

				if (direction == Qt::SortOrder::AscendingOrder)
				{
					return (cmp < 0);
				}
				else
				{
					return (cmp > 0);
				}
			}

			return (cmp < 0);
		}
	}

	return false;
}