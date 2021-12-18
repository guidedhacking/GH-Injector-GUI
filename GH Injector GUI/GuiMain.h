#pragma once

#include "pch.h"

#include "ui_GuiMain.h"

#include "CmdArg.h"
#include "DebugConsole.h"
#include "DragDropWindow.h"
#include "framelesswindow/framelesswindow.h"
#include "GuiProcess.h"
#include "GuiScanHook.h"
#include "Injection.h"
#include "InjectionLib.h"
#include "PDB Download.h"
#include "Process.h"
#include "ShortCut.h"
#include "StatusBox.h"
#include "Update.h"

class GuiMain : public QMainWindow
{
	Q_OBJECT

public:
	GuiMain(QWidget * parent = Q_NULLPTR);
	~GuiMain();

	// Static

	static int const EXIT_CODE_CLOSE;
	static int const EXIT_CODE_REBOOT;
	static int const EXIT_CODE_UPDATE;
	static int const EXIT_CODE_START_NATIVE;

	static int const Height_small;
	static int const Height_medium_s;
	static int const Height_medium_b;
	static int const Height_big;
	static int const Height_change_delay;

	static QPixmap GetIconFromFileW(const wchar_t * szPath, UINT size, int index = 0);
	void show();

private:
	Ui::GuiMainClass ui;

	FramelessWindow * framelessParent;

	FramelessWindow framelessPicker;
	FramelessWindow framelessScanner;
	FramelessWindow framelessUpdate;

	GuiProcess	* gui_Picker;
	GuiScanHook * gui_Scanner;

	DragDropWindow * drag_drop;

	QFileSystemModel model;

	// Settings
	Process_State_Struct	* pss;
	Process_Struct			* ps_picker;

	bool ignoreUpdate;
	bool onReset;
	bool onMove;
	bool native;
	bool consoleOpen;
	bool consoleFirst;
	bool tooltipsEnabled;
	bool setupDone;
	bool updateCheck;
	bool hijackWarning;

	int dockIndex;

	QPoint mouse_pos;

	std::wstring newest_version;
	std::wstring current_version;

	int current_dpi;
	int dragdrop_size;
	int dragdrop_offset;

	DWORD old_raw_pid;
	DWORD old_byname_pid;
	DWORD old_bypid_pid;

	QTimer * t_Auto_Inj;
	QTimer * t_Delay_Inj;
	QTimer * t_Update_Proc;
	QTimer * t_OnUserInput;
	QTimer * t_Update_DragDrop;
	QTimer * t_SetUp;
	QTimer * t_Update_Files;

	QPixmap pxm_banner;
	QPixmap pxm_lul;
	QPixmap pxm_error;
	QPixmap pxm_generic;

	QString lastPathStr;

	InjectionLib InjLib;

	QRegularExpressionValidator * rev_NumbersOnly;

	// General
	bool platformCheck();
	void reboot();
	void add_file_to_list(QString str, bool active);

	// Update GUI
	void toggleSelected();
	void rb_unset_all();
	void txt_pid_change();
	void cmb_proc_name_change();
	void btn_change();
	void update_proc_icon();
	void update_height();

	// Settings
	void save_settings();
	void load_settings();
	void default_settings();

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

public:
	void initSetup();

public slots:
	void get_from_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void get_from_scan_hook();

signals:
	void send_to_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void send_to_scan_hook(int pid);

private slots:
	// Titlebar
	void closeEvent(QCloseEvent * event) override;
	void close_clicked();
	void minimize_clicked();

	// Settings
	void rb_process_set();
	void rb_pid_set();
	void btn_pick_process_click();
	void update_process();

	// Auto, Reset
	void auto_inject();
	void auto_loop_inject();
	void reset_settings();

	// Method, Cloaking, Advanced
	void load_change(int index);
	void create_change(int index);
	void peh_change(int index);
	void cb_main_clicked();
	void cb_page_protection_clicked();
	void cb_cloak_clicked();
	void cb_hijack_clicked();

	// Files
	void add_file_dialog();
	void remove_file();
	void select_file();
	void update_file_list();

	// Inject
	void delay_inject();
	void inject_file();

	// Hook
	void btn_hook_scan_click();

	// Info
	void tooltip_change();
	void open_help();
	void generate_shortcut();
	void open_console();
	void open_log();

	// PDB
	void setup();

	// Update
	void update();
	void update_clicked();
	void update_after_height_change();
};