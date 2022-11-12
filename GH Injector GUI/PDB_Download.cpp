#include "pch.h"

#include "PDB_Download.h"

struct CallbackData
{
	InjectionLib * pInjLib = nullptr;
	std::vector<std::pair<int, bool>> info;

	bool connected			= false;
	bool download_finished	= false;
	bool import_finished	= false;
};

void __stdcall pdb_download_update_callback(DownloadProgressWindow * hProgressWindow, void * pData);

void ShowPDBDownload(InjectionLib * InjLib)
{
	CallbackData data;
	data.pInjLib = InjLib;

	std::vector<QString> labels;

	labels.push_back("ntdll.pdb");
	data.info.push_back({ 0, false });
#ifdef _WIN64
	labels.push_back("wntdll.pdb");
	data.info.push_back({ 0, true });
#endif

	auto CurrentOS = QOperatingSystemVersion::current();
	if (CurrentOS >= QOperatingSystemVersion::Windows7 && CurrentOS < QOperatingSystemVersion::Windows8) //no == operator provided
	{
		labels.push_back("kernel32.pdb");
		data.info.push_back({ 1, false });
#ifdef _WIN64
		labels.push_back("wkernel32.pdb");
		data.info.push_back({ 1, true });
#endif
	}

	DownloadProgressWindow * ProgressWindow = new(std::nothrow) DownloadProgressWindow("PDB Download", labels, "Waiting for internet connection...", 300, Q_NULLPTR);
	if (ProgressWindow == Q_NULLPTR)
	{
		THROW("Failed to create download progress window.");
	}

	ProgressWindow->SetCallbackFrequency(25);
	ProgressWindow->SetCallbackArg(&data);
	ProgressWindow->SetCallback(pdb_download_update_callback);
	ProgressWindow->SetCloseValue(PEB_ERR_INTERRUPTED);

	ProgressWindow->show();
	auto ret = ProgressWindow->Execute();

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

		StatusBox(false, error_msg);
	}
}

void __stdcall pdb_download_update_callback(DownloadProgressWindow * hProgressWindow, void * pData)
{
	if (!hProgressWindow || !pData)
	{
		THROW("Invalid data passed to update callback\n");

		return;
	}

	auto * data = reinterpret_cast<CallbackData *>(pData);

	auto lib = data->pInjLib;

	if (!data->download_finished)
	{
		if (lib->GetSymbolState() == INJ_ERR_SYMBOL_INIT_NOT_DONE)
		{
			for (UINT i = 0; i < data->info.size(); ++i)
			{
				float progress = lib->GetDownloadProgressEx(data->info[i].first, data->info[i].second);
				hProgressWindow->SetProgress(i, progress);
			}

			if (InternetCheckConnectionW(L"https://msdl.microsoft.com", FLAG_ICC_FORCE_CONNECTION, NULL) == FALSE)
			{
				if (GetLastError() == ERROR_INTERNET_CANNOT_CONNECT)
				{
					data->connected = false;

					hProgressWindow->SetStatus("Cannot connect to Microsoft Symbol Server...");
					hProgressWindow->SetDone(PEB_ERR_CONNECTION_BLOCKED);
				}
				else if (data->connected)
				{
					data->connected = false;

					hProgressWindow->SetStatus("Waiting for internet connection...");
					hProgressWindow->SetDone(PEB_ERR_NO_INTERNET);
				}
			}
			else
			{
				if (!data->connected)
				{
					data->connected = true;

					hProgressWindow->SetStatus("Downloading PDB files from the Microsoft Symbol Server...");
				}
			}
		}
		else
		{
			auto symbol_state = lib->GetSymbolState();
			if (symbol_state != INJ_ERR_SUCCESS)
			{
				hProgressWindow->SetDone((int)symbol_state);
			}
			else
			{
				for (UINT i = 0; i < data->info.size(); ++i)
				{
					hProgressWindow->SetProgress(i, 1.0f);
				}

				hProgressWindow->SetStatus("Download finished, resolving imports...");

				data->download_finished = true;
			}	
		}
	}
	else if (!data->import_finished)
	{
		if (lib->GetImportState() != INJ_ERR_IMPORT_HANDLER_NOT_DONE)
		{
			auto import_state = lib->GetImportState();
			if (import_state != INJ_ERR_SUCCESS)
			{
				hProgressWindow->SetDone((int)import_state);

				return;
			}
			else
			{
				hProgressWindow->SetStatus("Imports resolved...");

				data->import_finished = true;
			}
		}		
	}	
	else
	{
		hProgressWindow->SetDone(PDB_ERR_SUCCESS);
	}
}