#pragma once

#include "ui_GuiScanHook.h"
#include "InjectionLib.h"
#include "framelesswindow.h"

class GuiScanHook : public QWidget
{
	Q_OBJECT

public:
	GuiScanHook(QWidget * parent = Q_NULLPTR, FramelessWindow * FramelessParent = Q_NULLPTR, InjectionLib * lib = nullptr);
	~GuiScanHook();

private:
	Ui::Form ui;
	FramelessWindow		* frameless_parent;
	QStringListModel	* model;
	InjectionLib		* InjLib;
	QStringList			List;

	int m_pid;
	int m_err;

	void setItem(const std::vector<std::string> & item);
	std::vector<int> get_selected_indices();

signals:
	void send_to_inj_sh(int pid, int error);

public slots:
	void get_from_inj_to_sh(int pid, int error);

private slots:
	void scan_clicked();
	void unhook_clicked();

	void injec_status(bool ok, const QString msg);
	void update_title(const QString title);
};