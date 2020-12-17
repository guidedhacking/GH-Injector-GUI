#pragma once

#include "ui_GuiScanHook.h"
#include "InjectionLib.h"

class GuiScanHook : public QWidget
{
	Q_OBJECT

public:
	GuiScanHook(QWidget * parent = Q_NULLPTR);
	~GuiScanHook();

	InjectionLib InjLib;

private:
	Ui::Form ui;
	QStringListModel * model;
	QStringList List;

	int m_pid = 0;
	int m_err = 0;


	void setItem(const std::vector<std::string> & item);
	std::vector<std::string> getSelectedItem();

signals:
	void send_to_inj_sh(int pid, int error);

public slots:
	void get_from_inj_to_sh(int pid, int error);

private slots:

	void refresh_gui();
	void scan_clicked();
	void unhook_clicked();

	void injec_status(bool ok, const QString msg);
};