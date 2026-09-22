#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <string>
#include <algorithm>

static std::string guid_to_string(const GUID& guid) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "    VANI MARK 2 — PHASE 7C.1: RAW NATIVE WASAPI AUDIO CAPTURE DIAGNOSTIC        \n";
    std::cout << "================================================================================\n";
    std::cout << "  [BYPASS MODE] Directly using Windows Core Audio WASAPI (no miniaudio/VANI)\n\n";

    int duration_sec = 10;
    if (argc > 1) {
        try {
            duration_sec = std::stoi(argv[1]);
        } catch (...) {}
    }

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] CoInitialize failed: 0x" << std::hex << hr << "\n";
        return 1;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        std::cerr << "[FATAL] Failed to create MMDeviceEnumerator: 0x" << std::hex << hr << "\n";
        CoUninitialize();
        return 1;
    }

    IMMDevice* pDevice = nullptr;
    if (argc > 2) {
        // Enumerate and find device by index or name
        IMMDeviceCollection* pCol = nullptr;
        if (SUCCEEDED(pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCol)) && pCol) {
            UINT count = 0;
            pCol->GetCount(&count);
            std::string search_term = argv[2];
            for (UINT i = 0; i < count; ++i) {
                IMMDevice* pDev = nullptr;
                if (SUCCEEDED(pCol->Item(i, &pDev))) {
                    IPropertyStore* pStore = nullptr;
                    pDev->OpenPropertyStore(STGM_READ, &pStore);
                    std::string fname = "";
                    if (pStore) {
                        PROPVARIANT pv;
                        PropVariantInit(&pv);
                        if (SUCCEEDED(pStore->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.pwszVal) {
                            int sz = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, NULL, 0, NULL, NULL);
                            fname.resize(sz);
                            WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, &fname[0], sz, NULL, NULL);
                            if (!fname.empty() && fname.back() == '\0') fname.pop_back();
                        }
                        PropVariantClear(&pv);
                        pStore->Release();
                    }
                    if (std::to_string(i) == search_term || fname.find(search_term) != std::string::npos) {
                        pDevice = pDev;
                        std::cout << "  Selected Custom Device [" << i << "]: " << fname << "\n";
                        break;
                    }
                    pDev->Release();
                }
            }
            pCol->Release();
        }
    }

    if (!pDevice) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
        if (FAILED(hr) || !pDevice) {
            std::cerr << "[FATAL] Failed to get default capture endpoint: 0x" << std::hex << hr << "\n";
            pEnumerator->Release();
            CoUninitialize();
            return 1;
        }
    }

    IAudioClient* pAudioClient = nullptr;
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr) || !pAudioClient) {
        std::cerr << "[FATAL] Failed to activate IAudioClient: 0x" << std::hex << hr << "\n";
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    WAVEFORMATEX* pMixFormat = nullptr;
    hr = pAudioClient->GetMixFormat(&pMixFormat);
    if (FAILED(hr) || !pMixFormat) {
        std::cerr << "[FATAL] GetMixFormat failed: 0x" << std::hex << hr << "\n";
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    std::cout << "[NATIVE WASAPI MIX FORMAT]\n";
    std::cout << "  Sample Rate:      " << pMixFormat->nSamplesPerSec << " Hz\n";
    std::cout << "  Channels:         " << pMixFormat->nChannels << "\n";
    std::cout << "  Bits Per Sample:  " << pMixFormat->wBitsPerSample << " bits\n";
    std::cout << "  Block Align:      " << pMixFormat->nBlockAlign << " bytes\n";
    std::cout << "  Format Tag:       0x" << std::hex << pMixFormat->wFormatTag << std::dec << "\n";

    bool is_float = false;
    DWORD channel_mask = 0;
    if (pMixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        is_float = true;
    } else if (pMixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        auto* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pMixFormat);
        channel_mask = pExt->dwChannelMask;
        std::cout << "  Extensible Subtype: " << guid_to_string(pExt->SubFormat) << "\n";
        std::cout << "  Channel Mask:       0x" << std::hex << channel_mask << std::dec << "\n";
        if (pExt->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
            is_float = true;
        }
    }
    std::cout << "  Interpreted As:   " << (is_float ? "IEEE-754 32-bit Float" : "PCM Integer") << "\n\n";

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
        pDevice->Release();
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
        pDevice->Release();
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
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    REFERENCE_TIME defPeriod = 0, minPeriod = 0;
    pAudioClient->GetDevicePeriod(&defPeriod, &minPeriod);
    std::cout << "  Actual Device Period: " << (defPeriod / 10000.0) << " ms\n\n";

    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cerr << "[FATAL] IAudioClient::Start failed: 0x" << std::hex << hr << "\n";
        pCaptureClient->Release();
        CloseHandle(hAudioEvent);
        CoTaskMemFree(pMixFormat);
        pAudioClient->Release();
        pDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    std::cout << "================================================================================\n";
    std::cout << "                     HUMAN OPERATOR PROTOCOL (" << duration_sec << " SECONDS)                     \n";
    std::cout << "================================================================================\n";
    std::cout << "  0–3 sec:   SILENCE (measuring baseline noise floor)\n";
    std::cout << "  3–6 sec:   SPEAK NORMALLY (\"VANI, this is a microphone signal test\")\n";
    std::cout << "  6–8 sec:   SILENCE\n";
    std::cout << "  8–10 sec:  SPEAK LOUDLY (\"VANI!\")\n";
    std::cout << "================================================================================\n\n";

    UINT32 channels = pMixFormat->nChannels;
    UINT32 sample_rate = pMixFormat->nSamplesPerSec;

    auto t_start = std::chrono::steady_clock::now();
    uint64_t total_frames_captured = 0;
    int last_instruction_phase = -1;

    // Per-channel 100ms accumulators
    std::vector<std::vector<float>> ch_accum(channels);
    std::vector<float> mono_accum;
    std::vector<float> diff_accum;

    // Overall tracking
    double baseline_sum_rms = 0.0;
    size_t baseline_count = 0;
    float max_observed_rms_ch0 = 0.0f;
    float max_observed_rms_ch1 = 0.0f;
    float max_observed_rms_mono = 0.0f;
    float max_observed_rms_diff = 0.0f;

    auto last_100ms_report = std::chrono::steady_clock::now();

    while (true) {
        DWORD waitRes = WaitForSingleObject(hAudioEvent, 200);
        if (waitRes != WAIT_OBJECT_0) {
            // Check timeout or loop exit
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<double>(now - t_start).count() >= duration_sec) break;
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        double elapsed_s = std::chrono::duration<double>(now - t_start).count();
        if (elapsed_s >= duration_sec) break;

        // Drain available packets
        UINT32 packetLength = 0;
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        while (SUCCEEDED(hr) && packetLength > 0) {
            BYTE* pData = nullptr;
            UINT32 numFramesRead = 0;
            DWORD dwFlags = 0;
            hr = pCaptureClient->GetBuffer(&pData, &numFramesRead, &dwFlags, nullptr, nullptr);
            if (FAILED(hr)) break;

            bool is_silent_flag = (dwFlags & AUDCLNT_BUFFERFLAGS_SILENT);
            total_frames_captured += numFramesRead;

            for (UINT32 i = 0; i < numFramesRead; ++i) {
                float ch0_s = 0.0f;
                float ch1_s = 0.0f;

                for (UINT32 c = 0; c < channels; ++c) {
                    float val = 0.0f;
                    if (!is_silent_flag && pData) {
                        if (is_float) {
                            val = reinterpret_cast<const float*>(pData)[i * channels + c];
                        } else if (pMixFormat->wBitsPerSample == 16) {
                            val = reinterpret_cast<const int16_t*>(pData)[i * channels + c] / 32768.0f;
                        } else if (pMixFormat->wBitsPerSample == 24) {
                            // 24-bit in 3 bytes
                            const BYTE* sample_ptr = pData + (i * channels + c) * 3;
                            int32_t sample24 = (sample_ptr[0] << 8) | (sample_ptr[1] << 16) | (sample_ptr[2] << 24);
                            val = static_cast<float>(sample24) / 2147483648.0f;
                        }
                    }
                    ch_accum[c].push_back(val);
                    if (c == 0) ch0_s = val;
                    if (c == 1) ch1_s = val;
                }

                // Downmix calculations
                float mono_val = (channels >= 2) ? ((ch0_s + ch1_s) * 0.5f) : ch0_s;
                float diff_val = (channels >= 2) ? ((ch0_s - ch1_s) * 0.5f) : 0.0f;
                mono_accum.push_back(mono_val);
                diff_accum.push_back(diff_val);
            }

            pCaptureClient->ReleaseBuffer(numFramesRead);
            hr = pCaptureClient->GetNextPacketSize(&packetLength);
        }

        // Report every 100ms
        auto report_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_100ms_report).count();
        if (report_elapsed_ms >= 100 && !ch_accum[0].empty()) {
            last_100ms_report = now;

            // Phase indicator
            int current_phase = 0;
            if (elapsed_s < 3.0) current_phase = 1;
            else if (elapsed_s < 6.0) current_phase = 2;
            else if (elapsed_s < 8.0) current_phase = 3;
            else current_phase = 4;

            if (current_phase != last_instruction_phase) {
                last_instruction_phase = current_phase;
                std::cout << "\n>>> ";
                switch (current_phase) {
                    case 1: std::cout << "ACTION [0–3s]: SILENCE (baseline noise floor)"; break;
                    case 2: std::cout << "ACTION [3–6s]: SPEAK NORMALLY (\"VANI, this is a microphone signal test\")"; break;
                    case 3: std::cout << "ACTION [6–8s]: SILENCE"; break;
                    case 4: std::cout << "ACTION [8–10s]: SPEAK LOUDLY (\"VANI!\")"; break;
                }
                std::cout << " <<<\n";
            }

            // Compute metrics
            auto calc_rms_peak = [](const std::vector<float>& vec, float& out_rms, float& out_peak, float& out_min, float& out_max) {
                if (vec.empty()) { out_rms = 0; out_peak = 0; out_min = 0; out_max = 0; return; }
                double sum_sq = 0.0;
                out_peak = 0.0f;
                out_min = vec[0];
                out_max = vec[0];
                for (float s : vec) {
                    sum_sq += static_cast<double>(s * s);
                    float a = std::abs(s);
                    if (a > out_peak) out_peak = a;
                    if (s < out_min) out_min = s;
                    if (s > out_max) out_max = s;
                }
                out_rms = static_cast<float>(std::sqrt(sum_sq / vec.size()));
            };

            float rms0 = 0, peak0 = 0, min0 = 0, max0 = 0;
            calc_rms_peak(ch_accum[0], rms0, peak0, min0, max0);
            if (rms0 > max_observed_rms_ch0) max_observed_rms_ch0 = rms0;

            float rms1 = 0, peak1 = 0, min1 = 0, max1 = 0;
            if (channels >= 2) {
                calc_rms_peak(ch_accum[1], rms1, peak1, min1, max1);
                if (rms1 > max_observed_rms_ch1) max_observed_rms_ch1 = rms1;
            }

            float rms_mono = 0, peak_mono = 0, min_m = 0, max_m = 0;
            calc_rms_peak(mono_accum, rms_mono, peak_mono, min_m, max_m);
            if (rms_mono > max_observed_rms_mono) max_observed_rms_mono = rms_mono;

            float rms_diff = 0, peak_diff = 0, min_d = 0, max_d = 0;
            calc_rms_peak(diff_accum, rms_diff, peak_diff, min_d, max_d);
            if (rms_diff > max_observed_rms_diff) max_observed_rms_diff = rms_diff;

            if (elapsed_s < 2.8) {
                baseline_sum_rms += rms0;
                baseline_count++;
            }

            std::cout << "[T+" << std::fixed << std::setprecision(1) << std::setw(4) << elapsed_s << "s] "
                      << "Frames: " << std::setw(6) << total_frames_captured << " | "
                      << std::fixed << std::setprecision(6)
                      << "Ch0_RMS: " << rms0 << " (Pk: " << peak0 << ") | ";
            if (channels >= 2) {
                std::cout << "Ch1_RMS: " << rms1 << " (Pk: " << peak1 << ") | "
                          << "Mono_Avg: " << rms_mono << " | "
                          << "Diff: " << rms_diff << " | ";
            }
            if (rms0 >= 0.010000f || rms_mono >= 0.010000f) {
                std::cout << "** SPEECH DETECTED **";
            } else if (rms0 >= 0.003000f) {
                std::cout << "elevated";
            } else {
                std::cout << "silence/ambient";
            }
            std::cout << "\n";

            // Clear accumulators for next 100ms
            for (auto& v : ch_accum) v.clear();
            mono_accum.clear();
            diff_accum.clear();
        }
    }

    pAudioClient->Stop();

    float baseline_rms = baseline_count > 0 ? static_cast<float>(baseline_sum_rms / baseline_count) : 0.0001f;

    std::cout << "\n================================================================================\n";
    std::cout << "              RAW WASAPI CAPTURE FORENSIC SUMMARY                               \n";
    std::cout << "================================================================================\n";
    std::cout << "Total Frames Captured:  " << total_frames_captured << "\n";
    std::cout << "Expected Sample Rate:   " << sample_rate << " Hz\n";
    std::cout << "Channels:               " << channels << "\n";
    std::cout << "Baseline Noise RMS:     " << std::fixed << std::setprecision(6) << baseline_rms << "\n";
    std::cout << "Max Ch0 RMS:            " << max_observed_rms_ch0 << "\n";
    if (channels >= 2) {
        std::cout << "Max Ch1 RMS:            " << max_observed_rms_ch1 << "\n";
        std::cout << "Max Mono (Avg) RMS:     " << max_observed_rms_mono << "\n";
        std::cout << "Max Diff (Ch0-Ch1) RMS: " << max_observed_rms_diff << "\n";
    }
    std::cout << "================================================================================\n";

    pCaptureClient->Release();
    CloseHandle(hAudioEvent);
    CoTaskMemFree(pMixFormat);
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    return 0;
}
