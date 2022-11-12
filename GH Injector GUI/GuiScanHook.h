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
	GuiScanHook(QWidget * parent = Q_NULLPTR, FramelessWindow * FramelessParent = Q_NULLPTR, InjectionLib * InjectionLib = nullptr);
	~GuiScanHook();

private:
	Ui::Form ui;

	FramelessWindow		* m_FramelessParent	= Q_NULLPTR;
	QStringListModel	* m_Model			= Q_NULLPTR;
	InjectionLib		* m_InjectionLib	= nullptr;
	QStringList			m_HookList;

	int m_PID = 0;

	void setItem(const std::vector<std::string> & item);
	std::vector<int> get_selected_indices() const;

signals:
	void send_to_inj_sh();

public slots:
	void get_from_inj_to_sh(int PID);

private slots:
	void scan_clicked();
	void unhook_clicked();

	void update_title(const QString title);
};