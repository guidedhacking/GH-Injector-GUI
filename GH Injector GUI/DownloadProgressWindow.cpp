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
	
	m_new_status	= "";
	m_new_done		= 0;

	auto * main_layout = new QVBoxLayout();

	for (const auto & i : labels)
	{
		auto * layout = new QHBoxLayout();
		
		auto * bar = new QProgressBar();
		bar->setMinimum(0);
		bar->setMaximum(100);
		bar->setFixedWidth(width);

		if (i.length() > 0)
		{
			auto * txt = new QLabel();
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
		auto * layout = new QHBoxLayout();

		m_pStatus = new QLabel();
		m_pStatus->setText(status);

		layout->addWidget(m_pStatus);
		main_layout->addLayout(layout);
	}

	setLayout(main_layout);

	m_FramelessParent = new FramelessWindow();
	m_FramelessParent->setFixedSize(QSize(width, 20 + (int)labels.size() * 40));
	m_FramelessParent->setMinimizeButton(false);
	m_FramelessParent->setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	m_FramelessParent->setWindowTitle(title);
	m_FramelessParent->setContent(this);

	m_FramelessParent->installEventFilter(this);
	installEventFilter(this);
}

DownloadProgressWindow::~DownloadProgressWindow()
{
	if (m_FramelessParent)
	{
		m_FramelessParent->close();
	}
}

void DownloadProgressWindow::show()
{
	m_FramelessParent->show();
}

//this is the second worst code in the history of mankind but I really don't give a fuck
//because multithreading with QDialogs is the the worst thing in the history of mankind

bool DownloadProgressWindow::eventFilter(QObject * obj, QEvent * event)
{
	if (event->type() == QEvent::Close && obj == m_FramelessParent)
	{
		if (m_CloseCallback)
		{
			m_CloseCallback();
			m_CloseCallback = nullptr;
		}
	}

	if (obj == this || obj == m_FramelessParent)
	{
		if (m_update_status)
		{
			m_update_status = false;
			m_pStatus->setText(m_new_status);
		}

		if (m_update_progress)
		{
			m_update_progress = false;
			for (int i = 0; i < m_ProgressBarList.size(); ++i)
			{
				m_ProgressBarList[i]->setValue(m_new_progress[i]);
			}
		}

		if (m_update_done)
		{
			m_update_done = false;
			m_CloseCallback = nullptr;
			done(m_new_done);
		}
	}	

	g_Console->update_external();

	return QObject::eventFilter(obj, event);
}

void DownloadProgressWindow::SetProgress(int index, float value)
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
			progress = (int)value;
		}

		m_new_progress[index]	= progress;
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
	m_new_done		= code;
	m_update_done	= true;
	this->update();
}

int DownloadProgressWindow::Execute()
{
	m_FramelessParent->setWindowModality(Qt::WindowModality::ApplicationModal);
	m_running = true;
	auto r = exec();
	m_running = false;
	m_FramelessParent->setWindowModality(Qt::WindowModality::NonModal);

	return r;
}

bool DownloadProgressWindow::IsRunning()
{
	return m_running;
}

void DownloadProgressWindow::SetCloseCallback(std::function<void(void)> CloseCallback)
{
	m_CloseCallback = CloseCallback;
}

QString DownloadProgressWindow::GetStatus()
{
	return m_pStatus->text();
}

FramelessWindow * DownloadProgressWindow::GetParent()
{
	return m_FramelessParent;
}