#include "pch.h"

#include "DownloadProgressWindow.h"

DownloadProgressWindow::DownloadProgressWindow(QString title, std::vector<QString>(labels), QString status, int width, QWidget * parent, FramelessWindow * frameless_parent)
	: QDialog(parent)
{
	m_pParent = frameless_parent;
	m_pStatus = Q_NULLPTR;

	if (m_pParent)
	{
		m_pParent->setWindowTitle(title);
	}
	else
	{
		setWindowTitle(title);
	}

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

		m_ProgressBarList[index]->setValue(progress);
	}
}

void DownloadProgressWindow::SetStatus(QString status)
{
	m_pStatus->setText(status);
}

QString DownloadProgressWindow::GetStatus()
{
	return m_pStatus->text();
}