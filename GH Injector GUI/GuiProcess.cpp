#include "pch.h"

#include "GuiProcess.h"
#include "GuiMain.h"
#include "MyTreeWidget.h"

GuiProcess::GuiProcess(QWidget * parent, FramelessWindow * FramelessParent)
	: QWidget(parent)
{
	ui.setupUi(this);

	frameless_parent = FramelessParent;

	ps	= nullptr;
	pss = nullptr;
	sort_prev = SORT_PS::NUM_LOW;

	// Maybe sometime we need this
	delete ui.tree_process;
	ui.tree_process = new MyTreeWidget(this);

	//why is this necessary??
	ui.grid_proc->addWidget(ui.tree_process, 0, 0, 1, 3);

	QTreeWidgetItem * ___qtreewidgetitem = ui.tree_process->headerItem();
	___qtreewidgetitem->setText(3, QCoreApplication::translate("frm_proc", "Type", nullptr));
	___qtreewidgetitem->setText(2, QCoreApplication::translate("frm_proc", "Name", nullptr));
	___qtreewidgetitem->setText(1, QCoreApplication::translate("frm_proc", "PID", nullptr));

	QTreeWidgetItem * ___qtreewidgetitem1 = ui.tree_process->topLevelItem(0);
	___qtreewidgetitem1->setText(3, QCoreApplication::translate("frm_proc", "NONE", nullptr));
	___qtreewidgetitem1->setText(2, QCoreApplication::translate("frm_proc", "xxxxxxxxxxxxxxxxxxxxxxx", nullptr));
	___qtreewidgetitem1->setText(1, QCoreApplication::translate("frm_proc", "123456", nullptr));

	connect(ui.btn_refresh, SIGNAL(clicked()), this, SLOT(refresh_process()));
	connect(ui.cmb_arch,	SIGNAL(currentIndexChanged(int)), this, SLOT(filter_change(int)));
	connect(ui.txt_filter,	SIGNAL(textChanged(const QString &)), this, SLOT(name_change(const QString &)));
	connect(ui.btn_select,	SIGNAL(clicked()), this, SLOT(proc_select()));
	connect(ui.cb_session,	SIGNAL(stateChanged(int)), this, SLOT(session_change()));

	connect(ui.tree_process->header(), SIGNAL(sectionClicked(int)), this, SLOT(custom_sort(int)));
	connect(ui.tree_process, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(double_click_process(const QModelIndex &)));

	native = IsNativeProcess(GetCurrentProcessId());

	installEventFilter(this);
	ui.tree_process->installEventFilter(this);

	for (int i = 0; i <= 3; i++)
	{
		ui.tree_process->resizeColumnToContents(i);
	}
}

GuiProcess::~GuiProcess()
{

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
	ARCH target_arch = (ARCH)ui.cmb_arch->currentIndex();
	int own_session = getProcSession(GetCurrentProcessId());
	QString filter = ui.txt_filter->text();
	int proc_count = 0;

	QTreeWidgetItemIterator it(ui.tree_process);
	for (; *it; ++it)
	{
		QString strArch = (*it)->text(3);
		ARCH arch = GuiMain::str_to_arch(strArch);
		int pid = (*it)->text(1).toInt();

		if (pid == GetCurrentProcessId())
		{
			continue;
		}

		if (target_arch != ARCH::NONE && arch != target_arch)  // x86 or x64
		{
			(*it)->setHidden(true);

			continue;
		}

		if (ui.cb_session->isChecked())
		{
			int target_session = getProcSession(pid);
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

	if (frameless_parent)
	{
		frameless_parent->setWindowTitle("Select a process (" + QString::number(proc_count) + ')');
	}
	else
	{
		this->setWindowTitle("Select a process (" + QString::number(proc_count) + ')');
	}
}

void GuiProcess::refresh_process()
{
	std::vector<Process_Struct> all_proc;
	getProcessList(all_proc);
	sortProcessList(all_proc, sort_prev);

	ui.tree_process->clear();

	for (const auto & proc : all_proc)
	{
		if (proc.PID == GetCurrentProcessId())
		{
			continue;
		}

		QTreeWidgetItem * item = new QTreeWidgetItem(ui.tree_process);

		item->setText(1, QString::number(proc.PID));
		item->setText(2, QString::fromWCharArray(proc.szName));
		item->setText(3, GuiMain::arch_to_str(proc.Arch));

		// https://forum.qt.io/topic/62866/getting-icon-from-external-applications/4
		if (this->parentWidget())
		{
			QString path = QString::fromWCharArray(proc.szPath);

#ifndef _WIN64
			if (!native)
			{
				path.replace(":\\Windows\\System32\\", ":\\Windows\\Sysnative\\", Qt::CaseSensitivity::CaseInsensitive);
			}
#endif
			model.setRootPath(path);
			QIcon ic = model.fileIcon(model.index(path));
			item->setIcon(0, ic);
		}
	}

	emit refresh_gui();
}

void GuiProcess::filter_change(int i)
{
	emit refresh_process();
}

void GuiProcess::session_change()
{
	emit refresh_process();
}

void GuiProcess::name_change(const QString & str)
{
	emit refresh_process();
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
		ps->Arch = GuiMain::str_to_arch(item->text(3));
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
	// here you can get the order
	Qt::SortOrder order = ui.tree_process->header()->sortIndicatorOrder();

	if (column == 2 && order == Qt::AscendingOrder)
	{
		sort_prev = SORT_PS::ASCI_A;
	}
	else if (column == 2 && order == Qt::DescendingOrder)
	{
		sort_prev = SORT_PS::ASCI_Z;
	}
	else if (column == 1 && order == Qt::DescendingOrder)
	{
		sort_prev = SORT_PS::NUM_HIGH;
	}
	else
	{
		sort_prev = SORT_PS::NUM_LOW;
	}

	emit refresh_process();
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