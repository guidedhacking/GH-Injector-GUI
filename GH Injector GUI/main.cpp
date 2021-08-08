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

	//QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

	// Restart Application loop
	int currentExitCode = 0;
	do
	{
		QApplication a(argc, &sz_argv);

		QApplication::setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

		DarkStyle * dark = new DarkStyle;
		QApplication::setStyle(dark);
		QApplication::setPalette(QApplication::style()->standardPalette());
		
		GuiMain * MainWindow = new GuiMain();

		g_print("GH Injector V%ls\n", GH_INJ_GUI_VERSIONW);
		g_print("Initializing GUI\n");

		MainWindow->show();

		g_print("GUI initialized\n");

		MainWindow->initSetup();

		currentExitCode = a.exec();

		delete MainWindow;
	} 
	while (currentExitCode == GuiMain::EXIT_CODE_REBOOT);

	return currentExitCode;
}