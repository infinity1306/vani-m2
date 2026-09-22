#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <propkey.h>
#include <functiondiscoverykeys_devpkey.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <cstring>
#include <ctime>

#pragma pack(push, 1)
struct WavHeader {
    char riff_tag[4] = {'R', 'I', 'F', 'F'};
    uint32_t riff_length{0};
    char wave_tag[4] = {'W', 'A', 'V', 'E'};
    char fmt_tag[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_length{16};
    uint16_t audio_format{3}; // 3 = IEEE_FLOAT
    uint16_t num_channels{2};
    uint32_t sample_rate{48000};
    uint32_t byte_rate{48000 * 2 * sizeof(float)};
    uint16_t block_align{2 * sizeof(float)};
    uint16_t bits_per_sample{32};
    char data_tag[4] = {'d', 'a', 't', 'a'};
    uint32_t data_length{0};
};
#pragma pack(pop)

static std::string get_wall_time_str() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm_buf;
    localtime_s(&tm_buf, &now_c);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03lld", tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, ms.count());
    return std::string(buf);
}

// 100ms block statistics
struct IntervalStats {
    float rms{0.0f};
    float peak{0.0f};
    float min_val{0.0f};
    float max_val{0.0f};
    float zcr{0.0f};
    float non_zero_pct{0.0f};

