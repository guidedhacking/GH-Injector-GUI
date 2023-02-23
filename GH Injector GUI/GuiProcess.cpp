#include "pch.h"

#include "GuiProcess.h"

GuiProcess::GuiProcess(QWidget * parent, FramelessWindow * FramelessParent)
	: QWidget(parent)
{
	ui.setupUi(this);
		
	FramelessParent->setFixedHeight(550);
	setFixedHeight(500);

	m_FramelessParent = FramelessParent;
	m_SortSense	= SORT_SENSE::SS_ARCH_LO;

	connect(&m_TmrUpdateList, SIGNAL(timeout()), this, SLOT(refresh_process()));
	m_TmrUpdateList.start(std::chrono::milliseconds(1000));
	
	//Based on:
	//https://stackoverflow.com/questions/526761/set-qlineedit-focus-in-qt/622693#622693
	//Thanks, Ariya Hidayat & AAEM
	m_TmrFilterFocus.setSingleShot(true);
	m_TmrFilterFocus.setInterval(std::chrono::milliseconds(0));
	connect(&m_TmrFilterFocus, SIGNAL(timeout()), ui.txt_filter, SLOT(setFocus()));
	
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

	QMargins margins;
	margins.setBottom(10);
	setContentsMargins(margins);
}

GuiProcess::~GuiProcess()
{
	for (auto & i : m_ProcList)
	{
		SAFE_DELETE(i);
	}
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
	else if (event->type() == QEvent::Show && obj == this)
	{
		m_TmrFilterFocus.start();
	}
	else if (event->type() == QEvent::Hide && obj == this)
	{
		m_ProcessState->txtFilter	= ui.txt_filter->text();
		m_ProcessState->cmbArch		= ui.cmb_arch->currentIndex();
		m_ProcessState->cbSession	= ui.cb_session->isChecked();
	}

	return QObject::eventFilter(obj, event);
}

void GuiProcess::refresh_gui()
{
	auto	target_arch	= ARCHITECTURE(ui.cmb_arch->currentIndex());
	QString filter		= ui.txt_filter->text();
	UINT	proc_count	= 0;

	m_bSelectedFromList = false;

	QTreeWidgetItem * selected_item = Q_NULLPTR;

	QTreeWidgetItemIterator it(ui.tree_process);
	for (; *it; ++it)
	{
		auto arch = StrToArchW((*it)->text(3).toStdWString());

		if (target_arch != ARCH::NONE && arch != target_arch)
		{
			(*it)->setSelected(false);
			(*it)->setHidden(true);

			continue;
		}

		if (ui.cb_session->isChecked())
		{
			auto target_session = (*it)->text(4).toULong();
			if (target_session != g_SessionID)
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

	if (m_FramelessParent)
	{
		m_FramelessParent->setWindowTitle("Select a process (" + QString::number(proc_count) + ')');
	}
	else
	{
		setWindowTitle("Select a process (" + QString::number(proc_count) + ')');
	}

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
	if (!GetProcessList(m_ProcList))
	{
		return;
	}

	if (m_SortSense != SORT_SENSE::SS_PID_LO)
	{
		SortProcessList(m_ProcList, m_SortSense);
	}

	bool update_tree = false;

	for (int i = 0; i < ui.tree_process->topLevelItemCount(); )
	{
		DWORD pid = ui.tree_process->topLevelItem(i)->text(1).toULong();

		auto search_list = [&pid](const ProcessData * entry) -> bool
		{
			return ((*entry) == pid);
		};

		auto ret = std::find_if(m_ProcList.begin(), m_ProcList.end(), search_list);
		if (ret == m_ProcList.end())
		{
			delete ui.tree_process->topLevelItem(i);

			update_tree = true;

			continue;
		}

		++i;
	}

	for (UINT i = 0; i < m_ProcList.size(); ++i)
	{
		DWORD item_pid = 0;
		auto * current_item = ui.tree_process->topLevelItem(i);

		if (current_item)
		{
			item_pid = current_item->text(1).toInt();
		}

		DWORD list_pid = 0;
		m_ProcList[i]->GetProcessID(list_pid);

		if (list_pid != item_pid)
		{
			TreeWidgetItem * item = new(std::nothrow) TreeWidgetItem();
			if (item == Q_NULLPTR)
			{
				g_print("Failed to create new item for the process list\n");

				continue;
			}

			std::wstring name;
			ARCHITECTURE arch;
			ULONG session	= 0;
			HICON icon		= NULL;
			m_ProcList[i]->GetNameW(name);
			m_ProcList[i]->GetArchitecture(arch);
			m_ProcList[i]->GetSessionID(session);
			m_ProcList[i]->GetIcon(icon);

			item->setText(1, QString::number(list_pid));
			item->setText(2, QString::fromStdWString(name));
			item->setText(3, QString::fromStdWString(arch.ToStdWString()));
			item->setText(4, QString::number(session));

			QPixmap pxm_icon;
			if (icon)
			{
				pxm_icon = qt_pixmapFromWinHICON(icon);
			}

			if (pxm_icon.isNull())
			{
				pxm_icon = m_pxmGeneric;
			}

			item->setIcon(0, pxm_icon);

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

	auto * item = ui.tree_process->currentItem();
	if (ignore || !item)
	{
		m_ProcessData->UpdateData(0);
	}
	else
	{
		DWORD PID = item->text(1).toInt();
		m_ProcessData->UpdateData(PID);
	}

	emit send_to_inj();
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

void GuiProcess::get_from_inj(ProcessState * proc_state, ProcessData * proc_data)
{
	m_ProcessState	= proc_state;
	m_ProcessData	= proc_data;

	ui.txt_filter->setText(m_ProcessState->txtFilter);
	ui.cmb_arch->setCurrentIndex(m_ProcessState->cmbArch);
	ui.cb_session->setChecked(m_ProcessState->cbSession);

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