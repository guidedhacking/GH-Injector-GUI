#include "pch.h"

#include "DownloadProgress.h"

DownloadProgress::DownloadProgress()
{
	m_fProgress     = 0.0f;
	m_sStatus       = "";
    m_sUrl          = L"";
    m_bRedownload   = false;
}

DownloadProgress::DownloadProgress(std::wstring url, bool redownload)
{
    m_fProgress     = 0.0f;
    m_sStatus       = "";
    m_sUrl          = url;
    m_bRedownload   = redownload;

    if (m_bRedownload)
    {
        //yes, this is necessary because DeleteUrlCacheEntryW is shit
        auto str_pos = m_sUrl.find('\x20', 0);
        while (str_pos != std::string::npos)
        {
            m_sUrl.replace(str_pos, 1, L"%20");
            str_pos += 3;

            str_pos = m_sUrl.find('\x20', str_pos);
        }
    }
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
        //yes, this is necessary because UrlDownloadToFile keeps ignoring literally every flag I pass to it via grfBINDF
        DeleteUrlCacheEntryW(m_sUrl.c_str());
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