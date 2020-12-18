#pragma once

#include "ui_GuiProcess.h"
#include "Process.h"
#include "framelesswindow.h"

struct Process_State_Struct
{
	QString			txtFilter;
	int				cmbArch;
	bool			cbSession;
};

class GuiProcess : public QWidget
{
	Q_OBJECT

public:
	GuiProcess(QWidget * parent = Q_NULLPTR, FramelessWindow * FramelessParent = Q_NULLPTR);
	~GuiProcess();

private:

	Ui::frm_proc ui;
	FramelessWindow * frameless_parent;

	Process_State_Struct	*	pss;
	Process_Struct			*	ps;
	QFileSystemModel			model;
	SORT_PS						sort_prev;
	bool						native;

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

signals:
	void send_to_inj(Process_State_Struct * procStateStruct, Process_Struct * procStruct);

public slots:
	void get_from_inj(Process_State_Struct * procStateStruct, Process_Struct * procStruct);

private slots:

	void refresh_gui();
	void refresh_process();
	void filter_change(int i);
	void session_change();
	void name_change(const QString &);
	void proc_select(bool ignore = false);
	void custom_sort(int);
	void double_click_process(const QModelIndex & index);
};