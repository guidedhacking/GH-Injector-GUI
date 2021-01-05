#include "pch.h"

#include "DarkStyle.h"
#include "framelesswindow/framelesswindow.h"
#include "GuiMain.h"
#include "CmdArg.h"
#include "InjectionLib.h"
#include "DragDropWindow.h"
#include "resource.h"
#include "DebugConsole.h"

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

		DarkStyle * dark = new DarkStyle;
		QApplication::setStyle(dark);
		QApplication::setPalette(QApplication::style()->standardPalette());

		g_Console = new DebugConsole();
		
		GuiMain * MainWindow = new GuiMain();
		MainWindow->statusBar()->setSizeGripEnabled(false);
		MainWindow->show();
		MainWindow->open_console_if();

		currentExitCode = a.exec();

		delete MainWindow;
		delete g_Console;
	} 
	while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);

	Sleep(5000);

	return currentExitCode;
}