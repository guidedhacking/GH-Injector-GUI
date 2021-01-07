#include "pch.h"

#include "GuiScanHook.h"

GuiScanHook::GuiScanHook(QWidget * parent, FramelessWindow * FramelessParent, InjectionLib * lib)
	: QWidget(parent)
{
	ui.setupUi(this);

	InjLib = lib;

	m_PID = 0;

	frameless_parent = FramelessParent;

	ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);

	connect(ui.btn_scan, SIGNAL(clicked()), this, SLOT(scan_clicked()));
	connect(ui.btn_unhook, SIGNAL(clicked()), this, SLOT(unhook_clicked()));

	model = new QStringListModel(this);

	List << "Select a" << "process first";

	model->setStringList(List);
	ui.lv_scanhook->setModel(model);

	if (!InjLib->LoadingStatus())
	{
		emit ShowStatusbox(false, "The GH injection library couldn't be found or wasn't loaded correctly.");
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

void GuiScanHook::get_from_inj_to_sh(int PID)
{
	m_PID = PID;

	ui.btn_scan->setText("Scan PID " + QString::number(PID));

	emit scan_clicked();
}

void GuiScanHook::scan_clicked()
{
	update_title("Scan for hooks");

	if (!InjLib->LoadingStatus())
	{
		setItem({ "Injection library not loaded" });

		return;
	}

	if (m_PID == 0)
	{
		setItem({ "Please select a process" });

		return;
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_PID);
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

	bool val_ret = InjLib->ValidateInjectionFunctions(m_PID, tempHookList);
	if (!val_ret)
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
	if (!InjLib->LoadingStatus())
	{
		return;
	}

	if (m_PID == 0)
	{
		return;
	}

	std::vector<int> selected = get_selected_indices();
	if (!selected.size())
	{
		return;
	}

	bool res_ret = InjLib->RestoreInjectionFunctions(selected);
	if (!res_ret)
	{
		ShowStatusbox(false, "Failed to restore hook(s)");
	}

	List.clear();
	model->setStringList(List);
	scan_clicked();
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