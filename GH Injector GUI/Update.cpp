#include "pch.h"

#include "Update.h"

struct CallbackData_Update
{
	InjectionLib		* pInjLib	= nullptr;
	DownloadProgress	* pProgress	= nullptr;

	bool download_started	= false;
	bool download_finished	= false;
	bool cleanup_finished	= false;
	bool unzip_started		= false;
	bool unzip_finished		= false;
	bool launch_finished	= false;

	std::wstring path;
	std::wstring zip_path;
	std::wstring download_url;

	std::shared_future<HRESULT> download_thread_ret;

	ULONGLONG timer = 0;
};

void update_update_callback(DownloadProgressWindow * hProgressWindow, void * pData);
HRESULT download_thread(CallbackData_Update * data);

std::wstring get_newest_version()
{
	DownloadProgress progress(true);

	TCHAR szCacheFile[MAX_PATH]{ 0 };
	auto hr = URLDownloadToCacheFile(nullptr, GH_VERSION_URL.c_str(), szCacheFile, sizeof(szCacheFile) / sizeof(szCacheFile[0]), 0, &progress);

	if (FAILED(hr))
	{
		g_print("Failed to resolve newest version number: %08X\n", hr);

		return L"0.0";
	}

	std::wifstream infile(szCacheFile, std::ifstream::in);

	if (!infile.good())
	{
		DeleteFile(szCacheFile);

		g_print("Failed to read newest version number\n");

		return L"0.0";
	}

	std::wstring strVer;
	infile >> strVer;

	infile.close();

	DeleteFile(szCacheFile);

	return strVer;
}

