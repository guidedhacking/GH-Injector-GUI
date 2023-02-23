#include "pch.h"

#include "GuiMain.h"

const int GuiMain::EXIT_CODE_CLOSE			= 0;
const int GuiMain::EXIT_CODE_REBOOT			= -1;
const int GuiMain::EXIT_CODE_UPDATE			= -2;
const int GuiMain::EXIT_CODE_START_NATIVE	= -3;

const int GuiMain::Height_small			= 410;
const int GuiMain::Height_medium_s		= 480;
const int GuiMain::Height_medium_b		= 593;
const int GuiMain::Height_big			= 643;
const int GuiMain::Height_change_delay	= 100;

GuiMain::GuiMain(QWidget * parent)
	: QMainWindow(parent)
{
	framelessParent = new(std::nothrow) FramelessWindow();
	if (framelessParent == Q_NULLPTR)
	{
		THROW("Failed to create parent window for main GUI.");
	}

	framelessParent->setTitleBar(false);
	framelessParent->setContent(this);

	g_Console = new(std::nothrow) DebugConsole(framelessParent);
	if (framelessParent == Q_NULLPTR)
	{
		THROW("Failed to create debug console.");
	}

	drag_drop = nullptr;

	current_version = GH_INJ_GUI_VERSIONW;
	newest_version	= GH_INJ_GUI_VERSIONW;

	auto root_path = QCoreApplication::applicationDirPath();
	root_path.replace("/", "\\");
	g_RootPath = root_path.toStdWString() + L"\\";

	if (!platformCheck())
	{
		exit(GuiMain::EXIT_CODE_START_NATIVE);
	}
	
	if (!ProcessIdToSessionId(GetCurrentProcessId(), &g_SessionID))
	{
		THROW("Failed to resolve session identifier\n");
	}

	if (!FileExists(GH_INJ_MOD_NAME))
	{
		QString failMsg = "Injection library missing:\n";
		failMsg += GH_INJ_MOD_NAME;

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

	if (!FileExists(GH_INJ_SM_NAME86))
	{
		QString failMsg = "Injection component missing:\n";
		failMsg += GH_INJ_SM_NAME86;

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

	if (!FileExists(GH_INJ_DNP_NAME))
	{
		QString failMsg = "Injection component missing:\n";
		failMsg += GH_INJ_DNP_NAME;

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

#ifdef _WIN64

	if (!FileExists(GH_INJ_SM_NAME64))
	{
		QString failMsg = "Injection component missing:\n";
		failMsg += GH_INJ_SM_NAME64;

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

	if (!FileExists(GH_INJ_DNP_NAME86))
	{
		QString failMsg = "Injection component missing:\n";
		failMsg += GH_INJ_DNP_NAME86;

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

#endif

	if (!InjLib.Init())
	{
		QString failMsg = "Failed to load:\n";
		failMsg += QString::fromStdString(GH_INJ_MOD_NAMEA);

		emit StatusBox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

	InjLib.SetRawPrintCallback(g_print_to_console_raw_external);
	
	g_Console->update_external();

	if (!SetDebugPrivilege(true))
	{
		emit StatusBox(false, "Failed to enable debug privileges. This might affect the functionality of the injector.");
	}

	ui.setupUi(this);

	CurrentPID = 1;

	t_Delay_Inj.setSingleShot(true);
	t_OnUserInput.setSingleShot(true);
	t_Update_DragDrop.setSingleShot(true);
	t_SetUp.setSingleShot(true);

	t_Auto_Inj.setInterval(200);
	t_Update_Proc.setInterval(250);
	
	pxm_banner	= QPixmap(":/GuiMain/gh_resource/GH Banner.png");
	pxm_lul		= QPixmap(":/GuiMain/gh_resource/LUL Icon.png");
	pxm_generic = QPixmap(":/GuiMain/gh_resource/Generic Icon.png");
	pxm_error	= QPixmap(":/GuiMain/gh_resource/Error Icon.png");

	if (pxm_banner.isNull() || pxm_lul.isNull() || pxm_generic.isNull() || pxm_error.isNull())
	{
		emit StatusBox(false, "Failed to initialize one or multiple graphic files. This won't affect the functionality of the injector.");
	}

	ui.lbl_img->setPixmap(pxm_banner);
	
	ui.lbl_proc_icon->setStyleSheet("background: transparent");

	ui.cmb_proc->setModel(&mod_CmbProcNameModel);

	auto banner_height = pxm_banner.height();
	ui.btn_close->setFixedHeight(banner_height / 2);
	ui.btn_minimize->setFixedHeight(banner_height / 2);
	ui.btn_close->setFixedWidth(50);
	ui.btn_minimize->setFixedWidth(50);
		
	ui.tree_files->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustIgnored);
	ui.tree_files->setColumnWidth(FILE_LIST_IDX_CHECKBOX, 50);
	ui.tree_files->setColumnWidth(FILE_LIST_IDX_NAME, 135);
	ui.tree_files->setColumnWidth(FILE_LIST_IDX_PATH, 272);
	ui.tree_files->setColumnWidth(FILE_LIST_IDX_PLATFORM, 50);
	ui.tree_files->setColumnWidth(FILE_LIST_IDX_BUTTON_OPTIONS, 30);

	std::string v = "V";
	v += GH_INJ_GUI_VERSIONA;
	ui.btn_version->setText(v.c_str());

	ui.btn_openlog->setIcon(QIcon(":/GuiMain/gh_resource/Log Icon.ico"));
	ui.btn_console->setIcon(QIcon(":/GuiMain/gh_resource/Console.ico"));

	QApplication::instance()->installEventFilter(this);
	
	rev_NumbersOnly.setRegularExpression(QRegularExpression("[0-9]+"));
	ui.txt_pid->setValidator(&rev_NumbersOnly);

	if (this->statusBar())
	{
		this->statusBar()->hide();
	}

	onReset			= false;
	onMove			= false;
	consoleOpen		= true;
	tooltipsEnabled = true;
	setupDone		= false;
	updateCheck		= true;

	mouse_pos = { 0, 0 };

	// Window
	connect(ui.btn_close,		SIGNAL(clicked()), this, SLOT(btn_close_clicked()));
	connect(ui.btn_minimize,	SIGNAL(clicked()), this, SLOT(btn_minimize_clicked()));

	// Settings
	connect(ui.rb_proc,		SIGNAL(clicked()), this, SLOT(rb_process_set()));
	connect(ui.rb_pid,		SIGNAL(clicked()), this, SLOT(rb_pid_set()));
	connect(ui.btn_proc,	SIGNAL(clicked()), this, SLOT(btn_pick_process_click()));

	// Auto, Reset, Color
	connect(ui.cb_auto,		SIGNAL(clicked()), this, SLOT(cb_auto_inject()));
	connect(ui.btn_reset,	SIGNAL(clicked()), this, SLOT(btn_reset_settings()));
	connect(ui.btn_hooks,	SIGNAL(clicked()), this, SLOT(btn_hook_scan_click()));

	// Method, Cloaking, Advanced
	connect(ui.cmb_load,		SIGNAL(currentIndexChanged(int)),	this, SLOT(cmb_load_change(int)));
	connect(ui.cmb_create,		SIGNAL(currentIndexChanged(int)),	this, SLOT(cmb_create_change(int)));
	connect(ui.cb_main,			SIGNAL(clicked()),					this, SLOT(cb_main_clicked()));
	connect(ui.cb_protection,	SIGNAL(clicked()),					this, SLOT(cb_page_protection_clicked()));
	connect(ui.cmb_peh,			SIGNAL(currentIndexChanged(int)),	this, SLOT(cmb_peh_change(int)));
	connect(ui.cb_cloak,		SIGNAL(clicked()),					this, SLOT(cb_cloak_clicked()));
	connect(ui.cb_hijack,		SIGNAL(clicked()),					this, SLOT(cb_hijack_clicked()));

	// Files
	connect(ui.btn_add,		SIGNAL(clicked()),									this, SLOT(btn_add_file_dialog()));
	connect(ui.btn_inject,	SIGNAL(clicked()),									this, SLOT(btn_delay_inject()));
	connect(ui.btn_remove,	SIGNAL(clicked()),									this, SLOT(btn_remove_file()));
	connect(ui.tree_files,	SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),	this, SLOT(tree_select_file()));

	// Info
	connect(ui.btn_tooltip,		SIGNAL(clicked()), this, SLOT(btn_tooltip_change()));
	connect(ui.btn_shortcut,	SIGNAL(clicked()), this, SLOT(btn_generate_shortcut()));
	connect(ui.btn_help,		SIGNAL(clicked()), this, SLOT(btn_open_help()));
	connect(ui.btn_version,		SIGNAL(clicked()), this, SLOT(btn_update_clicked()));
	connect(ui.btn_console,		SIGNAL(clicked()), this, SLOT(btn_open_console()));
	connect(ui.btn_openlog,		SIGNAL(clicked()), this, SLOT(btn_open_log()));
	
	framelessPicker.setMinimizeButton(false);
	framelessPicker.setResizeHorizontal(true);

	framelessScanner.setMinimizeButton(false);
	framelessScanner.setResizeHorizontal(true);

	gui_Picker = new(std::nothrow) GuiProcess(&framelessPicker, &framelessPicker);
	if (gui_Picker == Q_NULLPTR)
	{
		THROW("Failed to create process picker window.");
	}

	gui_Scanner	= new(std::nothrow) GuiScanHook(&framelessScanner, &framelessScanner, &InjLib);
	if (gui_Picker == Q_NULLPTR)
	{
		THROW("Failed to create hook scanner window.");
	}

	framelessPicker.setWindowTitle("Select a process");
	framelessPicker.setContent(gui_Picker);
	framelessPicker.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessPicker.setWindowModality(Qt::WindowModality::ApplicationModal);
	
	framelessScanner.setWindowTitle("Scan for hooks");
	framelessScanner.setContent(gui_Scanner);
	framelessScanner.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessScanner.setWindowModality(Qt::WindowModality::ApplicationModal);

	auto Drop_Handler = [this](const QString & path)
	{
		add_file_to_list(path, true);
	};

	current_dpi = framelessParent->logicalDpiX();
	dragdrop_size	= (int)(30.0f * current_dpi / 96.0f + 0.5f);
	dragdrop_offset = (int)(10.0f * current_dpi / 96.0f + 0.5f);

	drag_drop = new(std::nothrow) DragDropWindow();
	if (drag_drop == nullptr)
	{
		THROW("Failed to create drag & drop window.");
	}

	drag_drop->CreateDragDropWindow(reinterpret_cast<HWND>(framelessParent->winId()), dragdrop_size);
	drag_drop->SetCallback(Drop_Handler);

	// Process Picker
	connect(this,		SIGNAL(send_to_picker(ProcessState *, ProcessData *)),	gui_Picker, SLOT(get_from_inj(ProcessState *, ProcessData *)));
	connect(gui_Picker, SIGNAL(send_to_inj()), this, SLOT(get_from_picker()));

	// Scan Hook
	connect(this,			SIGNAL(send_to_scan_hook(int)),	gui_Scanner,	SLOT(get_from_inj_to_sh(int)));
	connect(gui_Scanner,	SIGNAL(send_to_inj_sh()),		this,			SLOT(get_from_scan_hook()));

	connect(&t_Auto_Inj,		SIGNAL(timeout()), this, SLOT(auto_loop_inject()));
	connect(&t_Delay_Inj,		SIGNAL(timeout()), this, SLOT(inject_file()));
	connect(&t_Update_Proc,		SIGNAL(timeout()), this, SLOT(update_process()));
	connect(&t_Update_DragDrop,	SIGNAL(timeout()), this, SLOT(update_after_height_change()));
	connect(&t_SetUp,			SIGNAL(timeout()), this, SLOT(setup()));
	connect(&t_Update_Files,	SIGNAL(timeout()), this, SLOT(update_file_list()));	

	ui.fr_cloak->setVisible(false);

	ui.fr_adv->setVisible(false);
	ui.cb_unlink->setEnabled(true);

	load_settings();

	cmb_load_change(0);
	cmb_create_change(0);

	cb_main_clicked();
	cb_page_protection_clicked();
	cb_cloak_clicked();

	cmb_peh_change(0);
	btn_tooltip_change();

	if (dockIndex == DOCK_NONE)
	{
		dockIndex = DOCK_RIGHT;
	}

	if (!g_IsNative)
	{
		// Won't work if not native
		ui.cb_hijack->setDisabled(false);
		ui.cb_hijack->setChecked(false);
		ui.cb_hijack->setDisabled(true);
		ui.cb_hijack->setToolTip("Handle hijacking is not supported when running under WOW64.");
	}

	if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows10)
	{
		// Only exists on Win10+
		auto * cmb_model = qobject_cast<QStandardItemModel *>(ui.cmb_load->model());
		if (cmb_model)
		{
			if (ui.cmb_load->currentIndex() == (int)INJECTION_MODE::IM_LdrpLoadDllInternal)
			{
				ui.cmb_load->setCurrentIndex((int)INJECTION_MODE::IM_LdrpLoadDll);
			}

			auto * it = cmb_model->item((int)INJECTION_MODE::IM_LdrpLoadDllInternal);
			it->setFlags(it->flags() & ~Qt::ItemIsEnabled);

			auto * view = qobject_cast<QListView *>(ui.cmb_load->view());
			if (view != Q_NULLPTR)
			{
				view->setRowHidden((int)INJECTION_MODE::IM_LdrpLoadDllInternal, true);
			}
		}
	}

	ui.fr_method->setStyleSheet("QFrame { border-top: 1px solid grey; }");
	ui.fr_cloak->setStyleSheet("QFrame { border-top: 1px solid grey; }");
	ui.fr_adv->setStyleSheet("QFrame { border-top: 1px solid grey; }");

	cb_auto_inject();
}

GuiMain::~GuiMain()
{
	save_settings();

	SAFE_DELETE(drag_drop);
	SAFE_DELETE(gui_Scanner);
	SAFE_DELETE(gui_Picker);
	SAFE_DELETE(g_Console);

	InjLib.InterruptDownload();
	InjLib.Unload();
}

void GuiMain::update_process()
{
	auto pid_raw	= ui.txt_pid->text().toULong();
	auto name_raw	= ui.cmb_proc->currentText();

	auto by_pid = ui.rb_pid->isChecked();
	auto by_name = ui.rb_proc->isChecked();

	bool update_broihon = false;

	if (pid_raw == 1337 && by_pid && CurrentPID != 1337)
	{
		update_broihon = true;
	}
	else if (pid_raw == 1337 && by_pid && CurrentPID == 1337)
	{
		return;
	}
	else if (name_raw.compare("Broihon.exe", Qt::CaseInsensitive) == 0 && by_name && CurrentPID != 1337)
	{
		update_broihon = true;
	}
	else if (name_raw.compare("Broihon.exe", Qt::CaseInsensitive) == 0 && by_name && CurrentPID == 1337)
	{
		return;
	}

	if (update_broihon)
	{
		CurrentPID = 1337;
		CurrentName = L"Broihon.exe";

		ui.txt_arch->setText(QString::fromUtf8("\xF0\x9F\x98\x8E") + QString::fromUtf8("\xF0\x9F\x92\xA6"));
		ui.txt_arch->setToolTip("Doin your mom doin doin your mom\nDoin your mom doin doin your mom\nDoin doin your mom doin doin your mom\nYou know we straight with doin your mom");
		ui.txt_pid->setToolTip("");
		ui.cmb_proc->setToolTip("");
		ui.cmb_proc->setEditText("Broihon.exe");
		btn_change();
		update_proc_icon();
		ui.lbl_proc_icon->setToolTip(QString::fromWCharArray(L"Praise Broihon \u2665\u2665\u2665"));

		return;
	}

	proc_data_by_name.UpdateData(name_raw.toStdWString());
	proc_data_by_pid.UpdateData(pid_raw);

	DWORD by_name_pid = 0;
	proc_data_by_name.GetProcessID(by_name_pid);

	if (!proc_data_by_pid.IsValid() && by_pid)
	{
		if (!proc_data_by_name.IsValid() && CurrentPID)
		{
			CurrentPID = 0;
			ui.txt_pid->setText("0");
			ui.txt_arch->setText("---");
			ui.txt_pid->setToolTip("");
			ui.cmb_proc->setToolTip("");
			ui.txt_arch->setToolTip("Invalid process specified.");
			ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");

			btn_change();
			update_proc_icon();
		}
		else if (proc_data_by_name.IsValid() && (CurrentPID == 0 || CurrentPID != by_name_pid))
		{
			CurrentPID = by_name_pid;
			proc_data_by_name.GetNameW(CurrentName);
			proc_data_by_name.GetFullPathW(CurrentPath);
			proc_data_by_name.GetArchitecture(CurrentArchitecture);

			cmb_proc_name_change();
			btn_change();
			update_proc_icon();
		}
	}
	else if (proc_data_by_pid.IsValid() && by_pid)
	{
		if (pid_raw != CurrentPID)
		{
			CurrentPID = pid_raw;
			proc_data_by_pid.GetNameW(CurrentName);
			proc_data_by_pid.GetFullPathW(CurrentPath);
			proc_data_by_pid.GetArchitecture(CurrentArchitecture);

			txt_pid_change();
			btn_change();
			update_proc_icon();
		}
	}
	else if (!proc_data_by_name.IsValid())
	{
		if (!name_raw.endsWith(".exe", Qt::CaseInsensitive))
		{
			QString name_raw_exe;
			name_raw_exe = name_raw + ".exe";

			ProcessData data_local(name_raw_exe.toStdWString());
			if (data_local.IsValid())
			{
				auto index = ui.cmb_proc->findText(name_raw_exe, Qt::MatchFixedString);
				if (index == -1)
				{
					ui.cmb_proc->addItem(name_raw_exe);
				}
			}
		}

		if (CurrentPID)
		{
			CurrentPID = 0;
			ui.txt_pid->setText("0");
			ui.txt_arch->setText("---");
			ui.txt_pid->setToolTip("");
			ui.cmb_proc->setToolTip("");
			ui.txt_arch->setToolTip("Invalid process specified.");
			ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");

			btn_change();
			update_proc_icon();
		}
	}
	else
	{
		if (proc_data_by_pid.IsValid())
		{
			std::wstring byPIDName;
			std::wstring byNameName;
			proc_data_by_pid.GetNameW(byPIDName);
			proc_data_by_name.GetNameW(byNameName);
			if (!strcicmpW(byNameName.c_str(), byPIDName.c_str()))
			{
				return;
			}
		}

		DWORD new_pid = 0;
		proc_data_by_name.GetProcessID(new_pid);
		if (new_pid != CurrentPID)
		{
			CurrentPID = new_pid;
			proc_data_by_name.GetNameW(CurrentName);
			proc_data_by_name.GetFullPathW(CurrentPath);
			proc_data_by_name.GetArchitecture(CurrentArchitecture);

			cmb_proc_name_change();
			btn_change();
			update_proc_icon();
		}
	}
}

void GuiMain::update_proc_icon()
{
	int size = ui.btn_proc->height();
	ui.lbl_proc_icon->setFixedSize(QSize(size, size));
	
	QPixmap new_icon = QPixmap();

	if (CurrentPID == 1337)
	{
		new_icon = pxm_lul.scaled(size, size);
	}
	else if (!CurrentPID)
	{
		new_icon = pxm_error.scaled(size, size);
	}
	else
	{
		if (!get_icon_from_file(CurrentPath, size, 0, new_icon))
		{
			new_icon = pxm_generic.scaled(size, size);
		}
	}

	ui.lbl_proc_icon->setPixmap(new_icon);
}

void GuiMain::update_file_list()
{
	QTreeWidgetItemIterator it(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		bool b_ok = false;

#ifdef _WIN64
		auto hFile = reinterpret_cast<HANDLE>((*it)->text(FILE_LIST_IDX_FILEHANDLE).toULongLong(&b_ok, 0x10));
#else
		auto hFile = reinterpret_cast<HANDLE>((*it)->text(FILE_LIST_IDX_FILEHANDLE).toULong(&b_ok, 0x10));
#endif

		if (!hFile)
		{
			b_ok = false;
		}

		auto q_path = (*it)->text(FILE_LIST_IDX_PATH);

		if (!FileExistsW(q_path.toStdWString()))
		{
			if (b_ok && hFile == INVALID_HANDLE_VALUE)
			{
				continue;
			}

			if (!b_ok)
			{
				g_print("Deleted %ls from the list (invalid handle)\n", (*it)->text(FILE_LIST_IDX_NAME).toStdWString().c_str());

				delete (*it);

				continue;
			}
			
			wchar_t new_path[MAX_PATH * 2]{ 0 };
			auto ret = GetFinalPathNameByHandleW(hFile, new_path, sizeof(new_path) / sizeof(wchar_t), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
			if (!ret || ret && GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
			{
				CloseHandle(hFile);

				g_print("Deleted %ls from the list (invalid path)\n", (*it)->text(FILE_LIST_IDX_NAME).toStdWString().c_str());

				delete (*it);

				continue;
			}

			CloseHandle(hFile);

			const wchar_t * new_path_stripped = new_path + 4; //skip DOS prefix

			std::wstring s_new_path(new_path_stripped);
			if (s_new_path.find(L"$Recycle.Bin") != std::string::npos) //detect if file was moved to recycle bin
			{
				g_print("Deleted %ls from the list\n", (*it)->text(FILE_LIST_IDX_NAME).toStdWString().c_str());

				delete (*it);
			}
			else
			{
				if (s_new_path.find(L"$Extend") != std::string::npos) //file replaced, update item with original path
				{
					g_print("File probably replaced\n");

					(*it)->setText(FILE_LIST_IDX_FILEHANDLE, QString::number(reinterpret_cast<UINT_PTR>(INVALID_HANDLE_VALUE), 0x10));
					(*it)->setData(FILE_LIST_IDX_CHECKBOX, Qt::CheckStateRole, Qt::CheckState::PartiallyChecked);
					(*it)->setDisabled(true);

					continue;
				}

				bool is_dot_net = false;
				QString dot_net_options;
				QString dot_net_argument;
				auto flag = (*it)->text(FILE_LIST_IDX_FLAG).toInt();
				if (flag != FILE_LIST_FLAG_NATIVE)
				{
					dot_net_options = (*it)->text(FILE_LIST_IDX_DOTNET_OPTIONS);
					dot_net_argument = (*it)->text(FILE_LIST_IDX_DOTNET_ARGUMENT);

					is_dot_net = true;
				}

				delete (*it);

				g_print("New path: %ls\n", new_path_stripped);
				auto new_item = add_file_to_list(QString::fromWCharArray(new_path_stripped), true, flag);

				if (is_dot_net && new_item)
				{
					new_item->setText(FILE_LIST_IDX_DOTNET_OPTIONS, dot_net_options);
					new_item->setText(FILE_LIST_IDX_DOTNET_ARGUMENT, dot_net_argument);

					DotNetOptionsTree * new_tree = nullptr;

					if (!parse_dot_net_data(new_item, new_tree))
					{
						g_print("Failed to parse .NET data of:\n%ls\n", new_item->text(FILE_LIST_IDX_NAME).toStdWString().c_str());
					}
					
					new_item->setText(FILE_LIST_IDX_DOTNET_PARSER, QString::number(reinterpret_cast<UINT_PTR>(new_tree), 0x10));
				}
			}
		}
		else if (b_ok && hFile == INVALID_HANDLE_VALUE)
		{
			std::ifstream File(q_path.toStdWString(), std::ios::binary | std::ios::ate);

			if (File.fail())
			{
				File.close();

				continue;
			}

			File.close();

			delete (*it);

			add_file_to_list(q_path, true);
		}
	}
	
	ARCHITECTURE arch;
	if (ui.rb_proc->isChecked() && proc_data_by_pid.IsValid())
	{
		proc_data_by_pid.GetArchitecture(arch);
	}
	else if (proc_data_by_name.IsValid())
	{
		proc_data_by_name.GetArchitecture(arch);
	}

	if (arch == ARCH::NONE)
	{
		return;
	}

	it = QTreeWidgetItemIterator(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		bool is_dot_net = false;
		if ((*it)->text(FILE_LIST_IDX_FLAG).toInt() != FILE_LIST_FLAG_NATIVE)
		{
			is_dot_net = true;
		}

		auto f = (*it)->flags();
		if (StrToArchW((*it)->text(FILE_LIST_IDX_PLATFORM).toStdWString()) != arch && (is_dot_net && arch == ARCH::X86 || !is_dot_net) || is_dot_net && ui.cmb_load->currentIndex() == (int)INJECTION_MODE::IM_ManualMap)
		{
			if (f & Qt::ItemFlag::ItemIsUserCheckable)
			{
				(*it)->setFlags(f ^ Qt::ItemFlag::ItemIsUserCheckable);
				(*it)->setData(0, Qt::CheckStateRole, Qt::CheckState::PartiallyChecked);
			}
		}
		else if (!(f & Qt::ItemFlag::ItemIsUserCheckable))
		{
			(*it)->setFlags(f | Qt::ItemFlag::ItemIsUserCheckable);
			(*it)->setData(FILE_LIST_IDX_CHECKBOX, Qt::CheckStateRole, Qt::CheckState::Unchecked);
		}
	}
}

void GuiMain::update_after_height_change()
{
	auto pos = ui.tree_files->header()->pos();
	pos = ui.tree_files->header()->mapToGlobal(pos);
	drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, true);

	if (g_Console->is_open())
	{
		g_Console->dock(); //dock to update console height
	}
}

void GuiMain::dot_net_options()
{
	auto list = ui.tree_files->selectedItems();
	if (list.isEmpty())
	{
		return;
	}

	QTreeWidgetItem * item = Q_NULLPTR;

	for (const auto & i : list)
	{
		if (i->text(FILE_LIST_IDX_FLAG).toInt() != FILE_LIST_FLAG_NATIVE)
		{
			item = i;

			break;
		}
	}

	if (item == Q_NULLPTR)
	{
		return;
	}

	auto old_dot_net_options = item->text(FILE_LIST_IDX_DOTNET_OPTIONS);
	auto old_options = old_dot_net_options.split('!', Qt::SkipEmptyParts);
	while (old_options.size() < 3)
	{
		old_options.push_back(QString(""));
	}
	old_options.push_back(item->text(FILE_LIST_IDX_DOTNET_ARGUMENT));

	bool use_native = false;
	if (item->text(FILE_LIST_IDX_FLAG).toInt() == FILE_LIST_FLAG_DOTNET_NATIVE)
	{
		use_native = true;
	}

	bool b_ok = false;
#ifdef _WIN64
	auto parser = reinterpret_cast<DotNetOptionsTree *>(item->text(FILE_LIST_IDX_DOTNET_PARSER).toULongLong(&b_ok, 0x10));
#else
	auto parser = reinterpret_cast<DotNetOptionsTree *>(item->text(FILE_LIST_IDX_DOTNET_PARSER).toULong(&b_ok, 0x10));
#endif

	auto wnd = new(std::nothrow) DotNetOptionsWindow(QString("Enter .NET options"), old_options, parser, use_native, framelessParent);
	if (wnd == Q_NULLPTR)
	{
		g_print("Failed to open options window");

		return;
	}

	wnd->show();
	wnd->exec();

	std::vector<QString> results;
	wnd->GetResults(results, use_native);
	if (results.size())
	{
		QString dot_net_options = "";
		for (UINT i = 0; i < 3; ++i)
		{
			dot_net_options += results[i] + '!';
		}

		item->setText(FILE_LIST_IDX_DOTNET_OPTIONS, dot_net_options);
		item->setText(FILE_LIST_IDX_DOTNET_ARGUMENT, results[3]);
	}	

	if (use_native)
	{
		item->setText(FILE_LIST_IDX_FLAG, QString::number(FILE_LIST_FLAG_DOTNET_NATIVE));
	}
	else
	{
		item->setText(FILE_LIST_IDX_FLAG, QString::number(FILE_LIST_FLAG_DOTNET));
	}

	delete wnd;
}

bool GuiMain::parse_dot_net_data(QTreeWidgetItem * item, DotNetOptionsTree *& out)
{
	auto dll_path = item->text(FILE_LIST_IDX_PATH);
	auto dll_name = item->text(FILE_LIST_IDX_NAME);
	std::wstring pipe_name = dll_name.toStdWString();
	std::wstring pipe_name_full = L"\\\\.\\pipe\\" + pipe_name;

	auto hPipe = CreateNamedPipeW(pipe_name_full.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_NOWAIT, PIPE_UNLIMITED_INSTANCES, 0x1000, 0x10000, 0, nullptr);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		g_print("Failed to create pipe: %08X\n", GetLastError());

		return false;
	}
	
	ConnectNamedPipe(hPipe, nullptr);

	auto exe_path = g_RootPath + GH_INJ_DOTNET_PARSER;
	std::wstring CmdLine = L"\"" + exe_path + L"\" \"" + dll_path.toStdWString() + L"\" \"" + pipe_name + L"\"";

	wchar_t * szCmdLine = new(std::nothrow) wchar_t[CmdLine.size() + 1]();
	if (!szCmdLine)
	{
		g_print("Memory allocation failed\n");

		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);

		return false;
	}

	CmdLine.copy(szCmdLine, CmdLine.length());

	PROCESS_INFORMATION pi{ 0 };
	STARTUPINFOW		si{ 0 };
	si.cb			= sizeof(si);
	si.dwFlags		= STARTF_USESHOWWINDOW;
	si.wShowWindow	= SW_HIDE;

	auto bRet = CreateProcessW(nullptr, szCmdLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

	delete[] szCmdLine;

	if (!bRet)
	{
		printf("CreateProcessW failed: %08X\n", GetLastError());

		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);

		return false;
	}

	DWORD dwWaitRet = WaitForSingleObject(pi.hProcess, 1000);
	if (dwWaitRet != WAIT_OBJECT_0)
	{
		printf("GH .NET Parser.exe timed out\n");

		TerminateProcess(pi.hProcess, ERROR_SUCCESS);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);

		return false;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DWORD DataSize = 0;
	bRet = PeekNamedPipe(hPipe, nullptr, NULL, nullptr, &DataSize, nullptr);
	if (!bRet)
	{
		printf("Pekk failed: %08X\n", GetLastError());
	}

	printf("DATASIZE = %08X\n", DataSize);

	char * raw_data = new(std::nothrow) char[DataSize + 1]();
	if (!raw_data)
	{
		g_print("Memory allocation failed\n");

		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);

		return false;
	}

	bRet = PeekNamedPipe(hPipe, raw_data, DataSize, nullptr, nullptr, nullptr);
	if (!bRet)
	{
		g_print("PeekNamedPipe failed: %08X\n", GetLastError());

		delete[] raw_data;

		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);

		return false;
	}

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	DotNetOptionsTree * new_tree = new(std::nothrow) DotNetOptionsTree();
	if (!new_tree)
	{
		g_print("Memory allocation failed\n");

		delete[] raw_data;

		return false;
	}

	new_tree->ParseData(raw_data);

	out = new_tree;

	return true;
}

