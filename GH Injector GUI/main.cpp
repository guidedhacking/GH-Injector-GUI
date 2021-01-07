#include "pch.h"

#include "CmdArg.h"
#include "DarkStyle.h"
#include "DebugConsole.h"
#include "framelesswindow.h"
#include "GuiMain.h"

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup")

#define DEBUG_CONSOLE

int wmain(int argc, wchar_t * argv[])
{
#ifdef DEBUG_CONSOLE
	AllocConsole();
	FILE * pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
#endif

	if (argc > 1)
	{
		auto ret = CmdArg(argc, argv);

		if (ret != 0)
		{
			Sleep((DWORD)-1);
		}

		Sleep(1500);

		return 0;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	std::string s_argv = converter.to_bytes(argv[0]);
	char * sz_argv = const_cast<char*>(s_argv.c_str());

	QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

	// Restart Application loop
	int currentExitCode = 0;
	do
	{
		QApplication a(argc, &sz_argv);

		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

		DarkStyle * dark = new DarkStyle;
		QApplication::setStyle(dark);
		QApplication::setPalette(QApplication::style()->standardPalette());

		g_Console = new DebugConsole();

		g_print("GH Injector V%ls\n", GH_INJ_GUI_VERSIONW);
		g_print("Initializing GUI\n");
		
		GuiMain * MainWindow = new GuiMain();
		MainWindow->statusBar()->setSizeGripEnabled(false);
		MainWindow->show();
		MainWindow->open_console_if();

		g_print("GUI initialized\n");

		currentExitCode = a.exec();

		delete MainWindow;
		delete g_Console;
	} 
	while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);

	return currentExitCode;
}