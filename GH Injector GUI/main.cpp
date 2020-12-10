#include <QApplication>
#include "DarkStyle.h"
#include "framelesswindow/framelesswindow.h"
#include "mainwindow.h"
#include "GuiMain.h"
#include "CmdArg.hpp"
#include "InjectionLib.hpp"
#include "DragDropWindow.h"
#include "resource.h"

#pragma comment (lib, "URLMon.lib")

#ifdef _DEBUG

#define DEBUG_CMD_ARG
char* argument_value1[]{ "val1" };
char* argument_value2[]{ "val1", "-y" };
char* argument_value3[]{ "val1", "-f", "C:\\temp\\HelloWorld_x64.dll", "-p", "notepad.exe" };

#endif

int main(int argc, char* argv[]) {

	/*AllocConsole();
	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);*/

#ifdef DEBUG_CMD_ARG
	int res = CmdArg(ARRAYSIZE(argument_value1), argument_value1);
#else	
	int res = CmdArg(argc, argv);
#endif //DEBUG CMD

	if (res > 1)
	{
		return res;
	}
	
	// Restart Application loop
	int currentExitCode = 0;
	do {
		QApplication a(argc, argv);

		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
		
		FramelessWindow framelessWindow;
		if (!res)
		{
			DarkStyle* dark = new DarkStyle;
			QApplication::setStyle(dark);
			QApplication::setPalette(QApplication::style()->standardPalette());

			framelessWindow.setWindowTitle("GH Injector");
			framelessWindow.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

			GuiMain* MainWindow = new GuiMain(&framelessWindow);
			MainWindow->statusBar()->setSizeGripEnabled(false);
			
			HWND hDragnDrop = CreateDragDropWindow((HWND)framelessWindow.winId(), MainWindow);

			framelessWindow.setContent(MainWindow);
			framelessWindow.show();

			ShowWindow(hDragnDrop, SW_SHOW);
		}
		// Old performance style
		else
		{
			GuiMain* mainWindow = new GuiMain;
			mainWindow->show();
		}

		currentExitCode = a.exec();

		CloseDragDropWindow();

		int i = 42;
	} while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);
		
	return currentExitCode;
}