void GuiMain::closeEvent(QCloseEvent * event)
{
	Q_UNUSED(event);

	save_settings();
}

bool GuiMain::eventFilter(QObject * obj, QEvent * event)
{
	switch (event->type())
	{
		case QEvent::KeyPress:
		{
			if (obj == ui.tree_files)
			{
				auto * keyEvent = static_cast<QKeyEvent *>(event);
				if (keyEvent->key() == Qt::Key_Delete)
				{
					btn_remove_file();
				}
				else if (keyEvent->key() == Qt::Key_Space)
				{
					toggleSelected();
				}
			}
			else if (obj == ui.txt_pid)
			{
				auto * keyEvent = static_cast<QKeyEvent *>(event);
				if (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_9 || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Space)
				{
					t_OnUserInput.start(10000);
				}
			}

			if (g_Console->is_open() && !framelessParent->isMinimized())
			{
				auto * keyEvent = static_cast<QKeyEvent *>(event);
				if (keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::Cut))
				{
					g_Console->copy_data();
				}
				else if (keyEvent->modifiers() & Qt::KeyboardModifier::ControlModifier)
				{
					int new_index = DOCK_NONE;

					switch (keyEvent->key())
					{
						case Qt::Key_3:
						case Qt::Key_Right:
							new_index = DOCK_RIGHT;
							break;

						case Qt::Key_1:
						case Qt::Key_Left:
							new_index = DOCK_LEFT;
							break;
						case Qt::Key_5:
						case Qt::Key_Up:
							new_index = DOCK_TOP;
							break;

						case Qt::Key_2:
						case Qt::Key_Down:
							new_index = DOCK_BOTTOM;
							break;

						default:
							break;
					}

					bool ignore = false;
					if ((ui.cmb_proc->hasFocus() || ui.txt_pid->hasFocus()) && (new_index == DOCK_RIGHT || new_index == DOCK_LEFT))
					{
						auto pos = 0;
						auto len = 0;

						if (ui.cmb_proc->hasFocus())
						{
							pos = ui.cmb_proc->lineEdit()->cursorPosition();
							len = ui.cmb_proc->lineEdit()->text().length();							
						}
						else
						{
							pos = ui.txt_pid->cursorPosition();
							len = ui.txt_pid->text().length();
						}

						if (pos != len && new_index == DOCK_RIGHT || pos != 0 && new_index == DOCK_LEFT)
						{
							ignore = true;
						}
					}
					
					if (!ignore && new_index != DOCK_NONE && new_index != g_Console->get_dock_index())
					{
						dockIndex = new_index;
						g_Console->dock(dockIndex);

						return true; //don't send keypress to main gui to not move focus to different control
					}
				}				
			}
		}
		break;

		case QEvent::Resize:
		{
			if (obj == ui.tree_files && !framelessParent->isMinimized())
			{
				if (drag_drop)
				{
					auto pos = ui.tree_files->header()->pos();
					pos = ui.tree_files->header()->mapToGlobal(pos);
					drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, true);
				}
			}
		}
		break;

		case QEvent::Move:
		{
			if (obj == framelessParent && drag_drop && !framelessParent->isMinimized())
			{
				auto pos = ui.tree_files->header()->pos();
				pos = ui.tree_files->header()->mapToGlobal(pos);
				drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, true);
			}
		}
		break;

		case QEvent::WindowActivate:
		{
			if (obj == framelessParent)
			{
				auto pos = ui.tree_files->header()->pos();
				pos = ui.tree_files->header()->mapToGlobal(pos);
				drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, true);
			}
		}
		break;

		case QEvent::WindowDeactivate:
		{
			if (obj == framelessParent)
			{
				auto pos = ui.tree_files->header()->pos();
				pos = ui.tree_files->header()->mapToGlobal(pos);
				drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, false);
			}
		}
		break;

		case QEvent::ApplicationStateChange:
		{
			auto * ascEvent = static_cast<QApplicationStateChangeEvent *>(event);
			if (ascEvent->applicationState() == Qt::ApplicationState::ApplicationActive && framelessParent->isVisible() && drag_drop)
			{
				auto pos = ui.tree_files->header()->pos();
				pos = ui.tree_files->header()->mapToGlobal(pos);

				if (!framelessParent->isMinimized())
				{
					drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, false);
				}
			}
			else
			{
				if (framelessParent->isMinimized() && drag_drop)
				{
					drag_drop->SetPosition(-1, -1, true, false);
				}
				else if (drag_drop)
				{
					auto pos = ui.tree_files->header()->pos();
					pos = ui.tree_files->header()->mapToGlobal(pos);
					drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, false);
				}
			}
		}
		break;

		case QEvent::WindowStateChange:
		{
			if (framelessParent->isMinimized() && drag_drop)
			{
				drag_drop->SetPosition(-1, -1, true, false);
			}
		}
		break;

		case QEvent::ScreenChangeInternal:
		case QEvent::WindowChangeInternal:
		{
			if (current_dpi != framelessParent->logicalDpiX() && drag_drop)
			{
				update_height();

				drag_drop->Close();

				current_dpi = framelessParent->logicalDpiX();
				dragdrop_size = (int)(30.0f * (float)current_dpi / 96.0f + 0.5f);
				dragdrop_offset = (int)(10.0f * (float)current_dpi / 96.0f + 0.5f);

				drag_drop->CreateDragDropWindow(reinterpret_cast<HWND>(framelessParent->winId()), dragdrop_size);

				auto pos = ui.tree_files->header()->pos();
				pos = ui.tree_files->header()->mapToGlobal(pos);
				drag_drop->SetPosition(pos.x() + ui.tree_files->width() - dragdrop_size - dragdrop_offset, pos.y() + ui.tree_files->height() - dragdrop_size - dragdrop_offset, false, false);
			}
		}
		break;

		case QEvent::Close:
		{
			if (drag_drop)
			{
				drag_drop->SetPosition(-1, -1, false, true);
			}

			if (g_Console && (obj == this || obj == framelessParent))
			{
				g_Console->close();
			}
		}
		break;

		case QEvent::MouseButtonPress:
		{
			if (obj == ui.lbl_img)
			{
				auto * mouseEvent = static_cast<QMouseEvent *>(event);

				if (mouseEvent->buttons() & Qt::MouseButton::LeftButton)
				{
					mouse_pos = ui.lbl_img->mapToGlobal(mouseEvent->pos());

					onMove = true;
				}
			}
		}
		break;

		case QEvent::MouseMove:
		{
			if (obj == ui.lbl_img && onMove)
			{
				auto * mouseEvent = static_cast<QMouseEvent *>(event);

				if (mouseEvent->buttons() & Qt::MouseButton::LeftButton)
				{
					auto glb_mousepos = ui.lbl_img->mapToGlobal(mouseEvent->pos());
					auto delta = glb_mousepos - mouse_pos;

					if (delta.x() || delta.y())
					{
						auto newpos = framelessParent->pos() + delta;
						mouse_pos = glb_mousepos;

						framelessParent->move(newpos);
					}
				}
			}
		}
		break;

		case QEvent::MouseButtonRelease:
		{
			if (obj == ui.lbl_img && onMove)
			{
				auto pos = this->mapToGlobal(QPoint(0, 0));
				if (pos.y() < 0)
				{
					auto old_pos = framelessParent->pos();
					framelessParent->move(old_pos.x(), old_pos.y() - pos.y() + 1);
				}

				onMove = false;
			}
		}
		break;
	}

	return QObject::eventFilter(obj, event);
}

