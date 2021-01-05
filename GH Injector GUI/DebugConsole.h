#pragma once

#include "framelesswindow.h"

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

private:
	bool onDelete;

	FramelessWindow * m_FramelessParent;
	QListWidget * m_Content;
	QGridLayout * m_Layout;

	static void ImTheTrashMan(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line, uintptr_t pReserved);

protected:
	bool eventFilter(QObject * obj, QEvent * event) override;
};

inline DebugConsole * g_Console = nullptr;

#define g_print g_Console->print

void g_print_to_console_raw(const char * szText);