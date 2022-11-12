#include "pch.h"

#include "StatusBox.h"

void StatusBox(bool ok, const QString & msg)
{
	FramelessWindow f_parent;
	f_parent.setMinimizeButton(false);
	f_parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = Q_NULLPTR;

	if (ok)
	{
		f_parent.setWindowTitle("Success");
		box = new(std::nothrow) QMessageBox(QMessageBox::Icon::Information, "", msg, QMessageBox::StandardButton::Ok, &f_parent, Qt::WindowType::FramelessWindowHint);
	}
	else
	{
		f_parent.setWindowTitle("Error");
		box = new(std::nothrow) QMessageBox(QMessageBox::Icon::Critical, "", msg, QMessageBox::StandardButton::Ok, &f_parent, Qt::WindowType::FramelessWindowHint);
	}

	if (box == Q_NULLPTR)
	{
		if (ok)
		{
			MessageBoxW(NULL, msg.toStdWString().c_str(), L"Success", MB_ICONINFORMATION);
		}
		else
		{
			MessageBoxW(NULL, msg.toStdWString().c_str(), L"Error", MB_ICONERROR);
		}

		return;
	}

	f_parent.setContent(box);
	f_parent.setFixedWidth(box->width());	

	HWND hwnd = reinterpret_cast<HWND>(f_parent.winId());
	if (hwnd)
	{
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		f_parent.setWindowModality(Qt::WindowModality::ApplicationModal);
	}

	f_parent.show();

	auto buttons = box->buttons();
	for (const auto & i : buttons)
	{
		auto txt = i->text();
		if (txt.contains("ok", Qt::CaseInsensitive))
		{
			i->setFocus();
			break;
		}
	}

	box->exec();

	f_parent.close();

	delete box;
}

bool YesNoBox(const QString & title, const QString & msg, QWidget * parent)
{
	FramelessWindow f_parent;
	f_parent.setMinimizeButton(false);
	f_parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	f_parent.setWindowTitle(title);
	f_parent.setWindowModality(Qt::WindowModality::ApplicationModal);

	auto box = new(std::nothrow) QMessageBox(QMessageBox::Icon::Question, "", msg, QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, &f_parent, Qt::WindowType::FramelessWindowHint);
	if (box == Q_NULLPTR)
	{
		HWND hwnd_parent = NULL;
		if (parent != Q_NULLPTR)
		{
			hwnd_parent = reinterpret_cast<HWND>(parent->winId());
		}

		auto r = MessageBoxW(hwnd_parent, msg.toStdWString().c_str(), title.toStdWString().c_str(), MB_YESNO);

		return (r == IDYES);
	}

	f_parent.setContent(box);

	auto buttons = box->buttons();
	for (const auto & i : buttons)
	{
		auto txt = i->text();
		if (txt.contains("yes", Qt::CaseInsensitive))
		{
			i->setFocus();
			break;
		}
	}

	f_parent.setFixedWidth(box->width());	
	f_parent.show();

	HWND hwnd_f_parent = reinterpret_cast<HWND>(f_parent.winId());
	if (hwnd_f_parent)
	{
		SetWindowPos(hwnd_f_parent, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	if (parent != Q_NULLPTR)
	{
		auto parent_pos = parent->pos();
		auto parent_rect = parent->contentsRect();

		auto this_rect = f_parent.rect();
		QPoint this_pos = { parent_pos.x() + parent_rect.width() / 2 - this_rect.width() / 2, parent_pos.y() + parent_rect.height() / 2 - this_rect.height() / 2 };

		f_parent.move(this_pos);
	}
	else
	{
		auto main_screen = QGuiApplication::primaryScreen();
		if (main_screen)
		{
			auto main_screen_geometry = main_screen->geometry();
			auto main_rect = QRect(0, 0, main_screen_geometry.width(), main_screen_geometry.height());

			auto this_rect = f_parent.rect();
			QPoint this_pos = { main_rect.width() / 2 - this_rect.width() / 2, main_rect.height() / 2 - this_rect.height() / 2 };

			f_parent.move(this_pos);
		}
	}
	
	auto r = box->exec();

	f_parent.close();

	delete box;

	return (r == QMessageBox::StandardButton::Yes);
}