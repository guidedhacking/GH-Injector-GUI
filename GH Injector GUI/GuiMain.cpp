#include "GuiMain.h"

#include <qdesktopservices.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qstylefactory.h>
#include <qsettings.h>
#include <qdir.h>
#include <qmimedata.h>
#include <QtGui>

#include <urlmon.h>
#include <fstream>
#include <string>
#include <sstream>

#include <string>
#include "Banner.h"
#include "Process.h"
#include "Injection.h"
#include "Compress.h"
#include "Zip.h"
#include "ShortCut.h"
#include "DownloadProgress.h"

int const GuiMain::EXIT_CODE_REBOOT = -123456789;

GuiMain::GuiMain(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	parent->layout()->setSizeConstraint(QLayout::SetFixedSize);
	ui.grp_settings->layout()->setSizeConstraint(QLayout::SetFixedSize);
	ui.tree_files->setFixedWidth(800);

	// Settings
	connect(ui.rb_proc,  SIGNAL(clicked()), this, SLOT(rb_process_set()));
	connect(ui.rb_pid,   SIGNAL(clicked()), this, SLOT(rb_pid_set()));
	connect(ui.btn_proc, SIGNAL(clicked()), this, SLOT(btn_pick_process_click()));

	// Auto, Reset, Color
	connect(ui.cb_auto,   SIGNAL(clicked()), this, SLOT(auto_inject()));
	connect(ui.btn_reset, SIGNAL(clicked()), this, SLOT(reset_settings()));
	connect(ui.btn_hooks, SIGNAL(clicked()), this, SLOT(hook_Scan()));

	// Method, Cloaking, Advanced
	connect(ui.cmb_load,	SIGNAL(currentIndexChanged(int)),	this, SLOT(load_change(int)));
	connect(ui.cmb_create,	SIGNAL(currentIndexChanged(int)),	this, SLOT(create_change(int)));
	connect(ui.cb_main,		SIGNAL(clicked()),					this, SLOT(cb_main_clicked()));

	// Files
	connect(ui.btn_add,    SIGNAL(clicked()), this, SLOT(add_file_dialog()));
	connect(ui.btn_inject, SIGNAL(clicked()), this, SLOT(delay_inject()));
	connect(ui.btn_remove, SIGNAL(clicked()), this, SLOT(remove_file()));
	connect(ui.tree_files, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(select_file()));

	// Info
	connect(ui.btn_tooltip, SIGNAL(clicked()), this, SLOT(tooltip_change()));
	connect(ui.btn_help,    SIGNAL(clicked()), this, SLOT(open_help()));
	connect(ui.btn_log,     SIGNAL(clicked()), this, SLOT(open_log()));
	connect(ui.btn_version, SIGNAL(clicked()), this, SLOT(check_online_version()));

	ui.btn_version->setText("V" GH_INJ_VERSIONA);

	gui_Picker  = new GuiProcess();
	gui_Scanner = new GuiScanHook();
	t_Auto_Inj  = new QTimer(this);
	t_Delay_Inj = new QTimer(this);
	pss         = new Process_State_Struct;
	ps_picker   = new Process_Struct;

	if (this->parentWidget())
	{
		framelessPicker.setWindowTitle("Select a process");
		framelessPicker.setContent(gui_Picker);
		framelessPicker.resize(QSize(460, 500));
		framelessPicker.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	}

	if (this->parentWidget())
	{
		framelessScanner.setWindowTitle("Scan for hooks");
		framelessScanner.setContent(gui_Scanner);
		framelessScanner.resize(QSize(320, 230));
		framelessScanner.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	}
	
	onReset = false;

	t_Delay_Inj->setSingleShot(true);
	pss->cbSession = true;
	pss->cmbArch = 0;
	pss->txtFilter = "";
	memset(ps_picker, 0, sizeof(Process_Struct));
	lightMode = false;
	lbl_hide_banner = false;

	// Process Picker
	connect(this,	SIGNAL(send_to_picker(Process_State_Struct*, Process_Struct*)),
		gui_Picker, SLOT(get_from_inj(Process_State_Struct*, Process_Struct*)));
	connect(gui_Picker, SIGNAL(send_to_inj(Process_State_Struct*, Process_Struct*)),
		this, SLOT(get_from_picker(Process_State_Struct*, Process_Struct*)));

	// Scan Hook
	connect(this,		SIGNAL(send_to_scan_hook(int, int)),
		gui_Scanner,	SLOT(get_from_inj_to_sh(int, int)));
	connect(gui_Scanner, SIGNAL(send_to_inj_sh(int, int)),
		this,			SLOT(get_from_scan_hook(int, int)));

	
	connect(t_Auto_Inj, SIGNAL(timeout()), this, SLOT(auto_loop_inject()));
	connect(t_Delay_Inj,SIGNAL(timeout()), this, SLOT(inject_file()));


	ui.tree_files->setColumnWidth(0, 50);
	ui.tree_files->setColumnWidth(1, 178);
	ui.tree_files->setColumnWidth(2, 470);
	ui.tree_files->setColumnWidth(3, 100);
	ui.tree_files->setColumnWidth(4, 0);

	ui.tree_files->clear();
			
	load_settings();
	color_setup();
	color_change();
	load_banner();
	load_change(42);
	create_change(42);
#ifdef _DEBUG
	lbl_hide_banner = 1;
#endif _DEBUG
	hide_banner();

	if (!ignoreUpdate)
	{
		check_online_version();
	}

	if (!InjLib.Init())
	{
		QString failMsg = GH_INJ_MOD_NAMEA + QString(" not found");
		emit injec_status(false, failMsg);
	}

	// Reduce Height
	if(this->parentWidget())
	{
		QSize winSize = this->parentWidget()->size();
		winSize.setHeight(400);
		winSize.setWidth(1000);
		parentWidget()->resize(winSize);
	}
	else
	{
		QSize winSize = this->size();
		winSize.setHeight(400);
		winSize.setWidth(1000);
		this->resize(winSize);
	}

	this->installEventFilter(this);
	ui.tree_files->installEventFilter(this);
	ui.txt_pid->setValidator(new QRegExpValidator(QRegExp("[0-9]+")));
	ui.txt_pid->installEventFilter(this);

	ui.txt_delay->setValidator(new QRegExpValidator(QRegExp("[0-9]+")));

	platformCheck();

	bool status = SetDebugPrivilege(true);

	OnExit = false;
	process_update_thread = std::thread(&GuiMain::UpdateProcess, this, 100);

	int i = 42;
}