void GuiMain::initSetup()
{
	t_Update_Proc.start();
	t_SetUp.start(500);
}

void GuiMain::toggleSelected()
{
	QList<QTreeWidgetItem *> sel = ui.tree_files->selectedItems();
	bool all_selected = true;

	for (auto i : sel)
	{
		if (i->checkState(0) != Qt::CheckState::Checked)
		{
			all_selected = false;
			i->setCheckState(0, Qt::CheckState::Checked);
		}
	}

	if (!all_selected)
	{
		return;
	}

	for (auto i : sel)
	{
		i->setCheckState(0, Qt::CheckState::Unchecked);
	}
}

bool GuiMain::platformCheck()
{
#ifdef _WIN64
	g_IsNative = true;

	return true;
#else
	BOOL bWOW64 = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &bWOW64))
	{
		THROW("Critical platform check failed.");
	}

	if (bWOW64)
	{
		g_IsNative = false;
	}
	else
	{
		g_IsNative = true;

		return true;
	}

	if (!YesNoBox("Architecture conflict", "Since you're using a 64-bit version of Windows it's recommended\ntouse the 64-bit version of the injector.\n\nDo you want to switch to the 64-bit version?"))
	{
		return true;
	}

	if (!FileExistsW(GH_INJ_EXE_NAME64))
	{
		StatusBox(false, "\"GH Injector - x64.exe\" is missing.\n");

		return true;
	}

	PROCESS_INFORMATION pi{ 0 };
	STARTUPINFOW		si{ 0 };
	si.cb = sizeof(si);

	auto x64_path = QCoreApplication::applicationDirPath().toStdWString();
	x64_path += L"/";
	x64_path += GH_INJ_EXE_NAME64W;

	if (!CreateProcessW(x64_path.c_str(), nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi))
	{
		auto err = GetLastError();
		QString error_msg = "CreateProcessW failed. Error code: 0x";
		QString number = QStringLiteral("%1").arg(err, 8, 0x10, QLatin1Char('0'));
		error_msg += number;

		StatusBox(false, error_msg);

		return true;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return false;
#endif // _WIN64
}

void GuiMain::rb_process_set()
{
	ui.rb_proc->setChecked(true);
	ui.cmb_proc->setEnabled(true);
	ui.txt_pid->setEnabled(false);

	if (proc_data_by_name.IsRunning())
	{
		auto pid = ui.txt_pid->text().toULong();
		if (pid != CurrentPID)
		{
			ui.txt_pid->setText(QString::number(CurrentPID));
			txt_pid_change();
		}
	}
}

void GuiMain::rb_pid_set()
{
	ui.cmb_proc->setEnabled(false);
	ui.rb_pid->setChecked(true);
	ui.txt_pid->setEnabled(true);
}

