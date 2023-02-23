#pragma once

#include "pch.h"

#include "framelesswindow.h"
#include "InjectionLib.h"

class DownloadProgressWindow : public QDialog
{
	Q_OBJECT

public:
	DownloadProgressWindow(QString title, std::vector<QString>(labels), QString status, int width, QWidget * parent = nullptr);

	~DownloadProgressWindow();

private:
	std::vector<QProgressBar *>	m_ProgressBarList;
	FramelessWindow			 *	m_FramelessParent	= Q_NULLPTR;
	QLabel					 *	m_pStatus			= Q_NULLPTR;

	bool m_bRunning = false;

	bool m_bUpdateStatus	= false;
	bool m_bUpdateProgress	= false;
	bool m_bUpdateDone		= false;

	QString				m_NewStatus = QString("");
	std::vector<float>	m_NewProgress;

	int					m_NewDone			= 0;
	int					m_CustomCloseValue	= 0;

	bool	m_bTimerRunning	= false;
	QTimer	m_TmrCallback;

	void * m_pCustomArg													= nullptr;
	std::function<void(DownloadProgressWindow *, void *)> m_pCallback	= nullptr;

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
	void SetCallback(const decltype(m_pCallback) & callback);

	void SetCloseValue(int value);

	int Execute();

	QString GetStatus() const;
	FramelessWindow * GetParent() const;

public slots:
	void show();
};