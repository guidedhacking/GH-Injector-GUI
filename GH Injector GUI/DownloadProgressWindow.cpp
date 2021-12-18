#include "pch.h"

#include "DownloadProgressWindow.h"

DownloadProgressWindow::DownloadProgressWindow(QString title, std::vector<QString>(labels), QString status, int width, QWidget * parent)
	: QDialog(parent)
{
	m_pStatus = Q_NULLPTR;
	
	m_running = false;

	m_update_status		= false;
	m_update_progress	= false;
	m_update_done		= false;

	m_custom_close_value = 0;

	m_new_status	= "";
	m_new_done		= 0;

	m_frequency		= 100;
	m_timer_running = false;
	m_pCustomArg	= nullptr;
	m_pCallback		= nullptr;

	auto * main_layout = new(std::nothrow) QVBoxLayout();
	if (main_layout == Q_NULLPTR)
	{
		THROW("Failed to create main layout for download window.");
	}

	for (const auto & i : labels)
	{
		auto * layout = new(std::nothrow) QHBoxLayout();
		if (layout == Q_NULLPTR)
		{
			THROW("Failed to create layout for download window.");
		}
		
		auto * bar = new(std::nothrow) QProgressBar();
		if (bar == Q_NULLPTR)
		{
			THROW("Failed to create progress bar for download window.");
		}

		bar->setMinimum(0);
		bar->setMaximum(100);
		bar->setFixedWidth(width);

		if (i.length() > 0)
		{
			auto * txt = new(std::nothrow) QLabel();
			if (txt == Q_NULLPTR)
			{
				THROW("Failed to create label for download window.");
			}

			txt->setText(i);
			layout->addWidget(txt);
		}

		layout->addWidget(bar);

		main_layout->addLayout(layout);

		m_ProgressBarList.push_back(bar);

		m_new_progress.push_back(0.0f);
	}

	if (status.length() > 0)
	{
		auto * layout = new(std::nothrow) QHBoxLayout();
		if (layout == Q_NULLPTR)
		{
			THROW("Failed to create layout for download window.");
		}

		m_pStatus = new(std::nothrow) QLabel();
		if (m_pStatus == Q_NULLPTR)
		{
			THROW("Failed to create label for download window.");
		}

		m_pStatus->setText(status);

		layout->addWidget(m_pStatus);
		main_layout->addLayout(layout);
	}

	setLayout(main_layout);

	m_FramelessParent = new(std::nothrow) FramelessWindow();
	if (m_FramelessParent == Q_NULLPTR)
	{
		THROW("Failed to create parent window for download window.");
	}

	m_FramelessParent->setFixedSize(QSize(width, 20 + (int)labels.size() * 40));
	m_FramelessParent->setMinimizeButton(false);
	m_FramelessParent->setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	m_FramelessParent->setWindowTitle(title);
	m_FramelessParent->setContent(this);

	m_tCallback = new QTimer(this);

	connect(m_FramelessParent, SIGNAL(closeButton_clicked()), this, SLOT(on_close_button_clicked()));
	connect(m_tCallback, SIGNAL(timeout()), this, SLOT(on_timer_callback()));

	m_FramelessParent->installEventFilter(this);
	installEventFilter(this);
}

DownloadProgressWindow::~DownloadProgressWindow()
{
	if (m_FramelessParent)
	{
		m_FramelessParent->close();
	}

	SAFE_DELETE(m_tCallback);
}

void DownloadProgressWindow::on_close_button_clicked()
{
	done(m_custom_close_value);

	close();
}

void DownloadProgressWindow::show()
{
	m_FramelessParent->show();
}

//this is the second worst code in the history of mankind but I really don't give a fuck
//because multithreading with QDialogs is the the worst thing in the history of mankind

void DownloadProgressWindow::on_timer_callback()
{
	if (m_pCallback && m_running)
	{
		m_pCallback(this, m_pCustomArg);
	}
}

bool DownloadProgressWindow::eventFilter(QObject * obj, QEvent * event)
{
	if (!m_timer_running)
	{
		m_timer_running = true;

		m_tCallback->start();
	}

	if (event->type() == QEvent::Close && obj == m_FramelessParent)
	{
		m_tCallback->stop();
	}

	if (obj == this || obj == m_FramelessParent)
	{
		if (event->type() == QEvent::UpdateRequest)
		{
			if (m_update_status)
			{
				m_update_status = false;
				m_pStatus->setText(m_new_status);
			}

			if (m_update_progress)
			{
				m_update_progress = false;
				for (UINT i = 0; i < m_ProgressBarList.size(); ++i)
				{
					m_ProgressBarList[i]->setValue(static_cast<int>(m_new_progress[i]));
				}
			}
		}		

		if (m_update_done)
		{
			m_update_done = false;
			done(m_new_done);
		}
	}	

	g_Console->update_external();

	return QObject::eventFilter(obj, event);
}

void DownloadProgressWindow::SetProgress(UINT index, float value)
{
	if (index < m_ProgressBarList.size())
	{
		int progress = 0;

		if (value >= 1.0f)
		{
			progress = 100;
		}
		else if (value <= 0.0f)
		{
			progress = 0;
		}
		else
		{
			value *= 100.0f + 0.5f;
			progress = static_cast<int>(value);
		}

		m_new_progress[index]	= static_cast<float>(progress);
		m_update_progress		= true;
		this->update();
	}
}

void DownloadProgressWindow::SetStatus(QString status)
{
	m_new_status	= status;
	m_update_status = true;
	this->update();
}

void DownloadProgressWindow::SetDone(int code)
{
	m_new_done = code;
	m_update_done = true;
	this->update();
}

void DownloadProgressWindow::SetCallbackFrequency(int frequency)
{
	if (m_tCallback)
	{
		m_tCallback->setInterval(1000 / frequency);
	}
}

void DownloadProgressWindow::SetCallbackArg(void * argument)
{
	m_pCustomArg = argument;
}

void DownloadProgressWindow::SetCallback(f_DPW_Callback callback)
{
	m_pCallback = callback;
}

void DownloadProgressWindow::SetCloseValue(int value)
{
	m_custom_close_value = value;
}

int DownloadProgressWindow::Execute()
{
	HWND hwnd = reinterpret_cast<HWND>(m_FramelessParent->winId());

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	m_FramelessParent->setWindowModality(Qt::WindowModality::ApplicationModal);

	m_running = true;
	auto r = exec();
	m_running = false;
	
	SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	return r;
}

QString DownloadProgressWindow::GetStatus()
{
	return m_pStatus->text();
}

FramelessWindow * DownloadProgressWindow::GetParent()
{
	return m_FramelessParent;
}