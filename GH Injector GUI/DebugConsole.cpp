#include "pch.h"

#include "DebugConsole.h"

DebugConsole::DebugConsole(FramelessWindow * dock_parent, QWidget * parent)
	: QWidget(parent)
{
	m_OldSelection	= "";

	QSizePolicy policy;
	policy.setHorizontalPolicy(QSizePolicy::Policy::MinimumExpanding);
	policy.setVerticalPolicy(QSizePolicy::Policy::MinimumExpanding);

	m_Content = new(std::nothrow) QListWidget();
	if (m_Content == Q_NULLPTR)
	{
		THROW("Failed to create content list widget for debug console.");
	}

	m_Content->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
	m_Content->setSizePolicy(policy);
	m_Content->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_Content->installEventFilter(this);

	m_Layout = new(std::nothrow) QGridLayout();
	if (m_Layout == Q_NULLPTR)
	{
		THROW("Failed to create content layout for debug console.");
	}

	//m_Layout->setMargin(1);

	QMargins m = { 0, 0, 0, 0 };
	this->setSizePolicy(policy);
	this->setContentsMargins(m);
	this->setLayout(m_Layout);
	this->layout()->addWidget(m_Content);
	this->installEventFilter(this);

	m_FramelessParent = new(std::nothrow) FramelessWindow();
	if (m_FramelessParent == Q_NULLPTR)
	{
		THROW("Failed to create parent window for debug console.");
	}

	m_FramelessParent->setMinimizeButton(false);
	m_FramelessParent->setResizeHorizontal(true);
	m_FramelessParent->setResizeBottom(true);
	m_FramelessParent->setWindowTitle("Debug data");
	m_FramelessParent->setContent(this);
	m_FramelessParent->installEventFilter(this);
	m_FramelessParent->setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	m_FramelessParent->resize(QSize(150, 350));

	if (dock_parent)
	{
		m_DockParent = dock_parent;
		m_FramelessParent->setDockButton(true, false, 0);
		m_Docker = new(std::nothrow) WindowDocker(m_DockParent, m_FramelessParent);
		if (m_Docker == Q_NULLPTR)
		{
			THROW("Failed to create window docker for debug console.");
		}

		m_Docker->SetDocking(true, true, true, true);
		m_Docker->SetResizing(true, true);
	}
	else
	{
		m_Docker = nullptr;
	}

	m_ExternalLocked	= false;
	m_WaitForLock		= true;
}

DebugConsole::~DebugConsole()
{
	if (m_Layout)
	{
		delete m_Layout;
	}

	if (m_Content)
	{
		delete m_Content;
	}
}

void DebugConsole::open()
{
	if (m_FramelessParent->isMinimized())
	{
		m_FramelessParent->setWindowState(m_FramelessParent->windowState() & ~Qt::WindowMinimized);
	}

	m_FramelessParent->show();
	SetWindowPos((HWND)m_FramelessParent->winId(), (HWND)m_DockParent->winId(), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void DebugConsole::close()
{
	m_FramelessParent->hide();
}

void DebugConsole::move(const QPoint & p)
{
	m_FramelessParent->move(p);
}

void DebugConsole::set_size(const QSize & s)
{
	m_FramelessParent->resize(s);
}

int DebugConsole::print(const char * format, ...)
{
	int result = 0;
	int size = 1024;
	char * buffer = new(std::nothrow) char[size]();

	if (buffer == nullptr)
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
			buffer = new(std::nothrow) char[size]();

			if (buffer == nullptr)
			{
				break;
			}
		}
		else if (result < 0)
		{
			break;
		}
	} while (result < 0);

	_set_thread_local_invalid_parameter_handler(old);

	if (result > 0)
	{
		print_raw(buffer);

		m_Content->scrollToBottom();
	}

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

	char * copy = nullptr;
	bool use_copy = false;

	size_t len = lstrlenA(szText);

	if (len > 0)
	{
		if (szText[len - 1] == '\n')
		{
			use_copy = true;

			copy = new(std::nothrow) char[len]();
			if (copy == nullptr)
			{
				return;
			}

			memcpy(copy, szText, len - 1);
		}
	}
	else
	{
		return;
	}

	if (use_copy)
	{
		szText = copy;
	}

#ifdef DEBUG_CONSOLE_TO_CMD
	printf("CONSOLE: %s\n", szText);
#endif

	QListWidgetItem * new_item = new(std::nothrow) QListWidgetItem();
	new_item->setText(szText);
	m_Content->addItem(new_item);

	while (m_Content->count() > 200)
	{
		delete m_Content->item(0);
	}

	if (use_copy)
	{
		delete[] copy;
	}

	m_Content->scrollToBottom();
}

