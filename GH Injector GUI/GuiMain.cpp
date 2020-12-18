#include "pch.h"

#include "GuiMain.h"

#include "Banner.h"
#include "Process.h"
#include "Injection.h"
#include "Zip.h"
#include "ShortCut.h"
#include "DownloadProgress.h"
#include "DownloadProgressWindow.h"

int const GuiMain::EXIT_CODE_REBOOT = -123456789;

GuiMain::GuiMain(QWidget * parent)
	: QMainWindow(parent)
{
	settings_get_update();

	interrupt_download = false;
	pre_main_exec_update = true;

	if (!ignoreUpdate)
	{
		update_init();
	}

	pre_main_exec_update = false;

	if (!platformCheck())
	{
		ExitProcess(0);
	}

	native = IsNativeProcess(GetCurrentProcessId());

	if (!InjLib.Init())
	{
		QString failMsg = GH_INJ_MOD_NAMEA + QString(" not found");

		emit injec_status(false, failMsg);
	}

	if (InjLib.LoadingStatus() && !InjLib.SymbolStatus())
	{
		pdb_download();
	}

	if (!SetDebugPrivilege(true))
	{
		emit injec_status(false, "Failed to enable debug privileges. This might affect the functionality of the injector.");
	}

	ui.setupUi(this);

	if (this->statusBar())
	{
		this->statusBar()->hide();
	}

	parent->layout()->setSizeConstraint(QLayout::SetFixedSize);
	ui.grp_settings->layout()->setSizeConstraint(QLayout::SetFixedSize);
	ui.tree_files->setFixedWidth(800);
	ui.tree_files->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustIgnored);

	// Settings
	connect(ui.rb_proc,		SIGNAL(clicked()), this, SLOT(rb_process_set()));
	connect(ui.rb_pid,		SIGNAL(clicked()), this, SLOT(rb_pid_set()));
	connect(ui.btn_proc,	SIGNAL(clicked()), this, SLOT(btn_pick_process_click()));

	// Auto, Reset, Color
	connect(ui.cb_auto,		SIGNAL(clicked()), this, SLOT(auto_inject()));
	connect(ui.btn_reset,	SIGNAL(clicked()), this, SLOT(reset_settings()));
	connect(ui.btn_hooks,	SIGNAL(clicked()), this, SLOT(hook_Scan()));

	// Method, Cloaking, Advanced
	connect(ui.cmb_load,		SIGNAL(currentIndexChanged(int)),	this, SLOT(load_change(int)));
	connect(ui.cmb_create,		SIGNAL(currentIndexChanged(int)),	this, SLOT(create_change(int)));
	connect(ui.cb_main,			SIGNAL(clicked()),					this, SLOT(cb_main_clicked()));
	connect(ui.cb_protection,	SIGNAL(clicked()),					this, SLOT(cb_page_protection_clicked()));

	// Files
	connect(ui.btn_add,		SIGNAL(clicked()),									this, SLOT(add_file_dialog()));
	connect(ui.btn_inject,	SIGNAL(clicked()),									this, SLOT(delay_inject()));
	connect(ui.btn_remove,	SIGNAL(clicked()),									this, SLOT(remove_file()));
	connect(ui.tree_files,	SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),	this, SLOT(select_file()));

	// Info
	connect(ui.btn_tooltip,		SIGNAL(clicked()), this, SLOT(tooltip_change()));
	connect(ui.btn_help,		SIGNAL(clicked()), this, SLOT(open_help()));
	connect(ui.btn_openlog,		SIGNAL(clicked()), this, SLOT(open_log()));
	connect(ui.btn_shortcut,	SIGNAL(clicked()), this, SLOT(generate_shortcut()));
	connect(ui.btn_version,		SIGNAL(clicked()), this, SLOT(update_init()));

	ui.btn_version->setText("V" GH_INJ_VERSIONA);

	gui_Picker		= new GuiProcess(&framelessPicker, &framelessPicker);
	gui_Scanner		= new GuiScanHook(&framelessScanner, &framelessScanner, &InjLib);
	t_Auto_Inj		= new QTimer(this);
	t_Delay_Inj		= new QTimer(this);
	t_Update_Proc	= new QTimer(this);
	t_OnUserInput	= new QTimer(this);
	pss				= new Process_State_Struct;
	ps_picker		= new Process_Struct;

	framelessPicker.setWindowTitle("Select a process");
	framelessPicker.setContent(gui_Picker);
	framelessPicker.resize(QSize(460, 500));
	framelessPicker.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessPicker.setWindowModality(Qt::WindowModality::ApplicationModal);
	
	framelessScanner.setWindowTitle("Scan for hooks");
	framelessScanner.setContent(gui_Scanner);
	framelessScanner.resize(QSize(320, 230));
	framelessScanner.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessScanner.setWindowModality(Qt::WindowModality::ApplicationModal);
	
	ui.btn_openlog->setIcon(QIcon(":/GuiMain/gh_resource/log.ico"));

	t_Delay_Inj->setSingleShot(true);
	t_OnUserInput->setSingleShot(true);

	pss->cbSession	= true;
	pss->cmbArch	= 0;
	pss->txtFilter	= "";

	onReset			= false;
	lbl_hide_banner = false;

	memset(ps_picker, 0, sizeof(Process_Struct));

	// Process Picker
	connect(this,		SIGNAL(send_to_picker(Process_State_Struct *, Process_Struct *)),	gui_Picker, SLOT(get_from_inj(Process_State_Struct *, Process_Struct *)));
	connect(gui_Picker, SIGNAL(send_to_inj(Process_State_Struct *, Process_Struct *)),		this,		SLOT(get_from_picker(Process_State_Struct *, Process_Struct *)));

	// Scan Hook
	connect(this,			SIGNAL(send_to_scan_hook(int, int)),	gui_Scanner,	SLOT(get_from_inj_to_sh(int, int)));
	connect(gui_Scanner,	SIGNAL(send_to_inj_sh(int, int)),		this,			SLOT(get_from_scan_hook(int, int)));

	connect(t_Auto_Inj,		SIGNAL(timeout()), this, SLOT(auto_loop_inject()));
	connect(t_Delay_Inj,	SIGNAL(timeout()), this, SLOT(inject_file()));
	connect(t_Update_Proc,	SIGNAL(timeout()), this, SLOT(update_process()));

	ui.tree_files->setColumnWidth(0, 50);
	ui.tree_files->setColumnWidth(1, 178);
	ui.tree_files->setColumnWidth(2, 454);
	ui.tree_files->setColumnWidth(3, 100);

	ui.tree_files->clear();

	load_settings();
	load_banner();
	load_change(0);
	create_change(0);
	hide_banner();
	auto_inject();

	QSize winSize = this->parentWidget()->size();
	winSize.setHeight(400);
	winSize.setWidth(1000);
	parentWidget()->resize(winSize);

	this->installEventFilter(this);
	ui.tree_files->installEventFilter(this);
	ui.txt_pid->setValidator(new QRegExpValidator(QRegExp("[0-9]+")));
	ui.txt_pid->installEventFilter(this);

	ui.txt_delay->setValidator(new QRegExpValidator(QRegExp("[0-9]+")));

	ui.txt_timeout->setValidator(new QRegExpValidator(QRegExp("[0-9]+")));

	if (!native)
	{
		// Won't work if not native
		ui.cb_hijack->setChecked(false);
		ui.cb_hijack->setDisabled(true);
	}

	Process_Struct byName	= getProcessByNameW(ui.cmb_proc->currentText().toStdWString().c_str());
	Process_Struct byPID	= getProcessByPID(ui.txt_pid->text().toInt());

	if (ui.rb_pid->isChecked() && byPID.PID)
	{
		txt_pid_change();
	}
	else if (ui.rb_proc->isChecked() && byName.PID)
	{
		cmb_proc_name_change();
	}
	
	t_Update_Proc->start(100);

	btn_change();

	if (!InjLib.LoadingStatus() || !InjLib.SymbolStatus())
	{
		ui.btn_inject->setEnabled(false);
	}
}