    static IntervalStats compute(const float* data, size_t count, size_t stride, size_t offset) {
        IntervalStats s;
        if (count == 0) return s;

        double sum_sq = 0.0;
        float peak_v = 0.0f;
        float min_v = 1e9f;
        float max_v = -1e9f;
        size_t non_zero = 0;
        size_t zc = 0;

        float prev = 0.0f;
        bool has_prev = false;

        for (size_t i = 0; i < count; ++i) {
            float v = data[i * stride + offset];
            sum_sq += static_cast<double>(v * v);
            float a = std::abs(v);
            if (a > peak_v) peak_v = a;
            if (v < min_v) min_v = v;
            if (v > max_v) max_v = v;
            if (a > 1e-7f) non_zero++;

            if (has_prev) {
                if ((v > 0.0f && prev < 0.0f) || (v < 0.0f && prev > 0.0f)) {
                    zc++;
                }
            }
            prev = v;
            has_prev = true;
        }

        s.rms = static_cast<float>(std::sqrt(sum_sq / static_cast<double>(count)));
        s.peak = peak_v;
        s.min_val = min_v;
        s.max_val = max_v;
        s.non_zero_pct = (static_cast<float>(non_zero) / static_cast<float>(count)) * 100.0f;
        s.zcr = (count > 1) ? (static_cast<float>(zc) / static_cast<float>(count - 1)) : 0.0f;
        return s;
    }
};

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "    VANI MARK 2 — PHASE 7C.1: DEFINITIVE MICROPHONE CAPTURE RECORDING           \n";
    std::cout << "================================================================================\n\n";

    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] CoInitialize failed (0x" << std::hex << hr << ")\n";
        return 1;
    }

    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        std::cerr << "[FATAL] CoCreateInstance(MMDeviceEnumerator) failed\n";
        CoUninitialize();
        return 1;
    }

    // Locate "Microphone Array (Realtek(R) Audio)"
    IMMDevice* pTargetDevice = nullptr;
    IMMDeviceCollection* pCol = nullptr;
    std::string selected_name = "";
    std::string selected_id = "";

    if (SUCCEEDED(pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCol)) && pCol) {
        UINT count = 0;
        pCol->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDev = nullptr;
            if (SUCCEEDED(pCol->Item(i, &pDev)) && pDev) {
                IPropertyStore* pStore = nullptr;
                pDev->OpenPropertyStore(STGM_READ, &pStore);
                if (pStore) {
                    PROPVARIANT pv;
                    PropVariantInit(&pv);
                    if (SUCCEEDED(pStore->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.pwszVal) {
                        int sz = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, NULL, 0, NULL, NULL);
                        std::string fname(sz, 0);
                        WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, &fname[0], sz, NULL, NULL);
                        if (!fname.empty() && fname.back() == '\0') fname.pop_back();

                        if (fname.find("Microphone Array (Realtek(R) Audio)") != std::string::npos ||
                            fname.find("Microphone Array") != std::string::npos) {
                            pTargetDevice = pDev;
                            pTargetDevice->AddRef();
                            selected_name = fname;
                            LPWSTR pId = nullptr;
                            pDev->GetId(&pId);
                            if (pId) {
                                int id_sz = WideCharToMultiByte(CP_UTF8, 0, pId, -1, NULL, 0, NULL, NULL);
                                selected_id.resize(id_sz);
                                WideCharToMultiByte(CP_UTF8, 0, pId, -1, &selected_id[0], id_sz, NULL, NULL);
                                if (!selected_id.empty() && selected_id.back() == '\0') selected_id.pop_back();
                                CoTaskMemFree(pId);
                            }
                            PropVariantClear(&pv);
                            pStore->Release();
                            pDev->Release();
                            break;
                        }
                    }
                    PropVariantClear(&pv);
                    pStore->Release();
                }
                pDev->Release();
            }
        }
        pCol->Release();
    }

    if (!pTargetDevice) {
        hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pTargetDevice);
        if (FAILED(hr) || !pTargetDevice) {
            std::cerr << "[FATAL] Failed to obtain physical capture endpoint\n";
            pEnumerator->Release();
            CoUninitialize();
            return 1;
        }
        selected_name = "Default Capture Endpoint";
    }

    // Inspect Endpoint Volume and Mute
    IAudioEndpointVolume* pVol = nullptr;
    BOOL is_muted = FALSE;
    float master_vol = 0.0f;
    float master_vol_db = 0.0f;
    if (SUCCEEDED(pTargetDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pVol)) && pVol) {
        pVol->GetMute(&is_muted);
        pVol->GetMasterVolumeLevelScalar(&master_vol);
        pVol->GetMasterVolumeLevel(&master_vol_db);
        pVol->Release();
    }

    std::cout << "Physical Endpoint:     " << selected_name << "\n";
    std::cout << "Endpoint ID:           " << selected_id << "\n";
    std::cout << "IAudioEndpointVolume:  " << (is_muted ? "MUTED [WARNING]" : "UNMUTED [HEALTHY]") << "\n";
    std::cout << "Windows Input Volume:  " << std::fixed << std::setprecision(1) << (master_vol * 100.0f) << "% ("
              << std::setprecision(2) << master_vol_db << " dB)\n\n";

    // Initialize Audio Client
    IAudioClient* pClient = nullptr;
    hr = pTargetDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient);
    if (FAILED(hr) || !pClient) {
        std::cerr << "[FATAL] Failed to activate IAudioClient\n";
        pTargetDevice->Release();
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    WAVEFORMATEX* pMixFormat = nullptr;
    pClient->GetMixFormat(&pMixFormat);

    std::cout << "[NATIVE WASAPI FORMAT]\n";
    std::cout << "  Sample Rate:         " << pMixFormat->nSamplesPerSec << " Hz\n";
    std::cout << "  Channels:            " << pMixFormat->nChannels << "\n";
    std::cout << "  Bits Per Sample:     " << pMixFormat->wBitsPerSample << " bits\n";
    std::cout << "  Format Tag:          0x" << std::hex << pMixFormat->wFormatTag << std::dec << "\n\n";

    REFERENCE_TIME hnsBuffer = 10000000; // 1s
    hr = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsBuffer, 0, pMixFormat, nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] IAudioClient::Initialize failed: 0x" << std::hex << hr << "\n";
        return 1;
    }

    HANDLE hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    pClient->SetEventHandle(hEvent);

    IAudioCaptureClient* pCapture = nullptr;
    pClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCapture);

    hr = pClient->Start();
    if (FAILED(hr)) {
        std::cerr << "[FATAL] IAudioClient::Start failed\n";
        return 1;
    }

    std::cout << "================================================================================\n";
    std::cout << "                 30-SECOND HUMAN RECORDING PROTOCOL                             \n";
    std::cout << "================================================================================\n";
    std::cout << "   0–5 sec   SILENCE\n";
    std::cout << "   5–10 sec  Say: \"VANI this is a microphone test\"\n";
    std::cout << "  10–15 sec  SILENCE\n";
    std::cout << "  15–20 sec  Say continuously: \"VANI VANI VANI VANI\"\n";
    std::cout << "  20–25 sec  SILENCE\n";
    std::cout << "  25–30 sec  Say normally: \"VANI Chrome kholo\"\n";
    std::cout << "================================================================================\n\n";

    std::cout << "Starting countdown:\n";
    for (int c = 3; c > 0; --c) {
        std::cout << "  [" << c << "] Prepare for Phase 1 (SILENCE)... [Clock: " << get_wall_time_str() << "]\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "  >>> RECORDING ENGAGED NOW! <<<\n\n";

    // Storage for the full 30 seconds of raw stereo Float32 samples
    // 30 sec * 48000 frames/sec * 2 channels = 2,880,000 float samples (~11.5 MB)
    std::vector<float> recorded_pcm;
    recorded_pcm.reserve(30 * 48000 * 2);

    uint64_t total_buffer_calls = 0;
    uint64_t silent_flag_buffers = 0;

    auto t_start = std::chrono::steady_clock::now();
    auto last_telemetry_time = t_start;
    int last_phase_announced = -1;

    // Rolling 100ms accumulator
    std::vector<float> rolling_100ms;

    while (true) {
        DWORD waitRes = WaitForSingleObject(hEvent, 200);
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - t_start).count();
        if (elapsed >= 30.0) break;

        int phase = 1;
        if (elapsed < 5.0) phase = 1;
        else if (elapsed < 10.0) phase = 2;
        else if (elapsed < 15.0) phase = 3;
        else if (elapsed < 20.0) phase = 4;
        else if (elapsed < 25.0) phase = 5;
        else phase = 6;

        if (phase != last_phase_announced) {
            last_phase_announced = phase;
            std::cout << "\n>>> [T=" << std::fixed << std::setprecision(1) << elapsed << "s | " << get_wall_time_str() << "] ";
            switch (phase) {
                case 1: std::cout << "PHASE 1/6 (0–5s):   REMAIN SILENT ...\n"; break;
                case 2: std::cout << "PHASE 2/6 (5–10s):  SAY: \"VANI this is a microphone test\"\n"; break;
                case 3: std::cout << "PHASE 3/6 (10–15s): REMAIN SILENT ...\n"; break;
                case 4: std::cout << "PHASE 4/6 (15–20s): SAY CONTINUOUSLY: \"VANI VANI VANI VANI\"\n"; break;
                case 5: std::cout << "PHASE 5/6 (20–25s): REMAIN SILENT ...\n"; break;
                case 6: std::cout << "PHASE 6/6 (25–30s): SAY NORMALLY: \"VANI Chrome kholo\"\n"; break;
            }
        }

        if (waitRes == WAIT_OBJECT_0) {
            BYTE* pData = nullptr;
            UINT32 framesAvailable = 0;
            DWORD flags = 0;

            while (SUCCEEDED(pCapture->GetBuffer(&pData, &framesAvailable, &flags, nullptr, nullptr)) && framesAvailable > 0) {
                total_buffer_calls++;
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    silent_flag_buffers++;
                }

                if (pData) {
                    const float* fData = reinterpret_cast<const float*>(pData);
                    size_t sample_count = framesAvailable * 2;
                    for (size_t i = 0; i < sample_count; ++i) {
                        float s = (flags & AUDCLNT_BUFFERFLAGS_SILENT) ? 0.0f : fData[i];
                        recorded_pcm.push_back(s);
                        rolling_100ms.push_back(s);
                    }
                }
                pCapture->ReleaseBuffer(framesAvailable);
            }
        }

        // 100 ms Telemetry output
        if (std::chrono::duration<double>(now - last_telemetry_time).count() >= 0.100) {
            last_telemetry_time = now;
            size_t frames_100ms = rolling_100ms.size() / 2;
            if (frames_100ms > 0) {
                IntervalStats st0 = IntervalStats::compute(rolling_100ms.data(), frames_100ms, 2, 0);
                IntervalStats st1 = IntervalStats::compute(rolling_100ms.data(), frames_100ms, 2, 1);

                std::cout << "[T+" << std::fixed << std::setprecision(1) << std::setw(4) << elapsed << "s | " << get_wall_time_str() << "] "
                          << "P" << phase << " | "
                          << "Ch0 RMS: " << std::fixed << std::setprecision(5) << st0.rms << " (Pk:" << st0.peak << " ZCR:" << std::setprecision(2) << st0.zcr << ") | "
                          << "Ch1 RMS: " << std::fixed << std::setprecision(5) << st1.rms << " (Pk:" << st1.peak << " ZCR:" << std::setprecision(2) << st1.zcr << ")\n";

                rolling_100ms.clear();
            }
        }
    }

    pClient->Stop();
    pCapture->Release();
    CloseHandle(hEvent);
    CoTaskMemFree(pMixFormat);
    pClient->Release();
    pTargetDevice->Release();
    pEnumerator->Release();
    CoUninitialize();

    std::cout << "\nRecording completed. Total frames captured: " << (recorded_pcm.size() / 2) << "\n";

    // 2. Save RAW WAV
    std::string wav_path = "scratch/mic_forensics_30s.wav";
    std::ofstream wav_file(wav_path, std::ios::binary);
    bool wav_saved = false;
    if (wav_file.is_open()) {
        WavHeader hdr;
        hdr.num_channels = 2;
        hdr.sample_rate = 48000;
        hdr.bits_per_sample = 32;
        hdr.block_align = 2 * sizeof(float);
        hdr.byte_rate = 48000 * 2 * sizeof(float);
        hdr.data_length = static_cast<uint32_t>(recorded_pcm.size() * sizeof(float));
        hdr.riff_length = 36 + hdr.data_length;

        wav_file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        wav_file.write(reinterpret_cast<const char*>(recorded_pcm.data()), recorded_pcm.size() * sizeof(float));
        wav_file.close();
        wav_saved = true;
        std::cout << "RAW WAV successfully saved: " << wav_path << " (" << (hdr.riff_length + 8) << " bytes)\n";
    } else {
        std::cerr << "[ERROR] Failed to save WAV file to " << wav_path << "\n";
    }

    // 5. Offline Analysis over the recorded samples
    size_t total_frames = recorded_pcm.size() / 2;
    size_t sample_rate = 48000;

    auto get_window_samples = [&](double start_sec, double end_sec, int ch) -> std::vector<float> {
        size_t start_frame = static_cast<size_t>(start_sec * sample_rate);
        size_t end_frame = static_cast<size_t>(end_sec * sample_rate);
        if (start_frame >= total_frames) return {};
        if (end_frame > total_frames) end_frame = total_frames;

        std::vector<float> out;
        out.reserve(end_frame - start_frame);
        for (size_t f = start_frame; f < end_frame; ++f) {
            out.push_back(recorded_pcm[f * 2 + ch]);
        }
        return out;
    };

    auto calc_rms = [](const std::vector<float>& s) -> float {
        if (s.empty()) return 0.0f;
        double sum = 0.0;
        for (float v : s) sum += static_cast<double>(v * v);
        return static_cast<float>(std::sqrt(sum / static_cast<double>(s.size())));
    };

    auto calc_peak = [](const std::vector<float>& s) -> float {
        float p = 0.0f;
        for (float v : s) {
            float a = std::abs(v);
            if (a > p) p = a;
        }
        return p;
    };

    auto calc_mean = [](const std::vector<float>& s) -> float {
        if (s.empty()) return 0.0f;
        double sum = std::accumulate(s.begin(), s.end(), 0.0);
        return static_cast<float>(sum / static_cast<double>(s.size()));
    };

    // Protocol Windows
    // W1: 0-5s Silence
    // W2: 5-10s Speech: "VANI this is a microphone test"
    // W3: 10-15s Silence
    // W4: 15-20s VANI: "VANI VANI VANI VANI"
    // W5: 20-25s Silence
    // W6: 25-30s Speech: "VANI Chrome kholo"

    // Extract per-channel window segments
    std::vector<float> ch0_w1 = get_window_samples(0.0, 5.0, 0);
    std::vector<float> ch0_w2 = get_window_samples(5.0, 10.0, 0);
    std::vector<float> ch0_w3 = get_window_samples(10.0, 15.0, 0);
    std::vector<float> ch0_w4 = get_window_samples(15.0, 20.0, 0);
    std::vector<float> ch0_w5 = get_window_samples(20.0, 25.0, 0);
    std::vector<float> ch0_w6 = get_window_samples(25.0, 30.0, 0);

    std::vector<float> ch1_w1 = get_window_samples(0.0, 5.0, 1);
    std::vector<float> ch1_w2 = get_window_samples(5.0, 10.0, 1);
    std::vector<float> ch1_w3 = get_window_samples(10.0, 15.0, 1);
    std::vector<float> ch1_w4 = get_window_samples(15.0, 20.0, 1);
    std::vector<float> ch1_w5 = get_window_samples(20.0, 25.0, 1);
    std::vector<float> ch1_w6 = get_window_samples(25.0, 30.0, 1);

    // Combined Silence (W1, W3, W5)
    std::vector<float> ch0_silence = ch0_w1;
    ch0_silence.insert(ch0_silence.end(), ch0_w3.begin(), ch0_w3.end());
    ch0_silence.insert(ch0_silence.end(), ch0_w5.begin(), ch0_w5.end());

    std::vector<float> ch1_silence = ch1_w1;
    ch1_silence.insert(ch1_silence.end(), ch1_w3.begin(), ch1_w3.end());
    ch1_silence.insert(ch1_silence.end(), ch1_w5.begin(), ch1_w5.end());

    // Combined Normal Speech (W2, W6)
    std::vector<float> ch0_speech = ch0_w2;
    ch0_speech.insert(ch0_speech.end(), ch0_w6.begin(), ch0_w6.end());

    std::vector<float> ch1_speech = ch1_w2;
    ch1_speech.insert(ch1_speech.end(), ch1_w6.begin(), ch1_w6.end());

    // VANI Speech (W4)
    std::vector<float> ch0_vani = ch0_w4;
    std::vector<float> ch1_vani = ch1_w4;

    // Metrics Ch0
    float ch0_sil_rms = calc_rms(ch0_silence);
    float ch0_spk_rms = calc_rms(ch0_speech);
    float ch0_vani_rms = calc_rms(ch0_vani);
    float ch0_peak = std::max({calc_peak(ch0_w2), calc_peak(ch0_w4), calc_peak(ch0_w6)});
    float ch0_snr = (ch0_sil_rms > 0.0f) ? (ch0_spk_rms / ch0_sil_rms) : 0.0f;
    float ch0_vani_snr = (ch0_sil_rms > 0.0f) ? (ch0_vani_rms / ch0_sil_rms) : 0.0f;

    // Metrics Ch1
    float ch1_sil_rms = calc_rms(ch1_silence);
    float ch1_spk_rms = calc_rms(ch1_speech);
    float ch1_vani_rms = calc_rms(ch1_vani);
    float ch1_peak = std::max({calc_peak(ch1_w2), calc_peak(ch1_w4), calc_peak(ch1_w6)});
    float ch1_snr = (ch1_sil_rms > 0.0f) ? (ch1_spk_rms / ch1_sil_rms) : 0.0f;
    float ch1_vani_snr = (ch1_sil_rms > 0.0f) ? (ch1_vani_rms / ch1_sil_rms) : 0.0f;

    // Full 30s Ch0 and Ch1 vectors for cross-channel analysis
    std::vector<float> all_ch0(total_frames);
    std::vector<float> all_ch1(total_frames);
    for (size_t f = 0; f < total_frames; ++f) {
        all_ch0[f] = recorded_pcm[f * 2];
        all_ch1[f] = recorded_pcm[f * 2 + 1];
    }

    float dc_offset_ch0 = calc_mean(all_ch0);
    float dc_offset_ch1 = calc_mean(all_ch1);

    // Pearson Correlation between Ch0 and Ch1
    double sum_cross = 0.0, sum_sq0 = 0.0, sum_sq1 = 0.0;
    for (size_t f = 0; f < total_frames; ++f) {
        double d0 = all_ch0[f] - dc_offset_ch0;
        double d1 = all_ch1[f] - dc_offset_ch1;
        sum_cross += d0 * d1;
        sum_sq0 += d0 * d0;
        sum_sq1 += d1 * d1;
    }
    float channel_corr = (sum_sq0 > 0.0 && sum_sq1 > 0.0) ?
        static_cast<float>(sum_cross / (std::sqrt(sum_sq0) * std::sqrt(sum_sq1))) : 0.0f;
    float channel_energy_ratio = (sum_sq0 > 0.0) ? static_cast<float>(sum_sq1 / sum_sq0) : 0.0f;

    float ch0_contrast_db = (ch0_snr > 0.0f) ? (20.0f * std::log10(ch0_snr)) : 0.0f;
    float ch1_contrast_db = (ch1_snr > 0.0f) ? (20.0f * std::log10(ch1_snr)) : 0.0f;
    float ch0_dr_db = (ch0_sil_rms > 0.0f && ch0_peak > 0.0f) ? (20.0f * std::log10(ch0_peak / ch0_sil_rms)) : 0.0f;
    float ch1_dr_db = (ch1_sil_rms > 0.0f && ch1_peak > 0.0f) ? (20.0f * std::log10(ch1_peak / ch1_sil_rms)) : 0.0f;

    // 6. Test Mono Conversions Offline
    // Stream A: Ch0 only (already computed as ch0_snr)
    // Stream B: Ch1 only (already computed as ch1_snr)
    // Stream C: (Ch0 + Ch1) / 2
    // Stream D: max(abs(Ch0), abs(Ch1)) * sign
    std::vector<float> mono_c_silence, mono_c_speech;
    std::vector<float> mono_d_silence, mono_d_speech;

    for (size_t i = 0; i < ch0_silence.size(); ++i) {
        float s0 = ch0_silence[i];
        float s1 = ch1_silence[i];
        mono_c_silence.push_back(0.5f * (s0 + s1));
        float m = (std::abs(s0) >= std::abs(s1)) ? s0 : s1;
        mono_d_silence.push_back(m);
    }
    for (size_t i = 0; i < ch0_speech.size(); ++i) {
        float s0 = ch0_speech[i];
        float s1 = ch1_speech[i];
        mono_c_speech.push_back(0.5f * (s0 + s1));
        float m = (std::abs(s0) >= std::abs(s1)) ? s0 : s1;
        mono_d_speech.push_back(m);
    }

    float mono_c_sil_rms = calc_rms(mono_c_silence);
    float mono_c_spk_rms = calc_rms(mono_c_speech);
    float mono_c_snr = (mono_c_sil_rms > 0.0f) ? (mono_c_spk_rms / mono_c_sil_rms) : 0.0f;

    float mono_d_sil_rms = calc_rms(mono_d_silence);
    float mono_d_spk_rms = calc_rms(mono_d_speech);
    float mono_d_snr = (mono_d_sil_rms > 0.0f) ? (mono_d_spk_rms / mono_d_sil_rms) : 0.0f;

    // 8. Hardware Mute & Silent Buffer Check
    float silent_pct = (total_buffer_calls > 0) ? (static_cast<float>(silent_flag_buffers) / static_cast<float>(total_buffer_calls)) * 100.0f : 0.0f;

    // 10. Final Channel Decision
    std::string decision;
    std::string best_ch;
    std::string root_cause;

    if (is_muted || silent_pct > 99.0f) {
        decision = "CAPTURE_MUTED";
        best_ch = "NONE";
        root_cause = "Windows Audio Endpoint is MUTED in Windows Core Audio, zeroing all capture frames.";
    } else if (ch1_snr >= 1.5f && ch0_snr < 1.3f) {
        decision = "CHANNEL_1_VALID";
        best_ch = "CH1";
        root_cause = "Hardware microphone array useful speech signal is routed exclusively to Channel 1.";
    } else if (ch0_snr >= 1.5f && ch1_snr < 1.3f) {
        decision = "CHANNEL_0_VALID";
        best_ch = "CH0";
        root_cause = "Hardware microphone array useful speech signal is routed to Channel 0.";
    } else if (ch0_snr >= 1.5f && ch1_snr >= 1.5f) {
        decision = "BOTH_VALID";
        best_ch = (ch1_snr > ch0_snr) ? "CH1" : "CH0";
        root_cause = "Both physical microphone channels capture audible speech with distinct SNR.";
    } else if (ch0_snr < 1.2f && ch1_snr < 1.2f) {
        decision = "NEITHER_VALID";
        best_ch = "NONE";
        root_cause = "Neither channel demonstrates measurable acoustic speech separation above silence floor.";
    } else {
        decision = "INCONCLUSIVE";
        best_ch = "NONE";
        root_cause = "Speech response across windows is marginal or inconsistent.";
    }

    std::cout << "\n================================================================================\n";
    std::cout << "                 DEFINITIVE MICROPHONE TEST FINAL REPORT                        \n";
    std::cout << "================================================================================\n\n";

    std::cout << "RAW WASAPI CAPTURE:\n" << (total_frames > 0 ? "PASS" : "FAIL") << "\n\n";

    std::cout << "SILENT BUFFER FLAG:\n" << std::fixed << std::setprecision(2) << silent_pct << "%\n\n";

    std::cout << "WAV SAVED:\n" << (wav_saved ? "YES" : "NO") << "\n\n";

    std::cout << "CHANNEL 0:\n";
    std::cout << "  silence RMS: " << std::fixed << std::setprecision(6) << ch0_sil_rms << "\n";
    std::cout << "  speech RMS:  " << std::fixed << std::setprecision(6) << ch0_spk_rms << "\n";
    std::cout << "  VANI RMS:    " << std::fixed << std::setprecision(6) << ch0_vani_rms << "\n";
    std::cout << "  peak:        " << std::fixed << std::setprecision(6) << ch0_peak << "\n";
    std::cout << "  SNR:         " << std::fixed << std::setprecision(2) << ch0_snr << "x (VANI SNR: " << ch0_vani_snr << "x)\n\n";

    std::cout << "CHANNEL 1:\n";
    std::cout << "  silence RMS: " << std::fixed << std::setprecision(6) << ch1_sil_rms << "\n";
    std::cout << "  speech RMS:  " << std::fixed << std::setprecision(6) << ch1_spk_rms << "\n";
    std::cout << "  VANI RMS:    " << std::fixed << std::setprecision(6) << ch1_vani_rms << "\n";
    std::cout << "  peak:        " << std::fixed << std::setprecision(6) << ch1_peak << "\n";
    std::cout << "  SNR:         " << std::fixed << std::setprecision(2) << ch1_snr << "x (VANI SNR: " << ch1_vani_snr << "x)\n\n";

    std::cout << "CROSS-CHANNEL METRICS:\n";
    std::cout << "  Channel Correlation:   " << std::fixed << std::setprecision(4) << channel_corr << "\n";
    std::cout << "  Channel Energy Ratio:  " << std::fixed << std::setprecision(2) << channel_energy_ratio << "x (Ch1 / Ch0)\n";
    std::cout << "  Ch0 Contrast / DynR:   " << std::fixed << std::setprecision(2) << ch0_contrast_db << " dB / " << ch0_dr_db << " dB\n";
    std::cout << "  Ch1 Contrast / DynR:   " << std::fixed << std::setprecision(2) << ch1_contrast_db << " dB / " << ch1_dr_db << " dB\n";
    std::cout << "  DC Offset Ch0 / Ch1:   " << std::fixed << std::setprecision(6) << dc_offset_ch0 << " / " << dc_offset_ch1 << "\n\n";

    std::cout << "MONO CONVERSIONS OFFLINE:\n";
    std::cout << "  Stream A (Ch0 only):       SNR: " << std::fixed << std::setprecision(2) << ch0_snr << "x\n";
    std::cout << "  Stream B (Ch1 only):       SNR: " << std::fixed << std::setprecision(2) << ch1_snr << "x\n";
    std::cout << "  Stream C (Mono Average):   SNR: " << std::fixed << std::setprecision(2) << mono_c_snr << "x\n";
    std::cout << "  Stream D (Max Abs Sign):   SNR: " << std::fixed << std::setprecision(2) << mono_d_snr << "x\n\n";

    std::cout << "MARK 1 COMPARISON:\n";
    std::cout << "MARK1_LIVE_COMPARISON_UNAVAILABLE\n\n";

    std::cout << "BEST CHANNEL:\n" << best_ch << "\n\n";
    std::cout << "FINAL CHANNEL DECISION:\n" << decision << "\n\n";
    std::cout << "ROOT CAUSE:\n" << root_cause << "\n";
    std::cout << "================================================================================\n";

    return 0;
}
