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
	auto ret = ProgressWindow->Execute();
	worker.join();

	delete ProgressWindow;

	g_Console->update_external();

	QString error_msg = "";

	if (ret > 0)
	{
		error_msg = "Download/import failure. Error code: 0x";
		QString number = QStringLiteral("%1").arg(ret, 8, 0x10, QLatin1Char('0'));
		error_msg += number;
		error_msg += "\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
	}
	else if (ret < 0)
	{
		InjLib->InterruptDownload();

		switch (ret)
		{
			case PEB_ERR_INTERRUPTED:
				error_msg = "Download interrupted.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
				break;

			case PEB_ERR_CONNECTION_BLOCKED:
				error_msg = "Connection to Microsoft Symbol Server blocked.\nThis might be caused by a firewall rule.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
				break;

			case PEB_ERR_NO_INTERNET:
				error_msg = "Internet connection interrupted.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
				break;

			default:
				error_msg = "Unknown error code specified.\nThe injector cannot function without the PDB files.\nPlease restart the injector.";
		}
	}

	if (ret != PDB_ERR_SUCCESS)
	{
		g_Console->update_external();
		ShowStatusbox(false, error_msg);
	}
}

void pdb_download_update_thread(DownloadProgressWindow * ProgressWindow, InjectionLib * InjLib)
{
	while (!ProgressWindow->IsRunning())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	Sleep(100);

	bool bClosed = false;

	auto close_callback = [&]()
	{
		ProgressWindow->SetDone(PEB_ERR_INTERRUPTED);

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

		if (InternetCheckConnectionW(L"https://msdl.microsoft.com", FLAG_ICC_FORCE_CONNECTION, NULL) == FALSE)
		{
			if (GetLastError() == ERROR_INTERNET_CANNOT_CONNECT)
			{
				ProgressWindow->SetStatus("Cannot connect to Microsoft Symbol Server...");
				connected = false;
				ProgressWindow->SetDone(PEB_ERR_CONNECTION_BLOCKED);

				return;
			}
			else if (connected)
			{
				ProgressWindow->SetStatus("Waiting for internet connection...");
				connected = false;
				ProgressWindow->SetDone(PEB_ERR_NO_INTERNET);

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

	ProgressWindow->SetDone(PDB_ERR_SUCCESS);
}