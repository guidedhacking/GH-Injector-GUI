#include "pch.h"

#include "GuiScanHook.h"
#include "GuiMain.h"

GuiScanHook::GuiScanHook(QWidget * parent, FramelessWindow * FramelessParent, InjectionLib * lib)
	: QWidget(parent)
{
	ui.setupUi(this);

	InjLib = lib;

	m_pid = 0;
	m_err = 0;

	frameless_parent = FramelessParent;

	connect(ui.btn_scan, SIGNAL(clicked()), this, SLOT(scan_clicked()));
	connect(ui.btn_unhook, SIGNAL(clicked()), this, SLOT(unhook_clicked()));

	model = new QStringListModel(this);

	List << "Select a" << "process first";

	model->setStringList(List);
	ui.lv_scanhook->setModel(model);

	if (!InjLib->LoadingStatus())
	{
		emit injec_status(false, "The GH injection library couldn't be found or wasn't loaded correctly.");

		return;
	}
}

GuiScanHook::~GuiScanHook()
{

}

void GuiScanHook::setItem(const std::vector<std::string> & item)
{
	List.clear();

	for (const auto & i : item)
	{
		List << QString::fromStdString(i);
	}

	model->setStringList(List);
}

std::vector<int> GuiScanHook::get_selected_indices()
{
	std::vector<int> res;	

	foreach(const QModelIndex & index, ui.lv_scanhook->selectionModel()->selectedIndexes())
	{
		res.push_back(index.row());
	}

	return res;
}

void GuiScanHook::get_from_inj_to_sh(int pid, int error)
{
	m_pid = pid;
	m_err = 0;

	ui.btn_scan->setText("Scan PID " + QString::number(pid));

	emit scan_clicked();
}

void GuiScanHook::scan_clicked()
{
	update_title("Scan for hooks");

	if (m_pid == 0)
	{
		setItem({ "Please select a process" });

		return;
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);
	if (!hProc)
	{
		setItem({ "Please select a process" });

		return;
	}

	DWORD dwExitCode = STILL_ACTIVE;
	if (!GetExitCodeProcess(hProc, &dwExitCode))
	{
		setItem({ "Please select a process" });

		return;
	}

	CloseHandle(hProc);

	if (dwExitCode != STILL_ACTIVE)
	{
		setItem({ "Please select a process" });

		return;
	}

	std::vector<std::string> tempHookList;

	int fail = InjLib->ScanHook(m_pid, tempHookList);
	if (fail)
	{
		ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		setItem({ "Failed to scan for hooks" });
	}
	else if (tempHookList.empty())
	{
		ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		setItem({ "No hooks found" });
	}
	else
	{
		ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
		setItem(tempHookList);

		auto hook_count = tempHookList.size();

		if (hook_count == 1)
		{
			update_title("1 hook found");
		}
		else
		{
			update_title(QString::number(hook_count) + " hooks found");
		}
	}
}

void GuiScanHook::unhook_clicked()
{
	if (m_pid == 0)
	{
		return;
	}

	std::vector<int> selected = get_selected_indices();
	if (!selected.size())
	{
		return;
	}

	int fail = InjLib->RestoreHook(selected);
	if (fail)
	{
		injec_status(false, "Failed to restore hook(s)");
	}

	List.clear();
	model->setStringList(List);
	scan_clicked();
}

void GuiScanHook::injec_status(bool ok, const QString msg)
{
	if (ok)
	{
		QMessageBox messageBox;
		messageBox.information(0, "Success", msg);
		messageBox.setFixedSize(500, 200);
	}
	else
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", msg);
		messageBox.setFixedSize(500, 200);
	}
}

void GuiScanHook::update_title(const QString title)
{
	if (frameless_parent)
	{
		frameless_parent->setWindowTitle(title);
	}
	else
	{
		this->setWindowTitle(title);
	}
}