#pragma once

#include "pch.h"

#include "framelesswindow.h"
#include "InjectionLib.h"

class DownloadProgressWindow;

using f_DPW_Callback = void(__stdcall *)(DownloadProgressWindow * hProgressWindow, void * custom_arg);

class DownloadProgressWindow : public QDialog
{
	Q_OBJECT

public:
	DownloadProgressWindow(QString title, std::vector<QString>(labels), QString status, int width, QWidget * parent = nullptr);

	~DownloadProgressWindow();

private:
	std::vector<QProgressBar *>	m_ProgressBarList;
	FramelessWindow			 *	m_FramelessParent;
	QLabel					 *	m_pStatus;

	bool m_running;

	bool m_update_status;
	bool m_update_progress;
	bool m_update_done;

	QString				m_new_status;
	std::vector<float>	m_new_progress;
	int					m_new_done;
	int					m_custom_close_value;

	int m_frequency;
	bool m_timer_running;
	QTimer * m_tCallback;

	void * m_pCustomArg;
	f_DPW_Callback m_pCallback;

private slots:
	void on_close_button_clicked();
	void on_timer_callback();

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

public:
	void SetProgress(UINT index, float value);
	void SetStatus(QString status);
	void SetDone(int code);

	void SetCallbackFrequency(int frequency);
	void SetCallbackArg(void * argument);
	void SetCallback(f_DPW_Callback callback);

	void SetCloseValue(int value);

	int Execute();
	bool IsRunning();

	QString GetStatus();
	FramelessWindow * GetParent();

public slots:
	void show();
};