GuiMain::~GuiMain()
{
	save_settings();

	delete gui_Picker;
	delete gui_Scanner;
	delete t_Auto_Inj;
	delete t_Delay_Inj;
	delete t_Update_Proc;
	delete t_OnUserInput;
	delete pss;
	delete ps_picker;

	//force unload module because std::async increases LDR_DDAG_NODE::LoadCount
	//but std::async threads get terminated without calling the DllMain with DLL_THREAD_DETACH
	//so there's no way to free dependencies properly
	//wtf bill??????

	HINSTANCE hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	while (hMod)
	{
		FreeLibrary(hMod);
		hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	}
}

void GuiMain::update_process()
{
	int raw = ui.txt_pid->text().toInt();
	Process_Struct byName	= getProcessByNameW(ui.cmb_proc->currentText().toStdWString().c_str());
	Process_Struct byPID	= getProcessByPID(ui.txt_pid->text().toInt());

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
			}
		}
		else if (!t_OnUserInput->isActive())
		{
			if (byName.PID)
			{
				cmb_proc_name_change();
				btn_change();
			}
			else if (raw)
			{
				ui.txt_pid->setText("0");
				ui.txt_arch->setText("---");
				ui.txt_pid->setToolTip("");
				ui.cmb_proc->setToolTip("");
				btn_change();
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
			}
		}
		else if (ui.txt_pid->text().toInt() != 0)
		{
			ui.txt_pid->setText("0");
			ui.txt_arch->setText("---");
			ui.txt_pid->setToolTip("");
			ui.cmb_proc->setToolTip("");
			btn_change();
		}
	}	
}

ARCH GuiMain::str_to_arch(const QString str)
{
	if (str == "x64")
	{
		return ARCH::X64;
	}
	else if (str == "x86")
	{
		return ARCH::X86;
	}
	
	return ARCH::NONE;
}

QString GuiMain::arch_to_str(const ARCH arch)
{
	if (arch == ARCH::X64)
	{
		return QString("x64");
	}
	else if (arch == ARCH::X86)
	{
		return QString("x86");
	}
	
	return QString("---");
}

void GuiMain::closeEvent(QCloseEvent * event)
{
	save_settings();
}

std::string GuiMain::get_newest_version()
{
	wchar_t cacheFile[MAX_PATH]{ 0 };
	HRESULT hRes = URLDownloadToCacheFileW(nullptr, GH_VERSION_URL, cacheFile, sizeof(cacheFile) / sizeof(wchar_t), 0, nullptr);

	if (hRes != S_OK)
	{
		return "0.0";
	}

	// Read file 
	std::ifstream infile(cacheFile, std::ifstream::in);

	if (!infile.good())
	{
		return "0.0";
	}

	// ???????????????
	// Why is this different on local debug version
	std::string strVer;
	infile >> strVer;

	infile.close();

	return strVer;
}

bool GuiMain::eventFilter(QObject * obj, QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if (obj == ui.tree_files)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
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
			QKeyEvent * keyEvent = static_cast<QKeyEvent *>(event);
			if (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_9 || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Space)
			{
				t_OnUserInput->start(10000);
			}
		}
	}

	return QObject::eventFilter(obj, event);
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

#pragma region shitcode

