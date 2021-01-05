#include "pch.h"

#include "DebugConsole.h"

DebugConsole::DebugConsole(QWidget * parent)
	: QWidget(parent)
{
	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Policy::MinimumExpanding);
	policy.setVerticalPolicy(QSizePolicy::Policy::MinimumExpanding);

	m_Content = new QListWidget();
	m_Content->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_Content->setSizePolicy(policy);
	m_Content->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_Content->installEventFilter(this);

	m_Layout = new QGridLayout();
	m_Layout->setMargin(0);

	QMargins m = { 0, 0, 0, 0 };
	this->setSizePolicy(policy);
	this->setContentsMargins(m);
	this->setLayout(m_Layout);
	this->layout()->addWidget(m_Content);

	m_FramelessParent = new FramelessWindow();
	m_FramelessParent->setMinimizeButton(false);
	m_FramelessParent->setResizeHorizontal(true);
	m_FramelessParent->setResizeBottom(true);
	m_FramelessParent->setWindowTitle("Debug data");
	m_FramelessParent->setContent(this);

	onDelete = false;
}

DebugConsole::~DebugConsole()
{
	delete m_Layout;
	delete m_Content;
}

void DebugConsole::open()
{
	m_FramelessParent->show();
	show();
}

void DebugConsole::close()
{
	hide();
	m_FramelessParent->hide();
}

void DebugConsole::move(const QPoint & p)
{
	m_FramelessParent->move(p);
}

void DebugConsole::setSize(const QSize & s)
{
	m_FramelessParent->resize(s);
}

int DebugConsole::print(const char * format, ...)
{
	int result = 0;
	int size = 1024;
	char * buffer = new char[size]();

	if (!buffer)
	{
		return -1;
	}

	auto old = _set_thread_local_invalid_parameter_handler(DebugConsole::ImTheTrashMan);

	do
	{
		va_list args;
		va_start(args, format);

		int err = 0;
		result = vsprintf_s(buffer, size, format, args);

		if (result <= 0)
		{
			err = errno;
		}

		va_end(args);

		if (result < 0 && err == ERANGE)
		{
			delete[] buffer;

			size += 1024;
			buffer = new char[size]();

			if (!buffer)
			{
				break;
			}
		}
	} while (result < 0);

	_set_thread_local_invalid_parameter_handler(old);

	if (result > 0)
	{
		print_raw(buffer);

		while (m_Content->count() > 50)
		{
			delete m_Content->item(0);
		}
	}

	m_Content->scrollToBottom();

	if (buffer)
	{
		delete[] buffer;
	}

	return result;
}

void DebugConsole::print_raw(const char * szText)
{
	if (!szText)
	{
		return;
	}
	
	auto len = lstrlenA(szText);
	
	if (len > 1)
	{
		if (szText[len - 1] == '\n')
		{
			//yeah, idc honestly
			const_cast<char *>(szText)[len - 1] = '\0';
		}
	}

	QListWidgetItem * new_item = new QListWidgetItem();
	new_item->setText(szText);
	m_Content->addItem(new_item);

	while (m_Content->count() > 50)
	{
		delete m_Content->item(0);
	}

	m_Content->scrollToBottom();
}

void DebugConsole::ImTheTrashMan(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved)
{
	UNREFERENCED_PARAMETER(expression);
	UNREFERENCED_PARAMETER(function);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);
	UNREFERENCED_PARAMETER(pReserved);

	//take that, CRT
	//but for real, the CRT error "handlers" are the dumbest shit ever because other than some strings you get no info to actually handle the error
	//or I am too dumb
	//probably both
}

bool DebugConsole::eventFilter(QObject * obj, QEvent * event)
{
	if (event->type() == QEvent::KeyPress)
	{
		auto * keyEvent = static_cast<QKeyEvent *>(event);
		if (keyEvent->matches(QKeySequence::Copy) || keyEvent->matches(QKeySequence::Cut))
		{
			printf("copy xd");
		}
	}

	return QObject::eventFilter(obj, event);
}

void g_print_to_console_raw(const char * szText)
{
	if (g_Console)
	{
		g_Console->print_raw(szText);
	}
}