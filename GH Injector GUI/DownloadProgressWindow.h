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
	std::vector<QProgressBar*> m_ProgressBarList;
	FramelessWindow * m_FramelessParent;
	QLabel * m_pStatus;
	std::function<void(void)> m_CloseCallback;

	bool m_update_status;
	bool m_update_progress;
	bool m_update_done;

	QString				m_new_status;
	std::vector<float>	m_new_progress;
	int					m_new_done;

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;

public:
	void SetProgress(int index, float value);
	void SetStatus(QString status);
	void SetDone(int code);

	void SetCloseCallback(std::function<void(void)> callback);

	QString GetStatus();
	FramelessWindow * GetParent();

public slots:
	void show();
};