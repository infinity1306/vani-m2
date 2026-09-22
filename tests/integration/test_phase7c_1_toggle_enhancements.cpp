#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <propkey.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    CoInitialize(nullptr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
        std::cerr << "Failed CoCreateInstance\n";
        CoUninitialize();
        return 1;
    }

    IMMDevice* pDev = nullptr;
    if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDev)) || !pDev) {
        std::cerr << "Failed GetDefaultAudioEndpoint\n";
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    PROPERTYKEY PKEY_Disable_SysFx = { {0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}}, 5 };

    // Read current state
    IPropertyStore* pReadProps = nullptr;
    if (SUCCEEDED(pDev->OpenPropertyStore(STGM_READ, &pReadProps)) && pReadProps) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        pReadProps->GetValue(PKEY_Disable_SysFx, &pv);
        std::cout << "CURRENT Audio Enhancements (Disable_SysFx): ";
        if (pv.vt == VT_UI4) {
            std::cout << (pv.ulVal == 1 ? "1 (ENHANCEMENTS DISABLED / OFF)" : "0 (ENHANCEMENTS ENABLED / ON)") << "\n";
        } else if (pv.vt == VT_EMPTY) {
            std::cout << "NOT SET (Default = ENHANCEMENTS ENABLED / ON)\n";
        } else {
            std::cout << "Type=" << pv.vt << "\n";
        }
        PropVariantClear(&pv);
        pReadProps->Release();
    }

    if (argc > 1) {
        std::string cmd = argv[1];
        if (cmd == "disable" || cmd == "off" || cmd == "1") {
            std::cout << "Attempting to DISABLE Audio Enhancements (set Disable_SysFx = 1)...\n";
            IPropertyStore* pWriteProps = nullptr;
            HRESULT hr = pDev->OpenPropertyStore(STGM_READWRITE, &pWriteProps);
            if (SUCCEEDED(hr) && pWriteProps) {
                PROPVARIANT pv;
                PropVariantInit(&pv);
                pv.vt = VT_UI4;
                pv.ulVal = 1;
                hr = pWriteProps->SetValue(PKEY_Disable_SysFx, pv);
                if (SUCCEEDED(hr)) {
                    hr = pWriteProps->Commit();
                    if (SUCCEEDED(hr)) {
                        std::cout << "SUCCESS: Audio Enhancements DISABLED (Disable_SysFx = 1 committed).\n";
                    } else {
                        std::cout << "Commit failed: 0x" << std::hex << hr << " (Requires Admin?)\n";
                    }
                } else {
                    std::cout << "SetValue failed: 0x" << std::hex << hr << "\n";
                }
                PropVariantClear(&pv);
                pWriteProps->Release();
            } else {
                std::cout << "OpenPropertyStore(STGM_READWRITE) failed: 0x" << std::hex << hr << " (Requires Admin)\n";
            }
        } else if (cmd == "enable" || cmd == "on" || cmd == "0") {
            std::cout << "Attempting to ENABLE Audio Enhancements (set Disable_SysFx = 0)...\n";
            IPropertyStore* pWriteProps = nullptr;
            HRESULT hr = pDev->OpenPropertyStore(STGM_READWRITE, &pWriteProps);
            if (SUCCEEDED(hr) && pWriteProps) {
                PROPVARIANT pv;
                PropVariantInit(&pv);
                pv.vt = VT_UI4;
                pv.ulVal = 0;
                hr = pWriteProps->SetValue(PKEY_Disable_SysFx, pv);
                if (SUCCEEDED(hr)) {
                    hr = pWriteProps->Commit();
                    if (SUCCEEDED(hr)) {
                        std::cout << "SUCCESS: Audio Enhancements ENABLED (Disable_SysFx = 0 committed).\n";
                    } else {
                        std::cout << "Commit failed: 0x" << std::hex << hr << "\n";
                    }
                } else {
                    std::cout << "SetValue failed: 0x" << std::hex << hr << "\n";
                }
                PropVariantClear(&pv);
                pWriteProps->Release();
            } else {
                std::cout << "OpenPropertyStore(STGM_READWRITE) failed: 0x" << std::hex << hr << "\n";
            }
        }
    }

    pDev->Release();
    pEnumerator->Release();
    CoUninitialize();
    return 0;
}