GuiMain::~GuiMain()
{
	OnExit = true;

	process_update_thread.join();

	if (this->parentWidget())
		save_settings();

	delete gui_Picker;
	delete t_Auto_Inj;
	delete t_Delay_Inj;
	delete pss;
	delete ps_picker;
}

void GuiMain::UpdateProcess(int Interval)
{
	QTimer t;

	while (!OnExit)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(Interval));

		if (onUserInput)
		{
			t.setInterval(std::chrono::milliseconds(5000));
			t.start();

			onUserInput = false;
		}

		int raw = ui.txt_pid->text().toInt();
		Process_Struct byName	= getProcessByName(ui.cmb_proc->currentText().toStdString().c_str());
		Process_Struct byPID	= getProcessByPID(ui.txt_pid->text().toInt());

		if (ui.rb_pid->isChecked())
		{
			if (byPID.pid)
			{
				if (byPID.pid != byName.pid && stricmp(byPID.fullName, byName.fullName))
				{
					txt_pid_change();
				}

				if (t.isActive())
				{
					t.stop();
				}
			}
			else if (t.isActive())
			{
			}
			else
			{
				if (byName.pid)
				{
					cmb_proc_name_change();
				}
				else if (raw)
				{
					ui.txt_pid->setText("0");
					ui.txt_arch->setText("---");
					ui.txt_pid->setToolTip("");
					ui.cmb_proc->setToolTip("");
				}
			}
		}
		else if (ui.rb_proc->isChecked())
		{
			if (byName.pid)
			{
				if (byName.pid != byPID.pid && strcmp(byName.name, byPID.name))
				{
					cmb_proc_name_change();
				}				
			}
			else if (ui.txt_pid->text().toInt() != 0)
			{
				ui.txt_pid->setText("0");
				ui.txt_arch->setText("---");
				ui.txt_pid->setToolTip("");
				ui.cmb_proc->setToolTip("");
			}
		}
	}
}

int GuiMain::str_to_arch(const QString str)
{
	if (str == "x64") return X64;
	else if (str == "x86") return X86;
	else return NONE;
}

QString GuiMain::arch_to_str(const int arch)
{
	if (arch == 1) return QString("x64");
	else if (arch == 2) return QString("x86");
	else return QString("---");
}

void GuiMain::closeEvent(QCloseEvent* event)
{
	if (!this->parentWidget())
		save_settings();
}

