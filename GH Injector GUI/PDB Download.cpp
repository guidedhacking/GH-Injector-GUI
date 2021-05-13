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

	DownloadProgressWindow * ProgressWindow = new DownloadProgressWindow("PDB Download", labels, "Waiting for connection...", 300, Q_NULLPTR);

	auto worker = std::thread(&pdb_download_update_thread, ProgressWindow, InjLib);

	ProgressWindow->show();
	auto ret = ProgressWindow->exec();

	worker.join();

	delete ProgressWindow;

	g_Console->update_external();

	if (ret == -2)
	{
		InjLib->InterruptDownload();

		g_Console->update_external();

		QString error_msg = "Download interrupted.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
		ShowStatusbox(false, error_msg);
	}
	else if (ret == -1)
	{
		InjLib->InterruptDownload();

		g_Console->update_external();

		QString error_msg = "Internet connection interrupted.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
		ShowStatusbox(false, error_msg);
	}
	else if (ret != 0)
	{
		QString error_msg = "Download/import failure. Error code: 0x";
		QString number = QStringLiteral("%1").arg(ret, 8, 0x10, QLatin1Char('0'));
		error_msg += number;
		error_msg += "\nThe injector cannot function without the PDB files.\nPlease restart the injector.";

		ShowStatusbox(false, error_msg);
	}
}

void pdb_download_update_thread(DownloadProgressWindow * ProgressWindow, InjectionLib * InjLib)
{
	bool bClosed = false;

	auto close_callback = [&]()
	{
		ProgressWindow->SetDone(-2);

		bClosed = true;
	};

	ProgressWindow->SetCloseCallback(close_callback);

	ProgressWindow->SetStatus("Waiting for internet connection...");

	bool connected = false;

	while (InjLib->GetSymbolState() == INJ_ERR_SYMBOL_INIT_NOT_DONE)
	{
		float progress_0 = InjLib->GetDownloadProgress(false);
		ProgressWindow->SetProgress(0, progress_0);

#ifdef _WIN64
		float progress_1 = InjLib->GetDownloadProgress(true);
		ProgressWindow->SetProgress(1, progress_1);
#endif

		if (InternetCheckConnectionA("https://msdl.microsoft.com", FLAG_ICC_FORCE_CONNECTION, NULL) == FALSE)
		{
			if (connected)
			{
				ProgressWindow->SetStatus("Waiting for internet connection...");
				connected = false;
				ProgressWindow->SetDone(-1);

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

		if (bClosed)
		{
			return;
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	};

	auto symbol_state = InjLib->GetSymbolState();
	if (symbol_state != INJ_ERR_SUCCESS)
	{	
		ProgressWindow->SetDone((int)symbol_state);

		return;
	}

	ProgressWindow->SetProgress(0, 1);

#ifdef _WIN64
	ProgressWindow->SetProgress(1, 1);
#endif

	ProgressWindow->SetStatus("Download finished, resolving imports...");

	while (InjLib->GetImportState() == INJ_ERR_IMPORT_HANDLER_NOT_DONE)
	{
		if (bClosed)
		{
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	auto import_state = InjLib->GetImportState();
	if (import_state != INJ_ERR_SUCCESS)
	{		
		ProgressWindow->SetDone((int)import_state);

		return;
	}

	ProgressWindow->SetDone(0);
}