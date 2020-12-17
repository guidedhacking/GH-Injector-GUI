#pragma once

#include "pch.h"

#include "framelesswindow.h"
#include "InjectionLib.h"

class DownloadProgressWindow : public QDialog
{
	Q_OBJECT

public:
	DownloadProgressWindow(QString title, std::vector<QString>(labels), QString status, int width, QWidget * parent = nullptr, FramelessWindow * frameless_parent = nullptr);

private:
	std::vector<QProgressBar*> m_ProgressBarList;
	FramelessWindow * m_pParent;
	QLabel * m_pStatus;

public:
	void SetProgress(int index, float value);
	void SetStatus(QString status);
	QString GetStatus();
};