void GuiMain::rb_unset_all()
{
	ui.cmb_proc->setEnabled(false);
	ui.txt_pid->setEnabled(false);

	ui.rb_pid->setAutoExclusive(false);
	ui.rb_pid->setChecked(false);
	ui.rb_proc->setChecked(false);
	ui.rb_pid->setAutoExclusive(true);
}

void GuiMain::btn_pick_process_click()
{
	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, false);
	}

	framelessPicker.show();
	gui_Picker->show();

	emit send_to_picker(&proc_state, &proc_data_by_picker);
}

void GuiMain::cmb_proc_name_change()
{
	g_print("Attached to process:\n %ls\n %06X (%d)\n", CurrentName.c_str(), CurrentPID, CurrentPID);

	ui.txt_pid->setText(QString::number(CurrentPID));
	QString new_pid = QString::asprintf("0x%08X", CurrentPID);
	ui.txt_pid->setToolTip(new_pid);

	if (CurrentArchitecture == ARCH::X64)
	{
		ui.txt_arch->setToolTip("The target is a 64-bit process.");
	}
	else
	{
#ifdef _WIN64
		ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
#else
		if (!g_IsNative)
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
		}
		else
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process.");
		}
#endif
	}

	ui.cmb_proc->setToolTip(QString::fromStdWString(CurrentPath));
	ui.lbl_proc_icon->setToolTip(QString::fromStdWString(CurrentPath));
	ui.txt_arch->setText(QString::fromStdWString(CurrentArchitecture.ToStdWString()));

	auto index = ui.cmb_proc->findText(QString::fromStdWString(CurrentName), Qt::MatchFixedString);
	if (index == -1)
	{
		ui.cmb_proc->addItem(QString::fromStdWString(CurrentName));

		QStringList list = mod_CmbProcNameModel.stringList();
		list.sort(Qt::CaseInsensitive);
		mod_CmbProcNameModel.setStringList(list);

		index = ui.cmb_proc->findText(QString::fromStdWString(CurrentName), Qt::MatchFixedString);
	}	

	ui.cmb_proc->setCurrentIndex(index);
}

void GuiMain::txt_pid_change()
{
	g_print("Attached to process:\n %ls\n %06X (%d)\n", CurrentName.c_str(), CurrentPID, CurrentPID);

	QString new_pid = QString::asprintf("0x%08X", CurrentPID);
	ui.txt_pid->setToolTip(new_pid);

	if (CurrentArchitecture == ARCH::X64)
	{
		ui.txt_arch->setToolTip("The target is a 64-bit process.");
	}
	else
	{
#ifdef _WIN64
		ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
#else
		if (!g_IsNative)
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
		}
		else
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process.");
		}
#endif
	}

	ui.cmb_proc->setToolTip(QString::fromStdWString(CurrentPath));
	ui.lbl_proc_icon->setToolTip(QString::fromStdWString(CurrentPath));
	ui.txt_arch->setText(QString::fromStdWString(CurrentArchitecture.ToStdWString()));

	auto index = ui.cmb_proc->findText(QString::fromStdWString(CurrentName), Qt::MatchFixedString);
	if (index == -1)
	{
		ui.cmb_proc->addItem(QString::fromStdWString(CurrentName));
		
		QStringList list = mod_CmbProcNameModel.stringList();
		list.sort(Qt::CaseInsensitive);
		mod_CmbProcNameModel.setStringList(list);

		index = ui.cmb_proc->findText(QString::fromStdWString(CurrentName), Qt::MatchFixedString);
	}

	ui.cmb_proc->setCurrentIndex(index);
}

void GuiMain::btn_change()
{
	if (!InjLib.LoadingStatus() || InjLib.GetSymbolState() != INJ_ERR_SUCCESS || InjLib.GetImportState() != INJ_ERR_SUCCESS)
	{
		ui.btn_inject->setEnabled(false);
		ui.cb_auto->setEnabled(false);
		ui.cb_auto->setChecked(false);
		ui.btn_hooks->setEnabled(false);

		return;
	}

	ProcessData current_data(CurrentPID);

	if (!current_data.IsRunning())
	{
		ui.btn_hooks->setEnabled(false);
		ui.btn_inject->setEnabled(false);
	}
	else
	{
		ui.btn_hooks->setEnabled(true);
		ui.btn_inject->setEnabled(true);
	}

	if (CurrentPID == 1337)
	{
		ui.btn_inject->setEnabled(true);
	}
}

void GuiMain::get_from_picker()
{
	framelessPicker.hide();

	if (proc_data_by_picker.IsValid())
	{
		DWORD new_pid = 0;
		proc_data_by_picker.GetProcessID(new_pid);
		rb_unset_all();
		ui.txt_pid->setText(QString::number(new_pid));
		rb_pid_set();

		update_process();
	}

	btn_change();

	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, true);
	}
}

void GuiMain::get_from_scan_hook()
{
	framelessPicker.hide();

	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, true);
	}
}

void GuiMain::cb_auto_inject()
{
	if (ui.cb_auto->isChecked())
	{
		// Restart if running
		t_Auto_Inj.start(200);
	}
	else
	{
		t_Auto_Inj.stop();
	}
}

void GuiMain::auto_loop_inject()
{
	if (ui.cb_auto->isChecked())
	{
		DWORD			pid	= 0;
		ARCHITECTURE	proc_arch = ARCH::NONE;

		if (ui.rb_proc->isChecked() && proc_data_by_name.IsValid())
		{
			proc_data_by_name.GetProcessID(pid);
			proc_data_by_name.GetArchitecture(proc_arch);
		}
		else if (proc_data_by_pid.IsValid())
		{
			proc_data_by_pid.GetProcessID(pid);
			proc_data_by_pid.GetArchitecture(proc_arch);
		}

		if (!pid || proc_arch == ARCH::NONE)
		{
			return;
		}

		bool found = false;

		QTreeWidgetItemIterator it(ui.tree_files);
		for (; (*it) != Q_NULLPTR; ++it)
		{
			if ((*it)->checkState(FILE_LIST_IDX_CHECKBOX) != Qt::CheckState::Checked)
			{
				continue;
			}

			if (!FileExistsW((*it)->text(FILE_LIST_IDX_PATH).toStdWString()))
			{
				continue;
			}

			auto file_arch = StrToArchW((*it)->text(FILE_LIST_IDX_PLATFORM).toStdWString());
			if (file_arch == ARCH::NONE)
			{
				continue;
			}

			bool is_dot_net = false;
			if ((*it)->text(FILE_LIST_IDX_FLAG).toInt() == FILE_LIST_FLAG_DOTNET)
			{
				is_dot_net = true;
			}

			if (proc_arch != file_arch && (!is_dot_net || proc_arch == ARCH::X86))
			{
				continue;
			}

			if (is_dot_net)
			{
				auto dot_net_options		= (*it)->text(FILE_LIST_IDX_DOTNET_OPTIONS);
				auto dot_net_options_list	= dot_net_options.split('!', Qt::SkipEmptyParts);

				if (dot_net_options_list.length() != 3)
				{
					continue;
				}
			}			

			found = true;
			break;
		}

		if (!found)
		{
			return;
		}

		ui.cb_auto->setChecked(false);
		t_Auto_Inj.stop();

		emit btn_delay_inject();
	}
}

void GuiMain::btn_reset_settings()
{
	if (!YesNoBox("Reset", "Are you sure you want to reset all settings?", framelessParent))
	{
		return;
	}

	onReset = true;

	QFile iniFile(QString::fromStdWString(GH_SETTINGS_INIW));
	if (iniFile.exists())
	{
		iniFile.remove();
	}

	emit reboot();
}

void GuiMain::reboot()
{
	qApp->exit(GuiMain::EXIT_CODE_REBOOT);
}

void GuiMain::btn_close_clicked()
{
	save_settings();

	qApp->exit(GuiMain::EXIT_CODE_CLOSE);
}

void GuiMain::btn_minimize_clicked()
{
	framelessParent->on_minimizeButton_clicked();
}

void GuiMain::btn_hook_scan_click()
{
	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, false);
	}

	framelessScanner.show();
	gui_Scanner->show();

	auto PID = ui.txt_pid->text().toULong();

	g_print("Open hook scanner with PID = %06X\n", PID);

	emit send_to_scan_hook(PID);
}

void GuiMain::update_height()
{
	bool b1 = ui.fr_cloak->isVisible();
	bool b2 = ui.fr_adv->isVisible();

	if (!b1 && !b2)
	{
		framelessParent->setFixedHeight(Height_small);
	}
	else if (b1 && !b2)
	{
		framelessParent->setFixedHeight(Height_medium_s);
	}
	else if (!b1 && b2)
	{
		framelessParent->setFixedHeight(Height_medium_b);
	}
	else
	{
		framelessParent->setFixedHeight(Height_big);
	}

	//force window update beacause it doesn't update properly when decreasing the size after launch but all the other times???
	framelessParent->move(framelessParent->pos());

	t_Update_DragDrop.start(Height_change_delay);
}

