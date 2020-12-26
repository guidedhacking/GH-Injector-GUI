#pragma once

#include "framelesswindow/framelesswindow.h"
#include "ui_GuiMain.h"
#include "GuiProcess.h"
#include "GuiScanHook.h"
#include "Process.h"
#include "InjectionLib.h"
#include "Globals.h"
#include "DownloadProgress.h"
#include "DownloadProgressWindow.h"
#include "DragDropWindow.h"

class GuiMain : public QMainWindow
{
	Q_OBJECT

public:
	GuiMain(QWidget * parent = Q_NULLPTR, FramelessWindow * FramelessParent = Q_NULLPTR);
	~GuiMain();

	// Static

	static int const EXIT_CODE_REBOOT;
	static int const EXIT_CODE_UPDATE;

	static QPixmap GetIconFromFileW(const wchar_t * szPath, UINT size, int index = 0);

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
	bool native;

	std::wstring newest_version;
	std::wstring current_version;

	int current_dpi;
	int dragdrop_size;
	int dragdrop_offset;

	int old_raw_pid;
	int old_byname_pid;
	int old_bypid_pid;

	QTimer * t_Auto_Inj;
	QTimer * t_Delay_Inj;
	QTimer * t_Update_Proc;
	QTimer * t_OnUserInput;

	QPixmap pxm_banner;
	QPixmap pxm_lul;
	QPixmap pxm_error;
	QPixmap pxm_generic;

	QString lastPathStr;

	InjectionLib InjLib;

	QRegExpValidator * rev_NumbersOnly;

	// General
	bool platformCheck();
	void reboot();
	void add_file_to_list(QString str, bool active);

	void injec_status(bool ok, const QString msg);

	// Update GUI
	void toggleSelected();
	void rb_unset_all();
	void txt_pid_change();
	void cmb_proc_name_change();
	void btn_change();
	void update_proc_icon();

	// Settings
	void save_settings();
	void load_settings();
	void settings_get_update();

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

public slots:
	void get_from_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void get_from_scan_hook(int pid, int error);

signals:
	void send_to_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void send_to_scan_hook(int pid, int error);

private slots:
	// Titlebar
	void closeEvent(QCloseEvent * event) override;

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

	// Files
	void add_file_dialog();
	void remove_file();
	void select_file();

	// Inject
	void delay_inject();
	void inject_file();

	// Hook
	void btn_hook_scan_click();

	// Info
	void tooltip_change();
	void open_help();
	void generate_shortcut();
	void open_log();

	// Update
	void update_clicked();
};