bool DebugConsole::is_open()
{
	return (m_FramelessParent->isHidden() != true);
}

void DebugConsole::print_raw_external(const char * szText)
{
	if (m_ExternalLocked && !m_WaitForLock)
	{
		return;
	}

	while (m_ExternalLocked)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_ExternalLocked = true;

	m_ExternalDataBuffer.push_back(szText);

	m_ExternalLocked = false;
}

void DebugConsole::update_external()
{
	if (m_ExternalLocked && !m_WaitForLock)
	{
		return;
	}

	while (m_ExternalLocked)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_ExternalLocked = true;

	if (!m_ExternalDataBuffer.size())
	{
		m_ExternalLocked = false;

		return;
	}

	//initialize "new" vector object to prevent lnt-accidental-copy
	//reference won't work because the vector needs to be cleared asap
	//so that another thread can continue writing to it
	//thus a copy is needed before processing the data
	auto cpy = std::vector(m_ExternalDataBuffer);

	m_ExternalDataBuffer.clear();

	m_ExternalLocked = false;

	if (!m_Content)
	{
		return;
	}

	for (const auto & i : cpy)
	{
		print_raw(i.c_str());
	}

	m_Content->scrollToBottom();
}

void DebugConsole::dock()
{
	if (m_Docker)
	{
		m_Docker->Dock();
	}
}

void DebugConsole::dock(int direction)
{
	if (m_Docker)
	{
		switch (direction)
		{
			case 0:
				g_print("Console docked: right\n");
				break;

			case 1:
				g_print("Console docked: left\n");
				break;

			case 2:
				g_print("Console docked: top\n");
				break;

			case 3:
				g_print("Console docked: bottom\n");
				break;

			default:
				g_print("Invalid dock index specified\n");
		}

		m_Docker->Dock(direction);

		m_Content->scrollToBottom();
	}
}

int DebugConsole::get_dock_index()
{
	if (m_Docker)
	{
		return m_Docker->GetDockIndex();
	}

	return -1;
}

int DebugConsole::get_old_dock_index()
{
	if (m_Docker)
	{
		return m_Docker->GetOldDockIndex();
	}

	return -1;
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
			auto selected = m_Content->selectedItems();
			
			if (!selected.isEmpty())
			{
				std::vector<QListWidgetItem *> selection_sorted;
				for (int i = 0; i < m_Content->count(); ++i)
				{
					QListWidgetItem * it = m_Content->item(i);
					if (it->isSelected())
					{
						selection_sorted.push_back(it);
					}
				}

				QString cb_data;
				for (const auto & i : selection_sorted)
				{
					cb_data += i->text() + "\n";
				}

				if (cb_data != m_OldSelection)
				{
					qApp->clipboard()->setText(cb_data);

					m_OldSelection = cb_data;
				}

				return true;
			}
		}
	}
	else if (event->type() == QEvent::FocusOut)
	{
		m_OldSelection = "";
	}

	return QObject::eventFilter(obj, event);
}

void __stdcall g_print_to_console_raw(const char * szText)
{
	if (g_Console)
	{
		g_Console->print_raw(szText);
	}
	else
	{
		printf(szText);
	}
}

void __stdcall g_print_to_console_raw_external(const char * szText)
{
	if (g_Console)
	{
		g_Console->print_raw_external(szText);
	}
	else
	{
		printf(szText);
	}
}