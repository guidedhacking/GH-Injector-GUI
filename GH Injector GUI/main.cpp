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

	if (argc > 1)
	{
		auto ret = CmdArg(argc, argv);

		if (ret != 0)
		{
			std::cin.get();
		}

		Sleep(1000);

		return 0;
	}
	
	// Restart Application loop
	int currentExitCode = 0;
	do {
		QApplication a(argc, argv);

		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
		
		FramelessWindow framelessWindow;
		
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
		
		currentExitCode = a.exec();

		CloseDragDropWindow();

	} while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);
		
	return currentExitCode;
}