#include "pch.h"

#include "StatusBox.h"

void ShowStatusbox(bool ok, const QString & msg)
{
	FramelessWindow parent;
	parent.setMinimizeButton(false);

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
	parent.show();
	parent.setFixedWidth(box->width() + 40);
	box->exec();

	delete box;
}