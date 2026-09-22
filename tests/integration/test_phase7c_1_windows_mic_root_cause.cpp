#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <propkey.h>
#include <functiondiscoverykeys_devpkey.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cstring>

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

// Window statistics for a channel
struct WindowStats {
    uint64_t sample_count{0};
    double sum_sq{0.0};
    float peak{0.0f};
    float min_val{1e9f};
    float max_val{-1e9f};
    uint64_t non_zero_count{0};
    uint64_t clipping_count{0};

    void update(float s) {
        sample_count++;
        sum_sq += static_cast<double>(s * s);
        float a = std::abs(s);
        if (a > peak) peak = a;
        if (s < min_val) min_val = s;
        if (s > max_val) max_val = s;
        if (a > 1e-7f) non_zero_count++;
        if (a >= 0.999f) clipping_count++;
    }

    [[nodiscard]] float rms() const {
        if (sample_count == 0) return 0.0f;
        return static_cast<float>(std::sqrt(sum_sq / static_cast<double>(sample_count)));
    }

    [[nodiscard]] float non_zero_percent() const {
        if (sample_count == 0) return 0.0f;
        return static_cast<float>((static_cast<double>(non_zero_count) / static_cast<double>(sample_count)) * 100.0);
    }
};

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "        VANI MARK 2 — WINDOWS MIC ROOT-CAUSE DIAGNOSTIC (RAW WASAPI)            \n";
    std::cout << "================================================================================\n\n";

    double phase_sec = 5.0;
    if (argc > 1) {
        try {
            phase_sec = std::stod(argv[1]);
            if (phase_sec < 2.0) phase_sec = 2.0;
        } catch (...) {}
    }
    double total_sec = phase_sec * 5.0;

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] CoInitialize failed (0x" << std::hex << hr << ")\n";
        return 1;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        std::cerr << "[FATAL] CoCreateInstance(MMDeviceEnumerator) failed (0x" << std::hex << hr << ")\n";
        CoUninitialize();
        return 1;
    }

    // 1. Enumerate all Windows capture endpoints
    std::cout << "================================================================================\n";
    std::cout << "[1. ENUMERATION OF ALL WINDOWS AUDIO CAPTURE ENDPOINTS]\n";
    std::cout << "================================================================================\n";

    IMMDeviceCollection* pCol = nullptr;
    hr = pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATEMASK_ALL, &pCol);
    UINT device_count = 0;
    if (SUCCEEDED(hr) && pCol) {
        pCol->GetCount(&device_count);
    }
    std::cout << "Total Capture Endpoints Found in System: " << device_count << "\n\n";

    IMMDevice* pTargetDevice = nullptr;
    std::string target_endpoint_id = "";
    std::string target_friendly_name = "";

    for (UINT i = 0; i < device_count; ++i) {
        IMMDevice* pDev = nullptr;
        if (SUCCEEDED(pCol->Item(i, &pDev)) && pDev) {
            LPWSTR pId = nullptr;
            pDev->GetId(&pId);
            std::string id_str = "";
            if (pId) {
                int sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
                id_str.resize(sz);
                WideCharToMultiByte(CP_UTF8, 0, pId, -1, &id_str[0], sz, NULL, NULL);
                if (!id_str.empty() && id_str.back() == '\0') id_str.pop_back();
                CoTaskMemFree(pId);
            }

            DWORD dwState = 0;
            pDev->GetState(&dwState);

            std::string state_str = "UNKNOWN";
            if (dwState & DEVICE_STATE_ACTIVE) state_str = "ACTIVE";
            else if (dwState & DEVICE_STATE_DISABLED) state_str = "DISABLED";
            else if (dwState & DEVICE_STATE_NOTPRESENT) state_str = "NOTPRESENT";
            else if (dwState & DEVICE_STATE_UNPLUGGED) state_str = "UNPLUGGED";

            std::string friendly_name = "(Unknown)";
            IPropertyStore* pProps = nullptr;
            if (SUCCEEDED(pDev->OpenPropertyStore(STGM_READ, &pProps)) && pProps) {
                PROPVARIANT pv;
                PropVariantInit(&pv);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &pv))) {
                    friendly_name = propvariant_to_string(pv);
                }
                PropVariantClear(&pv);
                pProps->Release();
            }

            std::cout << "  [" << std::setw(2) << i << "] " << friendly_name << "\n"
                      << "       State: " << state_str << " (0x" << std::hex << dwState << std::dec << ")\n"
                      << "       ID:    " << id_str << "\n";

            if (friendly_name.find("Microphone Array (Realtek(R) Audio)") != std::string::npos ||
                friendly_name.find("Microphone Array") != std::string::npos) {
                if (!pTargetDevice && (dwState & DEVICE_STATE_ACTIVE)) {
                    pTargetDevice = pDev;
                    pTargetDevice->AddRef();
                    target_endpoint_id = id_str;
                    target_friendly_name = friendly_name;
                }
            }

            pDev->Release();
        }
    }
    if (pCol) pCol->Release();

    // Fallback: If not found by name, try default eConsole capture device
    if (!pTargetDevice) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pTargetDevice);
        if (SUCCEEDED(hr) && pTargetDevice) {
            LPWSTR pId = nullptr;
            pTargetDevice->GetId(&pId);
            if (pId) {
                int sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
                target_endpoint_id.resize(sz);
                WideCharToMultiByte(CP_UTF8, 0, pId, -1, &target_endpoint_id[0], sz, NULL, NULL);
                if (!target_endpoint_id.empty() && target_endpoint_id.back() == '\0') target_endpoint_id.pop_back();
                CoTaskMemFree(pId);
            }
            target_friendly_name = "Default Capture Endpoint";
        }
    }

    if (!pTargetDevice) {
        std::cerr << "[FATAL] Could not obtain 'Microphone Array (Realtek(R) Audio)' or default capture endpoint!\n";
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    // 2 & 3. Identify exact endpoint ID, state, roles, format, volume, and mute state
    std::cout << "\n================================================================================\n";
    std::cout << "[2 & 3. TARGET ENDPOINT PROPERTIES & HARDWARE STATE]\n";
    std::cout << "================================================================================\n";
    std::cout << "Selected Device:       " << target_friendly_name << "\n";
    std::cout << "Endpoint ID:           " << target_endpoint_id << "\n";

    DWORD target_state = 0;
    pTargetDevice->GetState(&target_state);
    std::cout << "Endpoint State:        0x" << std::hex << target_state << std::dec;
    if (target_state & DEVICE_STATE_ACTIVE) std::cout << " (ACTIVE)";
    if (target_state & DEVICE_STATE_DISABLED) std::cout << " (DISABLED)";
    if (target_state & DEVICE_STATE_NOTPRESENT) std::cout << " (NOTPRESENT)";
    if (target_state & DEVICE_STATE_UNPLUGGED) std::cout << " (UNPLUGGED)";
    std::cout << "\n";

    // Check Role Defaults
    auto check_role = [&](ERole role, const char* name) {
        IMMDevice* pDef = nullptr;
        bool is_def = false;
        if (SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eCapture, role, &pDef)) && pDef) {
            LPWSTR pId = nullptr;
            pDef->GetId(&pId);
            if (pId) {
                int sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
                std::string cur_id(sz, 0);
                WideCharToMultiByte(CP_UTF8, 0, pId, -1, &cur_id[0], sz, NULL, NULL);
                if (!cur_id.empty() && cur_id.back() == '\0') cur_id.pop_back();
                if (cur_id == target_endpoint_id) is_def = true;
                CoTaskMemFree(pId);
            }
            pDef->Release();
        }
        std::cout << "  Default " << std::left << std::setw(20) << name << ": " << (is_def ? "YES" : "NO") << "\n";
    };

    check_role(eConsole, "eConsole (System)");
    check_role(eCommunications, "eCommunications (Voice)");
    check_role(eMultimedia, "eMultimedia (Media)");

    // Endpoint Volume & Mute State
    IAudioEndpointVolume* pEndpointVolume = nullptr;
    BOOL is_muted = FALSE;
    float master_vol_scalar = 0.0f;
    float master_vol_db = 0.0f;
    float min_db = 0.0f, max_db = 0.0f, inc_db = 0.0f;
    UINT vol_channel_count = 0;
    DWORD hw_support_mask = 0;

    hr = pTargetDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pEndpointVolume);
    if (SUCCEEDED(hr) && pEndpointVolume) {
        pEndpointVolume->GetMute(&is_muted);
        pEndpointVolume->GetMasterVolumeLevelScalar(&master_vol_scalar);
        pEndpointVolume->GetMasterVolumeLevel(&master_vol_db);
        pEndpointVolume->GetVolumeRange(&min_db, &max_db, &inc_db);
        pEndpointVolume->GetChannelCount(&vol_channel_count);
        pEndpointVolume->QueryHardwareSupport(&hw_support_mask);

        std::cout << "\nWindows Input Volume:  " << std::fixed << std::setprecision(1) << (master_vol_scalar * 100.0f) << "% ("
                  << std::setprecision(2) << master_vol_db << " dB)\n";
        std::cout << "Windows Mute State:    " << (is_muted ? "MUTED [CRITICAL FACTOR!]" : "UNMUTED (Clean)") << "\n";
        std::cout << "Volume dB Range:       " << min_db << " dB to " << max_db << " dB (Step: " << inc_db << " dB)\n";
        std::cout << "Volume Channels:       " << vol_channel_count << "\n";

        for (UINT ch = 0; ch < vol_channel_count; ++ch) {
            float chVol = 0.0f;
            float chDb = 0.0f;
            pEndpointVolume->GetChannelVolumeLevelScalar(ch, &chVol);
            pEndpointVolume->GetChannelVolumeLevel(ch, &chDb);
            std::cout << "  Channel " << ch << " Volume:   " << (chVol * 100.0f) << "% (" << chDb << " dB)\n";
        }

        std::cout << "Hardware Support Mask: 0x" << std::hex << hw_support_mask << std::dec << "\n";
        std::cout << "  Hardware Volume:     " << ((hw_support_mask & 0x1) ? "YES" : "NO") << "\n";
        std::cout << "  Hardware Mute:       " << ((hw_support_mask & 0x2) ? "YES" : "NO") << "\n";
        std::cout << "  Hardware Meter:      " << ((hw_support_mask & 0x4) ? "YES" : "NO") << "\n";

        pEndpointVolume->Release();
    } else {
        std::cerr << "  [WARN] Failed to activate IAudioEndpointVolume (0x" << std::hex << hr << ")\n";
    }

    // Inspect Properties and Audio Enhancements / APOs
    std::cout << "\n--- Audio Enhancements & APO Properties ---\n";
    IPropertyStore* pStore = nullptr;
    hr = pTargetDevice->OpenPropertyStore(STGM_READ, &pStore);
    if (SUCCEEDED(hr) && pStore) {
        PROPERTYKEY PKEY_Disable_SysFx = { {0x1da5d803, 0xd492, 0x4edd, {0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e}}, 5 };
        PROPVARIANT pvSysFx;
        PropVariantInit(&pvSysFx);
        if (SUCCEEDED(pStore->GetValue(PKEY_Disable_SysFx, &pvSysFx))) {
            std::cout << "  Disable Audio Enhancements (SysFx): " << propvariant_to_string(pvSysFx) << "\n";
            if (pvSysFx.vt == VT_UI4 && pvSysFx.ulVal == 1) {
                std::cout << "  -> Windows Audio Enhancements are DISABLED (Bypassed)\n";
            } else {
                std::cout << "  -> Windows Audio Enhancements are ENABLED (Active)\n";
            }
        }
        PropVariantClear(&pvSysFx);

        PROPVARIANT pvDesc;
        PropVariantInit(&pvDesc);
        if (SUCCEEDED(pStore->GetValue(PKEY_Device_DeviceDesc, &pvDesc))) {
            std::cout << "  Device Description:                 " << propvariant_to_string(pvDesc) << "\n";
        }
        PropVariantClear(&pvDesc);

        pStore->Release();
    }

    // Inspect Native WASAPI Client Mix Format
    IAudioClient* pAudioClient = nullptr;
    hr = pTargetDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr) || !pAudioClient) {
        std::cerr << "[FATAL] Failed to activate IAudioClient (0x" << std::hex << hr << ")\n";
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    WAVEFORMATEX* pMixFormat = nullptr;
    hr = pAudioClient->GetMixFormat(&pMixFormat);
    if (FAILED(hr) || !pMixFormat) {
        std::cerr << "[FATAL] GetMixFormat failed (0x" << std::hex << hr << ")\n";
        pAudioClient->Release();
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    std::cout << "\nNative Channel Count:  " << pMixFormat->nChannels << "\n";
    std::cout << "Native Sample Rate:    " << pMixFormat->nSamplesPerSec << " Hz\n";
    std::cout << "Native Bits Per Sample:" << pMixFormat->wBitsPerSample << " bits\n";
    std::cout << "Native Block Align:    " << pMixFormat->nBlockAlign << " bytes\n";
    std::cout << "Format Tag:            0x" << std::hex << pMixFormat->wFormatTag << std::dec << "\n";

    bool is_float = false;
    if (pMixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        is_float = true;
        std::cout << "Sample Format:         IEEE-754 32-bit Float\n";
    } else if (pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        auto* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMixFormat);
        std::cout << "Sample Format:         WAVE_FORMAT_EXTENSIBLE (SubFormat: " << guid_to_string(pExt->SubFormat) << ")\n";
        if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            is_float = true;
            std::cout << "  Decoded Subtype:     IEEE-754 32-bit Float\n";
        } else if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
            std::cout << "  Decoded Subtype:     PCM Integer (" << pExt->Samples.wValidBitsPerSample << " valid bits)\n";
        }
    } else if (pMixFormat->wFormatTag == WAVE_FORMAT_PCM) {
        std::cout << "Sample Format:         PCM Integer 16/24-bit\n";
    }

    // 4. Initialize RAW WASAPI Direct Audio Capture Stream
    std::cout << "\n================================================================================\n";
    std::cout << "[4 & 5. DIRECT RAW WASAPI CAPTURE INITIALIZATION (BYPASSING VANI/MINIAUDIO)]\n";
    std::cout << "================================================================================\n";

    REFERENCE_TIME hnsRequestedDuration = 10000000; // 1 second buffer
    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        0,
        pMixFormat,
        nullptr
    );
    if (FAILED(hr)) {
        std::cerr << "[FATAL] IAudioClient::Initialize failed: 0x" << std::hex << hr << "\n";
        CoTaskMemFree(pMixFormat);
        pAudioClient->Release();
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    HANDLE hAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    hr = pAudioClient->SetEventHandle(hAudioEvent);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] SetEventHandle failed: 0x" << std::hex << hr << "\n";
        CloseHandle(hAudioEvent);
        CoTaskMemFree(pMixFormat);
        pAudioClient->Release();
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    IAudioCaptureClient* pCaptureClient = nullptr;
    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr) || !pCaptureClient) {
        std::cerr << "[FATAL] GetService(IAudioCaptureClient) failed: 0x" << std::hex << hr << "\n";
        CloseHandle(hAudioEvent);
        CoTaskMemFree(pMixFormat);
        pAudioClient->Release();
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "[FATAL] IAudioClient::Start failed: 0x" << std::hex << hr << "\n";
        pCaptureClient->Release();
        CloseHandle(hAudioEvent);
        CoTaskMemFree(pMixFormat);
        pAudioClient->Release();
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    std::cout << "RAW WASAPI Stream:     STARTED & STREAMING\n";
    std::cout << "Channels Captured:     " << pMixFormat->nChannels << " physical channels independently\n\n";

    // 6. Run Protocol:
    // 0–5 sec: silence
    // 5–10 sec: normal speech
    // 10–15 sec: silence
    // 15–20 sec: loud speech
    // 20–25 sec: silence

    std::cout << "================================================================================\n";
    std::cout << "                    25-SECOND PROTOCOL COMMENCING                               \n";
    std::cout << "================================================================================\n";
    std::cout << "  0–" << static_cast<int>(phase_sec) << "  sec:  SILENCE        (Remain quiet to measure noise floor)\n";
    std::cout << "  " << static_cast<int>(phase_sec) << "–" << static_cast<int>(phase_sec * 2) << " sec:  NORMAL SPEECH  (Speak naturally in normal conversational tone)\n";
    std::cout << " " << static_cast<int>(phase_sec * 2) << "–" << static_cast<int>(phase_sec * 3) << " sec:  SILENCE        (Remain quiet)\n";
    std::cout << " " << static_cast<int>(phase_sec * 3) << "–" << static_cast<int>(phase_sec * 4) << " sec:  LOUD SPEECH    (Speak or say \"VANI\" loudly)\n";
    std::cout << " " << static_cast<int>(phase_sec * 4) << "–" << static_cast<int>(total_sec) << " sec:  SILENCE        (Remain quiet)\n";
    std::cout << "================================================================================\n\n";

    std::cout << "Starting countdown:\n";
    for (int c = 3; c > 0; --c) {
        std::cout << "  [" << c << "] Prepare for Phase 1 (SILENCE)...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "  >>> BEGIN PROTOCOL NOW! <<<\n\n";

    // Stats storage: [phase 1..5][channel 0..channels-1]
    UINT channels = pMixFormat->nChannels;
    std::vector<std::vector<WindowStats>> stats(6, std::vector<WindowStats>(channels));

    auto t_start = std::chrono::steady_clock::now();
    int last_phase = 0;

    while (true) {
        DWORD waitRes = WaitForSingleObject(hAudioEvent, 200);
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - t_start).count();
        if (elapsed >= total_sec) break;

        int p = 1;
        if (elapsed < phase_sec) p = 1;
        else if (elapsed < phase_sec * 2.0) p = 2;
        else if (elapsed < phase_sec * 3.0) p = 3;
        else if (elapsed < phase_sec * 4.0) p = 4;
        else p = 5;

        if (p != last_phase) {
            last_phase = p;
            std::cout << "\n>>> [T=" << std::fixed << std::setprecision(1) << elapsed << "s] ";
            switch (p) {
                case 1: std::cout << "PHASE 1/5: REMAIN SILENT (0-5s) ...\n"; break;
                case 2: std::cout << "PHASE 2/5: SPEAK NORMALLY (5-10s) ...\n"; break;
                case 3: std::cout << "PHASE 3/5: REMAIN SILENT (10-15s) ...\n"; break;
                case 4: std::cout << "PHASE 4/5: SPEAK LOUDLY (15-20s) ...\n"; break;
                case 5: std::cout << "PHASE 5/5: REMAIN SILENT (20-25s) ...\n"; break;
            }
        }

        if (waitRes == WAIT_OBJECT_0) {
            BYTE* pData = nullptr;
            UINT32 numFramesAvailable = 0;
            DWORD flags = 0;

            while (SUCCEEDED(pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr)) && numFramesAvailable > 0) {
                if (pData) {
                    if (is_float) {
                        const float* float_data = reinterpret_cast<const float*>(pData);
                        for (UINT32 f = 0; f < numFramesAvailable; ++f) {
                            for (UINT ch = 0; ch < channels; ++ch) {
                                float val = (flags & AUDCLNT_BUFFERFLAGS_SILENT) ? 0.0f : float_data[f * channels + ch];
                                stats[p][ch].update(val);
                            }
                        }
                    } else if (pMixFormat->wBitsPerSample == 16) {
                        const int16_t* int_data = reinterpret_cast<const int16_t*>(pData);
                        for (UINT32 f = 0; f < numFramesAvailable; ++f) {
                            for (UINT ch = 0; ch < channels; ++ch) {
                                float val = (flags & AUDCLNT_BUFFERFLAGS_SILENT) ? 0.0f : (static_cast<float>(int_data[f * channels + ch]) / 32768.0f);
                                stats[p][ch].update(val);
                            }
                        }
                    }
                }
                pCaptureClient->ReleaseBuffer(numFramesAvailable);
            }
        }
    }

    pAudioClient->Stop();
    pCaptureClient->Release();
    CloseHandle(hAudioEvent);
    CoTaskMemFree(pMixFormat);
    pAudioClient->Release();
    pTargetDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    // 7. Report for every window and channel
    std::cout << "\n================================================================================\n";
    std::cout << "             RAW WASAPI WINDOW-BY-WINDOW MEASUREMENTS                           \n";
    std::cout << "================================================================================\n\n";

    const char* phase_names[] = {
        "",
        "Window 1 (0–5s: Silence)",
        "Window 2 (5–10s: Normal Speech)",
        "Window 3 (10–15s: Silence)",
        "Window 4 (15–20s: Loud Speech)",
        "Window 5 (20–25s: Silence)"
    };

    for (int p = 1; p <= 5; ++p) {
        std::cout << "--------------------------------------------------------------------------------\n";
        std::cout << "  " << phase_names[p] << "\n";
        std::cout << "--------------------------------------------------------------------------------\n";
        for (UINT ch = 0; ch < channels; ++ch) {
            const auto& st = stats[p][ch];
            float sil_baseline = stats[1][ch].rms();
            float ratio = (sil_baseline > 0.0f) ? (st.rms() / sil_baseline) : 0.0f;

            std::cout << "  Ch" << ch << ":\n";
            std::cout << "    RMS:                  " << std::fixed << std::setprecision(6) << st.rms() << "\n";
            std::cout << "    Peak:                 " << std::fixed << std::setprecision(6) << st.peak << "\n";
            std::cout << "    Min / Max:            " << std::fixed << std::setprecision(6) << st.min_val << " / " << st.max_val << "\n";
            std::cout << "    Non-zero Samples:     " << std::fixed << std::setprecision(2) << st.non_zero_percent() << "% (" << st.non_zero_count << "/" << st.sample_count << ")\n";
            std::cout << "    Clipping Count:       " << st.clipping_count << "\n";
            std::cout << "    Speech/Silence Ratio: " << std::fixed << std::setprecision(2) << ratio << "x (relative to Window 1 silence)\n";
        }
        std::cout << "\n";
    }

    // Compute Overall Summary Metrics
    float ch0_sil_rms = (stats[1][0].rms() + stats[3][0].rms() + stats[5][0].rms()) / 3.0f;
    float ch0_norm_rms = stats[2][0].rms();
    float ch0_loud_rms = stats[4][0].rms();
    float ch0_speech_rms = std::max(ch0_norm_rms, ch0_loud_rms);
    float ch0_ratio = (ch0_sil_rms > 0.0f) ? (ch0_speech_rms / ch0_sil_rms) : 0.0f;

    float ch1_sil_rms = (stats[1][1].rms() + stats[3][1].rms() + stats[5][1].rms()) / 3.0f;
    float ch1_norm_rms = stats[2][1].rms();
    float ch1_loud_rms = stats[4][1].rms();
    float ch1_speech_rms = std::max(ch1_norm_rms, ch1_loud_rms);
    float ch1_ratio = (ch1_sil_rms > 0.0f) ? (ch1_speech_rms / ch1_sil_rms) : 0.0f;

    std::cout << "================================================================================\n";
    std::cout << "                     OVERALL EMPIRICAL SUMMARY                                  \n";
    std::cout << "================================================================================\n";
    std::cout << "Channel 0 Summary:\n";
    std::cout << "  Baseline Silence RMS:   " << std::fixed << std::setprecision(6) << ch0_sil_rms << "\n";
    std::cout << "  Normal Speech RMS:      " << std::fixed << std::setprecision(6) << ch0_norm_rms << "\n";
    std::cout << "  Loud Speech RMS:        " << std::fixed << std::setprecision(6) << ch0_loud_rms << "\n";
    std::cout << "  Speech/Silence Ratio:   " << std::fixed << std::setprecision(2) << ch0_ratio << "x\n\n";

    std::cout << "Channel 1 Summary:\n";
    std::cout << "  Baseline Silence RMS:   " << std::fixed << std::setprecision(6) << ch1_sil_rms << "\n";
    std::cout << "  Normal Speech RMS:      " << std::fixed << std::setprecision(6) << ch1_norm_rms << "\n";
    std::cout << "  Loud Speech RMS:        " << std::fixed << std::setprecision(6) << ch1_loud_rms << "\n";
    std::cout << "  Speech/Silence Ratio:   " << std::fixed << std::setprecision(2) << ch1_ratio << "x\n\n";

    // Final Classification Logic:
    // A) WINDOWS_CAPTURE_HEALTHY
    // B) WINDOWS_AUDIO_PROCESSING_PROBLEM
    // C) DEVICE_DRIVER_PROBLEM
    // D) HARDWARE_MIC_PROBLEM
    // E) INCONCLUSIVE
    std::string classification;
    std::string reason;

    if (is_muted) {
        // The endpoint is flagged as MUTED in Windows Core Audio!
        classification = "B) WINDOWS_AUDIO_PROCESSING_PROBLEM";
        reason = "Windows Audio Endpoint Mute is currently ACTIVE (Endpoint Mute: MUTED).\n"
                 "When muted in Windows Core Audio, the Windows audio engine zeros or attenuates the stream to the ambient electrical noise floor (~0.0003 RMS), preventing speech from reaching ANY application, including Windows Sound Recorder.";
    } else if (ch0_ratio >= 2.0f || ch1_ratio >= 2.0f) {
        classification = "A) WINDOWS_CAPTURE_HEALTHY";
        reason = "Windows RAW WASAPI demonstrates healthy speech/silence separation (>= 2.0x increase during vocal windows).";
    } else if (stats[1][0].non_zero_percent() < 1.0f && stats[2][0].non_zero_percent() < 1.0f) {
        classification = "C) DEVICE_DRIVER_PROBLEM";
        reason = "Device driver streams zero/null buffers despite active stream state.";
    } else if (ch0_ratio < 1.2f && ch1_ratio < 1.2f) {
        classification = "D) HARDWARE_MIC_PROBLEM";
        reason = "Neither channel shows measurable speech response above silence baseline under unmuted conditions.";
    } else {
        classification = "E) INCONCLUSIVE";
        reason = "Speech increase is marginal or inconclusive under current test parameters.";
    }

    std::cout << "================================================================================\n";
    std::cout << "FINAL CLASSIFICATION:\n" << classification << "\n\n";
    std::cout << "RATIONALE:\n" << reason << "\n";
    std::cout << "================================================================================\n";

    return 0;
}
