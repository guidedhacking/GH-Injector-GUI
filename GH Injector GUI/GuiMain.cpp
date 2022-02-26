#include "pch.h"

#include "GuiMain.h"

int const GuiMain::EXIT_CODE_CLOSE			= 0;
int const GuiMain::EXIT_CODE_REBOOT			= -1;
int const GuiMain::EXIT_CODE_UPDATE			= -2;
int const GuiMain::EXIT_CODE_START_NATIVE	= -3;

int const GuiMain::Height_small			= 410;
int const GuiMain::Height_medium_s		= 480;
int const GuiMain::Height_medium_b		= 570;
int const GuiMain::Height_big			= 620;
int const GuiMain::Height_change_delay	= 100;

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

	if (!platformCheck())
	{
		exit(GuiMain::EXIT_CODE_START_NATIVE);
	}

	native = IsNativeProcess(GetCurrentProcessId());

	if (!InjLib.Init())
	{
		QString failMsg = "Failed to load ";
		failMsg += GH_INJ_MOD_NAMEA;
		failMsg += ".";

		emit ShowStatusbox(false, failMsg);

		exit(GuiMain::EXIT_CODE_CLOSE);
	}

	InjLib.SetRawPrintCallback(g_print_to_console_raw_external);
	
	g_Console->update_external();

	if (!SetDebugPrivilege(true))
	{
		emit ShowStatusbox(false, "Failed to enable debug privileges. This might affect the functionality of the injector.");
	}

	ui.setupUi(this);

	t_Auto_Inj			= new(std::nothrow) QTimer(this);
	t_Delay_Inj			= new(std::nothrow) QTimer(this);
	t_Update_Proc		= new(std::nothrow) QTimer(this);
	t_OnUserInput		= new(std::nothrow) QTimer(this);
	t_Update_DragDrop	= new(std::nothrow) QTimer(this);
	t_SetUp				= new(std::nothrow) QTimer(this);
	t_Update_Files		= new(std::nothrow) QTimer(this);

	if (t_Auto_Inj == Q_NULLPTR || t_Delay_Inj == Q_NULLPTR || t_Update_Proc == Q_NULLPTR || t_OnUserInput == Q_NULLPTR || t_Update_DragDrop == Q_NULLPTR || t_SetUp == Q_NULLPTR || t_Update_Files == Q_NULLPTR)
	{
		THROW("Failed to create timer object.\n");
	}

	t_Delay_Inj->setSingleShot(true);
	t_OnUserInput->setSingleShot(true);
	t_Update_DragDrop->setSingleShot(true);
	t_SetUp->setSingleShot(true);

	pss	= new(std::nothrow) Process_State_Struct();
	if (pss == nullptr)
	{
		THROW("Failed to create process state structure.\n");
	}

	ps_picker = new(std::nothrow) Process_Struct();
	if (ps_picker == nullptr)
	{
		THROW("Failed to create process data structure.\n");
	}
	
	pxm_banner	= QPixmap(":/GuiMain/gh_resource/GH Banner.png");
	pxm_lul		= QPixmap(":/GuiMain/gh_resource/LUL Icon.png");
	pxm_generic = QPixmap(":/GuiMain/gh_resource/Generic Icon.png");
	pxm_error	= QPixmap(":/GuiMain/gh_resource/Error Icon.png");

	if (pxm_banner.isNull() || pxm_lul.isNull() || pxm_generic.isNull() || pxm_error.isNull())
	{
		emit ShowStatusbox(false, "Failed to initialize one or multiple graphic files. This won't affect the functionality of the injector.");
	}

	ui.lbl_img->setPixmap(pxm_banner);
	
	ui.lbl_proc_icon->setStyleSheet("background: transparent");

	auto banner_height = pxm_banner.height();
	ui.btn_close->setFixedHeight(banner_height / 2);
	ui.btn_minimize->setFixedHeight(banner_height / 2);
	ui.btn_close->setFixedWidth(50);
	ui.btn_minimize->setFixedWidth(50);
		
	ui.tree_files->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustIgnored);
	ui.tree_files->setColumnWidth(0, 50);
	ui.tree_files->setColumnWidth(1, 135);
	ui.tree_files->setColumnWidth(2, 360);
	ui.tree_files->setColumnWidth(3, 50);

	std::string v = "V";
	v += GH_INJ_GUI_VERSIONA;
	ui.btn_version->setText(v.c_str());

	ui.btn_openlog->setIcon(QIcon(":/GuiMain/gh_resource/Log Icon.ico"));
	ui.btn_console->setIcon(QIcon(":/GuiMain/gh_resource/Console.ico"));

	QApplication::instance()->installEventFilter(this);
	
	rev_NumbersOnly = new(std::nothrow) QRegularExpressionValidator(QRegularExpression("[0-9]+"));
	if (rev_NumbersOnly == Q_NULLPTR)
	{
		emit ShowStatusbox(false, "Failed to create regular expression validator for PID input field.");
	}
	else
	{
		ui.txt_pid->setValidator(rev_NumbersOnly);
	}

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
	connect(ui.btn_close,		SIGNAL(clicked()), this, SLOT(close_clicked()));
	connect(ui.btn_minimize,	SIGNAL(clicked()), this, SLOT(minimize_clicked()));

	// Settings
	connect(ui.rb_proc,		SIGNAL(clicked()), this, SLOT(rb_process_set()));
	connect(ui.rb_pid,		SIGNAL(clicked()), this, SLOT(rb_pid_set()));
	connect(ui.btn_proc,	SIGNAL(clicked()), this, SLOT(btn_pick_process_click()));

	// Auto, Reset, Color
	connect(ui.cb_auto,		SIGNAL(clicked()), this, SLOT(auto_inject()));
	connect(ui.btn_reset,	SIGNAL(clicked()), this, SLOT(reset_settings()));
	connect(ui.btn_hooks,	SIGNAL(clicked()), this, SLOT(btn_hook_scan_click()));

	// Method, Cloaking, Advanced
	connect(ui.cmb_load,		SIGNAL(currentIndexChanged(int)),	this, SLOT(load_change(int)));
	connect(ui.cmb_create,		SIGNAL(currentIndexChanged(int)),	this, SLOT(create_change(int)));
	connect(ui.cb_main,			SIGNAL(clicked()),					this, SLOT(cb_main_clicked()));
	connect(ui.cb_protection,	SIGNAL(clicked()),					this, SLOT(cb_page_protection_clicked()));
	connect(ui.cmb_peh,			SIGNAL(currentIndexChanged(int)),	this, SLOT(peh_change(int)));
	connect(ui.cb_cloak,		SIGNAL(clicked()),					this, SLOT(cb_cloak_clicked()));
	connect(ui.cb_hijack,		SIGNAL(clicked()),					this, SLOT(cb_hijack_clicked()));

	// Files
	connect(ui.btn_add,		SIGNAL(clicked()),									this, SLOT(add_file_dialog()));
	connect(ui.btn_inject,	SIGNAL(clicked()),									this, SLOT(delay_inject()));
	connect(ui.btn_remove,	SIGNAL(clicked()),									this, SLOT(remove_file()));
	connect(ui.tree_files,	SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),	this, SLOT(select_file()));

	// Info
	connect(ui.btn_tooltip,		SIGNAL(clicked()), this, SLOT(tooltip_change()));
	connect(ui.btn_shortcut,	SIGNAL(clicked()), this, SLOT(generate_shortcut()));
	connect(ui.btn_help,		SIGNAL(clicked()), this, SLOT(open_help()));
	connect(ui.btn_version,		SIGNAL(clicked()), this, SLOT(update_clicked()));
	connect(ui.btn_console,		SIGNAL(clicked()), this, SLOT(open_console()));
	connect(ui.btn_openlog,		SIGNAL(clicked()), this, SLOT(open_log()));
	
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
	connect(this,		SIGNAL(send_to_picker(Process_State_Struct *, Process_Struct *)),	gui_Picker, SLOT(get_from_inj(Process_State_Struct *, Process_Struct *)));
	connect(gui_Picker, SIGNAL(send_to_inj(Process_State_Struct *, Process_Struct *)),		this,		SLOT(get_from_picker(Process_State_Struct *, Process_Struct *)));

	// Scan Hook
	connect(this,			SIGNAL(send_to_scan_hook(int)),	gui_Scanner,	SLOT(get_from_inj_to_sh(int)));
	connect(gui_Scanner,	SIGNAL(send_to_inj_sh()),		this,			SLOT(get_from_scan_hook()));

	connect(t_Auto_Inj,			SIGNAL(timeout()), this, SLOT(auto_loop_inject()));
	connect(t_Delay_Inj,		SIGNAL(timeout()), this, SLOT(inject_file()));
	connect(t_Update_Proc,		SIGNAL(timeout()), this, SLOT(update_process()));
	connect(t_Update_DragDrop,	SIGNAL(timeout()), this, SLOT(update_after_height_change()));
	connect(t_SetUp,			SIGNAL(timeout()), this, SLOT(setup()));
	connect(t_Update_Files,		SIGNAL(timeout()), this, SLOT(update_file_list()));	

	ui.fr_cloak->setVisible(false);

	ui.fr_adv->setVisible(false);
	ui.cb_unlink->setEnabled(true);

	load_settings();

	load_change(0);
	create_change(0);

	cb_main_clicked();
	cb_page_protection_clicked();
	cb_cloak_clicked();

	peh_change(0);
	tooltip_change();

	if (dockIndex == -1)
	{
		dockIndex = 0;
	}

	if (!native)
	{
		// Won't work if not native
		ui.cb_hijack->setDisabled(false);
		ui.cb_hijack->setChecked(false);
		ui.cb_hijack->setDisabled(true);
	}

	if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows10)
	{
		// Only exists on Win10+
		auto * cmb_model = qobject_cast<QStandardItemModel *>(ui.cmb_load->model());
		if (cmb_model)
		{
			if (ui.cmb_load->currentIndex() == 3)
			{
				ui.cmb_load->setCurrentIndex(0);
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

	update_process();
	update_proc_icon();
	btn_change();

	ui.fr_method->setStyleSheet("QFrame { border-top: 1px solid grey; }");
	ui.fr_cloak->setStyleSheet("QFrame { border-top: 1px solid grey; }");
	ui.fr_adv->setStyleSheet("QFrame { border-top: 1px solid grey; }");

	auto_inject();
}

GuiMain::~GuiMain()
{
	save_settings();

	SAFE_DELETE(drag_drop);
	SAFE_DELETE(gui_Scanner);
	SAFE_DELETE(gui_Picker);
	SAFE_DELETE(rev_NumbersOnly);
	SAFE_DELETE(pss);
	SAFE_DELETE(ps_picker);
	SAFE_DELETE(t_OnUserInput);
	SAFE_DELETE(t_Update_Proc);
	SAFE_DELETE(t_Delay_Inj);
	SAFE_DELETE(t_Auto_Inj);
	SAFE_DELETE(t_Update_DragDrop);
	SAFE_DELETE(t_SetUp);
	SAFE_DELETE(t_Update_Files);
	SAFE_DELETE(g_Console);

	InjLib.InterruptDownload();
	InjLib.Unload();
}

void GuiMain::update_process()
{
	DWORD raw = ui.txt_pid->text().toULong();
	Process_Struct byName	= getProcessByNameW(ui.cmb_proc->currentText().toStdWString().c_str());
	Process_Struct byPID	= getProcessByPID(ui.txt_pid->text().toULong());

	//avoid unnecessary updates
	if (raw != old_raw_pid || byName.PID != old_byname_pid || byPID.PID != old_bypid_pid || (ui.cmb_proc->currentText().compare("Broihon.exe") == 0 && old_raw_pid != 1337))
	{
		old_raw_pid		= raw;
		old_byname_pid	= byName.PID;
		old_bypid_pid	= byPID.PID;
	}
	else
	{
		return;
	}

#ifndef _WIN64
	if (!native)
	{
		if (byName.PID)
		{
			if (IsNativeProcess(byName.PID))
			{
				byName.PID = 0;
			}
		}

		if (byPID.PID)
		{
			if (IsNativeProcess(byPID.PID))
			{
				byPID.PID = 0;
			}
		}
	}
#endif

	if (ui.rb_pid->isChecked())
	{
		if (byPID.PID)
		{
			if (byPID.PID != byName.PID && strcicmpW(byPID.szPath, byName.szPath))
			{
				txt_pid_change();
				btn_change();
				update_proc_icon();
			}
		}
		else if (!t_OnUserInput->isActive() || raw == 1337)
		{
			if (byName.PID && byName.PID != raw && raw != 1337)
			{
				cmb_proc_name_change();
				btn_change();
				update_proc_icon();
			}
			else if (raw && raw != 1337)
			{
				ui.txt_pid->setText("0");
				ui.txt_arch->setText("---");
				ui.txt_pid->setToolTip("");
				ui.cmb_proc->setToolTip("");
				ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");

				old_raw_pid = 0;

				btn_change();
				update_proc_icon();
			}
			else if (raw == 1337)
			{
				ui.txt_arch->setText(QString::fromUtf8("\xF0\x9F\x98\x8E") + QString::fromUtf8("\xF0\x9F\x92\xA6"));
				ui.txt_arch->setToolTip("Doin your mom doin doin your mom\nDoin your mom doin doin your mom\nDoin doin your mom doin doin your mom\nYou know we straight with doin your mom");
				ui.txt_pid->setToolTip("");
				ui.cmb_proc->setToolTip("");
				ui.cmb_proc->setEditText("Broihon.exe");

				old_raw_pid		= 1337;
				old_byname_pid	= 0;
				old_bypid_pid	= 0;

				btn_change();
				update_proc_icon();

				ui.lbl_proc_icon->setToolTip(QString::fromWCharArray(L"Praise Broihon \u2665\u2665\u2665"));
			}
		}
	}
	else if (ui.rb_proc->isChecked())
	{
		if (byName.PID)
		{
			if (byName.PID != byPID.PID && strcicmpW(byPID.szPath, byName.szPath))
			{
				cmb_proc_name_change();
				btn_change();
				update_proc_icon();
			}
		}
		else if (raw != 0 && ui.cmb_proc->currentText().compare("Broihon.exe") != 0)
		{
			ui.txt_pid->setText("0");
			ui.txt_arch->setText("---");
			ui.txt_pid->setToolTip("");
			ui.cmb_proc->setToolTip("");
			ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");

			old_raw_pid = 0;

			btn_change();
			update_proc_icon();
		}
		else if (ui.cmb_proc->currentText().compare("Broihon.exe") == 0)
		{
			ui.txt_arch->setText(QString::fromUtf8("\xF0\x9F\x98\x8E") + QString::fromUtf8("\xF0\x9F\x92\xA6"));
			ui.txt_arch->setToolTip("Doin your mom doin doin your mom\nDoin your mom doin doin your mom\nDoin doin your mom doin doin your mom\nYou know we straight with doin your mom");
			ui.txt_pid->setToolTip("");
			ui.cmb_proc->setToolTip("");
			ui.txt_pid->setText("1337");

			old_raw_pid		= 1337;
			old_byname_pid	= 0;
			old_bypid_pid	= 0;

			btn_change();
			update_proc_icon();

			ui.lbl_proc_icon->setToolTip(QString::fromWCharArray(L"Praise Broihon \u2665\u2665\u2665"));
		}
	}	
}

void GuiMain::update_proc_icon()
{
	int size = ui.btn_proc->height();
	ui.lbl_proc_icon->setFixedSize(QSize(size, size));

	int pid = ui.txt_pid->text().toInt();

	Process_Struct ps = getProcessByPID(pid);

	QPixmap new_icon = QPixmap();

	if (pid == 1337)
	{
		new_icon = pxm_lul.scaled(size, size);
	}
	else if (!ps.PID || !lstrlenW(ps.szPath))
	{
		new_icon = pxm_error.scaled(size, size);
	}
	else
	{
		new_icon = GetIconFromFileW(ps.szPath, size);

		if (new_icon.isNull())
		{
			new_icon = pxm_generic.scaled(size, size);
		}
	}

	ui.lbl_proc_icon->setPixmap(new_icon);
}

void GuiMain::update_file_list()
{
	QTreeWidgetItemIterator it(ui.tree_files);

	for (; *it; ++it)
	{
		bool b_ok = false;

#ifdef _WIN64
		HANDLE hFile = (HANDLE)((*it)->text(4).toULongLong(&b_ok, 0x10));
#else
		HANDLE hFile = (HANDLE)((*it)->text(4).toULong(&b_ok, 0x10));
#endif

		if (!hFile)
		{
			b_ok = false;
		}

		auto q_path = (*it)->text(2);

		if (!FileExistsW(q_path.toStdWString().c_str()))
		{
			if (b_ok && hFile == INVALID_HANDLE_VALUE)
			{
				continue;
			}

			if (!b_ok)
			{
				g_print("Deleted %ls from the list (invalid handle)\n", (*it)->text(1).toStdWString().c_str());
				delete *it;

				continue;
			}
			
			wchar_t new_path[MAX_PATH * 2]{ 0 };
			auto ret = GetFinalPathNameByHandleW(hFile, new_path, sizeof(new_path) / sizeof(wchar_t), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
			if (!ret || ret && GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
			{
				CloseHandle(hFile);

				g_print("Deleted %ls from the list (invalid path)\n", (*it)->text(1).toStdWString().c_str());
				delete *it;

				continue;
			}

			CloseHandle(hFile);

			const wchar_t * new_path_stripped = new_path + 4; //skip DOS prefix

			std::wstring s_new_path(new_path_stripped);
			if (s_new_path.find(L"$Recycle.Bin") != std::string::npos) //detect if file was moved to recycle bin
			{
				g_print("Deleted %ls from the list\n", (*it)->text(1).toStdWString().c_str());
				delete *it;
			}
			else
			{
				if (s_new_path.find(L"$Extend") != std::string::npos) //file replaced, update item with original path
				{
					g_print("File probably replaced\n");
#ifdef _WIN64
					(*it)->setText(4, QString::number(reinterpret_cast<qulonglong>(INVALID_HANDLE_VALUE), 0x10));
#else
					(*it)->setText(4, QString::number(reinterpret_cast<unsigned long>(INVALID_HANDLE_VALUE), 0x10));
#endif

					(*it)->setData(0, Qt::CheckStateRole, Qt::CheckState::PartiallyChecked);
					(*it)->setDisabled(true);

					continue;
				}

				delete *it;

				g_print("New path: %ls\n", new_path_stripped);
				add_file_to_list(QString::fromWCharArray(new_path_stripped), true);
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

			delete *it;

			add_file_to_list(q_path, true);
		}
	}
	
	ARCH arch = ARCH::NONE;
	if (ui.rb_proc->isChecked())
	{
		QString proc = ui.cmb_proc->currentText();
		Process_Struct pl = getProcessByNameW(proc.toStdWString().c_str());
		arch = pl.Arch;
	}
	else
	{
		Process_Struct pl = getProcessByPID(ui.txt_pid->text().toInt());
		arch = pl.Arch;
	}

	if (arch == ARCH::NONE)
	{
		return;
	}

	it = QTreeWidgetItemIterator(ui.tree_files);
	for (; *it; ++it)
	{
		auto f = (*it)->flags();
		if (StrToArchW((*it)->text(3).toStdWString().c_str()) != arch)
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
			(*it)->setData(0, Qt::CheckStateRole, Qt::CheckState::Unchecked);
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
					remove_file();
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
					t_OnUserInput->start(10000);
				}
			}

			if (g_Console->is_open() && !framelessParent->isMinimized())
			{
				auto * keyEvent = static_cast<QKeyEvent *>(event);
				if (keyEvent->modifiers() & Qt::KeyboardModifier::ControlModifier)
				{
					int new_index = -1;

					switch (keyEvent->key())
					{
						case Qt::Key_3:
						case Qt::Key_Right:
							new_index = 0;
							break;

						case Qt::Key_1:
						case Qt::Key_Left:
							new_index = 1;
							break;
						case Qt::Key_5:
						case Qt::Key_Up:
							new_index = 2;
							break;

						case Qt::Key_2:
						case Qt::Key_Down:
							new_index = 3;
							break;

						default:
							break;
					}

					if (new_index != -1)
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

				auto * resizeEvent = static_cast<QResizeEvent *>(event);
				ui.tree_files->setColumnWidth(2, resizeEvent->size().width() - 250);
				ui.tree_files->setColumnWidth(3, 50);
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
				update_proc_icon();

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

			update_proc_icon();
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

				update_proc_icon();
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
	t_Update_Proc->start(250);
	t_SetUp->start(500);
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
	return true;
#else
	bool bPlatform = IsNativeProcess(GetCurrentProcessId());
	if (bPlatform == true)
	{
		return true;
	}

	if (!YesNoBox("Architecture conflict", "Since you're using a 64-bit version of Windows it's recommended\ntouse the 64-bit version of the injector.\n\nDo you want to switch to the 64-bit version?"))
	{
		return true;
	}

	if (!FileExistsW(GH_INJ_EXE_NAME64))
	{
		ShowStatusbox(false, "\"GH Injector - x64.exe\" is missing.\n");

		return true;
	}

	STARTUPINFOW		si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	auto x64_path = QCoreApplication::applicationDirPath().toStdWString();
	x64_path += L"/";
	x64_path += GH_INJ_EXE_NAME64W;

	if (!CreateProcessW(x64_path.c_str(), nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi))
	{
		auto err = GetLastError();
		QString error_msg = "CreateProcessW failed. Error code: 0x";
		QString number = QStringLiteral("%1").arg(err, 8, 0x10, QLatin1Char('0'));
		error_msg += number;

		ShowStatusbox(false, error_msg);

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

	// turn off all Radio Buttons
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

	emit send_to_picker(pss, ps_picker);
}

void GuiMain::cmb_proc_name_change()
{
	QString proc = ui.cmb_proc->currentText();
	Process_Struct pl = getProcessByNameW(proc.toStdWString().c_str());

	if (!pl.PID)
	{
		return;
	}

	g_print("Attached to process:\n %ls\n %06X (%d)\n", pl.szName, pl.PID, pl.PID);

	old_raw_pid		= pl.PID;
	old_byname_pid	= pl.PID;
	old_bypid_pid	= pl.PID;

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	ui.txt_pid->setText(QString::number(pl.PID));
	QString new_pid = QString::asprintf("0x%08X", pl.PID);
	ui.txt_pid->setToolTip(new_pid);

	if (pl.Arch == ARCH::X64)
	{
		ui.txt_arch->setToolTip("The target is a 64-bit process.");
	}
	else
	{
#ifdef _WIN64
		ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
#else
		if (!native)
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
		}
		else
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process.");
		}
#endif
	}

	if (lstrlenW(pl.szPath))
	{
		ui.cmb_proc->setToolTip(QString::fromWCharArray(pl.szPath));
		ui.lbl_proc_icon->setToolTip(QString::fromWCharArray(pl.szPath));
	}
	else
	{
		ui.cmb_proc->setToolTip("Can't resolve filepath.");
		ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");
		g_print("Can't resolve filepath\n");
	}

	ui.txt_arch->setText(QString::fromStdWString(ArchToStrW(pl.Arch)));

	if (ui.cmb_proc->findText(QString::fromWCharArray(pl.szName)) == -1)
	{
		ui.cmb_proc->addItem(QString::fromWCharArray(pl.szName));
	}
}

void GuiMain::txt_pid_change()
{
	QString s_PID = ui.txt_pid->text();
	Process_Struct pl = getProcessByPID(s_PID.toInt());

	if (!pl.PID)
	{
		return;
	}

	g_print("Attached to process:\n %ls\n %06X (%d)\n", pl.szName, pl.PID, pl.PID);

	old_raw_pid		= pl.PID;
	old_byname_pid	= pl.PID;
	old_bypid_pid	= pl.PID;

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	QString new_pid = QString::asprintf("0x%08X", pl.PID);
	ui.txt_pid->setToolTip(new_pid);

	if (pl.Arch == ARCH::X64)
	{
		ui.txt_arch->setToolTip("The target is a 64-bit process.");
	}
	else
	{
#ifdef _WIN64
		ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
#else
		if (!native)
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process running under WOW64.");
		}
		else
		{
			ui.txt_arch->setToolTip("The target is a 32-bit process.");
		}
#endif
	}

	ui.cmb_proc->setCurrentText(QString::fromWCharArray(pl.szName));

	if (lstrlenW(pl.szPath))
	{
		ui.cmb_proc->setToolTip(QString::fromWCharArray(pl.szPath));
		ui.lbl_proc_icon->setToolTip(QString::fromWCharArray(pl.szPath));
	}
	else
	{
		ui.cmb_proc->setToolTip("Can't resolve filepath.");
		ui.lbl_proc_icon->setToolTip("Can't resolve filepath.");
		g_print("Can't resolve filepath\n");
	}

	ui.txt_arch->setText(QString::fromStdWString(ArchToStrW(pl.Arch)));

	if (ui.cmb_proc->findText(QString::fromWCharArray(pl.szName)) == -1)
	{
		ui.cmb_proc->addItem(QString::fromWCharArray(pl.szName));
	}
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

	QString s_PID = ui.txt_pid->text();
	Process_Struct pl = getProcessByPID(s_PID.toInt());

	if (!pl.PID && ui.btn_hooks->isEnabled())
	{
		ui.btn_hooks->setEnabled(false);
		ui.btn_inject->setEnabled(false);
	}
	else if (pl.PID && !ui.btn_hooks->isEnabled())
	{
		ui.btn_hooks->setEnabled(true);
		ui.btn_inject->setEnabled(true);
	}

	if (s_PID.toInt() == 1337)
	{
		ui.btn_inject->setEnabled(true);
	}
}

void GuiMain::get_from_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct)
{
	pss			= procStateStruct;
	ps_picker	= procStruct;

	framelessPicker.hide();

	if (ps_picker->PID)
	{
		rb_unset_all();
		int index = ui.cmb_proc->findText(QString::fromWCharArray(ps_picker->szName));
		if (index == -1) // check exists
		{
			ui.cmb_proc->addItem(QString::fromWCharArray(ps_picker->szName));
		}

		ui.cmb_proc->setCurrentIndex(ui.cmb_proc->findText(QString::fromWCharArray(ps_picker->szName)));
		ui.txt_pid->setText(QString::number(ps_picker->PID));
		ui.txt_arch->setText(QString::fromStdWString(ArchToStrW(ps_picker->Arch)));

		update_proc_icon();
		txt_pid_change();
		rb_pid_set();
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

void GuiMain::auto_inject()
{
	if (ui.cb_auto->isChecked())
	{
		// Restart if running
		t_Auto_Inj->start(200);
	}
	else
	{
		t_Auto_Inj->stop();
	}
}

void GuiMain::auto_loop_inject()
{
	if (ui.cb_auto->isChecked())
	{
		int pid		= 0;
		ARCH arch	= ARCH::NONE;

		if (ui.rb_proc->isChecked())
		{
			QString proc = ui.cmb_proc->currentText();
			Process_Struct pl = getProcessByNameW(proc.toStdWString().c_str());
			pid		= pl.PID;
			arch	= pl.Arch;
		}
		else
		{
			Process_Struct pl = getProcessByPID(ui.txt_pid->text().toInt());
			pid		= pl.PID;
			arch	= pl.Arch;
		}

		if (!pid || arch == ARCH::NONE)
		{
			return;
		}

		bool found = false;

		QTreeWidgetItemIterator it(ui.tree_files);
		for (; *it; ++it)
		{
			if ((*it)->checkState(0) != Qt::CheckState::Checked)
			{
				continue;
			}

			if (!FileExistsW((*it)->text(2).toStdWString().c_str()))
			{
				continue;
			}

			if (StrToArchW((*it)->text(3).toStdWString().c_str()) != arch)
			{
				continue;
			}

			found = true;
			break;
		}

		if (!found)
		{
			return;
		}

		ui.cb_auto->setChecked(false);
		t_Auto_Inj->stop();

		emit delay_inject();
	}
}

void GuiMain::reset_settings()
{
	if (!YesNoBox("Reset", "Are you sure you want to reset all settings?", framelessParent))
	{
		return;
	}

	onReset = true;

	QFileDialog fDialog(this, "Select dll files", QApplication::applicationDirPath(), "Dynamic Link Libraries (*.dll)");

	QFile iniFile(GH_SETTINGS_INIA);
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

void GuiMain::close_clicked()
{
	save_settings();

	qApp->exit(GuiMain::EXIT_CODE_CLOSE);
}

void GuiMain::minimize_clicked()
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

	t_Update_DragDrop->start(Height_change_delay);
}

void GuiMain::save_settings()
{
	if (onReset)
	{
		onReset = false;

		return;
	}

	g_print(" Saving settings\n");

	QSettings settings(GH_SETTINGS_INIA, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.beginWriteArray("FILES");

	QTreeWidgetItemIterator it(ui.tree_files);
	for (int i = 0; *it; ++it, ++i)
	{
		if (!FileExistsW((*it)->text(2).toStdWString().c_str()))
		{
			continue;
		}

		settings.setArrayIndex(i);
		settings.setValue(QString::number(0), (*it)->text(2));
		settings.setValue(QString::number(1), (*it)->checkState(0) != Qt::CheckState::Unchecked);
	}
	settings.endArray();

	settings.beginWriteArray("PROCESS");
	for (int i = 0; i < ui.cmb_proc->count(); i++)
	{
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

	if (native)
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

	// Process picker
	settings.setValue("PROCNAMEFILTER", pss->txtFilter);
	settings.setValue("PROCESSTYPE", pss->cmbArch);
	settings.setValue("CURRENTSESSION", pss->cbSession);

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

	QFile iniFile(GH_SETTINGS_INIA);
	if (!iniFile.exists())
	{
		default_settings();

		return;
	}

	QSettings settings(GH_SETTINGS_INIA, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	int fileSize = settings.beginReadArray("FILES");
	for (int i = 0; i < fileSize; ++i)
	{
		settings.setArrayIndex(i);

		auto path = settings.value(QString::number(0), "").toString();

		add_file_to_list(path, settings.value(QString::number(1)).toBool());
	}
	settings.endArray();

	int procSize = settings.beginReadArray("PROCESS");
	for (int i = 0; i < procSize; ++i)
	{
		settings.setArrayIndex(i);
		ui.cmb_proc->addItem(settings.value(QString::number(0), "").toString());
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

	// Process picker
	pss->txtFilter	= settings.value("PROCNAMEFILTER", "").toString();
	pss->cmbArch	= settings.value("PROCESSTYPE", 0).toInt();
	pss->cbSession	= settings.value("CURRENTSESSION", true).toBool();

	// Info
	tooltipsEnabled = !settings.value("TOOLTIPSON", true).toBool();
	consoleOpen		= settings.value("CONSOLE", true).toBool();
	dockIndex		= settings.value("DOCKINDEX", 0).toInt();
	ignoreUpdate	= settings.value("IGNOREUPDATES", false).toBool();
	hijackWarning	= settings.value("HIJACKWARNING", true).toBool();

	// Not visible
	lastPathStr = settings.value("LASTDIR", "").toString();
	framelessParent->restoreGeometry(settings.value("GEOMETRY").toByteArray());

	auto old_second_count = settings.value("UPDATECHECK", 0).toLongLong();
	auto current_second_count = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	if (current_second_count - old_second_count < 24 * 60 * 60) //ignore leap/smear second
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
	ignoreUpdate	= false;
	tooltipsEnabled = false;
	dockIndex		= 0;
	pss->cbSession	= true;
	hijackWarning	= true;

	save_settings();
}

void GuiMain::load_change(int index)
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

void GuiMain::create_change(int index)
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

void GuiMain::peh_change(int index)
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
		auto ret = YesNoBox("Warning", "This option will try to hijack a handle from another process\nwhich can be a system process.\nUnder rare circumstances this can cause a crash.\nDo you want to enable this option anyway?", framelessParent);
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

void GuiMain::add_file_dialog()
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

void GuiMain::add_file_to_list(QString str, bool active)
{
#ifndef _WIN64
	if (!native)
	{
		str.replace(":/Windows/System32/", ":/Windows/Sysnative/", Qt::CaseSensitivity::CaseInsensitive);
	}
#else
	str.replace(":/Windows/Sysnative/", ":/Windows/System32/", Qt::CaseSensitivity::CaseInsensitive);
#endif

	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if ((*it)->text(2) == str)
		{
			g_print("File already in the list: %ls\n", str.toStdWString().c_str());

			return;
		}
	}

	QFileInfo fi(str);
	auto abs_path = fi.absoluteFilePath().toStdWString();

	ARCH arch = getFileArchW(abs_path.c_str());
	if (arch == ARCH::NONE)
	{
		return;
	}

	HANDLE hFile = CreateFileW(abs_path.c_str(), SYNCHRONIZE, FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		g_print("CreateFileW failed: %08X\n", GetLastError());

		return;
	}

	auto * item = new(std::nothrow) QTreeWidgetItem(ui.tree_files);
	if (item == Q_NULLPTR)
	{
		g_print("Failed to create new list item\n");

		CloseHandle(hFile);

		return;
	}

	item->setCheckState(0, Qt::CheckState::Unchecked);
	item->setText(1, fi.fileName());
	item->setText(2, fi.absoluteFilePath());
	item->setText(3, QString::fromStdWString(ArchToStrW(arch)));

#ifdef _WIN64
	item->setText(4, QString::number(reinterpret_cast<qulonglong>(hFile), 0x10));
#else
	item->setText(4, QString::number(reinterpret_cast<unsigned long>(hFile), 0x10));
#endif

	if (active)
	{
		item->setCheckState(0, Qt::CheckState::Checked);
	}

	if (ui.btn_tooltip->isChecked())
	{
		item->setToolTip(1, fi.fileName());
		item->setToolTip(2, fi.absoluteFilePath());
	}

#ifndef _WIN64
	if (arch != ARCH::X86)
	{
		item->setHidden(true);
		item->setCheckState(0, Qt::CheckState::Unchecked);
	}
#endif

	g_print("Added file: %ls\n", abs_path.c_str());
}

void GuiMain::remove_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	for (auto i : item)
	{
		g_print("Removed file: %ls\n", i->text(2).toStdWString().c_str());

		bool b_ok = false;

#ifdef _WIN64
		HANDLE hFile = (HANDLE)(i->text(4).toULongLong(&b_ok, 0x10));
#else
		HANDLE hFile = (HANDLE)(i->text(4).toULong(&b_ok, 0x10));
#endif

		if (b_ok)
		{
			CloseHandle(hFile);
		}

		delete i;
	}
}

void GuiMain::select_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	if (item.size() == 0)
	{
		return;
	}

	auto path = item[0]->text(2);
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

void GuiMain::delay_inject()
{
	int id = ui.txt_pid->text().toInt();
	if (id == 1337)
	{
		ShellExecuteW(0, 0, L"https://www.youtube.com/watch?v=5t53TcKIlMc", 0, 0, SW_SHOW);
		return;
	}

	int delay = ui.sp_delay->value();
	if (delay > 0)
	{
		t_Delay_Inj->start(delay);
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

	ARCH file_arch = ARCH::NONE;
	ARCH proc_arch = ARCH::NONE;

	// Process ID
	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		Process_Struct ps = getProcessByPID(id);

		if (ps.PID && ps.Arch != ARCH::NONE)
		{
			inj_data.ProcessID	= ps.PID;
			proc_arch			= ps.Arch;
		}
		else
		{
			emit ShowStatusbox(false, "Invalid PID");

			return;
		}
	}
	else // Process Name
	{
		int index = ui.cmb_proc->currentIndex();
		Process_Struct ps = getProcessByNameW(ui.cmb_proc->itemText(index).toStdWString().c_str());

		if (ps.PID && ps.Arch != ARCH::NONE)
		{
			inj_data.ProcessID	= ps.PID;
			proc_arch			= ps.Arch;
		}
		else
		{
			emit ShowStatusbox(false, "Invalid process name");

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

	if (!InjLib.LoadingStatus())
	{
		emit ShowStatusbox(false, "The GH injection library couldn't be found or wasn't loaded correctly.");

		return;
	}

	if (InjLib.GetSymbolState() != INJ_ERR_SUCCESS)
	{
		emit ShowStatusbox(false, "PDB download not finished.");

		return;
	}

	if (InjLib.GetImportState() != INJ_ERR_SUCCESS)
	{
		emit ShowStatusbox(false, "Import handler not finished.");

		return;
	}
	
	std::vector<std::pair<std::wstring, ARCH>> items;

	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if ((*it)->checkState(0) != Qt::CheckState::Checked)
		{
			continue;
		}

		file_arch = StrToArchW((*it)->text(3).toStdWString().c_str());
		if (file_arch == ARCH::NONE)
		{
			continue;
		}

		if (proc_arch != file_arch)
		{
			continue;
		}

		if (!FileExistsW((*it)->text(2).toStdWString().c_str()))
		{
			continue;
		}

		QString fileStr = (*it)->text(2);
		fileStr.replace('/', '\\');
	
		items.push_back(std::pair(fileStr.toStdWString(), file_arch));
	}

	if (items.empty())
	{
		emit ShowStatusbox(false, "No file(s) selected");

		return;
	}

	std::vector<std::string> results;
	int inj_count = 1;

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

	for (const auto & i : items)
	{
		wcscpy_s(inj_data.szDllPath, i.first.c_str());

		auto dll_name_pos = i.first.find_last_of('\\') + 1;
		auto dll_name = i.first.substr(dll_name_pos);

		DWORD res = 0;

		g_print("Launching injection thread\n");

		std::shared_future<DWORD> inj_result = std::async(std::launch::async, &InjectionLib::InjectW, &InjLib, &inj_data);

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

		char buffer[MAX_PATH * 2]{ 0 };

		if (res != 0)
		{
			sprintf_s(buffer, "Injection (%d/%d) failed:\n  Error = %08X\n", inj_count, (int)items.size(), res);
			g_print("Check the error log for more information\n");
		}
		else
		{
			sprintf_s(buffer, "Injection (%d/%d) succeeded:\n  %ls = %p\n", inj_count, (int)items.size(), dll_name.c_str(), inj_data.hDllOut);
		}

		results.push_back(std::string(buffer));

		g_print("Injection %d/%d finished\n", inj_count, items.size());

		++inj_count;
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

void GuiMain::tooltip_change()
{
	int duration = 1;

	if (!tooltipsEnabled)
	{
		duration = -1;
		ui.btn_tooltip->setText("Disable tooltips");
		tooltipsEnabled = true;

		g_print("Tooltips enabled\n");
	}
	else
	{
		ui.btn_tooltip->setText("Enable tooltips");
		tooltipsEnabled = false;

		g_print("Tooltips disabled\n");
	}

	// Settings
	ui.lbl_proc->setToolTipDuration(duration);
	ui.rb_proc->setToolTipDuration(duration);
	ui.cmb_proc->setToolTipDuration(duration);

	ui.lbl_pid->setToolTipDuration(duration);
	ui.rb_pid->setToolTipDuration(duration);
	ui.txt_pid->setToolTipDuration(duration);
	ui.btn_proc->setToolTipDuration(duration);
	ui.txt_arch->setToolTipDuration(duration);
	ui.lbl_arch->setToolTipDuration(duration);
	ui.lbl_proc_icon->setToolTipDuration(duration);

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

	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if (duration < 0)
		{
			(*it)->setToolTip(1, (*it)->text(1));
			(*it)->setToolTip(2, (*it)->text(2));
		}
		else
		{
			(*it)->setToolTip(1, "");
			(*it)->setToolTip(2, "");
		}
	}
}

void GuiMain::open_help()
{
	g_print("Openeing help thread: %ls\n", GH_HELP_URLW);

	auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(0, 0, GH_HELP_URLW, 0, 0, SW_SHOW));
	if (ret <= 32)
	{
		g_print("Failed to open browser:\n Error 1: %08X\n Error 2: %08X\n", ret, GetLastError());
	}
}

QPixmap GuiMain::GetIconFromFileW(const wchar_t * szPath, UINT size, int index)
{
	HICON icon = NULL;
	auto hr = SHDefExtractIconW(szPath, index, NULL, &icon, nullptr, size & 0xFFFF);
	if (FAILED(hr))
	{
		g_print("Failed to locate icon in file:\n path  = %ls\n error = %08X\n", szPath, hr);

		return QPixmap();
	}

	QPixmap pixmap = qt_pixmapFromWinHICON(icon);

	DestroyIcon(icon);

	return pixmap;
}

void GuiMain::show()
{
	framelessParent->show();
}

void GuiMain::generate_shortcut()
{
	std::wstring shortCut;
	QString fileName = "Injector_";

	ARCH target_arch = ARCH::NONE;

	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		if (id)
		{
			Process_Struct ps_local = getProcessByPID(id);
			shortCut += L"-p \"" + std::wstring(ps_local.szName) + L"\"";
			fileName += QString::fromWCharArray(ps_local.szName);

			target_arch = ps_local.Arch;
		}
		else
		{
			emit ShowStatusbox(false, "Invalid PID");
			return;
		}
	}
	else
	{
		int index = ui.cmb_proc->currentIndex();
		auto exeName = ui.cmb_proc->itemText(index).toStdWString();

		Process_Struct ps_local = getProcessByNameW(exeName.c_str());

		if (!ps_local.PID)
		{
			emit ShowStatusbox(false, "The specified process doesn't exist.");
			return;
		}

		shortCut += L"-p \"" + exeName + L"\"";
		fileName += ui.cmb_proc->itemText(index);

		target_arch = ps_local.Arch;
	}

	bool fileFound = false;
	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
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

		ARCH file_arch = StrToArchW((*it)->text(3).toStdWString().c_str());
		if (file_arch != target_arch)
		{
			continue;
		}

		fileName += QString("_") + (*it)->text(1);

		fileStr.replace("/", "\\");
		shortCut += L" -f \"" + fileStr.toStdWString() + L"\"";

		fileFound = true;

		break;
	}

	if (!fileFound)
	{
		emit ShowStatusbox(false, "No valid file selected.");

		return;
	}

	int delay = ui.sp_delay->value();
	if (delay > 0)
	{
		shortCut += L" -delay ";
		std::wstringstream stream;
		stream << delay;
		shortCut += stream.str();
	}

	int timeout = ui.sp_timeout->value();
	if (timeout > 0)
	{
		shortCut += L" -timeout ";
		std::wstringstream stream;
		stream << timeout;
		shortCut += stream.str();
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

		std::wstringstream stream;
		stream << std::hex << Flags;
		shortCut += L" -mmflags " + stream.str();
	}

	shortCut += L" -wait ";

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

			auto path = QCoreApplication::applicationDirPath();
			path.replace('/', '\\');

			std::wstring open_cmd = L"/Select,";

			open_cmd += path.toStdWString();
			open_cmd += L"\\";
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

		ShowStatusbox(false, error_msg);
	}
}

void GuiMain::open_console()
{
	g_Console->open();

	if (g_Console->get_dock_index() == -1)
	{
		int old_idx = g_Console->get_old_dock_index();

		if (old_idx == -1)
		{
			old_idx = dockIndex;
		}
		
		if (old_idx == -1)
		{
			old_idx		= 0;
			dockIndex	= 0;
		}

		g_Console->dock(old_idx);
	}

	consoleOpen = true;
}

void GuiMain::open_log()
{
	if (FileExistsW(GH_INJ_LOGW))
	{
		auto ret = reinterpret_cast<INT_PTR>(ShellExecuteW(NULL, L"edit", GH_INJ_LOGW, nullptr, nullptr, SW_SHOW));
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
		open_console();
	}
		
	char user_agent[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (JHTML, like Gecko) Chrome/74.0.3729.131 Safari/537.36";
	UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, user_agent, sizeof(user_agent), NULL);

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

	t_Update_Files->start(500);

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

void GuiMain::update_clicked()
{
	save_settings();

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