//the next three functions are the worst code ever written
void GuiMain::update_download_progress(DownloadProgressWindow * wnd, DownloadProgress * progress)
{
	while (progress->GetDownloadProgress() < 1.0f && !interrupt_download)
	{
		wnd->SetProgress(0, progress->GetDownloadProgress());
		wnd->SetStatus(progress->GetStatusText().c_str());

		Sleep(25);
	}

	wnd->SetProgress(0, progress->GetDownloadProgress());

	interrupt_download = false;
}

void GuiMain::update_update_thread(DownloadProgressWindow * wnd, std::string version)
{
	auto path = QCoreApplication::applicationDirPath().toStdString();
	path += "/";

	auto zip_path = path + GH_INJECTOR_ZIP;

	DeleteFileA(zip_path.c_str());
	std::string download_url = GH_DOWNLOAD_PREFIX;
	download_url += version;
	download_url += GH_DOWNLOAD_SUFFIX;

	DownloadProgress progress;
	auto thread = std::thread(&GuiMain::update_download_progress, this, wnd, &progress);

	HRESULT hr = URLDownloadToFileA(nullptr, download_url.c_str(), zip_path.c_str(), NULL, &progress);
	if (FAILED(hr))
	{
		interrupt_download = true;
		thread.join();

		std::stringstream stream;
		stream << std::hex << hr;
		std::string new_status = "URLDownlaodToFileA failed with 0x";
		new_status += stream.str();
		wnd->SetStatus(new_status.c_str());
		wnd->done(-1);

		return;
	}

	thread.join();

	if (!pre_main_exec_update)
	{
		save_settings();
	}

	HINSTANCE hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	while (hMod)
	{
		FreeLibrary(hMod);
		hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	}

	wnd->SetStatus("Injection module unlaoded");

	auto old_path = path + "OLD.exe";
	DeleteFileA(old_path.c_str());

	if (!MoveFileA(QCoreApplication::applicationFilePath().toStdString().c_str(), old_path.c_str()))
	{
		std::stringstream stream;
		stream << std::hex << GetLastError();
		std::string new_status = "MoveFileA failed with 0x%08X";
		new_status += stream.str();
		wnd->SetStatus(new_status.c_str());
		wnd->done(-1);

		return;
	}

	wnd->SetStatus("Removing old files...");

	DeleteFileW(GH_INJ_MOD_NAME86W);
	DeleteFileW(GH_INJ_MOD_NAME64W);
	DeleteFileA(GH_INJECTOR_SM_X86);
	DeleteFileA(GH_INJECTOR_SM_X64);

#ifdef _WIN64
	DeleteFileA(GH_INJECTOR_EXE_X86);
#else
	DeleteFileA(GH_INJECTOR_EXE_X64);
#endif

	wnd->SetStatus("Unzip new files...");

	if (Unzip(zip_path.c_str(), path.c_str()) != 0)
	{
		std::string new_status = "Failed to unzip files";
		wnd->SetStatus(new_status.c_str());
		wnd->done(-1);

		return;
	}

	DeleteFileA(zip_path.c_str());

	wnd->SetStatus("Launching updated version...");

	auto new_path = path + "GH Injector.exe ";

	STARTUPINFOA si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	std::stringstream stream;
	stream << GetCurrentProcessId();
	new_path += stream.str();

	CreateProcessA(nullptr, (char *)new_path.c_str(), nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	Sleep(1000);

	wnd->done(0);

	ExitProcess(0);
}

void GuiMain::update_injector(std::string version)
{	
	std::vector<QString> labels;
	labels.push_back("");

	DownloadProgressWindow * wnd = new DownloadProgressWindow(QString("Updating to V") + version.c_str(), labels, "Initializing...", 250, &framelessUpdate, &framelessUpdate);
	framelessUpdate.setContent(wnd);
	framelessUpdate.setFixedSize(QSize(300, 120));
	framelessUpdate.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessUpdate.setWindowModality(Qt::WindowModality::ApplicationModal);
	framelessUpdate.show();

	auto exec_thread = std::thread(&GuiMain::update_update_thread, this, wnd, version);
	auto ret = wnd->exec();
	if (ret == -1)
	{
		injec_status(false, wnd->GetStatus());
	}

	exec_thread.join();

	delete wnd;

	framelessUpdate.hide();
}

#pragma endregion

bool GuiMain::platformCheck()
{
#ifdef _WIN64
	return true;
#else
	// windows 64-bit == gh64.exe
	bool bPlatform = IsNativeProcess(GetCurrentProcessId());
	if (bPlatform == true)
	{
		return true;
	}

	QMessageBox::StandardButton reply;
	reply = QMessageBox::warning(nullptr, "Warning architecture conflict", "Since you're using a "\
		"64-bit version of Windows it's recommended to use the 64-bit version of the injector. "\
		"Do you want to switch to the 64-bit version?", QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::No)
	{
		return true;
	}

	STARTUPINFOA si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	auto x64_path = QCoreApplication::applicationDirPath().toStdString();
	x64_path += "/";
	x64_path += GH_INJ_EXE_NAME64A;
	CreateProcessA(x64_path.c_str(), nullptr, nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

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
	if (gui_Picker->parentWidget())
	{
		framelessPicker.show();
	}
	else
	{
		gui_Picker->show();
	}

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

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	ui.txt_pid->setText(QString::number(pl.PID));
	QString new_pid = QString::asprintf("0x%08X", pl.PID);
	ui.txt_pid->setToolTip(new_pid);

	ui.cmb_proc->setToolTip(QString::fromWCharArray(pl.szPath));

	ui.txt_arch->setText(GuiMain::arch_to_str(pl.Arch));

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

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	QString new_pid = QString::asprintf("0x%08X", pl.PID);
	ui.txt_pid->setToolTip(new_pid);

	ui.cmb_proc->setCurrentText(QString::fromWCharArray(pl.szName));
	ui.cmb_proc->setToolTip(QString::fromWCharArray(pl.szPath));

	ui.txt_arch->setText(GuiMain::arch_to_str(pl.Arch));

	if (ui.cmb_proc->findText(QString::fromWCharArray(pl.szName)) == -1)
	{
		ui.cmb_proc->addItem(QString::fromWCharArray(pl.szName));
	}
}

void GuiMain::btn_change()
{
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
}

void GuiMain::get_from_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct)
{
	pss			= procStateStruct;
	ps_picker	= procStruct;

	if (gui_Picker->parentWidget())
	{
		framelessPicker.hide();
	}
	else
	{
		gui_Picker->hide();
	}

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
		ui.txt_arch->setText(GuiMain::arch_to_str(ps_picker->Arch));

		txt_pid_change();
		rb_pid_set();
	}

	btn_change();
}