void GuiMain::save_settings()
{
	if (onReset)
	{
		onReset = false;

		return;
	}

	g_print(" Saving settings\n");

	QSettings settings(QString::fromStdString(GH_SETTINGS_INIA), QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.beginWriteArray("FILES");

	QTreeWidgetItemIterator it(ui.tree_files);
	for (int i = 0; (*it) != Q_NULLPTR; ++it, ++i)
	{
		if (!FileExistsW((*it)->text(FILE_LIST_IDX_PATH).toStdWString()))
		{
			continue;
		}

		settings.setArrayIndex(i);
		settings.setValue(QString::number(FILE_SETTINGS_IDX_PATH), (*it)->text(FILE_LIST_IDX_PATH));
		settings.setValue(QString::number(FILE_SETTINGS_IDX_CHECKED), (*it)->checkState(FILE_LIST_IDX_CHECKBOX) != Qt::CheckState::Unchecked);

		auto flag = (*it)->text(FILE_LIST_IDX_FLAG).toInt();
		settings.setValue(QString::number(FILE_SETTINGS_IDX_FLAG), flag);

		if (flag == FILE_LIST_FLAG_NATIVE)
		{
			continue;
		}

		auto dot_net_options = (*it)->text(FILE_LIST_IDX_DOTNET_OPTIONS);
		if (dot_net_options.length())
		{
			settings.setValue(QString::number(FILE_SETTINGS_IDX_DOTNET_OPTIONS), dot_net_options);
		}

		auto dot_net_argument = (*it)->text(FILE_LIST_IDX_DOTNET_ARGUMENT);
		if (dot_net_argument.length())
		{
			settings.setValue(QString::number(FILE_SETTINGS_IDX_DOTNET_ARGUMENT), dot_net_argument);
		}
	}
	settings.endArray();

	settings.beginWriteArray("PROCESS");
	for (int i = 0; i < ui.cmb_proc->count(); i++)
	{
		auto val = ui.cmb_proc->itemText(i);
		if (!val.length())
		{
			continue;
		}

		settings.setArrayIndex(i);
		settings.setValue(QString::number(0), ui.cmb_proc->itemText(i));
	}
	settings.endArray();

	settings.beginGroup("CONFIG");

	// Settings
	settings.setValue("PROCESS", ui.cmb_proc->currentIndex());
	settings.setValue("PID", ui.txt_pid->text());
	settings.setValue("PROCESSBYNAME", ui.rb_proc->isChecked());
	settings.setValue("ARCH", ui.txt_arch->text());
	settings.setValue("DELAY", ui.sp_delay->value());
	settings.setValue("AUTOINJ", ui.cb_auto->isChecked());
	settings.setValue("CLOSEONINJ", ui.cb_close->isChecked());
	settings.setValue("TIMEOUT", ui.sp_timeout->value());
	settings.setValue("ERRORLOG", ui.cb_error->isChecked());

	// Method
	settings.setValue("MODE", ui.cmb_load->currentIndex());
	settings.setValue("LAUNCHMETHOD", ui.cmb_create->currentIndex());
	settings.setValue("CLOAK", ui.cb_cloak->isChecked());

	if (ui.cb_cloak->isChecked())
	{
		settings.setValue("THREADATTACH", ui.cb_threadAttach->isChecked());
		settings.setValue("THREADHIDE", ui.cb_threadHide->isChecked());
		settings.setValue("THREADSTART", ui.cb_threadStart->isChecked());
		settings.setValue("THREADTID", ui.cb_threadTID->isChecked());
	}

	if (g_IsNative)
	{
		settings.setValue("HIJACK", ui.cb_hijack->isChecked());
	}

	// Cloaking
	settings.setValue("PEH", ui.cmb_peh->currentIndex());
	settings.setValue("UNLINKPEB", ui.cb_unlink->isChecked());
	settings.setValue("RANDOMIZE", ui.cb_random->isChecked());
	settings.setValue("DLLCOPY", ui.cb_copy->isChecked());

	// manual mapping
	settings.setValue("CLEANDIR", ui.cb_clean->isChecked());
	settings.setValue("INITCOOKIE", ui.cb_cookie->isChecked());
	settings.setValue("IMPORTS", ui.cb_imports->isChecked());
	settings.setValue("DELAYIMPORTS", ui.cb_delay->isChecked());
	settings.setValue("TLS", ui.cb_tls->isChecked());
	settings.setValue("EXCEPTION", ui.cb_seh->isChecked());
	settings.setValue("PROTECTION", ui.cb_protection->isChecked());
	settings.setValue("DLLMAIN", ui.cb_main->isChecked());
	settings.setValue("LDRLOCK", ui.cb_ldrlock->isChecked());
	settings.setValue("SHIFT", ui.cb_shift->isChecked());
	settings.setValue("MEMORY", ui.cb_memory->isChecked());
	//settings.setValue("LINK", ui.cb_link->isChecked());

	// Process picker
	settings.setValue("PROCNAMEFILTER", proc_state.txtFilter);
	settings.setValue("PROCESSTYPE", proc_state.cmbArch);
	settings.setValue("CURRENTSESSION", proc_state.cbSession);

	// Info
	settings.setValue("TOOLTIPSON", tooltipsEnabled);
	settings.setValue("IGNOREUPDATES", ignoreUpdate);
	settings.setValue("CONSOLE", g_Console->isVisible());
	settings.setValue("DOCKINDEX", g_Console->get_dock_index());
	settings.setValue("HIJACKWARNING", hijackWarning);

	// Not visible
	settings.setValue("LASTDIR", lastPathStr);
	settings.setValue("GEOMETRY", framelessParent->saveGeometry());	

	settings.endGroup();

	g_print(" Settings saved successfully\n");
}

void GuiMain::load_settings()
{
	g_print(" Loading settings\n");

	QFile iniFile(QString::fromStdString(GH_SETTINGS_INIA));
	if (!iniFile.exists())
	{
		default_settings();

		return;
	}

	QSettings settings(QString::fromStdString(GH_SETTINGS_INIA), QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	int fileSize = settings.beginReadArray("FILES");
	for (int i = 0; i < fileSize; ++i)
	{
		settings.setArrayIndex(i);

		auto path = settings.value(QString::number(FILE_SETTINGS_IDX_PATH), "").toString();
		auto flag = settings.value(QString::number(FILE_SETTINGS_IDX_FLAG), "").toInt();

		auto new_item = add_file_to_list(path, settings.value(QString::number(FILE_SETTINGS_IDX_CHECKED)).toBool(), flag);
		if (!new_item)
		{
			continue;
		}

		if (flag == FILE_LIST_FLAG_NATIVE)
		{
			continue;
		}

		auto dot_net_options = settings.value(QString::number(FILE_SETTINGS_IDX_DOTNET_OPTIONS), "").toString();
		if (dot_net_options.length() != 0)
		{
			new_item->setText(FILE_LIST_IDX_DOTNET_OPTIONS, dot_net_options);
		}

		auto dot_net_argument = settings.value(QString::number(FILE_SETTINGS_IDX_DOTNET_ARGUMENT), "").toString();
		if (dot_net_argument.length() != 0)
		{
			new_item->setText(FILE_LIST_IDX_DOTNET_ARGUMENT, dot_net_argument);
		}

		DotNetOptionsTree * new_tree = nullptr;

		if (!parse_dot_net_data(new_item, new_tree))
		{
			g_print("Failed to parse .NET data of:\n%ls\n", new_item->text(FILE_LIST_IDX_NAME).toStdWString().c_str());
		}

		new_item->setText(FILE_LIST_IDX_DOTNET_PARSER, QString::number(reinterpret_cast<UINT_PTR>(new_tree), 0x10));
	}
	settings.endArray();

	int procSize = settings.beginReadArray("PROCESS");
	for (int i = 0; i < procSize; ++i)
	{
		settings.setArrayIndex(i);
		auto val = settings.value(QString::number(0), "").toString();
		if (!val.length())
		{
			continue;
		}

		ui.cmb_proc->addItem(val);
	}

	settings.endArray();

	settings.beginGroup("CONFIG");

	if (procSize)
	{
		ui.cmb_proc->setCurrentIndex(settings.value("PROCESS").toInt());
	}

	// Settings
	ui.txt_pid->setText(settings.value("PID", "0").toString());
	ui.txt_arch->setText(settings.value("ARCH", "").toString());
	ui.sp_delay->setValue(settings.value("DELAY", 0).toInt());
	ui.cb_auto->setChecked(settings.value("AUTOINJ", false).toBool());
	ui.cb_close->setChecked(settings.value("CLOSEONINJ", false).toBool());
	ui.sp_timeout->setValue(settings.value("TIMEOUT", 2000).toInt());
	ui.cb_error->setChecked(settings.value("ERRORLOG", true).toBool());

	if (settings.value("PROCESSBYNAME", true).toBool())
	{
		rb_process_set();
		ui.txt_pid->setText(QString::number(0));
	}
	else
	{
		rb_pid_set();
	}

	// Method
	ui.cmb_load->setCurrentIndex(settings.value("MODE", 0).toInt());
	ui.cmb_create->setCurrentIndex(settings.value("LAUNCHMETHOD", 0).toInt());
	ui.cb_hijack->setChecked(settings.value("HIJACK", false).toBool());
	ui.cb_cloak->setChecked(settings.value("CLOAK", false).toBool());

	if (ui.cb_cloak->isChecked())
	{
		ui.cb_threadAttach->setChecked(settings.value("THREADATTACH", false).toBool());
		ui.cb_threadHide->setChecked(settings.value("THREADHIDE", true).toBool());
		ui.cb_threadStart->setChecked(settings.value("THREADSTART", true).toBool());
		ui.cb_threadTID->setChecked(settings.value("THREADTID", false).toBool());
	}

	// Cloaking
	ui.cmb_peh->setCurrentIndex(settings.value("PEH", 0).toInt());
	ui.cb_unlink->setChecked(settings.value("UNLINKPEB", false).toBool());
	ui.cb_random->setChecked(settings.value("RANDOMIZE", false).toBool());
	ui.cb_copy->setChecked(settings.value("DLLCOPY", false).toBool());

	// manual mapping
	ui.cb_clean->setChecked(settings.value("CLEANDIR", false).toBool());
	ui.cb_cookie->setChecked(settings.value("INITCOOKIE", false).toBool());
	ui.cb_imports->setChecked(settings.value("IMPORTS", false).toBool());
	ui.cb_delay->setChecked(settings.value("DELAYIMPORTS", false).toBool());
	ui.cb_tls->setChecked(settings.value("TLS", false).toBool());
	ui.cb_seh->setChecked(settings.value("EXCEPTION", false).toBool());
	ui.cb_protection->setChecked(settings.value("PROTECTION", false).toBool());
	ui.cb_main->setChecked(settings.value("DLLMAIN", false).toBool());
	ui.cb_ldrlock->setChecked(settings.value("LDRLOCK", false).toBool());
	ui.cb_shift->setChecked(settings.value("SHIFT", false).toBool());
	ui.cb_memory->setChecked(settings.value("MEMORY", false).toBool());
	//ui.cb_link->setChecked(settings.value("LINK", false).toBool());

	// Process picker
	proc_state.txtFilter	= settings.value("PROCNAMEFILTER", "").toString();
	proc_state.cmbArch		= settings.value("PROCESSTYPE", 0).toInt();
	proc_state.cbSession	= settings.value("CURRENTSESSION", true).toBool();

	// Info
	tooltipsEnabled = !settings.value("TOOLTIPSON", true).toBool();
	consoleOpen		= settings.value("CONSOLE", true).toBool();
	dockIndex		= settings.value("DOCKINDEX", DOCK_RIGHT).toInt();
	ignoreUpdate	= settings.value("IGNOREUPDATES", false).toBool();
	hijackWarning	= settings.value("HIJACKWARNING", true).toBool();

	// Not visible
	lastPathStr = settings.value("LASTDIR", "").toString();
	framelessParent->restoreGeometry(settings.value("GEOMETRY").toByteArray());

	auto old_second_count = settings.value("UPDATECHECK", 0).toLongLong();
	auto current_second_count = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (current_second_count - old_second_count < 86400) //ignore leap/smear second (86400 seconds = 1 day)
	{
		updateCheck = false;
	}
	else
	{
		settings.setValue("UPDATECHECK", current_second_count);
	}

	settings.endGroup();

	g_print(" Settings loaded successfully\n");
}

void GuiMain::default_settings()
{
	g_print("  No settings found\n  Loading default configuration\n");

	lastPathStr = QApplication::applicationDirPath();
	ui.cmb_proc->setEditText("Broihon.exe");
	ui.txt_pid->setText("1337");
	ignoreUpdate			= false;
	tooltipsEnabled			= false;
	dockIndex				= DOCK_RIGHT;
	proc_state.cbSession	= true;
	hijackWarning			= true;

	save_settings();
}

void GuiMain::cmb_load_change(int index)
{
	Q_UNUSED(index);

	INJECTION_MODE mode = (INJECTION_MODE)ui.cmb_load->currentIndex();
	switch (mode)
	{
		case INJECTION_MODE::IM_LdrLoadDll:
			ui.cmb_load->setToolTip("LdrLoadDll is an advanced injection method which uses LdrLoadDll and bypasses LoadLibrary(Ex) hooks.");
			break;

		case INJECTION_MODE::IM_LdrpLoadDll:
			ui.cmb_load->setToolTip("LdrpLoadDll is an advanced injection method which uses LdrpLoadDll and bypasses LdrLoadDll hooks.");
			break;

		case INJECTION_MODE::IM_LdrpLoadDllInternal:
			ui.cmb_load->setToolTip("LdrpLoadDllInternal is an experimental injection method which uses LdrpLoadDllInternal.");
			break;

		case INJECTION_MODE::IM_ManualMap:
			ui.cmb_load->setToolTip("ManualMap is an advanced injection technique which bypasses most module detection methods.");
			break;

		default:
			ui.cmb_load->setToolTip("LoadLibraryExW is the default injection method which simply uses LoadLibraryExW to load the dll(s).");
			break;
	}

	if (mode != INJECTION_MODE::IM_ManualMap && ui.fr_adv->isVisible())
	{
		ui.fr_adv->setVisible(false);
		ui.cb_unlink->setEnabled(true);		

		update_height();
	}
	else if (mode == INJECTION_MODE::IM_ManualMap && ui.fr_adv->isHidden())
	{	
		ui.fr_adv->setVisible(true);
		ui.cb_unlink->setEnabled(false);
		ui.cb_unlink->setChecked(false);
		cb_main_clicked();
		cb_page_protection_clicked();

		update_height();
	}
}

void GuiMain::cmb_create_change(int index)
{
	Q_UNUSED(index);

	LAUNCH_METHOD method = (LAUNCH_METHOD)ui.cmb_create->currentIndex();

	switch (method)
	{
		case LAUNCH_METHOD::LM_HijackThread:
			ui.cmb_create->setToolTip("Thread hijacking: Redirects a thread to a codecave to load the dll(s).");
			break;

		case LAUNCH_METHOD::LM_SetWindowsHookEx:
			ui.cmb_create->setToolTip("SetWindowsHookEx: Adds a hook into the window callback list which then loads the dll(s).");
			break;

		case LAUNCH_METHOD::LM_KernelCallback:
			ui.cmb_create->setToolTip("KernelCallback: Replaces the __fnCOPYDATA function from the kernel callback table to execute the codecave which then loads the dll(s).");
			break;

		case LAUNCH_METHOD::LM_QueueUserAPC:
			ui.cmb_create->setToolTip("QueueUserAPC: Registers an asynchronous procedure call to the process' threads which then loads the dll(s).");
			break;

		case LAUNCH_METHOD::LM_FakeVEH:
			ui.cmb_create->setToolTip("FakeVEH: Creates and registers a fake VEH which then loads the dll(s) after a page guard exception has been triggered.");
			break;

		default:
			ui.cmb_create->setToolTip("NtCreateThreadEx: Creates a simple remote thread to load the dll(s).");
			break;
	}

	if (method == LAUNCH_METHOD::LM_NtCreateThreadEx && !ui.cb_cloak->isEnabled())
	{
		ui.cb_cloak->setEnabled(true);
		cb_cloak_clicked();
	}
	else if (method != LAUNCH_METHOD::LM_NtCreateThreadEx && ui.cb_cloak->isEnabled())
	{
		ui.cb_cloak->setEnabled(false);
		ui.cb_cloak->setChecked(false);
		cb_cloak_clicked();
	}
}

void GuiMain::cmb_peh_change(int index)
{
	index = ui.cmb_peh->currentIndex();
	switch (index)
	{
		case 0:
			ui.cmb_peh->setToolTip("Keep PEH: Doesn't modify the PE header of the dll(s).");
			break;

		case 1:
			ui.cmb_peh->setToolTip("Erase PEH: Erases the PE header by wrting 0's to it to avoid detections.");
			break;

		default:
			ui.cmb_peh->setToolTip("Fake PEH: Replaces the PE header with the PE header of the ntdll.dll.");
			break;
	}
}

void GuiMain::cb_main_clicked()
{
	if (ui.cb_main->isChecked())
	{
		ui.cb_imports->setEnabled(false);
		ui.cb_imports->setChecked(true);
	}
	else
	{
		ui.cb_imports->setEnabled(true);
	}
}

void GuiMain::cb_page_protection_clicked()
{
	if (ui.cb_protection->isChecked())
	{
		ui.cb_clean->setEnabled(false);
		ui.cb_clean->setChecked(false);

		ui.cb_shift->setEnabled(false);
		ui.cb_shift->setChecked(false);
	}
	else
	{
		ui.cb_clean->setEnabled(true);
		ui.cb_shift->setEnabled(true);
	}
}

void GuiMain::cb_cloak_clicked()
{
	if (ui.cb_cloak->isChecked() && !ui.fr_cloak->isVisible())
	{
		ui.fr_cloak->setVisible(true);

		update_height();
	}
	else if (!ui.cb_cloak->isChecked() && ui.fr_cloak->isVisible())
	{
		ui.fr_cloak->setVisible(false);

		update_height();
	}
}

void GuiMain::cb_hijack_clicked()
{
	if (hijackWarning && ui.cb_hijack->isChecked())
	{
		auto ret = YesNoBox("Warning", "This option will try to hijack a handle from another process\nwhich can be a system process.\nUnder rare circumstances this can cause a system crash.\nDo you want to enable this option anyway?", framelessParent, QMessageBox::Icon::Warning);
		if (!ret)
		{
			ui.cb_hijack->setChecked(false);
		}
		else
		{
			hijackWarning = false;
		}
	}
}

void GuiMain::btn_add_file_dialog()
{
	QFileDialog fDialog(this, "Select dll files", lastPathStr, "Dynamic Link Libraries (*.dll)");
	fDialog.setFileMode(QFileDialog::ExistingFiles);

	if (fDialog.exec() != QDialog::Accepted)
	{
		return;
	}

	if (fDialog.selectedFiles().empty())
	{
		return;
	}

	for (auto & l : fDialog.selectedFiles())
	{
		add_file_to_list(l, true);
	}

	lastPathStr = QFileInfo(fDialog.selectedFiles().first()).path();
}

QTreeWidgetItem * GuiMain::add_file_to_list(QString path, bool active, int flag)
{
#ifndef _WIN64
	if (!g_IsNative)
	{
		path.replace(":/Windows/System32/", ":/Windows/Sysnative/", Qt::CaseSensitivity::CaseInsensitive);
	}
#else
	path.replace(":/Windows/Sysnative/", ":/Windows/System32/", Qt::CaseSensitivity::CaseInsensitive);
#endif

	QTreeWidgetItemIterator it(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		if ((*it)->text(FILE_LIST_IDX_PATH) == path)
		{
			g_print("File already in the list: %ls\n", path.toStdWString().c_str());

			return (*it);
		}
	}

	QFileInfo fi(path);
	auto abs_path = fi.absoluteFilePath().toStdWString();

	bool is_dot_net = false;

	auto arch = GetFileArchitectureW(abs_path.c_str(), is_dot_net);
	if (arch == ARCH::NONE)
	{
		return Q_NULLPTR;
	}

	HANDLE hFile = CreateFileW(abs_path.c_str(), SYNCHRONIZE, FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		g_print("CreateFileW failed: %08X\n", GetLastError());

		return Q_NULLPTR;
	}

	auto * item = new(std::nothrow) QTreeWidgetItem(ui.tree_files);
	if (item == Q_NULLPTR)
	{
		g_print("Failed to create new list item\n");

		CloseHandle(hFile);

		return Q_NULLPTR;
	}

	item->setCheckState(FILE_LIST_IDX_CHECKBOX, Qt::CheckState::Unchecked);
	item->setText(FILE_LIST_IDX_NAME, fi.fileName());
	item->setText(FILE_LIST_IDX_PATH, fi.absoluteFilePath());
	item->setText(FILE_LIST_IDX_PLATFORM, QString::fromStdWString(arch.ToStdWString()));
	item->setText(FILE_LIST_IDX_FLAG, QString::number(flag));

	if (is_dot_net)
	{
		auto btn_options = new(std::nothrow) QPushButton(this);
		if (btn_options == Q_NULLPTR)
		{
			g_print("Failed to create a button\n");

			SAFE_DELETE(item);
			CloseHandle(hFile);

			return Q_NULLPTR;
		}

		btn_options->setFixedWidth(20);
		btn_options->setFixedHeight(20);
		btn_options->setIcon(QIcon(":/GuiMain/gh_resource/cog.ico"));
		btn_options->setToolTip("Configure .NET launch options");
		
		ui.tree_files->setItemWidget(item, FILE_LIST_IDX_BUTTON_OPTIONS, btn_options);
		connect(btn_options, SIGNAL(clicked()), this, SLOT(dot_net_options()));

		if (flag == FILE_LIST_FLAG_NATIVE)
		{
			item->setText(FILE_LIST_IDX_FLAG, QString::number(FILE_LIST_FLAG_DOTNET));
		}

		DotNetOptionsTree * new_tree = nullptr;

		if (!parse_dot_net_data(item, new_tree))
		{
			g_print("Failed to parse .NET data of:\n%ls\n", item->text(FILE_LIST_IDX_NAME).toStdWString().c_str());
		}

		item->setText(FILE_LIST_IDX_DOTNET_PARSER, QString::number(reinterpret_cast<UINT_PTR>(new_tree), 0x10));
	}

	item->setText(FILE_LIST_IDX_FILEHANDLE, QString::number(reinterpret_cast<UINT_PTR>(hFile), 0x10));

	if (active)
	{
		item->setCheckState(0, Qt::CheckState::Checked);
	}

	if (ui.btn_tooltip->isChecked())
	{
		item->setToolTip(FILE_LIST_IDX_NAME, fi.fileName());
		item->setToolTip(FILE_LIST_IDX_PATH, fi.absoluteFilePath());
	}

#ifndef _WIN64
	if (arch != ARCH::X86)
	{
		item->setHidden(true);
		item->setCheckState(FILE_LIST_IDX_CHECKBOX, Qt::CheckState::Unchecked);
	}
#endif

	g_print("Added file: %ls\n", abs_path.c_str());

	return item;
}

void GuiMain::btn_remove_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	for (auto i : item)
	{
		g_print("Removed file: %ls\n", i->text(FILE_LIST_IDX_PATH).toStdWString().c_str());

		bool b_ok = false;

#ifdef _WIN64
		HANDLE hFile = (HANDLE)(i->text(FILE_LIST_IDX_FILEHANDLE).toULongLong(&b_ok, 0x10));
#else
		HANDLE hFile = (HANDLE)(i->text(FILE_LIST_IDX_FILEHANDLE).toULong(&b_ok, 0x10));
#endif

		if (b_ok)
		{
			CloseHandle(hFile);
		}

		if (i->text(FILE_LIST_IDX_FLAG) != FILE_LIST_FLAG_NATIVE)
		{
			b_ok = false;

#ifdef _WIN64
			auto parser = reinterpret_cast<DotNetOptionsTree *>(i->text(FILE_LIST_IDX_DOTNET_PARSER).toULongLong(&b_ok, 0x10));
#else
			auto parser = reinterpret_cast<DotNetOptionsTree *>(i->text(FILE_LIST_IDX_DOTNET_PARSER).toULong(&b_ok, 0x10));
#endif

			SAFE_DELETE(parser);
		}

		delete i;
	}
}

void GuiMain::tree_select_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	if (item.size() == 0)
	{
		return;
	}

	auto path = item[0]->text(FILE_LIST_IDX_PATH);
	path.replace('/', '\\');
	
	g_print("Opening explorer: %ls\n", path.toStdWString().c_str());

	//stolen from here:
	//https://stackoverflow.com/questions/15300999/open-windows-explorer-directory-select-a-specific-file-in-delphi
	//by user Andreas Rejbrand
	//Danke dir, Andi!

	std::wstring open_cmd = L"/Select,";
	open_cmd += path.toStdWString();

	auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(NULL, nullptr, L"explorer.exe", open_cmd.c_str(), nullptr, SW_SHOW));
	if (ret <= 32)
	{
		g_print("Failed to open explorer:\n Error 1: %08X\n Error 2: %08X\n", ret, GetLastError());
	}
}

