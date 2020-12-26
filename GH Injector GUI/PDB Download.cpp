#include "pch.h"

#include "PDB Download.h"

void pdb_download_update_thread(DownloadProgressWindow * hProgressWindow, InjectionLib * InjLib);

void ShowPDBDownload(InjectionLib * InjLib)
{
	std::vector<QString> labels;
	labels.push_back("ntdll.pdb");
#ifdef _WIN64
	labels.push_back("wntdll.pdb");
#endif

	FramelessWindow * framelessUpdate = new FramelessWindow();

	DownloadProgressWindow * ProgressWindow = new DownloadProgressWindow("PDB Download", labels, "Waiting for connection...", 300, framelessUpdate, framelessUpdate);
	framelessUpdate->setContent(ProgressWindow);
	framelessUpdate->resize(QSize(300, 170));
	framelessUpdate->setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessUpdate->show();

	auto worker = std::thread(&pdb_download_update_thread, ProgressWindow, InjLib);
	if (ProgressWindow->exec() == -1)
	{
		//injec_status(false, "Download interrupted. The PDB files are necessary\nfor the injector to work.\nMake sure your PC is connected to the internet.");
	}

	worker.join();

	delete ProgressWindow;

	framelessUpdate->hide();
	
	delete framelessUpdate;
}

void pdb_download_update_thread(DownloadProgressWindow * ProgressWindow, InjectionLib * InjLib)
{
	bool connected = false;

	while (!InjLib->SymbolStatus())
	{
		float progress_0 = InjLib->DownloadProgress(false);
		ProgressWindow->SetProgress(0, progress_0);

#ifdef _WIN64
		float progress_1 = InjLib->DownloadProgress(true);
		ProgressWindow->SetProgress(1, progress_1);
#endif

		if (InternetCheckConnectionA("https://msdl.microsoft.com", FLAG_ICC_FORCE_CONNECTION, NULL) == FALSE)
		{
			if (connected)
			{
				ProgressWindow->SetStatus("Waiting for internet connection...");
				connected = false;
				ProgressWindow->done(-1);
				return;
			}
		}
		else
		{
			if (!connected)
			{
				ProgressWindow->SetStatus("Downloading PDB files from the Microsoft Symbol Server...");
				connected = true;
			}
		}

		Sleep(25);
	};

	ProgressWindow->done(0);
}