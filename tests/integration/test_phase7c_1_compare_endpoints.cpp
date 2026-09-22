#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

static std::string guid_to_string(const GUID& guid) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

static std::string propvariant_to_string(const PROPVARIANT& pv) {
    switch (pv.vt) {
        case VT_LPWSTR: {
            if (!pv.pwszVal) return "(null)";
            int sz = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, NULL, 0, NULL, NULL);
            std::string str(sz, 0);
            WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, &str[0], sz, NULL, NULL);
            if (!str.empty() && str.back() == '\0') str.pop_back();
            return str;
        }
        case VT_UI4: return std::to_string(pv.ulVal);
        case VT_I4: return std::to_string(pv.lVal);
        case VT_BOOL: return (pv.boolVal == VARIANT_TRUE) ? "TRUE" : "FALSE";
        case VT_CLSID: return guid_to_string(*pv.puuid);
        case VT_BLOB: return "BLOB (" + std::to_string(pv.blob.cbSize) + " bytes)";
        default: return "Type=" + std::to_string(pv.vt);
    }
}

void inspect_device(IMMDevice* pDev, const std::string& default_id) {
    LPWSTR pId = nullptr;
    pDev->GetId(&pId);
    std::wstring wid = pId ? pId : L"";
    std::string id_str = "";
    if (pId) {
        int sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
        id_str.resize(sz);
        WideCharToMultiByte(CP_UTF8, 0, pId, -1, &id_str[0], sz, NULL, NULL);
        if (!id_str.empty() && id_str.back() == '\0') id_str.pop_back();
        CoTaskMemFree(pId);
    }

    IPropertyStore* pProps = nullptr;
    pDev->OpenPropertyStore(STGM_READ, &pProps);
    std::string friendly_name = "(Unknown)";
    std::string dev_desc = "(Unknown)";
    if (pProps) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &pv))) friendly_name = propvariant_to_string(pv);
        PropVariantClear(&pv);
        if (SUCCEEDED(pProps->GetValue(PKEY_Device_DeviceDesc, &pv))) dev_desc = propvariant_to_string(pv);
        PropVariantClear(&pv);
    }

    DWORD dwState = 0;
    pDev->GetState(&dwState);

    bool is_default = (id_str == default_id);

    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "ENDPOINT: " << friendly_name << "\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << "  Endpoint ID:          " << id_str << "\n";
    std::cout << "  Device Description:   " << dev_desc << "\n";
    std::cout << "  Is Default Endpoint:  " << (is_default ? "YES (CURRENT DEFAULT)" : "NO") << "\n";
    std::cout << "  Device State:         0x" << std::hex << dwState << std::dec;
    if (dwState & DEVICE_STATE_ACTIVE) std::cout << " [ACTIVE]";
    if (dwState & DEVICE_STATE_DISABLED) std::cout << " [DISABLED]";
    if (dwState & DEVICE_STATE_UNPLUGGED) std::cout << " [UNPLUGGED]";
    std::cout << "\n";

    // Volume info
    IAudioEndpointVolume* pVol = nullptr;
    if (SUCCEEDED(pDev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVol)) && pVol) {
        BOOL mute = FALSE;
        pVol->GetMute(&mute);
        float masterVol = 0.0f;
        pVol->GetMasterVolumeLevelScalar(&masterVol);
        float masterDb = 0.0f;
        pVol->GetMasterVolumeLevel(&masterDb);
        float minDb = 0.0f, maxDb = 0.0f, stepDb = 0.0f;
        pVol->GetVolumeRange(&minDb, &maxDb, &stepDb);
        UINT channels = 0;
        pVol->GetChannelCount(&channels);

        std::cout << "  Mute Status:          " << (mute ? "MUTED" : "UNMUTED (OK)") << "\n";
        std::cout << "  Master Volume:        " << std::fixed << std::setprecision(1) << (masterVol * 100.0f) << "% ("
                  << std::setprecision(2) << masterDb << " dB)\n";
        std::cout << "  Hardware Range (dB):  Min: " << minDb << " dB, Max: " << maxDb << " dB, Step: " << stepDb << " dB\n";
        std::cout << "  Volume Channels:      " << channels << "\n";
        for (UINT c = 0; c < channels; ++c) {
            float cVol = 0.0f, cDb = 0.0f;
            pVol->GetChannelVolumeLevelScalar(c, &cVol);
            pVol->GetChannelVolumeLevel(c, &cDb);
            std::cout << "    Ch " << c << " Volume:        " << (cVol * 100.0f) << "% (" << cDb << " dB)\n";
        }
        pVol->Release();
    } else {
        std::cout << "  IAudioEndpointVolume: Activation Failed\n";
    }

    // AudioClient info & Can it be opened?
    IAudioClient* pClient = nullptr;
    if (SUCCEEDED(pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient)) && pClient) {
        WAVEFORMATEX* pMix = nullptr;
        if (SUCCEEDED(pClient->GetMixFormat(&pMix)) && pMix) {
            std::cout << "  Mix Format:           " << pMix->nSamplesPerSec << " Hz, "
                      << pMix->nChannels << " Channels, " << pMix->wBitsPerSample << " bits\n";
            std::cout << "  Format Tag:           0x" << std::hex << pMix->wFormatTag << std::dec << "\n";
            if (pMix->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                auto* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMix);
                std::cout << "  Extensible Subtype:   " << guid_to_string(pExt->SubFormat);
                if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) std::cout << " (IEEE_FLOAT)\n";
                else if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) std::cout << " (PCM)\n";
                else std::cout << "\n";
                std::cout << "  Channel Mask:         0x" << std::hex << pExt->dwChannelMask << std::dec << "\n";
            }

            // Test opening in Shared Mode
            HRESULT hrInit = pClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                0,
                10000000,
                0,
                pMix,
                nullptr
            );
            std::cout << "  Can Be Opened (Init): " << (SUCCEEDED(hrInit) ? "YES (SUCCESS)" : "NO (Failed: 0x" + std::to_string(hrInit) + ")") << "\n";

            CoTaskMemFree(pMix);
        }
        pClient->Release();
    } else {
        std::cout << "  IAudioClient: Activation Failed\n";
    }

    // Inspect Enhancement property
    if (pProps) {
        PROPERTYKEY PKEY_Disable_SysFx = { {0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}}, 5 };
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(pProps->GetValue(PKEY_Disable_SysFx, &pv))) {
            std::cout << "  Disable SysFx (APO):  " << propvariant_to_string(pv) << "\n";
        } else {
            std::cout << "  Disable SysFx (APO):  (Not explicitly configured / Default)\n";
        }
        PropVariantClear(&pv);
        pProps->Release();
    }
    std::cout << "\n";
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "  VANI MARK 2 — PHASE 7C.1: COMPARE MICROPHONE CAPTURE ENDPOINTS                \n";
    std::cout << "================================================================================\n\n";

    CoInitialize(nullptr);

    IMMDeviceEnumerator* pEnumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator))) {
        std::cerr << "[FATAL] CoCreateInstance failed\n";
        CoUninitialize();
        return 1;
    }

    // Default endpoint ID
    std::string default_id = "";
    IMMDevice* pDefDev = nullptr;
    if (SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDefDev)) && pDefDev) {
        LPWSTR pId = nullptr;
        pDefDev->GetId(&pId);
        if (pId) {
            int sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
            default_id.resize(sz);
            WideCharToMultiByte(CP_UTF8, 0, pId, -1, &default_id[0], sz, NULL, NULL);
            if (!default_id.empty() && default_id.back() == '\0') default_id.pop_back();
            CoTaskMemFree(pId);
        }
        pDefDev->Release();
    }

    IMMDeviceCollection* pCol = nullptr;
    if (SUCCEEDED(pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCol)) && pCol) {
        UINT count = 0;
        pCol->GetCount(&count);
        std::cout << "Found " << count << " ACTIVE Capture Endpoints:\n\n";
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDev = nullptr;
            if (SUCCEEDED(pCol->Item(i, &pDev)) && pDev) {
                inspect_device(pDev, default_id);
                pDev->Release();
            }
        }
        pCol->Release();
    }

    pEnumerator->Release();
    CoUninitialize();

    return 0;
}