void GuiMain::btn_delay_inject()
{
	int id = ui.txt_pid->text().toInt();
	if (id == 1337)
	{
		std::vector<std::wstring> epic_links;
		epic_links.push_back(L"https://www.youtube.com/watch?v=mFyUrebJbDg");
		epic_links.push_back(L"https://www.youtube.com/watch?v=5t53TcKIlMc");
		epic_links.push_back(L"https://www.youtube.com/watch?v=AdfMVPNgNnc");
		epic_links.push_back(L"https://www.youtube.com/watch?v=OPaCXU4mwR8");
		epic_links.push_back(L"https://www.youtube.com/watch?v=-fGKrYq8_dk");
		epic_links.push_back(L"https://www.youtube.com/watch?v=0f3-tye6ldg");
		epic_links.push_back(L"https://www.youtube.com/watch?v=DOodQ14CEuo");
		epic_links.push_back(L"https://www.youtube.com/watch?v=ZZ5LpwO-An4");
		epic_links.push_back(L"https://www.youtube.com/watch?v=J-txgZrumpc");
		epic_links.push_back(L"https://www.youtube.com/watch?v=AcH7TUBz5-M");
		epic_links.push_back(L"https://www.youtube.com/watch?v=hzwf3H2bmAM");
		epic_links.push_back(L"https://www.youtube.com/watch?v=a-5VCZyAMz0");

		srand(GetTickCount64() & MAXUINT);
		int epic_index = rand() % epic_links.size();
		ShellExecuteW(0, 0, epic_links[epic_index].c_str(), 0, 0, SW_SHOW);

		return;
	}

	int delay = ui.sp_delay->value();
	if (delay > 0)
	{
		t_Delay_Inj.start(delay);
	}
	else
	{
		emit inject_file();
	}
}

