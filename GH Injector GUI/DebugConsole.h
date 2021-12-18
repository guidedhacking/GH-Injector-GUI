#pragma once

#include "pch.h"

#include "framelesswindow.h"

//#define DEBUG_CONSOLE_TO_CMD

class DebugConsole : public QWidget
{
	Q_OBJECT

public:
	DebugConsole(FramelessWindow * dock_parent = Q_NULLPTR, QWidget * parent = Q_NULLPTR);

	~DebugConsole();

	void open();
	void close();

	void move(const QPoint & p);
	void set_size(const QSize & s);

	int print(const char * format, ...);
	void print_raw(const char * szText);

	bool is_open();
	int get_dock_index();
	int get_old_dock_index();

	//from non qt thread call this function and not the regular print_raw or bad things happen to your GUI (sometimes)
	void print_raw_external(const char * szText);

	//call this function from the qt owner thread to update the console
	void update_external();

	//updates the current dock position (if already docked)
	void dock();

	//updates the dock position to the specified direction
	void dock(int direction);

private:
	WindowDocker * m_Docker;

	FramelessWindow * m_FramelessParent;
	FramelessWindow * m_DockParent;

	QListWidget * m_Content;
	QGridLayout * m_Layout;

	QString m_OldSelection;

	std::vector<std::string> m_ExternalDataBuffer;

	bool m_ExternalLocked;
	bool m_WaitForLock;

	static void ImTheTrashMan(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved);

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;
};

inline DebugConsole * g_Console = nullptr;

#define g_print g_Console->print
#define g_print_raw g_Console->print_raw

void __stdcall g_print_to_console_raw(const char * szText);

void __stdcall g_print_to_console_raw_external(const char * szText);