#include "pch.h"

#include "DownloadProgress.h"

DownloadProgress::~DownloadProgress()
{
    if (m_hInterruptEvent)
    {
        CloseHandle(m_hInterruptEvent);
    }
}

DownloadProgress::DownloadProgress(bool redownload)
{
    m_fProgress         = 0.0f;
    m_sStatus           = "";
    m_bRedownload       = redownload;
    m_hInterruptEvent   = nullptr;
}

HRESULT __stdcall DownloadProgress::QueryInterface(const IID & riid, void ** ppvObject)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(ppvObject);

    return E_NOINTERFACE;
}

ULONG __stdcall DownloadProgress::AddRef(void)
{
    return 1;
}

ULONG __stdcall DownloadProgress::Release(void)
{
    return 1;
}

HRESULT __stdcall DownloadProgress::OnStartBinding(DWORD dwReserved, IBinding * pib)
{
    UNREFERENCED_PARAMETER(dwReserved);
    UNREFERENCED_PARAMETER(pib);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::GetPriority(LONG * pnPriority)
{
    UNREFERENCED_PARAMETER(pnPriority);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnLowResource(DWORD reserved)
{
    UNREFERENCED_PARAMETER(reserved);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    UNREFERENCED_PARAMETER(hresult);
    UNREFERENCED_PARAMETER(szError);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::GetBindInfo(DWORD * grfBINDF, BINDINFO * pbindinfo)
{
    UNREFERENCED_PARAMETER(grfBINDF);
    UNREFERENCED_PARAMETER(pbindinfo);

    if (m_bRedownload)
    {
        if (grfBINDF)
        {
            *grfBINDF = BINDF_GETNEWESTVERSION | BINDF_NEEDFILE;
        }

        if (pbindinfo)
        {
            pbindinfo->dwOptions        = BINDINFO_OPTIONS_WININETFLAG;
            pbindinfo->dwOptionsFlags   = INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_RELOAD;
        }
    }    

    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{
    UNREFERENCED_PARAMETER(grfBSCF);
    UNREFERENCED_PARAMETER(dwSize);
    UNREFERENCED_PARAMETER(pformatetc);
    UNREFERENCED_PARAMETER(pstgmed);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnObjectAvailable(const IID & riid, IUnknown * punk)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(punk);

    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    UNREFERENCED_PARAMETER(szStatusText);

    if (m_hInterruptEvent && WaitForSingleObject(m_hInterruptEvent, 0) == WAIT_OBJECT_0)
    {
        return E_ABORT;
    }

	BINDSTATUS status = (BINDSTATUS)ulStatusCode;

	if (ulProgressMax)
	{
		m_fProgress = (float)ulProgress / ulProgressMax;
	}

	switch (status)
	{
		case BINDSTATUS::BINDSTATUS_CONNECTING:
			m_sStatus = "Connecting to server...";
			break;

		case BINDSTATUS::BINDSTATUS_BEGINDOWNLOADDATA:
			m_sStatus = "Beginning download...";
			break;

		case BINDSTATUS::BINDSTATUS_DOWNLOADINGDATA:
			m_sStatus = "Downloading...";
			break;

		case BINDSTATUS::BINDSTATUS_ENDDOWNLOADDATA:
			m_sStatus = "Download finished";
			break;
	}

	return S_OK;
}

float DownloadProgress::GetDownloadProgress()
{
	return m_fProgress;
}

std::string DownloadProgress::GetStatusText()
{
	return m_sStatus;
}

BOOL DownloadProgress::SetInterruptEvent(HANDLE hInterrupt)
{
    if (m_hInterruptEvent)
    {
        CloseHandle(m_hInterruptEvent);
    }

    return DuplicateHandle(GetCurrentProcess(), hInterrupt, GetCurrentProcess(), &m_hInterruptEvent, NULL, FALSE, DUPLICATE_SAME_ACCESS);
}