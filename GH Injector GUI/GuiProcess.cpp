#include "pch.h"

#include "GuiProcess.h"

GuiProcess::GuiProcess(QWidget * parent, FramelessWindow * FramelessParent)
	: QWidget(parent)
{
	ui.setupUi(this);
		
	FramelessParent->setFixedHeight(550);
	setFixedHeight(500);

	frameless_parent = FramelessParent;

	m_ProcessStruct	= nullptr;
	m_ProcessState	= nullptr;
	m_SortSense		= SORT_SENSE::SS_ARCH_LO;

	m_TmrUpdateList = new(std::nothrow) QTimer();
	if (m_TmrUpdateList == Q_NULLPTR)
	{
		emit StatusBox(false, "Failed to create timer object. The process list won't refresh automatically.");
	}
	else
	{
		connect(m_TmrUpdateList, SIGNAL(timeout()), this, SLOT(refresh_process()));
		m_TmrUpdateList->start(std::chrono::milliseconds(1000));
	}

	m_TmrFilterFocus = new(std::nothrow) QTimer();
	if (m_TmrFilterFocus)
	{
		//Based on:
		//https://stackoverflow.com/questions/526761/set-qlineedit-focus-in-qt/622693#622693
		//Thanks, Ariya Hidayat & AAEM

		m_TmrFilterFocus->setSingleShot(true);
		m_TmrFilterFocus->setInterval(std::chrono::milliseconds(0));

		connect(m_TmrFilterFocus, SIGNAL(timeout()), ui.txt_filter, SLOT(setFocus()));
	}

	m_pxmGeneric	= QPixmap(":/GuiMain/gh_resource/Generic Icon.png");
	m_pxmError		= QPixmap(":/GuiMain/gh_resource/Error Icon.png");

	if (m_pxmGeneric.isNull() || m_pxmError.isNull())
	{
		emit StatusBox(false, "Failed to initialize one or multiple graphic files. This won't affect the functionality of the injector.");
	}

	m_bSelectedFromList = false;

	m_ProcList.clear();

	connect(ui.btn_refresh, SIGNAL(clicked()),						this, SLOT(refresh_process()));
	connect(ui.cmb_arch,	SIGNAL(currentIndexChanged(int)),		this, SLOT(filter_change(int)));
	connect(ui.txt_filter,	SIGNAL(textChanged(const QString &)),	this, SLOT(name_change(const QString &)));
	connect(ui.btn_select,	SIGNAL(clicked()),						this, SLOT(proc_select()));
	connect(ui.cb_session,	SIGNAL(stateChanged(int)),				this, SLOT(session_change()));

	connect(ui.tree_process,			SIGNAL(doubleClicked(const QModelIndex &)),	this, SLOT(double_click_process(const QModelIndex &)));
	connect(ui.tree_process->header(),	SIGNAL(sectionClicked(int)),				this, SLOT(custom_sort(int)));	

	ui.tree_process->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustIgnored);
	ui.tree_process->setColumnWidth(0, 50);
	ui.tree_process->setColumnWidth(1, 50);
	ui.tree_process->setColumnWidth(2, 200);
	ui.tree_process->setColumnWidth(3, 50);
	ui.tree_process->setColumnWidth(4, 0);

	installEventFilter(this);
	ui.tree_process->installEventFilter(this);
	ui.txt_filter->installEventFilter(this);
	
	m_OwnSessionID = getProcSession(GetCurrentProcessId());

	QMargins margins;
	margins.setBottom(10);
	setContentsMargins(margins);
}

GuiProcess::~GuiProcess()
{
	for (const auto & i : m_ProcList)
	{
		if (i)
		{
			delete i;
		}
	}

	SAFE_DELETE(m_TmrUpdateList);
	SAFE_DELETE(m_TmrFilterFocus);
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
				event->accept();

				proc_select();

				return true;
			}
		}
		else if (obj == ui.txt_filter && m_bSelectedFromList)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
			{
				event->accept();

				proc_select();

				return true;
			}
			else if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
			{
				event->accept();

				QApplication::sendEvent(ui.tree_process, event);

				return true;
			}
		}
		else if (obj == this)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() == Qt::Key_Escape)
			{
				event->accept();

				proc_select(true);

				return true;
			}
		}
	}
	else if (event->type() == QEvent::Show && obj == this && m_TmrFilterFocus)
	{
		m_TmrFilterFocus->start();
	}

	return QObject::eventFilter(obj, event);
}

