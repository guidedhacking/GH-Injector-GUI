#pragma once

#include "pch.h"

#include "framelesswindow.h"
#include "ui_framelesswindow.h"

//#define DEBUG_CONSOLE_TO_CMD

class DebugConsole : public QWidget
{
	Q_OBJECT

public:
	DebugConsole(QWidget * parent = Q_NULLPTR);

	~DebugConsole();

	void open();
	void close();
	void move(const QPoint & p);
	void setSize(const QSize & s);
	int print(const char * format, ...);
	void print_raw(const char * szText);
	void add_parent(QWidget * parent);
	void add_dock_parent(QWidget * parent);
	void dock(bool docked);
	HWND get_parent();

	//from non qt thread call this function and not the regular print_raw or bad things happen to your GUI (sometimes)
	void print_raw_external(const char * szText);

	//call this function from the qt owner thread to update the console
	void update_external();

private:
	FramelessWindow * m_FramelessParent;
	QListWidget * m_Content;
	QGridLayout * m_Layout;

	QString m_OldSelection;

	QWidget * m_parent;
	QWidget * m_dock_parent;

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