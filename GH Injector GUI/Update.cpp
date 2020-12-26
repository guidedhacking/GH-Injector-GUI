#include "pch.h"

#include "Update.h"
#include "Zip.h"

void update_download_progress(DownloadProgressWindow * UpdateWnd, DownloadProgress * progress, HANDLE hInterrupt);
void update_update_thread(DownloadProgressWindow * UpdateWnd, std::wstring version);

std::wstring get_newest_version()
{
	DownloadProgress progress(GH_VERSION_URL, true);

	wchar_t cacheFile[MAX_PATH]{ 0 };
	HRESULT hRes = URLDownloadToCacheFileW(nullptr, GH_VERSION_URL, cacheFile, sizeof(cacheFile) / sizeof(wchar_t), 0, &progress);

	if (hRes != S_OK)
	{
		return L"0.0";
	}

	std::wifstream infile(cacheFile, std::ifstream::in);

	if (!infile.good())
	{
		return L"0.0";
	}

	std::wstring strVer;
	infile >> strVer;

	infile.close();

	return strVer;
}

bool update_injector(std::wstring newest_version, bool & ignore)
{
	std::wstring update_msg;

	QMessageBox box;

	int cmp = newest_version.compare(GH_INJ_VERSIONW);
	if (cmp > 0)
	{
		update_msg = L"This version of the GH Injector is outdated.\nThe newest version is V";
		update_msg += newest_version;
		update_msg += L".\n";
		update_msg += L"Do you want to update?";

		box.setWindowTitle("New version available");
		box.addButton(QMessageBox::Ignore);
	}
	else
	{
		update_msg = L"You are already using the newest version of the GH Injector.\nDo you want to redownload V";
		update_msg += GH_INJ_VERSIONW;
		update_msg += L"?";

		box.setWindowTitle("Redownload");
	}

	box.setText(QString::fromStdWString(update_msg));
	box.addButton(QMessageBox::Yes);
	box.addButton(QMessageBox::No);
	box.setDefaultButton(QMessageBox::Yes);
	box.setIcon(QMessageBox::Icon::Information);

	auto res = box.exec();

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

	FramelessWindow * framelessUpdate = new FramelessWindow();

	DownloadProgressWindow * UpdateWnd = new DownloadProgressWindow(QString("Downloading V") + QString::fromStdWString(newest_version), labels, "Initializing...", 250, framelessUpdate, framelessUpdate);
	framelessUpdate->setContent(UpdateWnd);
	framelessUpdate->setFixedSize(QSize(300, 120));
	framelessUpdate->setWindowIcon(QIcon(":/GuiMain/gh_resource/GH Icon.ico"));
	framelessUpdate->setWindowModality(Qt::WindowModality::ApplicationModal);
	framelessUpdate->show();

	auto worker = std::thread(update_update_thread, UpdateWnd, newest_version);
	auto ret = UpdateWnd->exec();
	if (ret == -1)
	{
		//injec_status(false, wnd->GetStatus());
	}

	worker.join();

	delete UpdateWnd;

	framelessUpdate->hide();

	delete framelessUpdate;

	return (ret != -1);
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

void update_update_thread(DownloadProgressWindow * UpdateWnd, std::wstring version)
{
	auto path = QCoreApplication::applicationDirPath().toStdWString();
	path += L"/";

	auto zip_path = path + GH_INJECTOR_ZIP;

	DeleteFileW(zip_path.c_str());
	std::wstring download_url = GH_DOWNLOAD_PREFIX;
	download_url += version;
	download_url += GH_DOWNLOAD_SUFFIX;

	HANDLE hInterrupt = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	DownloadProgress progress(download_url, true);
	auto worker = std::thread(update_download_progress, UpdateWnd, &progress, hInterrupt);

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
		UpdateWnd->done(-1);

		return;
	}

	worker.join();

	CloseHandle(hInterrupt);

	HINSTANCE hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	while (hMod)
	{
		FreeLibrary(hMod);
		hMod = GetModuleHandle(GH_INJ_MOD_NAME);
	}

	UpdateWnd->SetStatus("Injection module unlaoded");

	auto old_path = path + L"OLD.exe";
	DeleteFileW(old_path.c_str());

	if (!MoveFileW(QCoreApplication::applicationFilePath().toStdWString().c_str(), old_path.c_str()))
	{
		std::stringstream stream;
		stream << std::hex << GetLastError();
		std::string new_status = "MoveFileA failed with 0x%08X";
		new_status += stream.str();
		UpdateWnd->SetStatus(new_status.c_str());
		UpdateWnd->done(-1);

		return;
	}

	UpdateWnd->SetStatus("Removing old files...");

	DeleteFileW(GH_INJ_MOD_NAME86W);
	DeleteFileW(GH_INJ_MOD_NAME64W);
	DeleteFileA(GH_INJECTOR_SM_X86);
	DeleteFileA(GH_INJECTOR_SM_X64);

#ifdef _WIN64
	DeleteFileA(GH_INJECTOR_EXE_X86);
#else
	DeleteFileA(GH_INJECTOR_EXE_X64);
#endif

	UpdateWnd->SetStatus("Unzip new files...");

	if (Unzip(zip_path.c_str(), path.c_str()) != 0)
	{
		std::string new_status = "Failed to unzip files";
		UpdateWnd->SetStatus(new_status.c_str());
		UpdateWnd->done(-1);

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

	UpdateWnd->done(0);
}