bool update_injector(const std::wstring & newest_version, bool & ignore, InjectionLib * Lib)
{
	std::wstring update_msg;

	FramelessWindow parent;
	parent.setMinimizeButton(false);
	parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = new(std::nothrow) QMessageBox(QMessageBox::Icon::Information, "", "", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, &parent, Qt::WindowType::FramelessWindowHint);
	if (box == Q_NULLPTR)
	{
		THROW("Failed to create update dialog box.");
	}

	int cmp = newest_version.compare(GH_INJ_GUI_VERSIONW);
	if (cmp > 0)
	{
		update_msg = L"This version of the GH Injector is outdated.\nThe newest version is V";
		update_msg += newest_version;
		update_msg += L".\n";
		update_msg += L"Do you want to update?";

		parent.setWindowTitle("New version available");
		box->addButton(QMessageBox::Ignore);
	}
	else if (cmp == 0)
	{
		update_msg = L"You are already using the newest version of the GH Injector.\nDo you want to redownload V";
		update_msg += GH_INJ_GUI_VERSIONW;
		update_msg += L"?";

		parent.setWindowTitle("Redownload");
	}
	else
	{
		StatusBox(false, "Something went wrong with the version check.\nYour version appears to be from the future.");
		parent.close();
		delete box;

		return false;
	}

	box->setText(QString::fromStdWString(update_msg));
	box->setDefaultButton(QMessageBox::Yes);

	parent.setContent(box);
	parent.setFixedWidth(box->width() + 40);
	parent.show();
	
	auto res = box->exec();

	parent.close();
	delete box;

	if (res == QMessageBox::No)
	{
		ignore = false;

		return false;
	}
	else if (res == QMessageBox::Ignore)
	{
		ignore = true;

		return false;
	}

	std::vector<QString> labels;
	labels.push_back("");

	DownloadProgressWindow * UpdateWnd = new(std::nothrow) DownloadProgressWindow(QString("Downloading V") + QString::fromStdWString(newest_version), labels, "Initializing...", 250, Q_NULLPTR);	
	if (UpdateWnd == Q_NULLPTR)
	{
		THROW("Failed to create update progress window.");

		return false;
	}

	auto zip_path = g_RootPath + GH_INJ_ZIPW;

	DeleteFileW(zip_path.c_str());

	std::wstring download_url = GH_DOWNLOAD_PREFIXW;
	download_url += newest_version;
	download_url += GH_DOWNLOAD_SUFFIXW;

	HANDLE hInterrupt = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	DownloadProgress progress(true);
	progress.SetInterruptEvent(hInterrupt);

	CallbackData_Update data{ };
	data.download_url	= download_url;
	data.path			= g_RootPath;
	data.zip_path		= zip_path;
	data.pInjLib		= Lib;
	data.pProgress		= &progress;

	data.download_thread_ret = std::async(std::launch::async, &download_thread, &data);

	UpdateWnd->SetCallbackFrequency(25);
	UpdateWnd->SetCallbackArg(&data);
	UpdateWnd->SetCallback(update_update_callback);
	UpdateWnd->SetCloseValue(UPDATE_ERR_INTERRUPTED);

	UpdateWnd->show();
	int ret = UpdateWnd->Execute();

	delete UpdateWnd;

	g_Console->update_external();

	QString error_msg = "";

	if (ret > 0)
	{
		error_msg = "An unknown error occured. Please check if the new version was downloaded successfully.\nError code = ";
		error_msg += QString::number(ret);
	}
	else if (ret < 0)
	{
		switch (ret)
		{
			case UPDATE_ERR_DOWNLOAD_FAIL:
			{
				HRESULT hr = data.download_thread_ret.get();
				auto s_hr = std::format("{:08X}", (DWORD)hr);

				error_msg = "URLDownloadToFileW failed with 0x";
				error_msg += QString::fromStdString(s_hr);
			}
			break;

			case UPDATE_ERR_DELETE_FAIL:
				error_msg = "Failed to delete old files. Please close the injector and unzip the new version manually.";
				break;

			case UPDATE_ERR_RENAME_FAIL:
				error_msg = "Failed to rename old files. Please close the injector and unzip the new version manually.";
				break;

			case UPDATE_ERR_UNZIP_FAIL:
				error_msg = "Failed to unzip new files. Please close the injector and unzip the new version manually.";
				break;

			case UPDATE_ERR_INCOMPLETE:
				error_msg = "Download incomplete or unzip failure. Please close the injector and unzip the new version manually.";
				break;

			case UPDATE_ERR_LAUNCH_FAIL:
				error_msg = "Failed to launch new version. Please close the injector and launch the new version manually.";
				break;

			case UPDATE_ERR_INTERRUPTED:
				error_msg = "Update interrupted.";
				if (hInterrupt)
				{
					SetEvent(hInterrupt);
				}
				break;

			default:
				error_msg = "An unknown error occured. Please check if the new version was downloaded successfully.";
		}

		if (ret == UPDATE_ERR_DELETE_FAIL || ret == UPDATE_ERR_RENAME_FAIL || ret == UPDATE_ERR_UNZIP_FAIL || ret == UPDATE_ERR_INCOMPLETE || ret == UPDATE_ERR_LAUNCH_FAIL)
		{
			std::wstring open_cmd = L"/Select,";

			if (ret == UPDATE_ERR_LAUNCH_FAIL)
			{
				open_cmd += g_RootPath;
				open_cmd += GH_INJ_EXE_LAUNCHW;
			}
			else
			{
				QString q_zip_path = QString::fromStdWString(zip_path);
				q_zip_path.replace('/', '\\');
				open_cmd += q_zip_path.toStdWString();
			}

			auto shell_ret = reinterpret_cast<INT_PTR>(ShellExecuteW(NULL, nullptr, L"explorer.exe", open_cmd.c_str(), nullptr, SW_SHOW));
			if (shell_ret <= 32)
			{
				g_print("Failed to open explorer:\n Error 1: %08X\n Error 2: %08X\n", shell_ret, GetLastError());

				error_msg = "Failed to open file location. Please open this path and launch/unzip the new version manually:\n";
				error_msg += QString::fromStdWString(open_cmd);
			}
		}
	}

	if (hInterrupt)
	{
		CloseHandle(hInterrupt);
	}

	if (ret != UPDATE_ERR_SUCCESS)
	{
		StatusBox(false, error_msg);
	
		return false;
	}

	return true;
}

