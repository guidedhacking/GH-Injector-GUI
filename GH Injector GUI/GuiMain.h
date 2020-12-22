#pragma once

//forward declaration because project structure is just fucked lmao
class GuiMain;

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

	static int const EXIT_CODE_REBOOT;

	static ARCH str_to_arch(const QString str);
	static QString arch_to_str(const ARCH arch);
	Ui::GuiMainClass ui;

	void add_file_to_list(QString str, bool active);
	void slotReboot();

private:

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
	bool lbl_hide_banner;
	bool onReset;
	bool native;
	bool interrupt_download;
	bool pre_main_exec_update;

	std::string newest_version;
	std::string current_version;

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

	QPixmap pxm_lul;
	QPixmap pxm_error;
	QPixmap pxm_generic;

	QString lastPathStr;

	InjectionLib InjLib;

	void toggleSelected();

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

public slots:
	void get_from_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void get_from_scan_hook(int pid, int error);

signals:
	void send_to_picker(Process_State_Struct * procStateStruct, Process_Struct * procStruct);
	void send_to_scan_hook(int pid, int error);

private slots:
	// Titelbar
	void closeEvent(QCloseEvent * event) override;
	bool platformCheck();

	// Settings
	void rb_process_set();
	void rb_pid_set();
	void rb_unset_all();
	void cmb_proc_name_change();
	void txt_pid_change();
	void btn_pick_process_click();
	void update_process();
	void update_proc_icon();

	// Auto, Reset
	void auto_inject();
	void auto_loop_inject();
	void reset_settings();
	void hook_Scan();
	void btn_change();

	// Settings, Color
	void save_settings();
	void load_settings();
	void settings_get_update();
	void load_banner();
	void hide_banner();

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
	void delay_inject();
	void inject_file();
	void injec_status(bool ok, const QString msg);

	// Info
	void tooltip_change();
	void open_help();
	void generate_shortcut();
	void open_log();

	// Update
	void update_download_progress(DownloadProgressWindow * wnd, DownloadProgress * progress);
	void update_update_thread(DownloadProgressWindow * wnd, std::string version);
	void update_injector(std::string version);
	void update_init();
	std::string get_newest_version();

	// PDB Download
	void pdb_download();
	void pdb_download_update_thread(DownloadProgressWindow * wnd);
};