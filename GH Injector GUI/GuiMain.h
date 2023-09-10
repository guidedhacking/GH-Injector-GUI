/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#pragma once

#include "pch.h"

#include "ui_GuiMain.h"

#include "CmdArg.h"
#include "DebugConsole.h"
#include "DotNetOptions.h"
#include "DragDropWindow.h"
#include "framelesswindow/framelesswindow.h"
#include "GuiProcess.h"
#include "GuiScanHook.h"
#include "Injection.h"
#include "InjectionLib.h"
#include "PDB_Download.h"
#include "Process.h"
#include "ShortCut.h"
#include "StatusBox.h"
#include "Update.h"

#define FILE_LIST_IDX_CHECKBOX			0
#define FILE_LIST_IDX_NAME				1
#define FILE_LIST_IDX_PATH				2
#define FILE_LIST_IDX_PLATFORM			3
#define FILE_LIST_IDX_BUTTON_OPTIONS	4
#define FILE_LIST_IDX_FILEHANDLE		5
#define FILE_LIST_IDX_FLAG				6
#define FILE_LIST_IDX_DOTNET_OPTIONS	7
#define FILE_LIST_IDX_DOTNET_ARGUMENT	8
#define FILE_LIST_IDX_DOTNET_PARSER		9

#define FILE_LIST_FLAG_NATIVE			0
#define FILE_LIST_FLAG_DOTNET			1
#define FILE_LIST_FLAG_DOTNET_NATIVE	2

#define FILE_SETTINGS_IDX_PATH				0
#define FILE_SETTINGS_IDX_CHECKED			1
#define FILE_SETTINGS_IDX_FLAG				2
#define FILE_SETTINGS_IDX_DOTNET_OPTIONS	3
#define FILE_SETTINGS_IDX_DOTNET_ARGUMENT	4

class GuiMain : public QMainWindow
{
	Q_OBJECT

public:
	GuiMain(QWidget * parent = Q_NULLPTR);
	~GuiMain();

	// Static
	static const int EXIT_CODE_CLOSE;
	static const int EXIT_CODE_REBOOT;
	static const int EXIT_CODE_UPDATE;
	static const int EXIT_CODE_START_NATIVE;

	static const int Height_small;
	static const int Height_medium_s;
	static const int Height_medium_b;
	static const int Height_big;
	static const int Height_change_delay;
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

	ProcessState	proc_state;
	ProcessData		proc_data_by_picker;
	ProcessData		proc_data_by_name;
	ProcessData		proc_data_by_pid;

	DWORD			CurrentPID;
	ARCHITECTURE	CurrentArchitecture;
	std::wstring	CurrentName;
	std::wstring	CurrentPath;

	bool ignoreUpdate;
	bool onReset;
	bool onMove;
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

	QTimer t_Auto_Inj;
	QTimer t_Delay_Inj;
	QTimer t_Update_Proc;
	QTimer t_OnUserInput;
	QTimer t_Update_DragDrop;
	QTimer t_SetUp;
	QTimer t_Update_Files;

	QPixmap pxm_banner;
	QPixmap pxm_lul;
	QPixmap pxm_error;
	QPixmap pxm_generic;

	QString lastPathStr;

	InjectionLib InjLib;

	QRegularExpressionValidator rev_NumbersOnly;
	QStringListModel			mod_CmbProcNameModel;

	// General
	bool platformCheck();
	void reboot();

	QTreeWidgetItem * add_file_to_list(QString path, bool active, int flag = FILE_LIST_FLAG_NATIVE);
	bool get_icon_from_file(const std::wstring & path, UINT size, int index, QPixmap & pixmap);

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
	void get_from_picker();
	void get_from_scan_hook();

signals:
	void send_to_picker(ProcessState * state, ProcessData * data);
	void send_to_scan_hook(int pid);

private slots:
	// Titlebar
	void closeEvent(QCloseEvent * event) override;
	void btn_close_clicked();
	void btn_minimize_clicked();

	// Settings
	void rb_process_set();
	void rb_pid_set();
	void btn_pick_process_click();
	void update_process();

	// Auto, Reset
	void cb_auto_inject();
	void auto_loop_inject();
	void btn_reset_settings();

	// Method, Cloaking, Advanced
	void cmb_load_change(int index);
	void cmb_create_change(int index);
	void cmb_peh_change(int index);
	void cb_main_clicked();
	void cb_page_protection_clicked();
	void cb_cloak_clicked();
	void cb_hijack_clicked();

	// Files
	void btn_add_file_dialog();
	void btn_remove_file();
	void tree_select_file();
	void update_file_list();

	// Inject
	void btn_delay_inject();
	void inject_file();

	// Hook
	void btn_hook_scan_click();

	// Info
	void btn_tooltip_change();
	void btn_open_help();
	void btn_generate_shortcut();
	void btn_open_console();
	void btn_open_log();

	// PDB
	void setup();

	// Update
	void update();
	void btn_update_clicked();
	void update_after_height_change();

	//.NET options
	void dot_net_options();
	bool parse_dot_net_data(QTreeWidgetItem * item, DotNetOptionsTree *& out);
};