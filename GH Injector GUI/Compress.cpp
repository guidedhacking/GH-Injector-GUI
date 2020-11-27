//https://www.codeproject.com/Articles/280650/Zip-Unzip-using-Windows-Shell

#include "compress.h"
#include <Windows.h>
#include <WinDef.h>
#include <Shellapi.h>
#include <iostream>
#include <string>
#include <ShlDisp.h>

#pragma comment(lib, "shell32.lib")
using namespace std;

Compress::Compress()
{
}

Compress::~Compress()
{
}

void Compress::zip()
{
    // Create Zip file
    BYTE startBuffer[] = { 80, 75, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    FILE* f = fopen("C:\\temp\\test.zip", "wb");
    fwrite(startBuffer, sizeof(startBuffer), 1, f);
    fclose(f);

    WCHAR source[MAX_PATH] = L"C:\\temp\\test.txt\0\0";
    WCHAR dest[MAX_PATH] = L"C:\\temp\\test.zip\\\0\0";

    HRESULT          hResult;
    IShellDispatch* pISD;
    Folder* pToFolder = NULL;
    VARIANT          vDir, vFile, vOpt;

    CoInitialize(NULL);

    hResult = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pISD);

    if (SUCCEEDED(hResult))
    {
        VariantInit(&vDir);
        vDir.vt = VT_BSTR;
        vDir.bstrVal = dest;//L"C:\\test.zip\\\0\0";
        hResult = pISD->NameSpace(vDir, &pToFolder);

        if (SUCCEEDED(hResult))
        {
            // Now copy source file(s) to the zip
            VariantInit(&vFile);
            vFile.vt = VT_BSTR;
            vFile.bstrVal = source;//L"C:\\test.txt";
            // ******NOTE**** To copy multiple files into the zip, need to create a FolderItems object (see unzip implementation below for more details)

            VariantInit(&vOpt);
            vOpt.vt = VT_I4;
            vOpt.lVal = FOF_NO_UI;//4;          // Do not display a progress dialog box, not useful in our example

            // Copying and compressing the source files to our zip
            hResult = pToFolder->CopyHere(vFile, vOpt);

            /* CopyHere() creates a separate thread to copy files and it may happen that the main thread exits
             * before the copy thread is initialized. So we put the main thread to sleep for a second to
             * give time for the copy thread to start.*/
            Sleep(1000);
            pToFolder->Release();
        }
        pISD->Release();
    }
    CoUninitialize();
}

void Compress::unzip()
{
    WCHAR source[MAX_PATH] = L"C:\\temp\\test.zip\\\0\0";
    WCHAR dest[MAX_PATH] = L"C:\\temp\\test\\\0\0"; // Currently it is assumed that the there exist Folder "Test" in C:

    HRESULT          hResult;
    IShellDispatch* pISD;
    Folder* pToFolder = NULL;
    VARIANT          vDir, vFile, vOpt;

    CoInitialize(NULL);

    hResult = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pISD);

    if (SUCCEEDED(hResult))
    {
        VariantInit(&vDir);
        vDir.vt = VT_BSTR;
        vDir.bstrVal = dest;//L"C:\\test.zip\\\0\0";
        hResult = pISD->NameSpace(vDir, &pToFolder);

        if (SUCCEEDED(hResult))
        {
            Folder* pFromFolder = NULL;

            VariantInit(&vFile);
            vFile.vt = VT_BSTR;
            vFile.bstrVal = source;//L"C:\\test.txt";
            pISD->NameSpace(vFile, &pFromFolder);
            FolderItems* fi = NULL;
            pFromFolder->Items(&fi);
            VariantInit(&vOpt);
            vOpt.vt = VT_I4;
            vOpt.lVal = FOF_NO_UI;//4; // Do not display a progress dialog box

            // Creating a new Variant with pointer to FolderItems to be copied
            VARIANT newV;
            VariantInit(&newV);
            newV.vt = VT_DISPATCH;
            newV.pdispVal = fi;
            hResult = pToFolder->CopyHere(newV, vOpt);
            Sleep(1000);
            pFromFolder->Release();
            pToFolder->Release();
        }
        pISD->Release();
    }
    CoUninitialize();

}

void Compress::unzip_GH(WCHAR* source, WCHAR* dest)
{
    HRESULT         hResult;
    IShellDispatch* pISD;
    Folder*         pToFolder = NULL;
    VARIANT         vDir, vFile, vOpt;

    CoInitialize(NULL);

    hResult = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pISD);

    if (SUCCEEDED(hResult))
    {
        VariantInit(&vDir);
        vDir.vt = VT_BSTR;
        vDir.bstrVal = dest;//L"C:\\test.zip\\\0\0";
        hResult = pISD->NameSpace(vDir, &pToFolder);

        if (SUCCEEDED(hResult))
        {
            Folder* pFromFolder = NULL;

            VariantInit(&vFile);
            vFile.vt = VT_BSTR;
            vFile.bstrVal = source;//L"C:\\test.txt";
            pISD->NameSpace(vFile, &pFromFolder);
            FolderItems* fi = NULL;
            pFromFolder->Items(&fi);
            VariantInit(&vOpt);
            vOpt.vt = VT_I4;
            vOpt.lVal = FOF_NO_UI;//4; // Do not display a progress dialog box

            // Creating a new Variant with pointer to FolderItems to be copied
            VARIANT newV;
            VariantInit(&newV);
            newV.vt = VT_DISPATCH;
            newV.pdispVal = fi;
            hResult = pToFolder->CopyHere(newV, vOpt);
            Sleep(1000);
            pFromFolder->Release();
            pToFolder->Release();
        }
        pISD->Release();
    }
    CoUninitialize();
}