std::string GuiMain::getVersionFromIE()
{
	char cacheFile[MAX_PATH] = { 0 };
	HRESULT hRes = URLDownloadToCacheFileA(nullptr, GH_VERSION_URL, cacheFile, sizeof(cacheFile), 0, nullptr);

	if (hRes != S_OK)
		return "";

	// Read file 
	std::ifstream infile(cacheFile, std::ifstream::in);

	if (!infile.good())
		return "0.0";

	// ???????????????
	// Why is this different on local debug version
	std::string strVer;
	infile >> strVer;

	infile.close();

	return strVer;
}

void GuiMain::keyPressEvent(QKeyEvent * k)
{
}

void GuiMain::dragEnterEvent(QDragEnterEvent* e)
{
	
	("dragEnterEvent\n");
	if (e->mimeData()->hasUrls()) {
		e->acceptProposedAction();
	}
}

void GuiMain::dragMoveEvent(QDragMoveEvent* e)
{
	printf("dragMoveEvent\n");
	int i = 42;
}

void GuiMain::dragLeaveEvent(QDragLeaveEvent* e)
{
	printf("dragLeaveEvent\n");
	int i = 42;
}

bool GuiMain::eventFilter(QObject * obj, QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		if (obj == ui.tree_files)
		{
			QKeyEvent * keyEvent = static_cast<QKeyEvent*>(event);
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
			QKeyEvent * keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_9 || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Space)
			{
				printf("triggered event handler\n");
				onUserInput = true;
			}
		}
	}

	return QObject::eventFilter(obj, event);
}

