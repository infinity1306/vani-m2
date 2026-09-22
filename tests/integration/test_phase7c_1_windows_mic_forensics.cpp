#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <propkey.h>
#include <functiondiscoverykeys_devpkey.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

// Helper to convert GUID to string
static std::string guid_to_string(const GUID& guid) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

// Convert PROPVARIANT to readable string
static std::string propvariant_to_string(const PROPVARIANT& pv) {
    switch (pv.vt) {
        case VT_LPWSTR: {
            if (!pv.pwszVal) return "(null)";
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, NULL, 0, NULL, NULL);
            std::string str(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, &str[0], size_needed, NULL, NULL);
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

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "     VANI MARK 2 — PHASE 7C.1: WINDOWS AUDIO CAPTURE ENDPOINT FORENSICS         \n";
    std::cout << "================================================================================\n\n";

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] CoInitialize failed with HRESULT 0x" << std::hex << hr << "\n";
        return 1;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        std::cerr << "[FATAL] CoCreateInstance(MMDeviceEnumerator) failed: 0x" << std::hex << hr << "\n";
        CoUninitialize();
        return 1;
    }

    // Check default endpoints for all roles: eConsole, eCommunications, eMultimedia
    const ERole roles[] = { eConsole, eCommunications, eMultimedia };
    const char* role_names[] = { "eConsole (System/Games)", "eCommunications (Voice/Chat)", "eMultimedia (Music/Recording)" };

    std::cout << "[1. DEFAULT CAPTURE ENDPOINT ASSIGNMENTS]\n";
    for (int r = 0; r < 3; ++r) {
        IMMDevice* pDev = nullptr;
        hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, roles[r], &pDev);
        if (SUCCEEDED(hr) && pDev) {
            LPWSTR pId = nullptr;
            pDev->GetId(&pId);
            IPropertyStore* pProps = nullptr;
            pDev->OpenPropertyStore(STGM_READ, &pProps);
            std::string name = "(Unknown)";
            if (pProps) {
                PROPVARIANT pv;
                PropVariantInit(&pv);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &pv))) {
                    name = propvariant_to_string(pv);
                }
                PropVariantClear(&pv);
                pProps->Release();
            }
            std::wcout << L"  Role: " << role_names[r] << L"\n";
            std::cout  << "    Friendly Name: " << name << "\n";
            std::wcout << L"    Endpoint ID:   " << (pId ? pId : L"null") << L"\n\n";
            if (pId) CoTaskMemFree(pId);
            pDev->Release();
        } else {
            std::cout << "  Role: " << role_names[r] << " -> NONE (0x" << std::hex << hr << ")\n";
        }
    }

    // Inspect primary eConsole default capture device
    std::cout << "================================================================================\n";
    std::cout << "[2. FORENSIC INSPECTION OF PRIMARY DEFAULT CAPTURE ENDPOINT (eConsole)]\n";
    std::cout << "================================================================================\n";

    IMMDevice* pPrimaryDev = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pPrimaryDev);
    if (FAILED(hr) || !pPrimaryDev) {
        std::cerr << "[FATAL] Failed to get default capture endpoint (0x" << std::hex << hr << ")\n";
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    LPWSTR pDevId = nullptr;
    pPrimaryDev->GetId(&pDevId);
    std::wcout << L"Endpoint ID: " << (pDevId ? pDevId : L"null") << L"\n";
    if (pDevId) CoTaskMemFree(pDevId);

    // Device state flags
    DWORD dwState = 0;
    pPrimaryDev->GetState(&dwState);
    std::cout << "Device State Flags: 0x" << std::hex << dwState << std::dec;
    if (dwState & DEVICE_STATE_ACTIVE) std::cout << " [ACTIVE]";
    if (dwState & DEVICE_STATE_DISABLED) std::cout << " [DISABLED]";
    if (dwState & DEVICE_STATE_NOTPRESENT) std::cout << " [NOTPRESENT]";
    if (dwState & DEVICE_STATE_UNPLUGGED) std::cout << " [UNPLUGGED]";
    std::cout << "\n";
    std::cout << "Is Disabled:     " << ((dwState & DEVICE_STATE_DISABLED) ? "YES" : "NO") << "\n";
    std::cout << "Is Disconnected: " << ((dwState & DEVICE_STATE_UNPLUGGED) ? "YES" : "NO") << "\n\n";

    // Inspect Properties from PropertyStore
    IPropertyStore* pProps = nullptr;
    hr = pPrimaryDev->OpenPropertyStore(STGM_READ, &pProps);
    if (SUCCEEDED(hr) && pProps) {
        DWORD propCount = 0;
        pProps->GetCount(&propCount);
        std::cout << "--- Device Properties (Total " << propCount << " keys) ---\n";

        // Query key well-known properties
        auto query_prop = [&](const PROPERTYKEY& key, const char* label) {
            PROPVARIANT pv;
            PropVariantInit(&pv);
            if (SUCCEEDED(pProps->GetValue(key, &pv))) {
                std::cout << "  " << std::left << std::setw(38) << label << ": " << propvariant_to_string(pv) << "\n";
            } else {
                std::cout << "  " << std::left << std::setw(38) << label << ": (Query Failed)\n";
            }
            PropVariantClear(&pv);
        };

        query_prop(PKEY_Device_FriendlyName, "Device Friendly Name");
        query_prop(PKEY_Device_DeviceDesc, "Device Description");
        query_prop(PKEY_DeviceInterface_FriendlyName, "Interface Friendly Name");
        query_prop(PKEY_AudioEndpoint_FormFactor, "Form Factor");
        query_prop(PKEY_AudioEndpoint_ControlPanelPageProvider, "Control Panel Provider");
        query_prop(PKEY_AudioEndpoint_Association, "Endpoint Association");
        query_prop(PKEY_AudioEndpoint_Supports_EventDriven_Mode, "Supports Event-Driven");
        query_prop(PKEY_AudioEndpoint_FullRangeSpeakers, "Full Range Speakers");
        query_prop(PKEY_AudioEndpoint_PhysicalSpeakers, "Physical Speaker Mask");
        query_prop(PKEY_AudioEngine_OEMFormat, "OEM Default Format");
        query_prop(PKEY_AudioEngine_DeviceFormat, "Engine Device Format");

        // Realtek / Windows Audio Enhancement / SysFx properties:
        // PKEY_AudioEndpoint_Disable_SysFx = {1da5d803-d492-4edd-8c23-e0c0ffee7f0e}, 5
        PROPERTYKEY PKEY_Disable_SysFx = { {0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}}, 5 };
        query_prop(PKEY_Disable_SysFx, "Disable Audio Enhancements (SysFx)");

        // Iterate all property keys to reveal vendor-specific (Realtek/Intel) processing
        std::cout << "\n--- Full Property Store Dump ---\n";
        for (DWORD i = 0; i < propCount; ++i) {
            PROPERTYKEY pk;
            if (SUCCEEDED(pProps->GetAt(i, &pk))) {
                PROPVARIANT pv;
                PropVariantInit(&pv);
                pProps->GetValue(pk, &pv);
                std::cout << "  Key " << std::setw(3) << i << ": " << guid_to_string(pk.fmtid) << " [" << pk.pid << "] = " 
                          << propvariant_to_string(pv) << "\n";
                PropVariantClear(&pv);
            }
        }
        std::cout << "\n";
        pProps->Release();
    }

    // Inspect Endpoint Volume & Channel Information
    std::cout << "================================================================================\n";
    std::cout << "[3. ENDPOINT VOLUME & CHANNEL INFORMATION]\n";
    std::cout << "================================================================================\n";

    IAudioEndpointVolume* pEndpointVolume = nullptr;
    hr = pPrimaryDev->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pEndpointVolume);
    if (SUCCEEDED(hr) && pEndpointVolume) {
        BOOL isMuted = FALSE;
        pEndpointVolume->GetMute(&isMuted);
        float masterVolScalar = 0.0f;
        pEndpointVolume->GetMasterVolumeLevelScalar(&masterVolScalar);
        float masterVolDb = 0.0f;
        pEndpointVolume->GetMasterVolumeLevel(&masterVolDb);
        float minDb = 0.0f, maxDb = 0.0f, incDb = 0.0f;
        pEndpointVolume->GetVolumeRange(&minDb, &maxDb, &incDb);

        UINT channelCount = 0;
        pEndpointVolume->GetChannelCount(&channelCount);

        std::cout << "Endpoint Mute:        " << (isMuted ? "MUTED (WARNING!)" : "UNMUTED (OK)") << "\n";
        std::cout << "Endpoint Master Vol:  " << std::fixed << std::setprecision(1) << (masterVolScalar * 100.0f) << "% ("
                  << std::setprecision(2) << masterVolDb << " dB)\n";
        std::cout << "Volume Range (dB):    Min: " << minDb << " dB, Max: " << maxDb << " dB, Step: " << incDb << " dB\n";
        std::cout << "Volume Channels:      " << channelCount << "\n";

        for (UINT ch = 0; ch < channelCount; ++ch) {
            float chVol = 0.0f;
            float chDb = 0.0f;
            pEndpointVolume->GetChannelVolumeLevelScalar(ch, &chVol);
            pEndpointVolume->GetChannelVolumeLevel(ch, &chDb);
            std::cout << "  Channel " << ch << " Volume:   " << (chVol * 100.0f) << "% (" << chDb << " dB)\n";
        }
        std::cout << "\n";
        pEndpointVolume->Release();
    } else {
        std::cerr << "  Failed to activate IAudioEndpointVolume (0x" << std::hex << hr << ")\n";
    }

    // Inspect Native Engine Format & WASAPI Client Parameters
    std::cout << "================================================================================\n";
    std::cout << "[4. WASAPI AUDIO CLIENT NATIVE MIX FORMAT & PERIODICITY]\n";
    std::cout << "================================================================================\n";

    IAudioClient* pAudioClient = nullptr;
    hr = pPrimaryDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (SUCCEEDED(hr) && pAudioClient) {
        WAVEFORMATEX* pMixFormat = nullptr;
        hr = pAudioClient->GetMixFormat(&pMixFormat);
        if (SUCCEEDED(hr) && pMixFormat) {
            std::cout << "Native Mix Format Tag:     " << pMixFormat->wFormatTag;
            if (pMixFormat->wFormatTag == WAVE_FORMAT_PCM) std::cout << " (WAVE_FORMAT_PCM)\n";
            else if (pMixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) std::cout << " (WAVE_FORMAT_IEEE_FLOAT)\n";
            else if (pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) std::cout << " (WAVE_FORMAT_EXTENSIBLE)\n";
            else std::cout << "\n";

            std::cout << "Native Channels:           " << pMixFormat->nChannels << "\n";
            std::cout << "Native Sample Rate:        " << pMixFormat->nSamplesPerSec << " Hz\n";
            std::cout << "Native Bits Per Sample:    " << pMixFormat->wBitsPerSample << " bits\n";
            std::cout << "Native Block Align:        " << pMixFormat->nBlockAlign << " bytes\n";
            std::cout << "Native Avg Bytes/Sec:      " << pMixFormat->nAvgBytesPerSec << " bytes/sec\n";

            if (pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                auto* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMixFormat);
                std::cout << "  SubFormat GUID:          " << guid_to_string(pExt->SubFormat);
                if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) std::cout << " (IEEE_FLOAT)\n";
                else if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) std::cout << " (PCM)\n";
                else std::cout << "\n";
                std::cout << "  Channel Mask:            0x" << std::hex << pExt->dwChannelMask << std::dec << "\n";
                std::cout << "  Valid Bits Per Sample:   " << pExt->Samples.wValidBitsPerSample << "\n";
            }

            // Test if 16000 Hz Mono Float32 is supported in Shared Mode
            WAVEFORMATEX testFmt{};
            testFmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
            testFmt.nChannels = 1;
            testFmt.nSamplesPerSec = 16000;
            testFmt.wBitsPerSample = 32;
            testFmt.nBlockAlign = 4;
            testFmt.nAvgBytesPerSec = 16000 * 4;
            testFmt.cbSize = 0;

            WAVEFORMATEX* pClosestMatch = nullptr;
            HRESULT hrSupp = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &testFmt, &pClosestMatch);
            std::cout << "\nShared-Mode Support for 16000 Hz Mono Float32:\n";
            if (hrSupp == S_OK) {
                std::cout << "  Status: DIRECTLY SUPPORTED BY WASAPI ENGINE (S_OK)\n";
            } else if (hrSupp == S_FALSE) {
                std::cout << "  Status: NOT DIRECTLY SUPPORTED - PROPOSED CLOSEST MATCH AVAILABLE (S_FALSE)\n";
                if (pClosestMatch) {
                    std::cout << "  Closest Match Rate:     " << pClosestMatch->nSamplesPerSec << " Hz\n";
                    std::cout << "  Closest Match Channels: " << pClosestMatch->nChannels << "\n";
                    std::cout << "  Closest Match Bits:     " << pClosestMatch->wBitsPerSample << "\n";
                    CoTaskMemFree(pClosestMatch);
                }
            } else {
                std::cout << "  Status: UNSUPPORTED (0x" << std::hex << hrSupp << std::dec << ")\n";
            }

            CoTaskMemFree(pMixFormat);
        }

        REFERENCE_TIME defPeriod = 0, minPeriod = 0;
        if (SUCCEEDED(pAudioClient->GetDevicePeriod(&defPeriod, &minPeriod))) {
            std::cout << "\nDefault Device Period:     " << (defPeriod / 10000.0) << " ms (" << defPeriod << " * 100ns)\n";
            std::cout << "Minimum Device Period:     " << (minPeriod / 10000.0) << " ms (" << minPeriod << " * 100ns)\n";
        }

        pAudioClient->Release();
    } else {
        std::cerr << "  Failed to activate IAudioClient (0x" << std::hex << hr << ")\n";
    }

    pPrimaryDev->Release();

    // Enumerate ALL other capture endpoints on system
    std::cout << "\n================================================================================\n";
    std::cout << "[5. ALL AUDIO CAPTURE ENDPOINTS IN SYSTEM]\n";
    std::cout << "================================================================================\n";
    IMMDeviceCollection* pCollection = nullptr;
    hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATEMASK_ALL, &pCollection);
    if (SUCCEEDED(hr) && pCollection) {
        UINT count = 0;
        pCollection->GetCount(&count);
        std::cout << "Total Capture Endpoints Found: " << count << "\n";
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDev = nullptr;
            if (SUCCEEDED(pCollection->Item(i, &pDev))) {
                LPWSTR pId = nullptr;
                pDev->GetId(&pId);
                DWORD state = 0;
                pDev->GetState(&state);
                IPropertyStore* pDevProps = nullptr;
                pDev->OpenPropertyStore(STGM_READ, &pDevProps);
                std::string fname = "(Unknown)";
                if (pDevProps) {
                    PROPVARIANT pv;
                    PropVariantInit(&pv);
                    if (SUCCEEDED(pDevProps->GetValue(PKEY_Device_FriendlyName, &pv))) {
                        fname = propvariant_to_string(pv);
                    }
                    PropVariantClear(&pv);
                    pDevProps->Release();
                }
                std::cout << "  [" << i << "] " << fname << "\n";
                std::wcout << L"      ID: " << (pId ? pId : L"null") << L"\n";
                std::cout  << "      State: 0x" << std::hex << state << std::dec;
                if (state & DEVICE_STATE_ACTIVE) std::cout << " (ACTIVE)";
                if (state & DEVICE_STATE_DISABLED) std::cout << " (DISABLED)";
                if (state & DEVICE_STATE_NOTPRESENT) std::cout << " (NOTPRESENT)";
                if (state & DEVICE_STATE_UNPLUGGED) std::cout << " (UNPLUGGED)";
                std::cout << "\n";
                if (pId) CoTaskMemFree(pId);
                pDev->Release();
            }
        }
        pCollection->Release();
    }

    pEnumerator->Release();
    CoUninitialize();

    std::cout << "================================================================================\n";
    std::cout << "                 FORENSICS ENDPOINT INSPECTION COMPLETE                         \n";
    std::cout << "================================================================================\n";

    return 0;
}
