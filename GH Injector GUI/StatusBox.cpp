#include "pch.h"

#include "StatusBox.h"

void ShowStatusbox(bool ok, const QString & msg)
{
	FramelessWindow parent;
	parent.setMinimizeButton(false);
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

bool YesNoBox(const QString & title, const QString & msg, QWidget * parent)
{
	FramelessWindow f_parent;
	f_parent.setMinimizeButton(false);
	f_parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = Q_NULLPTR;
	
	f_parent.setWindowTitle(title);
	box = new QMessageBox(QMessageBox::Icon::Question, "", msg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, &f_parent, Qt::WindowType::FramelessWindowHint);
	box->setDefaultButton(QMessageBox::StandardButton::Yes);
	
	if (box == Q_NULLPTR)
	{
		return false;
	}

	f_parent.setContent(box);
	f_parent.setFixedWidth(box->width());

	if (parent != Q_NULLPTR)
	{
		auto parent_pos = parent->pos();
		auto parent_rect = parent->contentsRect();

		auto this_rect = f_parent.rect();
		QPoint this_pos = { parent_pos.x() + parent_rect.width() / 2 - this_rect.width() / 2, parent_pos.y() + parent_rect.height() / 2 - this_rect.height() / 2 };

		f_parent.move(this_pos);
	}

	f_parent.show();	

	auto r = box->exec();

	delete box;

	return (r == QMessageBox::StandardButton::Yes);
}