void GuiMain::toggleSelected()
{
	QList<QTreeWidgetItem*> sel = ui.tree_files->selectedItems();
	
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

bool GuiMain::update_injector(std::string version)
{
	AllocConsole();

	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);

	printf("Downloading new version...\n");

	std::string download_url = GH_DOWNLOAD_PREFIX;
	download_url += version;
	download_url += "/GH Injector.zip";
	printf("%s\n", download_url.c_str());

	DownloadProgress progress;
	if (FAILED(URLDownloadToFileA(nullptr, download_url.c_str(), GH_INJECTOR_ZIP, 0, &progress)))
	{
		FreeConsole();

		return false;
	}

	save_settings();

	gui_Scanner->InjLib.Unload();
	InjLib.Unload();

	auto mod = GetModuleHandle(GH_INJ_MOD_NAME);
	while (mod)
	{
		FreeLibrary(mod);
		mod = GetModuleHandleA("GH Injector - x64.dll");
	}

	printf("Removing old files...\n");

	DeleteFileW(GH_INJ_MOD_NAME86W);
	DeleteFileW(GH_INJ_MOD_NAME64W);
	DeleteFileA(GH_INJECTOR_SM_X86);
	DeleteFileA(GH_INJECTOR_SM_X64);

	auto path = QCoreApplication::applicationDirPath().toStdString();
	path += "/";

	auto old_path = path + "OLD.exe";
	MoveFileA(QCoreApplication::applicationFilePath().toStdString().c_str(), old_path.c_str());

	auto zip_path = path + GH_INJECTOR_ZIP;

	printf("Extracting new files...\n");

	Unzip(zip_path.c_str(), path.c_str());

	DeleteFileA(zip_path.c_str());
	
	auto new_path = path + "GH Injector.exe ";

	STARTUPINFOA si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	char szPID[12]{ 0 };
	_itoa_s(GetCurrentProcessId(), szPID, 10);
	printf("%s\n", new_path.c_str());
	new_path += szPID;

	printf("Launching updated version...\n");
	CreateProcessA(nullptr, (char*)new_path.c_str(), nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	ExitProcess(0);
}

void GuiMain::platformCheck()
{
#ifndef _DEBUG
#ifndef _WIN64

	// windows 64-bit == gh64.exe
	bool bPlatform = isCorrectPlatform();
	if (bPlatform == true)
		return;

	// Won't work
	ui.cb_hijack->setChecked(false);
	ui.cb_hijack->setDisabled(true);


	QMessageBox::StandardButton reply;
	reply = QMessageBox::warning(nullptr, "Warning architecture conflict", "Since you're using a \
64-bit version of Windows it's recommended to use the 64-bit version of the injector. \
Do you want to switch to the 64-bit version?", QMessageBox::Yes | QMessageBox::No);

	if (reply == QMessageBox::No)
		return;
		
	bool bStart = StartProcess(GH_INJ_EXE_NAME64A);
	if (bStart == false)
		return; // can't start, maybe msgbox


	//qApp->quit();
	// I am qt and do not want to close
	QTimer::singleShot(250, qApp, SLOT(quit()));

#endif // _WIN64
#endif _DEBUG
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
	if(gui_Picker->parentWidget())
		framelessPicker.show();
	else
		gui_Picker->show();
	emit send_to_picker(pss, ps_picker);
}

void GuiMain::cmb_proc_name_change()
{
	QString proc = ui.cmb_proc->currentText();
	Process_Struct pl = getProcessByName(proc.toStdString().c_str());

	if (!pl.pid)
	{
		return;
	}

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	ui.txt_pid->setText(QString::number(pl.pid));
	QString new_pid = QString::asprintf("0x%08X", pl.pid);
	ui.txt_pid->setToolTip(new_pid);

	ui.cmb_proc->setToolTip(pl.fullName);

	ui.txt_arch->setText(GuiMain::arch_to_str(pl.arch));

	if (ui.cmb_proc->findText(pl.name) == -1)
	{
		ui.cmb_proc->addItem(pl.name);
	}
}

void GuiMain::txt_pid_change()
{
	QString s_PID = ui.txt_pid->text();
	Process_Struct pl = getProcessByPID(s_PID.toInt());

	if (!pl.pid)
	{
		return;
	}

	memcpy(ps_picker, &pl, sizeof(Process_Struct));

	QString new_pid = QString::asprintf("0x%08X", pl.pid);
	ui.txt_pid->setToolTip(new_pid);

	ui.cmb_proc->setCurrentText(pl.name);
	ui.cmb_proc->setToolTip(pl.fullName);

	ui.txt_arch->setText(GuiMain::arch_to_str(pl.arch));

	if (ui.cmb_proc->findText(pl.name) == -1)
	{
		ui.cmb_proc->addItem(pl.name);
	}
}

void GuiMain::btn_hook_scan_change()
{
	if (ui.rb_proc->isChecked())
	{
		if(ps_picker->arch)
			ui.btn_hooks->setEnabled(true);

		else
			ui.btn_hooks->setEnabled(false);
	}
}

void GuiMain::get_from_picker(Process_State_Struct* procStateStruct, Process_Struct* procStruct)
{
	pss = procStateStruct;
	ps_picker = procStruct;
	if (gui_Picker->parentWidget())
		framelessPicker.hide();
	else
		gui_Picker->hide();

	if (ps_picker->pid)
	{
		rb_unset_all();
		int index = ui.cmb_proc->findText(ps_picker->name);
		if(index == -1) // check exists
			ui.cmb_proc->addItem(ps_picker->name);
		ui.cmb_proc->setCurrentIndex(ui.cmb_proc->findText(ps_picker->name));
		ui.txt_pid->setText(QString::number(ps_picker->pid));
		ui.txt_arch->setText(GuiMain::arch_to_str(ps_picker->arch));
		rb_pid_set();
	}
}

void GuiMain::get_from_scan_hook(int pid, int error)
{
	if (gui_Picker->parentWidget())
		framelessPicker.hide();
	else
		gui_Picker->hide();
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
		if (ui.rb_proc->isChecked())
		{
			QString proc = ui.cmb_proc->currentText();
			Process_Struct pl = getProcessByName(proc.toStdString().c_str());

			if (pl.pid)
			{
				ui.cb_auto->setChecked(false);
				t_Auto_Inj->stop();
				emit delay_inject();
				//injec_status(true, "Test Message");
			}
		}
	}
	else
	{
		t_Auto_Inj->stop();
	}
}