void update_update_callback(DownloadProgressWindow * hProgressWindow, void * pData)
{
	if (!hProgressWindow || !pData)
	{
		THROW("Invalid data passed to update callback\n");

		return;
	}

	auto * data = reinterpret_cast<CallbackData_Update *>(pData);
	auto * progress = data->pProgress;
	
	if (!data->download_started)
	{
		if (data->download_thread_ret.valid())
		{
			data->download_started = true;
		}
	}
	else if (!data->download_finished)
	{
		if (data->download_thread_ret.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
		{
			hProgressWindow->SetProgress(0, progress->GetDownloadProgress());
			hProgressWindow->SetStatus(progress->GetStatusText().c_str());
		}
		else
		{
			auto hr = data->download_thread_ret.get();
			
			if (SUCCEEDED(hr))
			{
				hProgressWindow->SetProgress(0, 1);

				hProgressWindow->SetStatus("Removing old files...");

				data->download_finished = true;
			}
			else
			{
				std::string new_status = "URLDownloadToFileW failed with 0x";
				new_status += std::format("{:08X}", (DWORD)hr);

				hProgressWindow->SetStatus(new_status.c_str());
				hProgressWindow->SetDone(UPDATE_ERR_DOWNLOAD_FAIL);
			}
		}
	}
	else if (!data->cleanup_finished)
	{
		data->pInjLib->InterruptDownload();
		data->pInjLib->Unload();

		BOOL bRet = TRUE;

		auto old_path = data->path + L"OLD.exe";
		DeleteFileW(old_path.c_str());

		if (FileExistsW(GH_INJ_EXE_LAUNCHW))
		{
			bRet &= DeleteFile(GH_INJ_LAUNCHER_EXE.c_str());
		}

		if (FileExistsW(GH_INJ_MOD_NAME86W))
		{
			bRet &= DeleteFile(GH_INJ_MOD_NAME86.c_str());
		}

		if (FileExistsW(GH_INJ_MOD_NAME64W))
		{
			bRet &= DeleteFile(GH_INJ_MOD_NAME64.c_str());
		}

		if (FileExistsW(GH_INJ_SM_NAME86W))
		{
			bRet &= DeleteFile(GH_INJ_SM_NAME86.c_str());
		}

		if (FileExistsW(GH_INJ_SM_NAME64W))
		{
			bRet &= DeleteFile(GH_INJ_SM_NAME64.c_str());
		}

#ifdef _WIN64
		if (FileExistsW(GH_INJ_EXE_NAME86W))
		{
			bRet &= DeleteFile(GH_INJ_EXE_NAME86.c_str());
		}
#else
		if (FileExistsW(GH_INJ_EXE_NAME64W))
		{
			bRet &= DeleteFile(GH_INJ_EXE_NAME64.c_str());
		}
#endif

		if (!bRet)
		{
			std::string new_status = "DeleteFile failed with 0x%08X";
			new_status += std::format("{:08X}", GetLastError());

			hProgressWindow->SetStatus(new_status.c_str());
			hProgressWindow->SetDone(UPDATE_ERR_DELETE_FAIL);
		}
		else if (!MoveFileW(QCoreApplication::applicationFilePath().toStdWString().c_str(), old_path.c_str()))
		{
			std::string new_status = "MoveFileW failed with 0x%08X";
			new_status += std::format("{:08X}", GetLastError());

			hProgressWindow->SetStatus(new_status.c_str());
			hProgressWindow->SetDone(UPDATE_ERR_RENAME_FAIL);
		}
		else
		{
			hProgressWindow->SetStatus("Unzipping new files...");

			data->cleanup_finished = true;
		}
	}
	else if (!data->unzip_started)
	{
		if (Unzip(data->zip_path, data->path) != 0)
		{
			hProgressWindow->SetStatus("Failed to unzip files...");

			hProgressWindow->SetDone(UPDATE_ERR_UNZIP_FAIL);
		}
		else
		{
			data->unzip_started = true;
		}
	}
	else if (!data->unzip_finished)
	{
		if (!data->timer)
		{
			data->timer = GetTickCount64();
		}
		
		if (GetTickCount64() > data->timer + 1000)
		{
			hProgressWindow->SetStatus("Failed to unzip files...");

			hProgressWindow->SetDone(UPDATE_ERR_INCOMPLETE);
		}
		else
		{
			bool all_files_unzipped = true;

			all_files_unzipped &= FileExists(GH_INJ_LAUNCHER_EXE);
			all_files_unzipped &= FileExists(GH_INJ_MOD_NAME86);
			all_files_unzipped &= FileExists(GH_INJ_MOD_NAME64);
			all_files_unzipped &= FileExists(GH_INJ_SM_NAME86);
			all_files_unzipped &= FileExists(GH_INJ_SM_NAME64);
			all_files_unzipped &= FileExists(GH_INJ_EXE_NAME86);
			all_files_unzipped &= FileExists(GH_INJ_EXE_NAME64);

			data->unzip_finished = all_files_unzipped;

			if (all_files_unzipped)
			{
				DeleteFileW(data->zip_path.c_str());

				hProgressWindow->SetStatus("Launching updated version...");
			}
		}
	}
	else if (!data->launch_finished)
	{
		auto new_path = data->path + L"GH Injector.exe ";
		new_path += std::format(L"{:d}", GetCurrentProcessId());

		PROCESS_INFORMATION pi{ 0 };
		STARTUPINFOW		si{ 0 };
		si.cb = sizeof(si);

		if (CreateProcessW(nullptr, const_cast<wchar_t *>(new_path.c_str()), nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			data->launch_finished = true;
		}
		else
		{
			hProgressWindow->SetStatus("Failed to launch updated version...");

			hProgressWindow->SetDone(UPDATE_ERR_LAUNCH_FAIL);
		}
	}
	else
	{
		hProgressWindow->SetDone(UPDATE_ERR_SUCCESS);
	}
}

HRESULT download_thread(CallbackData_Update * data)
{
	return URLDownloadToFileW(nullptr, data->download_url.c_str(), data->zip_path.c_str(), NULL, data->pProgress);
}