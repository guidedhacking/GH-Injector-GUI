#pragma once

#include <QtWidgets/QMainWindow>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qtimer.h>
#include <qtreewidget.h>
#include <qevent.h>

#include <thread>


#include "framelesswindow/framelesswindow.h"
#include "ui_GuiMain.h"
#include "GuiProcess.h"
#include "GuiScanHook.h"
#include "Process.h"
#include "InjectionLib.hpp"
#include "Globals.h"
#include "DownloadProgress.h"


enum class UPDATE
{
	NOTHING,
	UPDATE1,
	DOWNLOAD,
};


class GuiMain : public QMainWindow
{
	Q_OBJECT

public:
	GuiMain(QWidget* parent = Q_NULLPTR);
	~GuiMain();

	static int const EXIT_CODE_REBOOT;

	static int str_to_arch(const QString str);
	static QString arch_to_str(const int arch);
	Ui::GuiMainClass ui;

	void add_file_to_list(QString str, bool active);
	void slotReboot();

private:

	FramelessWindow framelessPicker;
	FramelessWindow framelessScanner;
	GuiProcess* gui_Picker = NULL;
	GuiScanHook* gui_Scanner = NULL;

	// Design
	QPalette normalPalette;
	QString  normalSheet;
	QPalette darkPalette;
	QString	 darkSheet;

	// Settings
	Process_State_Struct*	pss;
	Process_Struct*			ps_picker;
	//Process_Struct*		ps_main;

	QString		lastPathStr;
	bool		ignoreUpdate;
	bool		lightMode;
	bool		lbl_hide_banner;
	UPDATE		update;
	bool		onReset;
	bool		onUserInput;

	std::thread process_update_thread;
	bool OnExit;
	
	QTimer* t_Auto_Inj;
	QTimer* t_Delay_Inj;

	//HINSTANCE hInjectionMod;
	//f_InjectA injectFunc;
	InjectionLib InjLib;

	std::string getVersionFromIE();

	void keyPressEvent(QKeyEvent * k);
	void UpdateProcess(int Interval = 100);
	
protected:
	void dragEnterEvent(QDragEnterEvent* e);
	void dragMoveEvent(QDragMoveEvent* e);
	void dragLeaveEvent(QDragLeaveEvent* e);
	//void dropEvent(QDropEvent* e); 
	
	bool eventFilter(QObject *obj, QEvent *event) override;
	
	void toggleSelected();

public slots:
	void get_from_picker(Process_State_Struct* procStateStruct, Process_Struct* procStruct);
	void get_from_scan_hook(int pid, int error);

signals:
	void send_to_picker(Process_State_Struct* procStateStruct, Process_Struct* procStruct);
	void send_to_scan_hook(int pid, int error);

private slots:
	// Titelbar
	void closeEvent(QCloseEvent* event) override;
	void platformCheck();

	// Settings
	void rb_process_set();
	void rb_pid_set();
	void rb_unset_all();
	void cmb_proc_name_change();
	void txt_pid_change();
	void btn_pick_process_click();

	// Auto, Reset
	void auto_inject();
	void auto_loop_inject();
	void reset_settings();
	void hook_Scan();
	void btn_hook_scan_change();

	// Settings, Color
	void save_settings();
	void load_settings();
	void color_setup();
	void color_change();
	void load_banner();
	void hide_banner();

	// Method, Cloaking, Advanced
	void load_change(int i);
	void create_change(int i);
	void cb_main_clicked();

	// Files
	void add_file_dialog();
	void remove_file();
	void select_file();
	void delay_inject();
	void inject_file();
	void injec_status(bool ok, const QString msg);
	void load_Dll();

	// Info
	void tooltip_change();
	void open_help();
	void open_log();

	// Update
	bool update_injector(std::string version);
	void check_online_version();
};