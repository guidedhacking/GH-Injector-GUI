#pragma once

#include <qfilesystemmodel.h>
#include <QWidget>
#include "ui_GuiProcess.h"
#include "Process.h"

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
	GuiProcess(QWidget* parent = Q_NULLPTR);
	~GuiProcess();

private:
	Ui::frm_proc ui;
	Process_State_Struct*	pss;
	Process_Struct*			ps;
	QFileSystemModel		model;
	SORT_PS					sort_prev;

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

signals:
	void send_to_inj(Process_State_Struct* procStateStruct, Process_Struct* procStruct);

public slots:
	void get_from_inj(Process_State_Struct* procStateStruct, Process_Struct* procStruct);

private slots:
	
	void refresh_gui();
	void refresh_process();
	void filter_change(int i);
	void session_change();
	void name_change(const QString&);
	void proc_select(bool ignore = false);
	void customSort(int);
	void on_treeView_doubleClicked(const QModelIndex & index);
};