void GuiMain::color_setup()
{
	if (!this->parentWidget())
	{
		// https://gist.github.com/QuantumCD/6245215
		// https://github.com/Jorgen-VikingGod/Qt-Frameless-Window-DarkStyle/blob/master/DarkStyle.cpp
		// Style bullshit
		qApp->setStyle(QStyleFactory::create("Fusion"));
		normalPalette = qApp->palette();
		normalSheet = qApp->styleSheet();

		darkPalette.setColor(QPalette::Window, QColor(0x2D, 0x2D, 0x2D));
		darkPalette.setColor(QPalette::WindowText, Qt::white);
		darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
		darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
		darkPalette.setColor(QPalette::ToolTipText, Qt::white);
		darkPalette.setColor(QPalette::Text, Qt::white);
		darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
		darkPalette.setColor(QPalette::ButtonText, Qt::white);
		darkPalette.setColor(QPalette::BrightText, Qt::red);
		darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
		darkPalette.setColor(QPalette::HighlightedText, Qt::black);

		darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(50, 50, 50));
		darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0, 0, 0));

		darkSheet = ("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
		/*ui.cb_auto->setStyleSheet()*/

	}
}
void GuiMain::color_change()
{
	//idk this check is weird
	if (!this->parentWidget())
	{
		if (lightMode)
		{
			qApp->setPalette(normalPalette);
			qApp->setStyleSheet(normalSheet);

			QPixmap pix_banner;
			pix_banner.loadFromData(getBannerWhite(), getBannerWhiteLen(), "JPG");
			ui.lbl_img->setPixmap(pix_banner);
		}
		else
		{
			qApp->setPalette(darkPalette);
			qApp->setStyleSheet(darkSheet);

			QPixmap pix_banner;
			pix_banner.loadFromData(getBanner(), getBannerLen(), "JPG");
			ui.lbl_img->setPixmap(pix_banner);
		}
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
	if(lbl_hide_banner == true)
		ui.lbl_img->setVisible(false);
}

void GuiMain::reset_settings()
{
	onReset = true;

	QFileDialog fDialog(this, "Select dll files", QApplication::applicationDirPath(), "Dynamic Link Libraries (*.dll)");
	
	// delete file
	QString iniName = QCoreApplication::applicationName() + ".ini";
	QFile iniFile(iniName);
	if (iniFile.exists())
	{
		iniFile.remove();
	}

	emit slotReboot();
}

void GuiMain::slotReboot()
{
	//qDebug() << "Performing application reboot...";
	qApp->exit(GuiMain::EXIT_CODE_REBOOT);
}

void GuiMain::hook_Scan()
{
	if (gui_Scanner->parentWidget())
		framelessScanner.show();
	else
		gui_Scanner->show();
	
	emit send_to_scan_hook(ps_picker->pid, 0);
}

void GuiMain::save_settings()
{
	if (onReset)
	{
		onReset = false;

		return;
	}

	QSettings settings((QCoreApplication::applicationName() + ".ini"), QSettings::IniFormat);

	settings.beginWriteArray("FILES");
	int i = 0;
	QTreeWidgetItemIterator it(ui.tree_files);
	for (; *it; ++it, ++i)
	{
		if (!FileExistsW((*it)->text(2).toStdWString().c_str()))
			continue;

		settings.setArrayIndex(i);
		settings.setValue(QString::number(0), (*it)->text(2));
		settings.setValue(QString::number(1), (*it)->checkState(0) == Qt::CheckState::Checked);
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
	settings.setValue("PROCESS",		ui.cmb_proc->currentIndex());
	settings.setValue("PID",			ui.txt_pid->text());
	settings.setValue("PROCESSBYNAME",	ui.rb_proc->isChecked());
	settings.setValue("ARCH",			ui.txt_arch->text());	
	settings.setValue("DELAY",			ui.txt_delay->text());	
	settings.setValue("AUTOINJ",		ui.cb_auto->isChecked());
	settings.setValue("CLOSEONINJ",		ui.cb_close->isChecked());

	// Method
	settings.setValue("MODE",			ui.cmb_load->currentIndex());
	settings.setValue("LAUNCHMETHOD",	ui.cmb_create->currentIndex());
	settings.setValue("HIJACK",			ui.cb_hijack->isChecked());
	settings.setValue("CLOAK",			ui.cb_clock->isChecked());

	// Cloaking
	settings.setValue("PEH",			ui.cmb_peh->currentIndex());
	settings.setValue("UNLINKPEB",		ui.cb_unlink->isChecked());
	settings.setValue("RANDOMIZE",		ui.cb_random->isChecked());
	settings.setValue("DLLCOPY",		ui.cb_copy->isChecked());

	// manual mapping
	settings.setValue("CLEANDIR",		ui.cb_clean->isChecked());
	settings.setValue("INITCOOKIE",		ui.cb_cookie->isChecked());
	settings.setValue("IMPORTS",		ui.cb_imports->isChecked());
	settings.setValue("DELAYIMPORTS",	ui.cb_delay->isChecked());
	settings.setValue("TLS",			ui.cb_tls->isChecked());
	settings.setValue("SEH",			ui.cb_seh->isChecked());
	settings.setValue("PROTECTION",		ui.cb_protection->isChecked());
	settings.setValue("DLLMAIN",		ui.cb_main->isChecked());

	// Process picker
	settings.setValue("PROCNAMEFILTER", pss->txtFilter);
	settings.setValue("PROCESSTYPE",	pss->cmbArch);
	settings.setValue("CURRENTSESSION", pss->cbSession);

	// Info
	settings.setValue("TOOLTIPSON",		ui.btn_tooltip->isChecked());

	// Not visible
	settings.setValue("LASTDIR",		lastPathStr);
	settings.setValue("IGNOREUPDATES",	ignoreUpdate);
	settings.setValue("LIGHTMODE",		lightMode);
	settings.setValue("HIDEBANNER",		lbl_hide_banner);
	settings.setValue("STATE",		saveState());
	settings.setValue("GEOMETRY", saveGeometry());	
	// Broken on frameless window

	settings.endGroup();
}

void GuiMain::load_settings()
{
	QFile iniFile((QCoreApplication::applicationName() + ".ini"));
	if (!iniFile.exists())
		return;

	QSettings settings((QCoreApplication::applicationName() + ".ini"), QSettings::IniFormat);

	int fileSize = settings.beginReadArray("FILES");
	for (int i = 0; i < fileSize; ++i)
	{
		settings.setArrayIndex(i);
		
		auto path = settings.value(QString::number(0)).toString();

		if (!FileExistsW(path.toStdWString().c_str()))
			continue;

		add_file_to_list(
			path,
			settings.value(QString::number(1)).toBool()
		);
	}
	settings.endArray();

	int procSize = settings.beginReadArray("PROCESS");
	for (int i = 0; i < procSize; ++i) {
		settings.setArrayIndex(i);
		ui.cmb_proc->addItem(settings.value(QString::number(0)).toString());
	}
	settings.endArray();


	settings.beginGroup("CONFIG");

	// Settings
	ui.cmb_proc		->setCurrentIndex(settings.value("PROCESS").toInt());
	ui.txt_pid		->setText(settings.value("PID").toString());
	ui.rb_proc		->setChecked(settings.value("PROCESSBYNAME").toBool());
	ui.txt_arch		->setText(settings.value("ARCH").toString());
	ui.txt_delay	->setText(settings.value("DELAY").toString());
	ui.cb_auto		->setChecked(settings.value("AUTOINJ").toBool());
	ui.cb_close		->setChecked(settings.value("CLOSEONINJ").toBool());

	// Method
	ui.cmb_load		->setCurrentIndex(settings.value("MODE").toInt());
	ui.cmb_create	->setCurrentIndex(settings.value("LAUNCHMETHOD").toInt());
	ui.cb_hijack	->setChecked(settings.value("HIJACK").toBool());
	ui.cb_clock		->setChecked(settings.value("CLOAK").toBool());

	// Cloaking
	ui.cmb_peh		->setCurrentIndex(settings.value("PEH").toInt());
	ui.cb_unlink	->setChecked(settings.value("UNLINKPEB").toBool());
	ui.cb_random	->setChecked(settings.value("RANDOMIZE").toBool());
	ui.cb_copy		->setChecked(settings.value("DLLCOPY").toBool());

	// manual mapping
	ui.cb_clean		->setChecked(settings.value("CLEANDIR").toBool());
	ui.cb_cookie	->setChecked(settings.value("INITCOOKIE").toBool());
	ui.cb_imports	->setChecked(settings.value("IMPORTS").toBool());
	ui.cb_delay		->setChecked(settings.value("DELAYIMPORTS").toBool());
	ui.cb_tls		->setChecked(settings.value("TLS").toBool());
	ui.cb_seh		->setChecked(settings.value("SEH").toBool());
	ui.cb_protection->setChecked(settings.value("PROTECTION").toBool());
	ui.cb_main		->setChecked(settings.value("DLLMAIN").toBool());

	// Process picker
	pss->txtFilter	= settings.value("PROCNAMEFILTER").toString();
	pss->cmbArch	= settings.value("PROCESSTYPE").toInt();
	pss->cbSession	= settings.value("CURRENTSESSION").toBool();

	// Info
	ui.btn_tooltip	->setChecked(settings.value("TOOLTIPSON").toBool());

	// Not visible
	lastPathStr		= settings.value("LASTDIR").toString();
	ignoreUpdate	= settings.value("IGNOREUPDATES").toBool();
	lightMode		= settings.value("LIGHTMODE", false).toBool();
	lbl_hide_banner = settings.value("HIDEBANNER", false).toBool();
	restoreState	(settings.value("STATE").toByteArray());
	restoreGeometry	(settings.value("GEOMETRY").toByteArray());

	settings.endGroup();
}

void GuiMain::load_change(int i)
{
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
			break;
	}
}

void GuiMain::create_change(int i)
{
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

void GuiMain::add_file_dialog()
{
	QFileDialog fDialog(this, "Select dll files", lastPathStr, "Dynamic Link Libraries (*.dll)");
	fDialog.setFileMode(QFileDialog::ExistingFiles);
	fDialog.exec();

	if (fDialog.selectedFiles().empty())
		return;

	for (auto l : fDialog.selectedFiles())
			GuiMain::add_file_to_list(l, "");

	lastPathStr = QFileInfo(fDialog.selectedFiles().first()).path();
}

void GuiMain::add_file_to_list(QString str, bool active)
{
	// nop, not the same files
	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
		if ((*it)->text(2) == str)
			return;

	QFileInfo fi(str);
	int arch = (int)getFileArch(fi.absoluteFilePath().toStdWString().c_str());

	if (arch == ARCH::NONE)
		return;

	QTreeWidgetItem* item = new QTreeWidgetItem(ui.tree_files);

	item->setCheckState(0, Qt::CheckState::Unchecked);
	item->setText(1, fi.fileName());
	item->setText(2, fi.absoluteFilePath());
	item->setText(3, arch_to_str(arch));

	if (active)
	{
		item->setCheckState(0, Qt::CheckState::Checked);
	}
}

void GuiMain::remove_file()
{
	QList<QTreeWidgetItem*> item = ui.tree_files->selectedItems();

	for (auto i : item)
	{
		delete i;
	}
}


void GuiMain::select_file()
{
	QList<QTreeWidgetItem*> item = ui.tree_files->selectedItems();

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

	//Process_Struct ps_inject;
	//memset(&ps_inject, 0, sizeof(Process_Struct));

	int fileType	= NONE;
	int processType = NONE;
	int injCounter  = 0;

	// Process ID
	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		if (id)
		{
			data.ProcessID = id;
			processType = getProcArch(id);
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
		Process_Struct p = getProcessByName(ui.cmb_proc->itemText(index).toStdString().c_str());
		if (p.pid)
		{
			data.ProcessID = p.pid;
			processType = p.arch;
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


	data.GenerateErrorLog = true;

	if (!InjLib.LoadingStatus())
	{
		emit injec_status(false, "Library or function not found!");
		return;
	}

	if (!InjLib.SymbolStatus())
	{
		emit injec_status(false, "PDB download not finished!");
		return;
	}
	
	for (QTreeWidgetItemIterator it(ui.tree_files); (*it) != nullptr; ++it)
	{
		// Find Item
		if ((*it)->checkState(0) != Qt::CheckState::Checked)
			continue;

		// Convert String
		QString fileStr	= (*it)->text(2);
		fileStr.replace('/', '\\');

		wcscpy_s(data.szDllPath, fileStr.toStdWString().c_str());
		
		// Check Architecture
		fileType = str_to_arch((*it)->text(3));
		if (fileType == NONE)
		{
			continue;
		}

		if (processType != fileType || processType == NULL || fileType == NULL)
		{
			continue;
		}

		data.Timeout = 2000;
		DWORD res = InjLib.InjectFuncW(&data);
		if (res)
		{
			QString failMsg = "Inject failed with 0x" + QString("%1").arg(res, 8, 16, QLatin1Char('0')).toUpper();
			emit injec_status(false, failMsg);
			continue;
		}

		injCounter++;
	}

	if(injCounter)
	{
		emit injec_status(true, "Success Injection");
	}
	else
	{
		emit injec_status(true, "No file selected");
	}
	
	
	if (ui.cb_close->isChecked())
	{
		qApp->exit(0);
		return;
	}
	
	return;
}

void GuiMain::injec_status(bool ok, const QString msg)
{
	if(ok)
	{
		QMessageBox messageBox;
		messageBox.information(0, "Success", msg);
		messageBox.setFixedSize(500, 200);

		if (ui.cb_close->isChecked())
		{
			this->close();
		}
	}
	else
	{
		QMessageBox messageBox;
		messageBox.critical(0, "Error", msg);
		messageBox.setFixedSize(500, 200);
	}
}

void GuiMain::load_Dll()
{

}


void GuiMain::tooltip_change()
{
	if (ui.btn_tooltip->isChecked())
		ui.btn_tooltip->setText("Disable tooltips");
	else
		ui.btn_tooltip->setText("Enable tooltips");

	int duration = 1;
	if (ui.btn_tooltip->isChecked())
		duration = -1;

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
	ui.btn_log->setToolTipDuration(duration);
	ui.btn_version->setToolTipDuration(duration);
}

void GuiMain::open_help()
{
	bool ok = QDesktopServices::openUrl(QUrl(GH_HELP_URL, QUrl::TolerantMode));
}

void GuiMain::open_log()
{
	//bool ok = QDesktopServices::openUrl(QUrl(GH_LOG_URL, QUrl::TolerantMode));

	std::string shortCut;
	QString fileName = "Injector_";

	int fileType = NONE;
	int processType = NONE;

	// Process ID
	if (ui.rb_pid->isChecked())
	{
		int id = ui.txt_pid->text().toInt();
		if (id)
		{
			Process_Struct ps_local =  getProcessByPID(id);
			shortCut += " -p " + std::string(ps_local.name);
			fileName += ps_local.name;
			processType = ps_local.arch;
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
		Process_Struct p = getProcessByName(ui.cmb_proc->itemText(index).toStdString().c_str());
		if (p.pid)
		{
			shortCut += " -p " + std::string(p.name);
			fileName += p.name;
			processType = p.arch;
		}
		else
		{
			emit injec_status(false, "Invalid Process Name");
			return;
		}
	}

	int fileFound = 0;
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

		// Check Architecture
		fileType = str_to_arch((*it)->text(3));
		if (fileType == NONE)
		{
			emit injec_status(false, "File Architecture invalid");
			return;
		}

		// Check File Selected
		if (processType == NONE)
		{
			emit injec_status(false, "File not selected");
			return;
		}

		if (processType != fileType || processType == NULL || fileType == NULL)
		{
			emit injec_status(false, "File and Process are incompatible");
			return;
		}

		shortCut += " -f " + fileStr.toStdString();
		fileFound++;
	}

	if(fileFound == 0)
	{
		emit injec_status(false, "No file selected");
		return;
	}

	int delay = ui.txt_delay->text().toInt();
	if (delay > 0)
	{
		shortCut += " -delay " + delay;
	}

	// We need a own checkbox for this

	//int wait = ui.cb_auto->isChecked();
	//if (wait)
	//{
	//	shortCut += " -wait";
	//}


	switch (ui.cmb_load->currentIndex())
	{
	case 1:  shortCut += " -load ldr";			break;
	case 2:  shortCut += " -load ldrp";			break;
	case 3:  shortCut += " -load manual";		break;
	default: /*shortCut += " -load loadlib"*/;	break;
	}

	switch (ui.cmb_create->currentIndex())
	{
	case 1:  shortCut += " -start hijack";		break;
	case 2:  shortCut += " -start hook";		break;
	case 3:  shortCut += " -start apc";			break;
	default: /*shortCut += " -start create";*/	break;
	}


	if (ui.cmb_peh->currentIndex() == 1)	shortCut += " -peh erase";
	if (ui.cmb_peh->currentIndex() == 2)	shortCut += " -peh fake";
	if (ui.cb_unlink->isChecked())			shortCut += " -unlink";
	if (ui.cb_clock->isChecked())			shortCut += " -cloak";
	if (ui.cb_random->isChecked())			shortCut += " -randomize";
	if (ui.cb_copy->isChecked())			shortCut += " -copy";
	if (ui.cb_hijack->isChecked())			shortCut += " -hijack";

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

		// ToDo:: Set Security Cookie ???

		std::stringstream stream;
		stream << "0x" << std::hex << Flags;
		shortCut += " -mapping " + stream.str();
	}

	fileName.replace(".", "_");

	bool bLink = CreateLinkWrapper(fileName, QString::fromStdString(shortCut));
	if (bLink)
	{
		QString msg = fileName + " \n" + QString::fromStdString(shortCut);
		QMessageBox messageBox;
		messageBox.information(0, "Success", msg);
		messageBox.setFixedSize(500, 200);

	}
	else
	{
		emit injec_status(false, "Shortcut generation failed");
	}

}

void GuiMain::check_online_version()
{
	std::string online_version = getVersionFromIE();
	std::string current_version = GH_INJ_VERSIONA;

	if (online_version.compare(current_version) > 0)
	{
		std::string update_msg = "This version of the GH Injector is outdated.\nThe newest version is V";
		update_msg += online_version;
		update_msg += ".\n";
		update_msg += "Do you want to update?";

		QMessageBox::StandardButton update_box;
		update_box = QMessageBox::information(nullptr, "New version available", update_msg.c_str(), QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore);

		if (update_box == QMessageBox::No)
			return;

		if (update_box == QMessageBox::Ignore)
		{
			ignoreUpdate = true;
			return;
		}

		ignoreUpdate = false;

		update_injector(online_version);
	}

	return;	
}