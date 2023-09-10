/*
 * Author:       Broihon
 * Copyright:    Guided Hacking™ © 2012-2023 Guided Hacking LLC
*/

#include "pch.h"

#include "CmdArg.h"
#include "DarkStyle.h"
#include "DebugConsole.h"
#include "framelesswindow.h"
#include "GuiMain.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup")

int wmain(int argc, wchar_t * argv[])
{
#ifdef DEBUG_CONSOLE_TO_CMD
	AllocConsole();
	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
#endif

	if (argc > 1)
	{
		bool silent = false;
		auto ret = CmdArg(argc, argv, silent);

		if (silent)
		{
			return 0;
		}

		if (ret != 0)
		{
			Sleep((DWORD)-1);
		}

		Sleep(1500);
		
		return 0;
	}

	auto s_argv = WCharArrayToStdString(argv[0]);
	auto qt_argv = new char[s_argv.length() + 1]();
	s_argv.copy(qt_argv, s_argv.length());
		
	SetProcessDPIAware();
	//QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	//QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	int currentExitCode = 0;
	do
	{
		QApplication a(argc, &qt_argv);

		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

		DarkStyle * dark = new(std::nothrow) DarkStyle;
		if (dark == Q_NULLPTR)
		{
			THROW("Failed to create window style.");
		}

		QApplication::setStyle(dark);
		
		GuiMain * MainWindow = new(std::nothrow) GuiMain();
		if (MainWindow == Q_NULLPTR)
		{
			THROW("Failed to create main window.");
		}

		g_print("GH Injector V%ls\n", GH_INJ_GUI_VERSIONW.c_str());
		g_print("Initializing GUI\n");

		MainWindow->show();

		g_print("GUI initialized\n");

		MainWindow->initSetup();
		
		currentExitCode = a.exec();

		delete MainWindow;

		printf("REBOOT\n");
	} 
	while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);

	delete[] qt_argv;

	return currentExitCode;
}