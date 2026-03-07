#include <windows.h>
#include <initguid.h>
#include <dshow.h>
#include <vector>
#include <string>

#ifdef _MSC_VER
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#endif

std::vector<std::string> GetWindowsWebcamNames() {
    std::vector<std::string> deviceNames;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;

    if (SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum)))) {
        if (pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0) == S_OK) {
            IMoniker* pMoniker = NULL;
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag* pPropBag;
                if (SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag)))) {
                    VARIANT var;
                    VariantInit(&var);
                    if (SUCCEEDED(pPropBag->Read(L"FriendlyName", &var, 0))) {
                        // Convert BSTR (Wide string) to std::string for libVLC
                        int size = WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, NULL, 0, NULL, NULL);
                        std::string name(size, 0);
                        WideCharToMultiByte(CP_UTF8, 0, var.bstrVal, -1, &name[0], size, NULL, NULL);

                        // Remove the null terminator added by WideCharToMultiByte
                        if (!name.empty() && name.back() == '\0') name.pop_back();

                        deviceNames.push_back(name);
                        VariantClear(&var);
                    }
                    pPropBag->Release();
                }
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    CoUninitialize();
    return deviceNames;
}
