#pragma once

#include "pch.h"

#include "ui_GuiScanHook.h"

#include "framelesswindow.h"
#include "InjectionLib.h"
#include "StatusBox.h"

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

	int m_PID;

	void setItem(const std::vector<std::string> & item);
	std::vector<int> get_selected_indices();

signals:
	void send_to_inj_sh();

public slots:
	void get_from_inj_to_sh(int PID);

private slots:
	void scan_clicked();
	void unhook_clicked();

	void update_title(const QString title);
};