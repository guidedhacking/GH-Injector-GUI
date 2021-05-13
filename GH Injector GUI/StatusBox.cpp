#include "pch.h"

#include "StatusBox.h"

void ShowStatusbox(bool ok, const QString & msg)
{
	FramelessWindow parent;
	parent.setMinimizeButton(false);
	parent.setDockButton(false);
	parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = Q_NULLPTR;

	if (ok)
	{
		parent.setWindowTitle("Success");
		box = new QMessageBox(QMessageBox::Icon::Information, "", msg, QMessageBox::StandardButton::Ok, &parent, Qt::WindowType::FramelessWindowHint);
	}
	else
	{
		parent.setWindowTitle("Error");
		box = new QMessageBox(QMessageBox::Icon::Critical, "", msg, QMessageBox::StandardButton::Ok, &parent, Qt::WindowType::FramelessWindowHint);
	}

	if (box == Q_NULLPTR)
	{
		return;
	}

	parent.setContent(box);
	parent.setFixedWidth(box->width());
	parent.show();

	box->exec();

	delete box;
}

bool YesNoBox(const QString & title, const QString & msg)
{
	FramelessWindow parent;
	parent.setMinimizeButton(false);
	parent.setDockButton(false);
	parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = Q_NULLPTR;
	
	parent.setWindowTitle(title);
	box = new QMessageBox(QMessageBox::Icon::Question, "", msg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, &parent, Qt::WindowType::FramelessWindowHint);
	box->setDefaultButton(QMessageBox::StandardButton::Yes);
	
	if (box == Q_NULLPTR)
	{
		return false;
	}

	parent.setContent(box);
	parent.setFixedWidth(box->width());
	parent.show();

	auto r = box->exec();

	delete box;

	return (r == QMessageBox::StandardButton::Yes);
}