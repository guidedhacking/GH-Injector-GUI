#include "pch.h"

#include "DarkStyle.h"
#include "framelesswindow/framelesswindow.h"
#include "GuiMain.h"
#include "CmdArg.h"
#include "InjectionLib.h"
#include "DragDropWindow.h"
#include "resource.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup")

int wmain(int argc, wchar_t * argv[])
{
	AllocConsole();
	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);

	if (argc > 1)
	{
		auto ret = CmdArg(argc, argv);

		if (ret != 0)
		{
			Sleep(-1);
		}

		Sleep(1500);

		return 0;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string s_argv = converter.to_bytes(argv[0]);
	char * sz_argv = const_cast<char*>(s_argv.c_str());

	// Restart Application loop
	int currentExitCode = 0;
	do
	{
		QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
		QApplication a(argc, &sz_argv);

		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

		FramelessWindow framelessWindow;

		DarkStyle * dark = new DarkStyle;
		QApplication::setStyle(dark);
		QApplication::setPalette(QApplication::style()->standardPalette());

		framelessWindow.setWindowTitle("GH Injector");
		framelessWindow.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

		GuiMain * MainWindow = new GuiMain(&framelessWindow, &framelessWindow);
		MainWindow->statusBar()->setSizeGripEnabled(false);

		framelessWindow.setContent(MainWindow);
		framelessWindow.show();

		currentExitCode = a.exec();
	} while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);

	return currentExitCode;
}