void GuiMain::inject_file()
{
	INJECTIONDATAW inj_data;
	memset(&inj_data, 0, sizeof(INJECTIONDATAW));

	DOTNET_INJECTIONDATAW dot_net_inj_data;
	memset(&dot_net_inj_data, 0, sizeof(DOTNET_INJECTIONDATAW));

	MEMORY_INJECTIONDATA memory_inj_data;
	memset(&memory_inj_data, 0, sizeof(MEMORY_INJECTIONDATA));

	ARCHITECTURE file_arch;
	ARCHITECTURE proc_arch;

	if (ui.rb_pid->isChecked())
	{
		if (proc_data_by_pid.IsValid())
		{
			proc_data_by_pid.GetProcessID(inj_data.ProcessID);
			proc_data_by_pid.GetArchitecture(proc_arch);
		}
		else
		{
			emit StatusBox(false, "Invalid PID");

			return;
		}
	}
	else
	{
		if (proc_data_by_name.IsValid())
		{
			proc_data_by_name.GetProcessID(inj_data.ProcessID);
			proc_data_by_name.GetArchitecture(proc_arch);
		}
		else
		{
			emit StatusBox(false, "Invalid process name");

			return;
		}
	}

	switch (ui.cmb_load->currentIndex())
	{
	case 1:  inj_data.Mode = INJECTION_MODE::IM_LdrLoadDll;				break;
	case 2:  inj_data.Mode = INJECTION_MODE::IM_LdrpLoadDll;			break;
	case 3:  inj_data.Mode = INJECTION_MODE::IM_LdrpLoadDllInternal;	break;
	case 4:  inj_data.Mode = INJECTION_MODE::IM_ManualMap;				break;
	default: inj_data.Mode = INJECTION_MODE::IM_LoadLibraryExW;			break;
	}

	switch (ui.cmb_create->currentIndex())
	{
	case 1:  inj_data.Method = LAUNCH_METHOD::LM_HijackThread;		break;
	case 2:  inj_data.Method = LAUNCH_METHOD::LM_SetWindowsHookEx;	break;
	case 3:  inj_data.Method = LAUNCH_METHOD::LM_QueueUserAPC;		break;
	case 4:  inj_data.Method = LAUNCH_METHOD::LM_KernelCallback;	break;
	case 5:	 inj_data.Method = LAUNCH_METHOD::LM_FakeVEH;			break;
	default: inj_data.Method = LAUNCH_METHOD::LM_NtCreateThreadEx;	break;
	}

	if (ui.cmb_peh->currentIndex() == 1)	inj_data.Flags |= INJ_ERASE_HEADER;
	if (ui.cmb_peh->currentIndex() == 2)	inj_data.Flags |= INJ_FAKE_HEADER;
	if (ui.cb_unlink->isChecked())			inj_data.Flags |= INJ_UNLINK_FROM_PEB;
	if (ui.cb_cloak->isChecked())			inj_data.Flags |= INJ_THREAD_CREATE_CLOAKED;
	if (ui.cb_random->isChecked())			inj_data.Flags |= INJ_SCRAMBLE_DLL_NAME;
	if (ui.cb_copy->isChecked())			inj_data.Flags |= INJ_LOAD_DLL_COPY;
	if (ui.cb_hijack->isChecked())			inj_data.Flags |= INJ_HIJACK_HANDLE;

	if (ui.cb_cloak->isChecked())
	{
		if (ui.cb_threadStart->isChecked())		inj_data.Flags |= INJ_CTF_FAKE_START_ADDRESS;
		if (ui.cb_threadHide->isChecked())		inj_data.Flags |= INJ_CTF_HIDE_FROM_DEBUGGER;
		if (ui.cb_threadAttach->isChecked())	inj_data.Flags |= INJ_CTF_SKIP_THREAD_ATTACH;
		if (ui.cb_threadTID->isChecked())		inj_data.Flags |= INJ_CTF_FAKE_TEB_CLIENT_ID;
	}

	if (inj_data.Mode == INJECTION_MODE::IM_ManualMap)
	{
		if (ui.cb_clean->isChecked())		inj_data.Flags |= INJ_MM_CLEAN_DATA_DIR;
		if (ui.cb_cookie->isChecked())		inj_data.Flags |= INJ_MM_INIT_SECURITY_COOKIE;
		if (ui.cb_imports->isChecked())		inj_data.Flags |= INJ_MM_RESOLVE_IMPORTS;
		if (ui.cb_delay->isChecked())		inj_data.Flags |= INJ_MM_RESOLVE_DELAY_IMPORTS;
		if (ui.cb_tls->isChecked())			inj_data.Flags |= INJ_MM_EXECUTE_TLS;
		if (ui.cb_seh->isChecked())			inj_data.Flags |= INJ_MM_ENABLE_EXCEPTIONS;
		if (ui.cb_protection->isChecked())	inj_data.Flags |= INJ_MM_SET_PAGE_PROTECTIONS;
		if (ui.cb_main->isChecked())		inj_data.Flags |= INJ_MM_RUN_DLL_MAIN;
		if (ui.cb_ldrlock->isChecked())		inj_data.Flags |= INJ_MM_RUN_UNDER_LDR_LOCK;
		if (ui.cb_shift->isChecked())		inj_data.Flags |= INJ_MM_SHIFT_MODULE_BASE;
		if (ui.cb_memory->isChecked())		inj_data.Flags |= INJ_MM_MAP_FROM_MEMORY;
	}

	inj_data.GenerateErrorLog = ui.cb_error->isChecked();

	int Timeout = ui.sp_timeout->value();
	if (Timeout > 0)
	{
		inj_data.Timeout = Timeout;
	}
	else
	{
		inj_data.Timeout = 2000;
	}

	if (inj_data.Mode != INJECTION_MODE::IM_ManualMap)
	{
		dot_net_inj_data.ProcessID			= inj_data.ProcessID;
		dot_net_inj_data.Mode				= inj_data.Mode;
		dot_net_inj_data.Method				= inj_data.Method;
		dot_net_inj_data.Flags				= inj_data.Flags;
		dot_net_inj_data.Timeout			= inj_data.Timeout;
		dot_net_inj_data.GenerateErrorLog	= inj_data.GenerateErrorLog;
	}
	else if (inj_data.Flags & INJ_MM_MAP_FROM_MEMORY)
	{		
		memory_inj_data.ProcessID			= inj_data.ProcessID;
		memory_inj_data.Mode				= inj_data.Mode;
		memory_inj_data.Method				= inj_data.Method;
		memory_inj_data.Flags				= inj_data.Flags;
		memory_inj_data.Timeout				= inj_data.Timeout;
		memory_inj_data.GenerateErrorLog	= inj_data.GenerateErrorLog;
	}

	if (!InjLib.LoadingStatus())
	{
		emit StatusBox(false, "The GH injection library couldn't be found or wasn't loaded correctly.");

		return;
	}

	if (InjLib.GetSymbolState() != INJ_ERR_SUCCESS)
	{
		emit StatusBox(false, "PDB download not finished.");

		return;
	}

	if (InjLib.GetImportState() != INJ_ERR_SUCCESS)
	{
		emit StatusBox(false, "Import handler not finished.");

		return;
	}

	std::vector<std::wstring> items; //native dlls: { path }
	std::vector<std::pair<std::wstring, std::vector<std::wstring>>> dot_net_items; //dot net dlls: { path, { namespace, class, method, arg }}

	QTreeWidgetItemIterator it(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		if ((*it)->checkState(FILE_LIST_IDX_CHECKBOX) != Qt::CheckState::Checked)
		{
			continue;
		}

		file_arch = StrToArchW((*it)->text(FILE_LIST_IDX_PLATFORM).toStdWString());
		if (file_arch == ARCH::NONE)
		{
			continue;
		}

		bool is_dot_net = false;
		if ((*it)->text(FILE_LIST_IDX_FLAG).toInt() == FILE_LIST_FLAG_DOTNET)
		{
			is_dot_net = true;
		}

		if (proc_arch != file_arch && (!is_dot_net || proc_arch == ARCH::X86))
		{
			continue;
		}

		QString fileStr = (*it)->text(FILE_LIST_IDX_PATH);
		fileStr.replace('/', '\\');

		if (!FileExistsW(fileStr.toStdWString()))
		{
			continue;
		}

		if (is_dot_net)
		{
			auto dot_net_arg = (*it)->text(FILE_LIST_IDX_DOTNET_ARGUMENT);
			auto dot_net_options = (*it)->text(FILE_LIST_IDX_DOTNET_OPTIONS);
			auto dot_net_options_list = dot_net_options.split('!', Qt::SkipEmptyParts);

			if (dot_net_options_list.length() != 3)
			{
				continue;
			}

			dot_net_items.push_back({ fileStr.toStdWString(), { dot_net_options_list.at(0).toStdWString(), dot_net_options_list.at(1).toStdWString(), dot_net_options_list.at(2).toStdWString(), dot_net_arg.toStdWString()} }); //good code
		}
		else
		{
			items.push_back(fileStr.toStdWString());
		}
	}

	if (items.empty() && dot_net_items.empty())
	{
		emit StatusBox(false, "No file(s) selected");

		return;
	}

	std::vector<std::string> results;
	int inj_count = 1;
	int dot_net_inj_count = 1;

	auto good_sleep = [](DWORD time)
	{
		if (time < 10)
		{
			Sleep(time);

			return;
		}

		auto current = GetTickCount64();
		auto t = current + time;

		while (current < t)
		{
			QCoreApplication::processEvents();

			g_Console->update_external();

			auto elapsed = current - GetTickCount64();

			if (elapsed < 10)
			{
				Sleep(10 - (DWORD)elapsed);
			}

			current += 10;
		}
	};

	auto copy_string_w_s = [](const std::wstring & src, wchar_t * dest, size_t cb) -> bool
	{
		auto len = src.length();
		auto max_len = cb / sizeof(wchar_t) - 1;

		if (len >= max_len)
		{
			g_print("String exceeds %d characters (%d required):\n\t%ls\n", max_len, len + 1, src.c_str());

			return false;
		}

		src.copy(dest, src.length());
		dest[len] = 0;

		return true;
	};

	auto map_file = [](const INJECTIONDATAW & src, MEMORY_INJECTIONDATA & dest) -> bool
	{
		std::ifstream File(src.szDllPath, std::ios::binary | std::ios::ate);
		if (!File.good())
		{
			g_print("Can't open file:\n\t%ls\n", src.szDllPath);

			return false;
		}

		dest.RawSize = static_cast<DWORD>(File.tellg());
		dest.RawData = new(std::nothrow) BYTE[dest.RawSize];
		if (!dest.RawData)
		{
			g_print("Memory allocation of %08X bytes failed for:\n\t%ls\n", dest.RawSize, src.szDllPath);

			return false;
		}

		File.seekg(0, std::ios::beg);
		File.read(reinterpret_cast<char *>(dest.RawData), dest.RawSize);
		File.close();

		return true;
	};

	bool fromMemory = false;
	if (inj_data.Mode == INJECTION_MODE::IM_ManualMap && (inj_data.Flags & INJ_MM_MAP_FROM_MEMORY))
	{
		fromMemory = true;
	}

	for (const auto & i : items)
	{
		if (!copy_string_w_s(i, inj_data.szDllPath, sizeof(inj_data.szDllPath)))
		{
			continue;
		}

		if (fromMemory)
		{
			if (!map_file(inj_data, memory_inj_data))
			{
				continue;
			}
		}

		auto dll_name_pos = i.find_last_of('\\') + 1;
		auto dll_name = i.substr(dll_name_pos);

		DWORD res = 0;

		g_print("Launching injection thread\n");

		std::shared_future<DWORD> inj_result;
		if (fromMemory)
		{
			inj_result = std::async(std::launch::async, &InjectionLib::Memory_Inject, &InjLib, &memory_inj_data);
		}
		else
		{
			inj_result = std::async(std::launch::async, &InjectionLib::InjectW, &InjLib, &inj_data);
		}

		auto end_tick = GetTickCount64() + inj_data.Timeout + 500;
		while (inj_result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready && end_tick > GetTickCount64())
		{
			good_sleep(50);
		}

		if (inj_result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
		{
			bool success = InjLib.InterruptInjection();

			g_print("Injection thread timed out\n");

			if (success)
			{
				g_print("Interrupted injection thread successfully\n");

				if (fromMemory)
				{
					delete[] memory_inj_data.RawData;
				}
			}
			else
			{
				g_print("Failed to interrupt injection thread\n");
			}

			return;
		}
		else
		{
			g_print("Injection thread returned\n");

			res = inj_result.get();
		}

		if (fromMemory)
		{
			delete[] memory_inj_data.RawData;
		}

		char buffer[MAX_PATH * 2]{ 0 };

		if (res != 0)
		{
			sprintf_s(buffer, "Injection (%d/%d) failed:\n  Error = %08X\n", inj_count, (int)items.size(), res);
			g_print("Check the error log for more information\n");
		}
		else
		{
			auto result = inj_data.hDllOut;
			if (fromMemory)
			{
				result = memory_inj_data.hDllOut;
			}

			sprintf_s(buffer, "Injection (%d/%d) succeeded:\n  %ls = %p\n", inj_count, (int)items.size(), dll_name.c_str(), result);
		}

		results.push_back(std::string(buffer));

		g_print("Injection %d/%d finished\n", inj_count, items.size());

		++inj_count;
	}

	for (const auto & i : dot_net_items)
	{
		if (!copy_string_w_s(i.first, dot_net_inj_data.szDllPath, sizeof(dot_net_inj_data.szDllPath)))
		{
			continue;
		}

		auto & Namespace	= i.second.at(0);
		auto & ClassName	= i.second.at(1);
		auto & MethodName	= i.second.at(2);
		auto & Argument		= i.second.at(3);

		if (!copy_string_w_s(Namespace, dot_net_inj_data.szNamespace, sizeof(dot_net_inj_data.szNamespace)))
		{
			continue;
		}

		if (!copy_string_w_s(ClassName, dot_net_inj_data.szClassName, sizeof(dot_net_inj_data.szClassName)))
		{
			continue;
		}

		if (!copy_string_w_s(MethodName, dot_net_inj_data.szMethodName, sizeof(dot_net_inj_data.szMethodName)))
		{
			continue;
		}

		if (!copy_string_w_s(Argument, dot_net_inj_data.szArgument, sizeof(dot_net_inj_data.szArgument)))
		{
			continue;
		}

		auto dll_name_pos = i.first.find_last_of('\\') + 1;
		auto dll_name = i.first.substr(dll_name_pos);

		DWORD res = 0;

		g_print("Launching .NET injection thread\n");

		std::shared_future<DWORD> inj_result = std::async(std::launch::async, &InjectionLib::DotNet_InjectW, &InjLib, &dot_net_inj_data);

		auto end_tick = GetTickCount64() + inj_data.Timeout + 500;
		while (inj_result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready && end_tick > GetTickCount64())
		{
			good_sleep(50);
		}

		if (inj_result.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
		{
			bool success = InjLib.InterruptInjection();

			g_print(".NET injection thread timed out\n");

			if (success)
			{
				g_print("Interrupted .NET injection thread successfully\n");
			}
			else
			{
				g_print("Failed to interrupt .NET injection thread\n");
			}

			return;
		}
		else
		{
			g_print(".NET Injection thread returned\n");

			res = inj_result.get();
		}

		char buffer[MAX_PATH * 2]{ 0 };

		if (res != 0)
		{
			sprintf_s(buffer, ".NET injection (%d/%d) failed:\n  Error = %08X\n", dot_net_inj_count, (int)dot_net_items.size(), res);
			g_print("Check the error log for more information\n");
		}
		else
		{
			sprintf_s(buffer, ".NET injection (%d/%d) succeeded:\n  %ls = %p\n", dot_net_inj_count, (int)dot_net_items.size(), dll_name.c_str(), dot_net_inj_data.hDllOut);
		}

		results.push_back(std::string(buffer));

		g_print(".NET injection %d/%d finished\n", dot_net_inj_count, dot_net_items.size());

		++dot_net_inj_count;
	}

	if (ui.cb_close->isChecked())
	{
		save_settings();

		qApp->exit(EXIT_CODE_CLOSE);
	}

	for (const auto & i : results)
	{
		g_print_raw(i.c_str());
	}
}

void GuiMain::btn_tooltip_change()
{
	int duration = 1;
	int duration_perma = -1;

	if (!tooltipsEnabled)
	{
		duration = -1;
		ui.btn_tooltip->setText("&Disable tooltips");
		tooltipsEnabled = true;

		g_print("Tooltips enabled\n");
	}
	else
	{
		ui.btn_tooltip->setText("&Enable tooltips");
		tooltipsEnabled = false;

		g_print("Tooltips disabled\n");
	}

	//always enabled
	ui.txt_pid->setToolTipDuration(duration_perma);
	ui.lbl_proc_icon->setToolTipDuration(duration_perma);

	// Settings
	ui.lbl_proc->setToolTipDuration(duration);
	ui.rb_proc->setToolTipDuration(duration);
	ui.cmb_proc->setToolTipDuration(duration);

	ui.lbl_pid->setToolTipDuration(duration);
	ui.rb_pid->setToolTipDuration(duration);
	ui.btn_proc->setToolTipDuration(duration);
	ui.txt_arch->setToolTipDuration(duration);
	ui.lbl_arch->setToolTipDuration(duration);

	ui.sp_delay->setToolTipDuration(duration);
	ui.cb_close->setToolTipDuration(duration);
	ui.cb_auto->setToolTipDuration(duration);
	ui.sp_timeout->setToolTipDuration(duration);
	ui.cb_error->setToolTipDuration(duration);

	// Method
	ui.cmb_load->setToolTipDuration(duration);
	ui.cb_hijack->setToolTipDuration(duration);
	ui.cmb_create->setToolTipDuration(duration);
	ui.cb_cloak->setToolTipDuration(duration);

	// Cloaking
	ui.cmb_peh->setToolTipDuration(duration);
	ui.cb_unlink->setToolTipDuration(duration);
	ui.cb_random->setToolTipDuration(duration);
	ui.cb_copy->setToolTipDuration(duration);

	// manual mapping
	ui.cb_clean->setToolTipDuration(duration);
	ui.cb_cookie->setToolTipDuration(duration);
	ui.cb_imports->setToolTipDuration(duration);
	ui.cb_delay->setToolTipDuration(duration);
	ui.cb_tls->setToolTipDuration(duration);
	ui.cb_seh->setToolTipDuration(duration);
	ui.cb_protection->setToolTipDuration(duration);
	ui.cb_main->setToolTipDuration(duration);
	ui.cb_ldrlock->setToolTipDuration(duration);
	ui.cb_shift->setToolTipDuration(duration);
	ui.cb_memory->setToolTipDuration(duration);
	ui.cb_link->setToolTipDuration(duration);

	ui.btn_reset->setToolTipDuration(duration);
	ui.btn_hooks->setToolTipDuration(duration);

	// Files
	ui.btn_add->setToolTipDuration(duration);
	ui.btn_inject->setToolTipDuration(duration);
	ui.btn_remove->setToolTipDuration(duration);

	// Info
	ui.btn_tooltip->setToolTipDuration(duration);
	ui.btn_help->setToolTipDuration(duration);
	ui.btn_shortcut->setToolTipDuration(duration);
	ui.btn_version->setToolTipDuration(duration);
	ui.btn_openlog->setToolTipDuration(duration);
	ui.btn_console->setToolTipDuration(duration);

	QTreeWidgetItemIterator it(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		if (duration < 0)
		{
			(*it)->setToolTip(FILE_LIST_IDX_NAME, (*it)->text(FILE_LIST_IDX_NAME));
			(*it)->setToolTip(FILE_LIST_IDX_PATH, (*it)->text(FILE_LIST_IDX_PATH));
		}
		else
		{
			(*it)->setToolTip(FILE_LIST_IDX_NAME, "");
			(*it)->setToolTip(FILE_LIST_IDX_PATH, "");
		}
	}
}

void GuiMain::btn_open_help()
{
	g_print("Openeing help thread: %ls\n", GH_HELP_URLW.c_str());

	auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(0, 0, GH_HELP_URLW.c_str(), 0, 0, SW_SHOW));
	if (ret <= 32)
	{
		g_print("Failed to open browser:\n Error 1: %08X\n Error 2: %08X\n", ret, GetLastError());
	}
}

bool GuiMain::get_icon_from_file(const std::wstring & path, UINT size, int index, QPixmap & pixmap)
{
	HICON icon = NULL;
	auto hr = SHDefExtractIconW(path.c_str(), index, NULL, &icon, nullptr, size & 0xFFFF);
	if (FAILED(hr))
	{
		g_print("Failed to locate icon in file:\n path  = %ls\n error = %08X\n", path.c_str(), hr);

		return false;
	}

	pixmap = qt_pixmapFromWinHICON(icon);

	DestroyIcon(icon);

	return (pixmap.isNull() == false);
}

void GuiMain::show()
{
	framelessParent->show();
}

void GuiMain::btn_generate_shortcut()
{
	std::wstring shortCut;
	QString fileName = "Injector_";

	ARCHITECTURE proc_arch;

	if (ui.rb_pid->isChecked())
	{		
		if (proc_data_by_pid.IsValid())
		{
			std::wstring name;
			proc_data_by_pid.GetNameW(name);
			proc_data_by_pid.GetArchitecture(proc_arch);

			shortCut += L"-p \"" + name + L"\"";
			fileName += QString::fromStdWString(name);
		}
		else
		{
			emit StatusBox(false, "Invalid PID");

			return;
		}
	}
	else
	{
		if (proc_data_by_name.IsValid())
		{
			std::wstring name;
			proc_data_by_pid.GetNameW(name);
			proc_data_by_pid.GetArchitecture(proc_arch);

			shortCut += L"-p \"" + name + L"\"";
			fileName += QString::fromStdWString(name);
		}
		else
		{
			emit StatusBox(false, "The specified process doesn't exist.");

			return;
		}
	}

	bool is_dot_net = false;
	bool fileFound	= false;
	std::vector<std::wstring> DotNetOptions;

	QTreeWidgetItemIterator it(ui.tree_files);
	for (; (*it) != Q_NULLPTR; ++it)
	{
		if ((*it)->checkState(0) != Qt::CheckState::Checked)
		{
			continue;
		}

		QString fileStr = (*it)->text(2);

		QFile qf(fileStr);
		if (!qf.exists())
		{
			continue;
		}

		auto file_arch = StrToArchW((*it)->text(3).toStdWString());
		if (file_arch == ARCH::NONE)
		{
			continue;
		}

		if ((*it)->text(FILE_LIST_IDX_FLAG).toInt() == FILE_LIST_FLAG_DOTNET)
		{
			is_dot_net = true;
		}

		if (proc_arch != file_arch && (!is_dot_net || proc_arch == ARCH::X86))
		{
			continue;
		}

		if (is_dot_net)
		{
			auto dot_net_arg			= (*it)->text(FILE_LIST_IDX_DOTNET_ARGUMENT);
			auto dot_net_options		= (*it)->text(FILE_LIST_IDX_DOTNET_OPTIONS);
			auto dot_net_options_list	= dot_net_options.split('!', Qt::SkipEmptyParts);

			if (dot_net_options_list.length() != 3)
			{
				continue;
			}

			DotNetOptions.push_back(dot_net_options_list[0].toStdWString());
			DotNetOptions.push_back(dot_net_options_list[1].toStdWString());
			DotNetOptions.push_back(dot_net_options_list[2].toStdWString());
			DotNetOptions.push_back(dot_net_arg.toStdWString());

			is_dot_net = true;
		}

		fileName += QString("_") + (*it)->text(1);

		fileStr.replace("/", "\\");
		shortCut += L" -f \"" + fileStr.toStdWString() + L"\"";

		fileFound = true;

		break;
	}

	if (!fileFound)
	{
		emit StatusBox(false, "No valid file selected.");

		return;
	}

	int delay = ui.sp_delay->value();
	if (delay > 0)
	{
		shortCut += L" -delay ";
		shortCut += std::format(L"{:d}", delay);
	}

	int timeout = ui.sp_timeout->value();
	if (timeout > 0)
	{
		shortCut += L" -timeout ";
		shortCut += std::format(L"{:d}", timeout);
	}

	if (ui.cb_error->isChecked())
	{
		shortCut += L" -log";
	}

	switch (ui.cmb_load->currentIndex())
	{
		case 1:		shortCut += L" -l 1";	break;
		case 2:		shortCut += L" -l 2";	break;
		case 3:		shortCut += L" -l 3";	break;
		case 4:		shortCut += L" -l 4";	break;
		default: break;
	}

	switch (ui.cmb_create->currentIndex())
	{
		case 1:		shortCut += L" -s 1"; break;
		case 2:		shortCut += L" -s 2"; break;
		case 3:		shortCut += L" -s 3"; break;
		case 4:		shortCut += L" -s 4"; break;
		default: break;
	}

	if (ui.cmb_peh->currentIndex() == 1)	shortCut += L" -peh 1";
	if (ui.cmb_peh->currentIndex() == 2)	shortCut += L" -peh 2";
	if (ui.cb_unlink->isChecked())			shortCut += L" -unlink";
	if (ui.cb_cloak->isChecked())			shortCut += L" -cloak";
	if (ui.cb_random->isChecked())			shortCut += L" -random";
	if (ui.cb_copy->isChecked())			shortCut += L" -copy";
	if (ui.cb_hijack->isChecked())			shortCut += L" -hijack";

	DWORD Flags = 0;
	if (ui.cmb_load->currentIndex() == (int)INJECTION_MODE::IM_ManualMap)
	{
		if (ui.cb_main->isChecked())		Flags |= INJ_MM_RUN_DLL_MAIN;
		if (ui.cb_ldrlock->isChecked())		Flags |= INJ_MM_RUN_UNDER_LDR_LOCK;
		if (ui.cb_imports->isChecked())		Flags |= INJ_MM_RESOLVE_IMPORTS;
		if (ui.cb_delay->isChecked())		Flags |= INJ_MM_RESOLVE_DELAY_IMPORTS;
		if (ui.cb_tls->isChecked())			Flags |= INJ_MM_EXECUTE_TLS;
		if (ui.cb_protection->isChecked())	Flags |= INJ_MM_SET_PAGE_PROTECTIONS;
		if (ui.cb_seh->isChecked())			Flags |= INJ_MM_ENABLE_EXCEPTIONS;
		if (ui.cb_cookie->isChecked())		Flags |= INJ_MM_INIT_SECURITY_COOKIE;
		if (ui.cb_clean->isChecked())		Flags |= INJ_MM_CLEAN_DATA_DIR;
		if (ui.cb_shift->isChecked())		Flags |= INJ_MM_SHIFT_MODULE_BASE;
		if (ui.cb_memory->isChecked())		Flags |= INJ_MM_MAP_FROM_MEMORY;
		//if (ui.cb_link->isChecked())		Flags |= INJ_MM_SHIFT_MODULE_BASE;

		shortCut += L" -mmflags ";
		shortCut += std::format(L"{:X}", Flags);
	}

	shortCut += L" -wait ";

	if (is_dot_net)
	{
		shortCut += L" -dotnet";
		shortCut += L" -namespace ";
		shortCut += DotNetOptions[0];
		shortCut += L" -class ";
		shortCut += DotNetOptions[1];
		shortCut += L" -method ";
		shortCut += DotNetOptions[2];

		if (!DotNetOptions[3].empty())
		{
			shortCut += L" -argument ";
			shortCut += DotNetOptions[3];
		}
	}

	auto silent = YesNoBox("Silent mode", "Do you want the shortcut to be in silent mode?\nThis means that no console window will be spawned and\nthere will be no notifications on the injection status.", framelessParent);
	if (silent)
	{
		shortCut += L"-silent ";
	}

	fileName.replace(".", "_");
	fileName.replace(" ", "_");

	auto hr = CreateLinkWrapper(fileName, QString::fromStdWString(shortCut));
	if (SUCCEEDED(hr))
	{
		QString msg = "The shortcut was created succesfully with the following name:\n" + fileName + "\n\nOpen shortcut location?";
		if (YesNoBox("Success", "The shortcut was created succesfully with the following name:\n" + fileName + "\n\nDo you want to open the shortcut location?", framelessParent))
		{
			//stolen from here:
			//https://stackoverflow.com/questions/15300999/open-windows-explorer-directory-select-a-specific-file-in-delphi
			//by user Andreas Rejbrand
			//Danke dir, Andi!

			std::wstring open_cmd = L"/Select,";
			open_cmd += g_RootPath;
			open_cmd += fileName.toStdWString();
			open_cmd += L".lnk";

			auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(NULL, nullptr, L"explorer.exe", open_cmd.c_str(), nullptr, SW_SHOW));
			if (ret <= 32)
			{
				g_print("Failed to open explorer:\n Error 1: %08X\n Error 2: %08X\n", ret, GetLastError());
			}
		}
	}
	else
	{
		QString error_msg = "Shortcut generation failed. Error code: 0x";
		QString number = QStringLiteral("%1").arg((DWORD)hr, 8, 0x10, QLatin1Char('0'));
		error_msg += number;

		StatusBox(false, error_msg);
	}
}