void GuiProcess::refresh_gui()
{
	ARCH target_arch	= (ARCH)ui.cmb_arch->currentIndex();
	QString filter		= ui.txt_filter->text();
	int proc_count		= 0;

	m_bSelectedFromList = false;

	QTreeWidgetItem * selected_item = Q_NULLPTR;

	QTreeWidgetItemIterator it(ui.tree_process);
	for (; *it; ++it)
	{
		ARCH arch = StrToArchW((*it)->text(3).toStdWString().c_str());

		if (target_arch != ARCH::NONE && arch != target_arch)
		{
			(*it)->setSelected(false);
			(*it)->setHidden(true);

			continue;
		}

		if (ui.cb_session->isChecked())
		{
			auto target_session = (*it)->text(4).toULong();
			if (target_session != m_OwnSessionID && m_OwnSessionID != INVALID_SESSION_ID)
			{
				(*it)->setSelected(false);
				(*it)->setHidden(true);

				continue;
			}
		}

		if (!filter.isEmpty())
		{
			bool contains = (*it)->text(2).contains(filter, Qt::CaseInsensitive);
			if (!contains)
			{
				(*it)->setSelected(false);
				(*it)->setHidden(true);

				continue;
			}
		}

		(*it)->setHidden(false);

		if (!m_bSelectedFromList && (*it)->isSelected())
		{
			ui.tree_process->setCurrentItem(*it);

			selected_item = (*it);
			m_bSelectedFromList = true;
		}

		++proc_count;
	}

	frameless_parent->setWindowTitle("Select a process (" + QString::number(proc_count) + ')');

	if (!m_bSelectedFromList)
	{
		for (it = QTreeWidgetItemIterator(ui.tree_process); *it; ++it)
		{
			if ((*it)->isHidden())
			{
				continue;
			}

			(*it)->setSelected(true);
			ui.tree_process->setCurrentItem(*it);

			selected_item = (*it);
			m_bSelectedFromList = true;

			break;
		}
	}

	if (selected_item)
	{
		auto r = ui.tree_process->visualItemRect(selected_item);
		if (r.y() + r.height() + 10 >= ui.tree_process->height())
		{
			ui.tree_process->scrollToItem(selected_item);
		}
	}

	g_print("Processlist updated\n");
}

void GuiProcess::refresh_process()
{
	if (!getProcessList(m_ProcList, true))
	{
		return;
	}

	if (m_SortSense != SORT_SENSE::SS_PID_LO)
	{
		sortProcessList(m_ProcList, m_SortSense);
	}

	bool update_tree = false;

	for (int i = 0; i < ui.tree_process->topLevelItemCount(); )
	{
		DWORD pid = ui.tree_process->topLevelItem(i)->text(1).toULong();

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

	for (UINT i = 0; i < m_ProcList.size(); ++i)
	{
		DWORD pid = 0;
		auto * current_item = ui.tree_process->topLevelItem(i);

		if (current_item)
		{
			pid = current_item->text(1).toInt();
		}

		if (m_ProcList[i]->PID != pid)
		{
			TreeWidgetItem * item = new(std::nothrow) TreeWidgetItem();
			if (item == Q_NULLPTR)
			{
				g_print("Failed to create new item for the process list\n");

				continue;
			}

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
					icon = m_pxmGeneric;
				}
				else
				{
					icon = m_pxmError;
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
	UNREFERENCED_PARAMETER(i);

	emit refresh_gui();
}

void GuiProcess::session_change()
{
	emit refresh_gui();
}

void GuiProcess::name_change(const QString & str)
{
	UNREFERENCED_PARAMETER(str);

	emit refresh_gui();
}

void GuiProcess::proc_select(bool ignore)
{
	m_ProcessState->txtFilter	= ui.txt_filter->text();
	m_ProcessState->cmbArch		= ui.cmb_arch->currentIndex();
	m_ProcessState->cbSession	= ui.cb_session->isChecked();

	QTreeWidgetItem * item = ui.tree_process->currentItem();
	if (item)
	{
		m_ProcessStruct->PID = item->text(1).toInt();
		m_ProcessStruct->Arch = StrToArchW(item->text(3).toStdWString().c_str());
		lstrcpyW(m_ProcessStruct->szName, item->text(2).toStdWString().c_str());
	}

	if (ignore)
	{
		m_ProcessStruct->PID = 0;
	}

	emit send_to_inj(m_ProcessState, m_ProcessStruct);
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
					m_SortSense = SORT_SENSE::SS_PID_LO;
					break;

				case 2:
					m_SortSense = SORT_SENSE::SS_NAME_LO;
					break;

				case 3:
					m_SortSense = SORT_SENSE::SS_ARCH_LO;
					break;
			}
			break;
		
		case Qt::AscendingOrder:
			switch (column)
			{
				case 1:
					m_SortSense = SORT_SENSE::SS_PID_HI;
					break;

				case 2:
					m_SortSense = SORT_SENSE::SS_NAME_HI;
					break;

				case 3:
					m_SortSense = SORT_SENSE::SS_ARCH_HI;
					break;
			}
			break;
	}
}

void GuiProcess::double_click_process(const QModelIndex & index)
{
	UNREFERENCED_PARAMETER(index);

	proc_select();
}

void GuiProcess::get_from_inj(Process_State_Struct * procStateStruct, Process_Struct * procStruct)
{
	m_ProcessState	= procStateStruct;
	m_ProcessStruct = procStruct;

	ui.txt_filter->setText(m_ProcessState->txtFilter);
	ui.cmb_arch->setCurrentIndex(m_ProcessState->cmbArch);
	ui.cb_session->setChecked(m_ProcessState->cbSession);

	memset(m_ProcessStruct, 0, sizeof(Process_Struct));

	ui.tree_process->setFocus();

#ifndef _WIN64
	ui.cmb_arch->setDisabled(true);
	ui.cmb_arch->setCurrentIndex((int)ARCH::X86);
#endif

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