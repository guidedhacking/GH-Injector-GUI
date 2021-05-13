#include "pch.h"

#include "Update.h"

void update_download_progress(DownloadProgressWindow * UpdateWnd, DownloadProgress * progress, HANDLE hInterrupt);
void update_update_thread(DownloadProgressWindow * UpdateWnd, std::wstring version, InjectionLib * Lib);

std::wstring get_newest_version()
{
	DownloadProgress progress(GH_VERSION_URLW, true);

	wchar_t szCacheFile[MAX_PATH]{ 0 };
	HRESULT hRes = URLDownloadToCacheFileW(nullptr, GH_VERSION_URLW, szCacheFile, sizeof(szCacheFile) / sizeof(szCacheFile[0]), 0, &progress);

	if (hRes != S_OK)
	{
		return L"0.0";
	}

	std::wifstream infile(szCacheFile, std::ifstream::in);

	if (!infile.good())
	{
		DeleteFileW(szCacheFile);

		return L"0.0";
	}

	std::wstring strVer;
	infile >> strVer;

	infile.close();

	DeleteFileW(szCacheFile);

	return strVer;
}

bool update_injector(std::wstring newest_version, bool & ignore, InjectionLib * Lib)
{
	std::wstring update_msg;

	FramelessWindow parent;
	parent.setMinimizeButton(false);
	parent.setDockButton(false);
	parent.setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));

	QMessageBox * box = new QMessageBox(QMessageBox::Icon::Information, "", "", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, &parent, Qt::WindowType::FramelessWindowHint);

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

	DownloadProgressWindow * UpdateWnd = new DownloadProgressWindow(QString("Downloading V") + QString::fromStdWString(newest_version), labels, "Initializing...", 250, Q_NULLPTR);	

	auto worker = std::thread(update_update_thread, UpdateWnd, newest_version, Lib);

	UpdateWnd->show();
	auto ret = UpdateWnd->exec();
	auto err = UpdateWnd->GetStatus();
	
	worker.join();

	delete UpdateWnd;

	g_Console->update_external();

	if (ret != 0)
	{
		ShowStatusbox(false, err);
	}

	return (ret == 0);
}

void update_update_thread(DownloadProgressWindow * UpdateWnd, std::wstring version, InjectionLib * Lib)
{
	auto path = QCoreApplication::applicationDirPath().toStdWString();
	path += L"/";

	auto zip_path = path;
	zip_path += GH_INJ_ZIPW;

	DeleteFileW(zip_path.c_str());

	std::wstring download_url = GH_DOWNLOAD_PREFIXW;
	download_url += version;
	download_url += GH_DOWNLOAD_SUFFIXW;

	HANDLE hInterrupt = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	DownloadProgress progress(download_url, true);
	progress.SetInterruptEvent(hInterrupt);

	auto worker = std::thread(update_download_progress, UpdateWnd, &progress, hInterrupt);

	auto close_callback = [&]()
	{
		SetEvent(hInterrupt);
		UpdateWnd->SetDone(-1);
	};

	UpdateWnd->SetCloseCallback(close_callback);

	HRESULT hr = URLDownloadToFileW(nullptr, download_url.c_str(), zip_path.c_str(), NULL, &progress);
	if (FAILED(hr))
	{
		SetEvent(hInterrupt);
		worker.join();

		CloseHandle(hInterrupt);

		std::stringstream stream;
		stream << std::hex << hr;
		std::string new_status = "URLDownloadToFileW failed with 0x";
		new_status += stream.str();
		UpdateWnd->SetStatus(new_status.c_str());
		UpdateWnd->SetDone(-1);

		return;
	}

	worker.join();

	CloseHandle(hInterrupt);

	Lib->InterruptDownload();
	Lib->Unload();

	auto old_path = path + L"OLD.exe";
	DeleteFileW(old_path.c_str());

	if (!MoveFileW(QCoreApplication::applicationFilePath().toStdWString().c_str(), old_path.c_str()))
	{
		std::stringstream stream;
		stream << std::hex << GetLastError();
		std::string new_status = "MoveFileA failed with 0x%08X";
		new_status += stream.str();
		UpdateWnd->SetStatus(new_status.c_str());
		UpdateWnd->SetDone(-1);

		return;
	}

	UpdateWnd->SetStatus("Removing old files...");

	DeleteFile(GH_INJ_MOD_NAME86);
	DeleteFile(GH_INJ_MOD_NAME64);
	DeleteFile(GH_INJ_SM_NAME86);
	DeleteFile(GH_INJ_SM_NAME64);

#ifdef _WIN64
	DeleteFile(GH_INJ_EXE_NAME86);
#else
	DeleteFile(GH_INJ_EXE_NAME64);
#endif

	UpdateWnd->SetStatus("Unzip new files...");

	if (Unzip(zip_path.c_str(), path.c_str()) != 0)
	{
		std::string new_status = "Failed to unzip files";
		UpdateWnd->SetStatus(new_status.c_str());
		UpdateWnd->SetDone(-1);

		return;
	}

	DeleteFileW(zip_path.c_str());

	UpdateWnd->SetStatus("Launching updated version...");

	auto new_path = path + L"GH Injector.exe ";

	STARTUPINFOW si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	std::wstringstream stream;
	stream << GetCurrentProcessId();
	new_path += stream.str();

	CreateProcessW(nullptr, const_cast<wchar_t*>(new_path.c_str()), nullptr, nullptr, FALSE, NULL, nullptr, nullptr, &si, &pi);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	Sleep(1000);

	UpdateWnd->SetDone(0);
}

void update_download_progress(DownloadProgressWindow * UpdateWnd, DownloadProgress * progress, HANDLE hInterrupt)
{
	while (progress->GetDownloadProgress() < 1.0f)
	{
		UpdateWnd->SetProgress(0, progress->GetDownloadProgress());
		UpdateWnd->SetStatus(progress->GetStatusText().c_str());

		if (WaitForSingleObject(hInterrupt, 0) == WAIT_OBJECT_0)
		{
			return;
		}

		Sleep(25);
	}

	UpdateWnd->SetProgress(0, 1);
}