void GuiMain::btn_open_console()
{
	g_Console->open();

	if (g_Console->get_dock_index() == DOCK_NONE)
	{
		int old_idx = g_Console->get_old_dock_index();

		if (old_idx == DOCK_NONE)
		{
			old_idx = dockIndex;
		}
		
		if (old_idx == -1)
		{
			old_idx		= DOCK_RIGHT;
			dockIndex	= DOCK_RIGHT;
		}

		g_Console->dock(old_idx);
	}

	consoleOpen = true;
}

void GuiMain::btn_open_log()
{
	if (FileExistsW(GH_INJ_LOGW))
	{
		auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(NULL, L"edit", GH_INJ_LOGW.c_str(), nullptr, nullptr, SW_SHOW));
		if (ret <= 32)
		{
			g_print("Failed to open log file:\n Error 1: %08X\n Error 2: %08X\n", ret, GetLastError());
		}
	}
	else
	{
		g_print("Log file doesn't exist\n");
	}
}

void GuiMain::setup()
{
	if (!consoleOpen)
	{
		g_Console->close();
	}
	else
	{
		btn_open_console();
	}

	if (updateCheck)
	{
		update();
	}

	if (InjLib.GetSymbolState() != INJ_ERR_SUCCESS || InjLib.GetImportState() != INJ_ERR_SUCCESS)
	{
		g_Console->update_external();

		QString msg = "The injector requires PDB files for the ntdll.dll to work.\nThese files will be downloaded from the Microsoft Symbol Server\nand will take up about ";

		auto CurrentOS = QOperatingSystemVersion::current();
		if (CurrentOS >= QOperatingSystemVersion::Windows7 && CurrentOS < QOperatingSystemVersion::Windows8) //no == operator provided
		{
			msg += QString::number(10); //Win7 also requires kernel32 PDB files
		}
		else
		{
			msg += QString::number(5);
		}

		msg += "MB.\n\nDo you want to download the files now?";
		
		if (YesNoBox("PDB Download", msg, framelessParent))
		{
			InjLib.StartDownload();
			ShowPDBDownload(&InjLib);
		}
		else
		{
			InjLib.InterruptDownload();
		}

		g_Console->update_external();
	}

	if (InjLib.GetSymbolState() == INJ_ERR_SUCCESS && InjLib.GetImportState() == INJ_ERR_SUCCESS)
	{
		ui.btn_inject->setEnabled(true);
		ui.cb_auto->setEnabled(true);
		ui.btn_hooks->setEnabled(true);

		g_print("Injector ready\n");
	}

	t_Update_Files.start(200);

	setupDone = true;
}

void GuiMain::update()
{
	g_print("Checking for updates\n");

	newest_version = get_newest_version();

	auto cmp = newest_version.compare(current_version);
	if (cmp > 0)
	{
		std::wstring update_txt = L"This version of the GH Injector is outdated and might contain life-threatening bugs. The newest version is V" + newest_version + L". Click to update.";
		ui.btn_version->setToolTip(QString::fromStdWString(update_txt));
		ui.btn_version->setStyleSheet("background-color: red");
	}
	else if (cmp < 0)
	{
		ui.btn_version->setToolTip("Holy shit, your version is from the future.");
	}
	else
	{
		ui.btn_version->setToolTip("You are using the newest version of the GH Injector.");

		g_print("This version is up to date\n");
	}

	if (!ignoreUpdate && cmp > 0)
	{
		if (update_injector(newest_version, ignoreUpdate, &InjLib))
		{
			exit(GuiMain::EXIT_CODE_UPDATE);
		}
	}
}

void GuiMain::btn_update_clicked()
{
	save_settings();

	auto cmp = newest_version.compare(current_version);
	if (cmp == 0)
	{
		newest_version = get_newest_version();
	}

	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, false);
	}

	if (update_injector(newest_version, ignoreUpdate, &InjLib))
	{
		qApp->exit(EXIT_CODE_UPDATE);
	}

	if (drag_drop)
	{
		drag_drop->SetPosition(-1, -1, false, true);
	}
}