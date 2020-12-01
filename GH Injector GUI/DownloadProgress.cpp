#include "DownloadProgress.h"

HRESULT __stdcall DownloadProgress::QueryInterface(const IID & riid, void ** ppvObject)
{
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
    return S_OK;
}

HRESULT __stdcall DownloadProgress::GetPriority(LONG *pnPriority)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnLowResource(DWORD reserved)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM * pstgmed)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnObjectAvailable(const IID & riid, IUnknown * punk)
{
    return S_OK;
}

HRESULT __stdcall DownloadProgress::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BINDSTATUS status = (BINDSTATUS)ulStatusCode;

    switch (status)
    {
        case BINDSTATUS::BINDSTATUS_CONNECTING:
            printf("Connecting to server\n");
            break;

        case BINDSTATUS::BINDSTATUS_BEGINDOWNLOADDATA:
        case BINDSTATUS::BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS::BINDSTATUS_ENDDOWNLOADDATA:
        {
            if (status == BINDSTATUS::BINDSTATUS_BEGINDOWNLOADDATA)
            {
                printf("Beginning download\n");
            }

            float percentage        = (float)ulProgress / ulProgressMax;
            float mibs_downloaded   = (float)ulProgress / 1024 / 1024;
            float mibs_max          = (float)ulProgressMax / 1024 / 1024;

            if (status == BINDSTATUS_ENDDOWNLOADDATA)
            {
                percentage = 1;
                mibs_downloaded = mibs_max;
            }

            int char_count = 50;

            printf("[");

            for (int i = 0; i < char_count; ++i)
            {
                if (i > char_count * percentage)
                {
                    printf(".");
                }
                else
                {
                    printf("#");
                }
            }

            printf("] %.2f%% (%.2f/%.2f MiB) %\r", (double)percentage * 100, mibs_downloaded, mibs_max);

            std::cout.flush();

            if (status == BINDSTATUS_ENDDOWNLOADDATA)
            {
                printf("\nDownload finished\n");
            }
        }
        break;
    }

    return S_OK;
}