#include "pch.h"

#include "GuiScanHook.h"

GuiScanHook::GuiScanHook(QWidget * parent, FramelessWindow * FramelessParent, InjectionLib * InjectionLib)
	: QWidget(parent)
{
	ui.setupUi(this);

	if (!InjectionLib)
	{
		THROW("Injection library pointer is NULL.");
	}

	m_InjectionLib		= InjectionLib;
	m_FramelessParent	= FramelessParent;

	ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);

	connect(ui.btn_scan, SIGNAL(clicked()), this, SLOT(scan_clicked()));
	connect(ui.btn_unhook, SIGNAL(clicked()), this, SLOT(unhook_clicked()));

	m_Model = new(std::nothrow) QStringListModel(this);
	if (m_Model == Q_NULLPTR)
	{
		THROW("Failed to create string list model for hook scanner.");
	}

	m_HookList << "Select a" << "process first";

	m_Model->setStringList(m_HookList);
	ui.lv_scanhook->setModel(m_Model);

	if (!m_InjectionLib->LoadingStatus())
	{
		emit StatusBox(false, "The GH injection library couldn't be found or wasn't loaded correctly.");
	}
}

GuiScanHook::~GuiScanHook()
{

}

void GuiScanHook::setItem(const std::vector<std::wstring> & item)
{
	m_HookList.clear();

	for (const auto & i : item)
	{
		m_HookList << QString::fromStdWString(i);
	}

	m_Model->setStringList(m_HookList);
}

std::vector<int> GuiScanHook::get_selected_indices() const
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

	if (!m_InjectionLib->LoadingStatus())
	{
		setItem({ L"Injection library not loaded" });
		g_print("Injection library not loaded\n");

		return;
	}

	if (m_PID == 0)
	{
		setItem({ L"Please select a process" });
		g_print("No process selected\n");

		return;
	}

	HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_PID);
	if (!hProc)
	{
		setItem({ L"Please select a process" });
		g_print("Invalid process id\n");

		return;
	}

	DWORD dwExitCode = STILL_ACTIVE;
	if (!GetExitCodeProcess(hProc, &dwExitCode))
	{
		setItem({ L"Please select a process" });
		g_print("Process doesn't exist\n");

		return;
	}

	CloseHandle(hProc);

	if (dwExitCode != STILL_ACTIVE)
	{
		setItem({ L"Please select a process" });
		g_print("Process doesn't exist\n");

		return;
	}

	std::vector<std::wstring> tempHookList;

	bool val_ret = m_InjectionLib->ValidateInjectionFunctions(m_PID, tempHookList);
	if (!val_ret)
	{
		ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		setItem({ L"Failed to scan for hooks" });
	}
	else if (tempHookList.empty())
	{
		ui.lv_scanhook->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
		setItem({ L"No hooks found" });
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
	if (!m_InjectionLib->LoadingStatus())
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

	bool res_ret = m_InjectionLib->RestoreInjectionFunctions(selected);
	if (!res_ret)
	{
		StatusBox(false, "Failed to restore hook(s)");
	}

	m_HookList.clear();
	m_Model->setStringList(m_HookList);
	scan_clicked();
}

void GuiScanHook::update_title(const QString title)
{
	if (m_FramelessParent)
	{
		m_FramelessParent->setWindowTitle(title);
	}
	else
	{
		this->setWindowTitle(title);
	}
}