void GuiMain::get_from_scan_hook(int pid, int error)
{
	if (gui_Picker->parentWidget())
	{
		framelessPicker.hide();
	}
	else
	{
		gui_Picker->hide();
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

			if (str_to_arch((*it)->text(3)) != arch)
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

void GuiMain::load_banner()
{
	QPixmap pix_banner;
	pix_banner.loadFromData(getBanner(), getBannerLen(), "JPG");
	ui.lbl_img->setPixmap(pix_banner);
}

void GuiMain::hide_banner()
{
	if (lbl_hide_banner == true)
	{
		ui.lbl_img->setVisible(false);
	}
}

void GuiMain::reset_settings()
{
	onReset = true;

	QFileDialog fDialog(this, "Select dll files", QApplication::applicationDirPath(), "Dynamic Link Libraries (*.dll)");

	QFile iniFile(GH_SETTINGS_INI);
	if (iniFile.exists())
	{
		iniFile.remove();
	}

	emit slotReboot();
}

void GuiMain::slotReboot()
{
	qApp->exit(GuiMain::EXIT_CODE_REBOOT);
}

void GuiMain::hook_Scan()
{
	if (gui_Scanner->parentWidget())
	{
		framelessScanner.show();
	}
	else
	{
		gui_Scanner->show();
	}

	emit send_to_scan_hook(ps_picker->PID, 0);
}

void GuiMain::settings_get_update()
{
	QFile iniFile(GH_SETTINGS_INI);
	if (!iniFile.exists())
	{
		printf("file doesn't exist\n");
		ignoreUpdate = false;
	}

	QSettings settings(GH_SETTINGS_INI, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.beginGroup("CONFIG");
	ignoreUpdate = settings.value("IGNOREUPDATES").toBool();
	settings.endGroup();
}

void GuiMain::save_settings()
{
	if (onReset)
	{
		onReset = false;

		return;
	}

	QSettings settings(GH_SETTINGS_INI, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	settings.beginWriteArray("FILES");
	int i = 0;

	QTreeWidgetItemIterator it(ui.tree_files);
	for (; *it; ++it, ++i)
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
	settings.setValue("DELAY", ui.txt_delay->text());
	settings.setValue("AUTOINJ", ui.cb_auto->isChecked());
	settings.setValue("CLOSEONINJ", ui.cb_close->isChecked());
	settings.setValue("TIMEOUT", ui.txt_timeout->text());
	settings.setValue("ERRORLOG", ui.cb_error->isChecked());

	// Method
	settings.setValue("MODE", ui.cmb_load->currentIndex());
	settings.setValue("LAUNCHMETHOD", ui.cmb_create->currentIndex());
	settings.setValue("HIJACK", ui.cb_hijack->isChecked());
	settings.setValue("CLOAK", ui.cb_clock->isChecked());

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

	// Process picker
	settings.setValue("PROCNAMEFILTER", pss->txtFilter);
	settings.setValue("PROCESSTYPE", pss->cmbArch);
	settings.setValue("CURRENTSESSION", pss->cbSession);

	// Info
	settings.setValue("TOOLTIPSON", ui.btn_tooltip->isChecked());

	// Not visible
	settings.setValue("LASTDIR", lastPathStr);
	settings.setValue("IGNOREUPDATES", ignoreUpdate);
	settings.setValue("HIDEBANNER", lbl_hide_banner);
	settings.setValue("STATE", saveState());
	settings.setValue("GEOMETRY", saveGeometry());
	// Broken on frameless window

	settings.endGroup();
}

void GuiMain::load_settings()
{
	QFile iniFile(GH_SETTINGS_INI);
	if (!iniFile.exists())
	{
		lastPathStr = QApplication::applicationDirPath();
		ignoreUpdate = false;
		return;
	}

	QSettings settings(GH_SETTINGS_INI, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	int fileSize = settings.beginReadArray("FILES");
	for (int i = 0; i < fileSize; ++i)
	{
		settings.setArrayIndex(i);

		auto path = settings.value(QString::number(0)).toString();

		add_file_to_list(
			path,
			settings.value(QString::number(1)).toBool()
		);
	}
	settings.endArray();

	int procSize = settings.beginReadArray("PROCESS");
	for (int i = 0; i < procSize; ++i)
	{
		settings.setArrayIndex(i);
		ui.cmb_proc->addItem(settings.value(QString::number(0)).toString());
	}
	settings.endArray();

	settings.beginGroup("CONFIG");

	// Settings
	ui.cmb_proc->setCurrentIndex(settings.value("PROCESS").toInt());
	ui.txt_pid->setText(settings.value("PID").toString());
	ui.rb_proc->setChecked(settings.value("PROCESSBYNAME").toBool());
	ui.txt_arch->setText(settings.value("ARCH").toString());
	ui.txt_delay->setText(settings.value("DELAY").toString());
	ui.cb_auto->setChecked(settings.value("AUTOINJ").toBool());
	ui.cb_close->setChecked(settings.value("CLOSEONINJ").toBool());
	ui.txt_timeout->setText(settings.value("TIMEOUT").toString());
	ui.cb_error->setChecked(settings.value("ERRORLOG").toBool());

	// Method
	ui.cmb_load->setCurrentIndex(settings.value("MODE").toInt());
	ui.cmb_create->setCurrentIndex(settings.value("LAUNCHMETHOD").toInt());
	ui.cb_hijack->setChecked(settings.value("HIJACK").toBool());
	ui.cb_clock->setChecked(settings.value("CLOAK").toBool());

	// Cloaking
	ui.cmb_peh->setCurrentIndex(settings.value("PEH").toInt());
	ui.cb_unlink->setChecked(settings.value("UNLINKPEB").toBool());
	ui.cb_random->setChecked(settings.value("RANDOMIZE").toBool());
	ui.cb_copy->setChecked(settings.value("DLLCOPY").toBool());

	// manual mapping
	ui.cb_clean->setChecked(settings.value("CLEANDIR").toBool());
	ui.cb_cookie->setChecked(settings.value("INITCOOKIE").toBool());
	ui.cb_imports->setChecked(settings.value("IMPORTS").toBool());
	ui.cb_delay->setChecked(settings.value("DELAYIMPORTS").toBool());
	ui.cb_tls->setChecked(settings.value("TLS").toBool());
	ui.cb_seh->setChecked(settings.value("EXCEPTION").toBool());
	ui.cb_protection->setChecked(settings.value("PROTECTION").toBool());
	ui.cb_main->setChecked(settings.value("DLLMAIN").toBool());

	// Process picker
	pss->txtFilter = settings.value("PROCNAMEFILTER").toString();
	pss->cmbArch = settings.value("PROCESSTYPE").toInt();
	pss->cbSession = settings.value("CURRENTSESSION").toBool();

	// Info
	ui.btn_tooltip->setChecked(settings.value("TOOLTIPSON").toBool());

	// Not visible
	lastPathStr = settings.value("LASTDIR").toString();
	ignoreUpdate = settings.value("IGNOREUPDATES").toBool();
	lbl_hide_banner = settings.value("HIDEBANNER", false).toBool();
	restoreState(settings.value("STATE").toByteArray());
	restoreGeometry(settings.value("GEOMETRY").toByteArray());

	settings.endGroup();
}

void GuiMain::load_change(int index)
{
	UNREFERENCED_PARAMETER(index);

	INJECTION_MODE mode = (INJECTION_MODE)ui.cmb_load->currentIndex();

	switch (mode)
	{
		case INJECTION_MODE::IM_LoadLibraryExW:
			ui.cmb_load->setToolTip("LoadLibraryExW is the default injection method which simply uses LoadLibraryExW.");
			ui.grp_adv->setVisible(false);
			ui.cb_unlink->setEnabled(true);
			break;
		case INJECTION_MODE::IM_LdrLoadDll:
			ui.cmb_load->setToolTip("LdrLoadDll is an advanced injection method which uses LdrLoadDll and bypasses LoadLibrary(Ex) hooks.");
			ui.grp_adv->setVisible(false);
			ui.cb_unlink->setEnabled(true);
			break;
		case INJECTION_MODE::IM_LdrpLoadDll:
			ui.cmb_load->setToolTip("LdrpLoadDll is an advanced injection method which uses LdrpLoadDll and bypasses LdrLoadDll hooks.");
			ui.grp_adv->setVisible(false);
			ui.cb_unlink->setEnabled(true);
			break;
		default:
			ui.cmb_load->setToolTip("ManualMap is an advanced injection technique which bypasses most module detection methods.");
			ui.grp_adv->setVisible(true);
			ui.cb_unlink->setEnabled(false);
			ui.cb_unlink->setChecked(false);
			cb_main_clicked();
			cb_page_protection_clicked();
			break;
	}
}

void GuiMain::create_change(int index)
{
	UNREFERENCED_PARAMETER(index);

	LAUNCH_METHOD method = (LAUNCH_METHOD)ui.cmb_create->currentIndex();

	switch (method)
	{
		case LAUNCH_METHOD::LM_NtCreateThreadEx:
			ui.cmb_create->setToolTip("NtCreateThreadEx: Creates a simple remote thread to load the dll(s).");
			ui.cb_clock->setEnabled(true);
			break;
		case LAUNCH_METHOD::LM_HijackThread:
			ui.cmb_create->setToolTip("Thread hijacking: Redirects a thread to a codecave to load the dll(s).");
			ui.cb_clock->setEnabled(false);
			ui.cb_clock->setChecked(false);
			break;
		case LAUNCH_METHOD::LM_SetWindowsHookEx:
			ui.cmb_create->setToolTip("SetWindowsHookEx: Adds a hook into the window callback list which then loads the dll(s).");
			ui.cb_clock->setEnabled(false);
			ui.cb_clock->setChecked(false);
			break;
		default:
			ui.cmb_create->setToolTip("QueueUserAPC: Registers an asynchronous procedure call to the process' threads which then loads the dll(s).");
			ui.cb_clock->setEnabled(false);
			ui.cb_clock->setChecked(false);
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
	}
	else
	{
		ui.cb_clean->setEnabled(true);
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
	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if ((*it)->text(2) == str)
		{
			return;
		}
	}

#ifndef _WIN64
	if (!native)
	{
		str.replace(":/Windows/System32/", ":/Windows/Sysnative/", Qt::CaseSensitivity::CaseInsensitive);
	}
#else
	str.replace(":/Windows/Sysnative/", ":/Windows/System32/", Qt::CaseSensitivity::CaseInsensitive);
#endif

	QFileInfo fi(str);

	ARCH arch = getFileArchW(fi.absoluteFilePath().toStdWString().c_str());
	if (arch == ARCH::NONE)
	{
		return;
	}

	QTreeWidgetItem * item = new QTreeWidgetItem(ui.tree_files);

	item->setCheckState(0, Qt::CheckState::Unchecked);
	item->setText(1, fi.fileName());
	item->setText(2, fi.absoluteFilePath());
	item->setText(3, arch_to_str(arch));

	if (active)
	{
		item->setCheckState(0, Qt::CheckState::Checked);
	}

#ifndef _WIN64
	if (arch != ARCH::X86)
	{
		item->setHidden(true);
		item->setCheckState(0, Qt::CheckState::Unchecked);
	}
#endif
}

void GuiMain::remove_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	for (auto i : item)
	{
		delete i;
	}
}

void GuiMain::select_file()
{
	QList<QTreeWidgetItem *> item = ui.tree_files->selectedItems();

	auto path = item[0]->text(2).toStdWString();
	auto pos = path.find_last_of('/');
	path.resize(pos);

	ShellExecuteW(NULL, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
}

void GuiMain::delay_inject()
{
	int delay = ui.txt_delay->text().toInt();
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
	INJECTIONDATAW data;
	memset(&data, 0, sizeof(INJECTIONDATAW));

	ARCH file_arch = ARCH::NONE;
	ARCH proc_arch = ARCH::NONE;

	// Process ID
	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		Process_Struct ps = getProcessByPID(id);

		if (ps.PID && ps.Arch != ARCH::NONE)
		{
			data.ProcessID	= ps.PID;
			proc_arch		= ps.Arch;
		}
		else
		{
			emit injec_status(false, "Invalid PID");

			return;
		}
	}
	else // Process Name
	{
		int index = ui.cmb_proc->currentIndex();
		Process_Struct ps = getProcessByNameW(ui.cmb_proc->itemText(index).toStdWString().c_str());

		if (ps.PID && ps.Arch != ARCH::NONE)
		{
			data.ProcessID	= ps.PID;
			proc_arch		= ps.Arch;
		}
		else
		{
			emit injec_status(false, "Invalid Process Name");

			return;
		}
	}

	switch (ui.cmb_load->currentIndex())
	{
		case 1:  data.Mode = INJECTION_MODE::IM_LdrLoadDll;		break;
		case 2:  data.Mode = INJECTION_MODE::IM_LdrpLoadDll;	break;
		case 3:  data.Mode = INJECTION_MODE::IM_ManualMap;		break;
		default: data.Mode = INJECTION_MODE::IM_LoadLibraryExW; break;
	}

	switch (ui.cmb_create->currentIndex())
	{
		case 1:  data.Method = LAUNCH_METHOD::LM_HijackThread;		break;
		case 2:  data.Method = LAUNCH_METHOD::LM_SetWindowsHookEx;	break;
		case 3:  data.Method = LAUNCH_METHOD::LM_QueueUserAPC;		break;
		default: data.Method = LAUNCH_METHOD::LM_NtCreateThreadEx;	break;
	}

	if (ui.cmb_peh->currentIndex() == 1)	data.Flags |= INJ_ERASE_HEADER;
	if (ui.cmb_peh->currentIndex() == 2)	data.Flags |= INJ_FAKE_HEADER;
	if (ui.cb_unlink->isChecked())			data.Flags |= INJ_UNLINK_FROM_PEB;
	if (ui.cb_clock->isChecked())			data.Flags |= INJ_THREAD_CREATE_CLOAKED;
	if (ui.cb_random->isChecked())			data.Flags |= INJ_SCRAMBLE_DLL_NAME;
	if (ui.cb_copy->isChecked())			data.Flags |= INJ_LOAD_DLL_COPY;
	if (ui.cb_hijack->isChecked())			data.Flags |= INJ_HIJACK_HANDLE;

	if (data.Mode == INJECTION_MODE::IM_ManualMap)
	{
		if (ui.cb_clean->isChecked())		data.Flags |= INJ_MM_CLEAN_DATA_DIR;
		if (ui.cb_cookie->isChecked())		data.Flags |= INJ_MM_INIT_SECURITY_COOKIE;
		if (ui.cb_imports->isChecked())		data.Flags |= INJ_MM_RESOLVE_IMPORTS;
		if (ui.cb_delay->isChecked())		data.Flags |= INJ_MM_RESOLVE_DELAY_IMPORTS;
		if (ui.cb_tls->isChecked())			data.Flags |= INJ_MM_EXECUTE_TLS;
		if (ui.cb_seh->isChecked())			data.Flags |= INJ_MM_ENABLE_EXCEPTIONS;
		if (ui.cb_protection->isChecked())	data.Flags |= INJ_MM_SET_PAGE_PROTECTIONS;
		if (ui.cb_main->isChecked())		data.Flags |= INJ_MM_RUN_DLL_MAIN;
	}

	data.GenerateErrorLog = ui.cb_error->isChecked();

	int Timeout = ui.txt_timeout->text().toInt();
	if (Timeout > 0)
	{
		data.Timeout = Timeout;
	}
	else
	{
		data.Timeout = 2000;
	}

	if (!InjLib.LoadingStatus())
	{
		emit injec_status(false, "The GH injection library couldn't be found or wasn't loaded correctly.");
		return;
	}

	if (!InjLib.SymbolStatus())
	{
		emit injec_status(false, "PDB download not finished.");
		return;
	}
	
	std::vector<std::pair<std::wstring, ARCH>> items;

	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if ((*it)->checkState(0) != Qt::CheckState::Checked)
		{
			continue;
		}

		file_arch = str_to_arch((*it)->text(3));
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
		emit injec_status(false, "No file(s) selected");
		return;
	}

	std::wstring results;

	for (const auto & i : items)
	{
		wcscpy_s(data.szDllPath, i.first.c_str());

		auto dll_name_pos = i.first.find_last_of('\\') + 1;
		auto dll_name = i.first.substr(dll_name_pos);
		results += dll_name + L":\n";

		std::wstringstream stream;
		std::wstring result;

		DWORD res = InjLib.InjectFuncW(&data);
		if (res != ERROR_SUCCESS)
		{
			stream << std::hex << res;

			//manually add leading 0's to error code
			result = std::wstring(8 - stream.str().length(), '0') + stream.str();
			results += L"     Error = 0x";
		}
		else
		{
			stream << std::hex << reinterpret_cast<UINT_PTR>(data.hDllOut);

			//manually add leading 0's to error code
			if (i.second == ARCH::X86)
			{
				result = std::wstring(8 - stream.str().length(), '0') + stream.str();
			}
			else
			{
				result = std::wstring(0x10 - stream.str().length(), '0') + stream.str();
			}

			results += L"     DllBase = 0x";
		}

		results += result;
		results += L"\n\n";
	}

	if (ui.cb_close->isChecked())
	{
		qApp->exit(0);

		return; //just in case lmao
	}

	QMessageBox messageBox;
	messageBox.information(0, "Injection result(s)", QString::fromWCharArray(results.c_str()));
}

void GuiMain::injec_status(bool ok, const QString msg)
{
	if (ok)
	{
		QMessageBox messageBox;
		messageBox.information(0, "Success", msg);
	}
	else
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", msg);
	}
}

void GuiMain::tooltip_change()
{
	if (ui.btn_tooltip->isChecked())
	{
		ui.btn_tooltip->setText("Disable tooltips");
	}
	else
	{
		ui.btn_tooltip->setText("Enable tooltips");
	}

	int duration = 1;
	if (ui.btn_tooltip->isChecked())
	{
		duration = -1;
	}

	// Settings
	ui.lbl_proc->setToolTipDuration(duration);
	ui.rb_proc->setToolTipDuration(duration);
	ui.cmb_proc->setToolTipDuration(duration);

	ui.lbl_pid->setToolTipDuration(duration);
	ui.rb_pid->setToolTipDuration(duration);
	ui.txt_pid->setToolTipDuration(duration);
	ui.btn_proc->setToolTipDuration(duration);

	ui.lbl_delay->setToolTipDuration(duration);
	ui.txt_delay->setToolTipDuration(duration);
	ui.cb_close->setToolTipDuration(duration);
	ui.cb_auto->setToolTipDuration(duration);
	ui.lbl_timeout->setToolTipDuration(duration);
	ui.txt_timeout->setToolTipDuration(duration);
	ui.cb_error->setToolTipDuration(duration);

	// Method
	ui.cmb_load->setToolTipDuration(duration);
	ui.cb_hijack->setToolTipDuration(duration);
	ui.cmb_create->setToolTipDuration(duration);
	ui.cb_clock->setToolTipDuration(duration);

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
}

void GuiMain::open_help()
{
	bool ok = QDesktopServices::openUrl(QUrl(GH_HELP_URL, QUrl::TolerantMode));
}

void GuiMain::pdb_download_update_thread(DownloadProgressWindow * wnd)
{
	bool connected = false;

	while (!InjLib.SymbolStatus())
	{
		float progress_0 = InjLib.DownloadProgress(false);
		wnd->SetProgress(0, progress_0);

#ifdef _WIN64
		float progress_1 = InjLib.DownloadProgress(true);
		wnd->SetProgress(1, progress_1);
#endif

		if (InternetCheckConnectionA("https://msdl.microsoft.com", FLAG_ICC_FORCE_CONNECTION, NULL) == FALSE)
		{
			if (connected)
			{
				wnd->SetStatus("Waiting for internet connection...");
				connected = false;
				wnd->done(-1);
				return;
			}
		}
		else
		{
			if (!connected)
			{
				wnd->SetStatus("Downloading PDB files from the Microsoft Symbol Server...");
				connected = true;
			}
		}

		Sleep(25);
	};

	wnd->done(0);
}

//this code is terrible that it proves that god is dead
void GuiMain::pdb_download()
{
	std::vector<QString> labels;
	labels.push_back("ntdll.pdb");
#ifdef _WIN64
	labels.push_back("wntdll.pdb");
#endif

	DownloadProgressWindow * wnd = new DownloadProgressWindow("PDB Download", labels, "Waiting for connection...", 300, &framelessUpdate, &framelessUpdate);
	framelessUpdate.setContent(wnd);
	framelessUpdate.resize(QSize(300, 170));
	framelessUpdate.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessUpdate.show();

	auto exec_thread = std::thread(&GuiMain::pdb_download_update_thread, this, wnd);
	if (wnd->exec() == -1)
	{
		injec_status(false, "Download interrupted. The PDB files are necessary\nfor the injector to work.\nMake sure your PC is connected to the internet.");
	}
	exec_thread.join();

	delete wnd;

	framelessUpdate.hide();
}

// -p "ProcessName"
// -f "FileName"
// -d delay (default = 0)
// -t timeout (default = 2000)
// -l 0-3 (launchmethod, default = 0)
// -s 0-3 (startmethod, default = 0)
// -peh 0-2 (default = 0)
// -log
// -unlink
// -cloak
// -randomize
// -copy
// -hijack
// -mmflags hex_value (manual mapping flags)

void GuiMain::generate_shortcut()
{
	std::wstring shortCut;
	QString fileName = "Injector_";

	// Process ID
	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		if (id)
		{
			Process_Struct ps_local = getProcessByPID(id);
			shortCut += L"-p \"" + std::wstring(ps_local.szName) + L"\"";
			fileName += QString::fromWCharArray(ps_local.szName);
		}
		else
		{
			emit injec_status(false, "Invalid PID");
			return;
		}
	}
	else // Process Name
	{
		int index = ui.cmb_proc->currentIndex();
		shortCut += L"-p \"" + ui.cmb_proc->itemText(index).toStdWString() + L"\"";
		fileName += ui.cmb_proc->itemText(index);
	}

	bool fileFound = false;
	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		if (fileFound)
			break;

		// Find Item
		if ((*it)->checkState(0) != Qt::CheckState::Checked)
			continue;

		// Convert String
		QString fileStr = (*it)->text(2);

		fileName += QString("_") + (*it)->text(1);

		// Check Existens
		QFile qf(fileStr);
		if (!qf.exists())
		{
			emit injec_status(false, "File not found");
			return;
		}

		shortCut += L" -f \"" + fileStr.toStdWString() + L"\"";
		fileFound = true;
	}

	if (!fileFound)
	{
		emit injec_status(false, "No file selected");
		return;
	}

	int delay = ui.txt_delay->text().toInt();
	if (delay > 0)
	{
		shortCut += L" -delay ";
		std::wstringstream stream;
		stream << delay;
		shortCut += stream.str();
	}

	int timeout = ui.txt_timeout->text().toInt();
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
		default: break;
	}

	switch (ui.cmb_create->currentIndex())
	{
		case 1:		shortCut += L" -s 1"; break;
		case 2:		shortCut += L" -s 2"; break;
		case 3:		shortCut += L" -s 3"; break;
		default: break;
	}

	if (ui.cmb_peh->currentIndex() == 1)	shortCut += L" -peh 1";
	if (ui.cmb_peh->currentIndex() == 2)	shortCut += L" -peh 2";
	if (ui.cb_unlink->isChecked())			shortCut += L" -unlink";
	if (ui.cb_clock->isChecked())			shortCut += L" -cloak";
	if (ui.cb_random->isChecked())			shortCut += L" -random";
	if (ui.cb_copy->isChecked())			shortCut += L" -copy";
	if (ui.cb_hijack->isChecked())			shortCut += L" -hijack";

	DWORD Flags = 0;
	if (ui.cmb_load->currentIndex() == 3)
	{
		if (ui.cb_clean->isChecked())		Flags |= INJ_MM_CLEAN_DATA_DIR;
		if (ui.cb_cookie->isChecked())		Flags |= INJ_MM_INIT_SECURITY_COOKIE;
		if (ui.cb_imports->isChecked())		Flags |= INJ_MM_RESOLVE_IMPORTS;
		if (ui.cb_delay->isChecked())		Flags |= INJ_MM_RESOLVE_DELAY_IMPORTS;
		if (ui.cb_tls->isChecked())			Flags |= INJ_MM_EXECUTE_TLS;
		if (ui.cb_seh->isChecked())			Flags |= INJ_MM_ENABLE_EXCEPTIONS;
		if (ui.cb_protection->isChecked())	Flags |= INJ_MM_SET_PAGE_PROTECTIONS;
		if (ui.cb_main->isChecked())		Flags |= INJ_MM_RUN_DLL_MAIN;

		std::wstringstream stream;
		stream << std::hex << Flags;
		shortCut += L" -mmflags " + stream.str();
	}

	shortCut += L" -wait ";

	fileName.replace(".", "_");
	fileName.replace(" ", "_");

	printf("Link = \n%ls\n", shortCut.c_str());

	bool bLink = CreateLinkWrapper(fileName, QString::fromStdWString(shortCut));
	if (bLink)
	{
		QString msg = fileName + " \n" + QString::fromStdWString(shortCut);
		QMessageBox messageBox;
		messageBox.information(0, "Success", msg);
		messageBox.setFixedSize(500, 200);
	}
	else
	{
		emit injec_status(false, "Shortcut generation failed");
	}
}

void GuiMain::open_log()
{
	if (FileExistsW(GH_INJECTOR_LOGW))
	{
		ShellExecuteW(NULL, L"edit", GH_INJECTOR_LOGW, nullptr, nullptr, SW_SHOW);
	}
}

void GuiMain::update_init()
{
	std::string online_version = get_newest_version();
	std::string current_version = GH_INJ_VERSIONA;

	if (online_version.compare(current_version) > 0 || true)
	{
		std::string update_msg = "This version of the GH Injector is outdated.\nThe newest version is V";
		update_msg += online_version;
		update_msg += ".\n";
		update_msg += "Do you want to update?";

		QMessageBox box;
		box.setWindowTitle("New version available");
		box.setText(update_msg.c_str());
		box.addButton(QMessageBox::Yes);
		box.addButton(QMessageBox::No);
		box.addButton(QMessageBox::Ignore);
		box.setDefaultButton(QMessageBox::Yes);
		box.setIcon(QMessageBox::Icon::Information);

		auto res = box.exec();

		if (res == QMessageBox::Yes)
		{
			ignoreUpdate = false;

			update_injector(online_version);

			return;
		}

		if (res == QMessageBox::Ignore)
		{
			ignoreUpdate = true;
		}
	}

	return;
}