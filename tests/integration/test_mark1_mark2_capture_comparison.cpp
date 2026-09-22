#define INITGUID
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#include "../../audio/input/miniaudio_audio_input.hpp"
#include "../../audio/ring_buffer.hpp"
#include "../../audio/audio_format.hpp"
#include "../../adapters/vad/real_silero_vad_adapter.hpp"
#include "../../adapters/wakeword/real_sherpa_kws_adapter.hpp"

#include "../../audio/miniaudio/miniaudio.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <span>
#include <algorithm>
#include <string>

using namespace vani;

struct WindowMetrics {
    double sum_sq{0.0};
    uint64_t count{0};
    float peak{0.0f};
    float min_val{1e9f};
    float max_val{-1e9f};

    void add_sample(float s) {
        sum_sq += static_cast<double>(s * s);
        count++;
        float abs_s = std::abs(s);
        if (abs_s > peak) peak = abs_s;
        if (s < min_val) min_val = s;
        if (s > max_val) max_val = s;
    }

    [[nodiscard]] float rms() const {
        if (count == 0) return 0.0f;
        return static_cast<float>(std::sqrt(sum_sq / static_cast<double>(count)));
    }
};

static std::string render_meter(float rms, int width = 16) {
    float normalized = std::clamp(rms / 0.050000f, 0.0f, 1.0f);
    int filled = static_cast<int>(normalized * width);
    std::string m = "[";
    for (int i = 0; i < width; ++i) {
        m += (i < filled) ? "#" : ".";
    }
    m += "]";
    return m;
}

int main(int argc, char* argv[]) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "================================================================================\n";
    std::cout << "    VANI MARK 1 vs MARK 2 FORENSIC MICROPHONE CAPTURE COMPARISON                \n";
    std::cout << "================================================================================\n";
    std::cout << "  Simultaneous Physical Capture from SAME hardware endpoint:                    \n";
    std::cout << "    Path A: Mark 1 Capture Style (Channel 0 Direct, unmixed, PortAudio-style)   \n";
    std::cout << "    Path B: Mark 2 Capture Style (Miniaudio WASAPI 16kHz Mono Downmixed)        \n";
    std::cout << "================================================================================\n\n";

    int duration_sec = 10;
    if (argc > 1) {
        try { duration_sec = std::stoi(argv[1]); } catch (...) {}
    }

    // 1. Initialize COM
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cerr << "[FATAL] CoInitialize failed: 0x" << std::hex << hr << "\n";
        return 1;
    }

    // 2. Query Windows Core Audio Endpoint Information
    IMMDeviceEnumerator* pEnumerator = nullptr;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
        std::cerr << "[FATAL] Failed to create MMDeviceEnumerator: 0x" << std::hex << hr << "\n";
        CoUninitialize();
        return 1;
    }

    IMMDevice* pDevice = nullptr;
    hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &pDevice);
    if (FAILED(hr) || !pDevice) {
        std::cerr << "[FATAL] Failed to get default capture endpoint: 0x" << std::hex << hr << "\n";
        pEnumerator->Release();
        CoUninitialize();
        return 1;
    }

    std::string friendly_name = "Unknown Microphone";
    IPropertyStore* pStore = nullptr;
    if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pStore)) && pStore) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        if (SUCCEEDED(pStore->GetValue(PKEY_Device_FriendlyName, &pv)) && pv.pwszVal) {
            int sz = WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, NULL, 0, NULL, NULL);
            friendly_name.resize(sz);
            WideCharToMultiByte(CP_UTF8, 0, pv.pwszVal, -1, &friendly_name[0], sz, NULL, NULL);
            if (!friendly_name.empty() && friendly_name.back() == '\0') friendly_name.pop_back();
        }
        PropVariantClear(&pv);
        pStore->Release();
    }

    IAudioEndpointVolume* pEndpointVolume = nullptr;
    pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&pEndpointVolume);
    float master_vol = 0.0f;
    BOOL is_muted = FALSE;
    if (pEndpointVolume) {
        pEndpointVolume->GetMasterVolumeLevelScalar(&master_vol);
        pEndpointVolume->GetMute(&is_muted);
        pEndpointVolume->Release();
    }

    std::cout << "[HARDWARE ENDPOINT TELEMETRY]\n";
    std::cout << "  Endpoint Name:    " << friendly_name << "\n";
    std::cout << "  Endpoint Mute:    " << (is_muted ? "MUTED (CRITICAL)" : "UNMUTED (OK)") << "\n";
    std::cout << "  Endpoint Volume:  " << std::fixed << std::setprecision(1) << (master_vol * 100.0f) << "%\n\n";

    // 3. Initialize Raw WASAPI Capture (Path A Hardware Stream: 48kHz, 2ch, Float32)
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
    pAudioClient->GetMixFormat(&pMixFormat);
    std::cout << "[RAW WASAPI STREAM 1 - NATIVE CAPTURE]\n";
    std::cout << "  Native Rate:      " << pMixFormat->nSamplesPerSec << " Hz\n";
    std::cout << "  Native Channels:  " << pMixFormat->nChannels << "\n";
    std::cout << "  Bits Per Sample:  " << pMixFormat->wBitsPerSample << "\n\n";

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
        return 1;
    }

    HANDLE hAudioEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    pAudioClient->SetEventHandle(hAudioEvent);

    IAudioCaptureClient* pCaptureClient = nullptr;
    pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    pAudioClient->Start();

    // 4. Initialize Miniaudio Audio Input (Path B: Mark 2 production capture, 16kHz mono Float32)
    std::cout << "[MINIAUDIO STREAM 2 - MARK 2 PRODUCTION CAPTURE]\n";
    audio::AudioFormat mic_fmt{.sample_rate = 16000, .channels = 1, .format = audio::SampleFormat::Float32};
    auto mark2_mic = std::make_shared<audio::MiniaudioAudioInput>(mic_fmt, 64000);

    // Instrument Ring Buffer & Callback metrics
    std::mutex mark2_mutex;
    std::vector<float> mark2_recent_samples;
    uint64_t mark2_total_samples = 0;
    double mark2_callback_sum_sq = 0.0;
    uint64_t mark2_callback_count = 0;

    mark2_mic->set_callback([&](std::span<const float> samples, const audio::AudioFrame& /*meta*/) {
        std::lock_guard<std::mutex> lock(mark2_mutex);
        for (float s : samples) {
            mark2_callback_sum_sq += static_cast<double>(s * s);
            mark2_callback_count++;
            mark2_recent_samples.push_back(s);
        }
        mark2_total_samples += samples.size();
    });

    auto start_res = mark2_mic->start();
    if (!start_res.is_ok()) {
        std::cerr << "[FATAL] Failed to start Mark 2 Miniaudio: " << start_res.error().message << "\n";
        return 1;
    }
    std::cout << "  Miniaudio Device: " << mark2_mic->device_name() << "\n";
    std::cout << "  Requested Format: 16,000 Hz, 1 Channel (Mono), Float32\n";
    std::cout << "  Status:           STREAMING CONCURRENTLY\n\n";

    // 5. Initialize Resampler for Stage-by-Stage Forensic Testing (48kHz -> 16kHz)
    ma_resampler_config resampler_cfg = ma_resampler_config_init(
        ma_format_f32,
        1,       // 1 channel
        48000,   // sampleRateIn
        16000,   // sampleRateOut
        ma_resample_algorithm_linear
    );
    ma_resampler ch0_resampler;
    ma_result ma_res = ma_resampler_init(&resampler_cfg, NULL, &ch0_resampler);
    if (ma_res != MA_SUCCESS) {
        std::cerr << "[WARN] Failed to init ma_resampler for Ch0: " << ma_res << "\n";
    }

    ma_resampler avg_resampler;
    ma_res = ma_resampler_init(&resampler_cfg, NULL, &avg_resampler);

    // 6. Initialize Silero VAD for Stage Testing
    adapters::vad::RealSileroVADAdapter vad;
    bool vad_ok = vad.is_model_loaded();
    std::cout << "  Silero VAD Loaded: " << (vad_ok ? "YES" : "NO") << "\n";

    // 7. Initialize Sherpa KWS for Stage Testing
    audio::wakeword::WakeWordConfig kws_cfg;
    kws_cfg.enabled = true;
    adapters::wakeword::RealSherpaKwsAdapter kws(adapters::wakeword::RealSherpaKwsAdapter::Options{}, kws_cfg);
    bool kws_ok = kws.is_healthy();
    std::cout << "  Sherpa KWS Loaded: " << (kws_ok ? "YES" : "NO") << "\n\n";

    // 8. Human Operator Protocol Instructions
    std::cout << "================================================================================\n";
    std::cout << "         10-SECOND COMPARISON PROTOCOL (SPEAK CLEARLY INTO MIC)                 \n";
    std::cout << "================================================================================\n";
    std::cout << "  0–3 sec:   SILENCE       (Measuring ambient noise floor)\n";
    std::cout << "  3–6 sec:   NORMAL SPEECH (Say: \"VANI, test microphone signal comparison\")\n";
    std::cout << "  6–8 sec:   SILENCE       (Measuring trailing silence)\n";
    std::cout << "  8–10 sec:  LOUD SPEECH   (Say: \"VANI, wake up!\")\n";
    std::cout << "================================================================================\n\n";

    // Trackers for the whole session and specific windows
    // Windows: 0=All, 1=Silence1 (0-3s), 2=SpeechNormal (3-6s), 3=Silence2 (6-8s), 4=SpeechLoud (8-10s)
    WindowMetrics wm_ch0[5];
    WindowMetrics wm_ch1[5];
    WindowMetrics wm_stereo[5];
    WindowMetrics wm_mono_avg[5];
    WindowMetrics wm_mono_diff[5];
    WindowMetrics wm_mono_max[5];
    WindowMetrics wm_resample_ch0[5];
    WindowMetrics wm_resample_avg[5];
    WindowMetrics wm_mark2_cb[5];
    WindowMetrics wm_mark2_ring[5];

    uint64_t vad_speech_frames_count = 0;
    uint64_t vad_total_frames_count = 0;
    float kws_max_score = 0.0f;
    uint64_t kws_detections = 0;

    auto t_start = std::chrono::steady_clock::now();
    auto last_100ms = t_start;
    int last_phase = -1;

    std::cout << std::left
              << std::setw(8)  << "Time"
              << std::setw(12) << "48k_Ch0"
              << std::setw(12) << "48k_Ch1"
              << std::setw(12) << "48k_Avg"
              << std::setw(12) << "48k_Diff"
              << std::setw(12) << "16k_Ch0"
              << std::setw(12) << "Mark2_16k"
              << std::setw(18) << "Signal Level"
              << "Protocol Status\n";
    std::cout << "--------------------------------------------------------------------------------------------------------\n";

    while (true) {
        DWORD waitRes = WaitForSingleObject(hAudioEvent, 50);
        auto now = std::chrono::steady_clock::now();
        double elapsed_s = std::chrono::duration<double>(now - t_start).count();
        if (elapsed_s >= duration_sec) break;

        int phase = 0;
        if (elapsed_s < 3.0) phase = 1;       // Silence 1
        else if (elapsed_s < 6.0) phase = 2;  // Normal speech
        else if (elapsed_s < 8.0) phase = 3;  // Silence 2
        else phase = 4;                       // Loud speech

        if (phase != last_phase) {
            last_phase = phase;
            std::cout << ">>> ";
            switch (phase) {
                case 1: std::cout << "ACTION [0s-3s]: REMAIN SILENT (measuring noise floor)"; break;
                case 2: std::cout << "ACTION [3s-6s]: SPEAK NORMALLY: \"VANI, test microphone signal comparison\""; break;
                case 3: std::cout << "ACTION [6s-8s]: REMAIN SILENT"; break;
                case 4: std::cout << "ACTION [8s-10s]: SPEAK LOUDLY: \"VANI, wake up!\""; break;
            }
            std::cout << " <<<\n";
        }

        // Drain Raw WASAPI packets
        UINT32 packetLength = 0;
        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        std::vector<float> batch_ch0;
        std::vector<float> batch_avg;

        while (SUCCEEDED(hr) && packetLength > 0) {
            BYTE* pData = nullptr;
            UINT32 numFramesRead = 0;
            DWORD dwFlags = 0;
            hr = pCaptureClient->GetBuffer(&pData, &numFramesRead, &dwFlags, nullptr, nullptr);
            if (FAILED(hr)) break;

            bool is_silent = (dwFlags & AUDCLNT_BUFFERFLAGS_SILENT);
            const float* fData = reinterpret_cast<const float*>(pData);

            for (UINT32 i = 0; i < numFramesRead; ++i) {
                float c0 = 0.0f;
                float c1 = 0.0f;
                if (!is_silent && fData) {
                    c0 = fData[i * 2 + 0];
                    c1 = fData[i * 2 + 1];
                }

                float s_avg = (c0 + c1) * 0.5f;
                float s_diff = (c0 - c1) * 0.5f;
                float s_max = (std::abs(c0) >= std::abs(c1)) ? c0 : c1;

                batch_ch0.push_back(c0);
                batch_avg.push_back(s_avg);

                // Add to trackers
                wm_ch0[0].add_sample(c0);
                wm_ch0[phase].add_sample(c0);

                wm_ch1[0].add_sample(c1);
                wm_ch1[phase].add_sample(c1);

                wm_stereo[0].add_sample(c0);
                wm_stereo[0].add_sample(c1);
                wm_stereo[phase].add_sample(c0);
                wm_stereo[phase].add_sample(c1);

                wm_mono_avg[0].add_sample(s_avg);
                wm_mono_avg[phase].add_sample(s_avg);

                wm_mono_diff[0].add_sample(s_diff);
                wm_mono_diff[phase].add_sample(s_diff);

                wm_mono_max[0].add_sample(s_max);
                wm_mono_max[phase].add_sample(s_max);
            }

            pCaptureClient->ReleaseBuffer(numFramesRead);
            hr = pCaptureClient->GetNextPacketSize(&packetLength);
        }

        // Resample batch_ch0 and batch_avg (48k -> 16k)
        if (!batch_ch0.empty()) {
            ma_uint64 inFrames = batch_ch0.size();
            ma_uint64 outFramesExpected = (inFrames * 16000 + 47999) / 48000 + 16;
            std::vector<float> resampled_ch0(outFramesExpected);
            ma_uint64 actualIn = inFrames;
            ma_uint64 actualOut = outFramesExpected;
            ma_resampler_process_pcm_frames(&ch0_resampler, batch_ch0.data(), &actualIn, resampled_ch0.data(), &actualOut);

            for (size_t i = 0; i < actualOut; ++i) {
                float s = resampled_ch0[i];
                wm_resample_ch0[0].add_sample(s);
                wm_resample_ch0[phase].add_sample(s);
            }

            actualIn = inFrames;
            actualOut = outFramesExpected;
            std::vector<float> resampled_avg(outFramesExpected);
            ma_resampler_process_pcm_frames(&avg_resampler, batch_avg.data(), &actualIn, resampled_avg.data(), &actualOut);

            for (size_t i = 0; i < actualOut; ++i) {
                float s = resampled_avg[i];
                wm_resample_avg[0].add_sample(s);
                wm_resample_avg[phase].add_sample(s);
            }

            // Feed resampled Ch0 into VAD and KWS to see if acoustic engines trigger!
            if (vad_ok && actualOut >= 512) {
                for (size_t offset = 0; offset + 512 <= actualOut; offset += 512) {
                    std::span<const float> vad_frame(resampled_ch0.data() + offset, 512);
                    auto vr = vad.process(vad_frame);
                    vad_total_frames_count++;
                    if (vr.is_speech) vad_speech_frames_count++;
                }
            }
            if (kws_ok && actualOut > 0) {
                std::span<const float> kws_frame(resampled_ch0.data(), actualOut);
                auto kr = kws.process(kws_frame);
                if (kr.confidence > kws_max_score) kws_max_score = kr.confidence;
                if (kr.detected) kws_detections++;
            }
        }

        // Drain Mark 2 Miniaudio recent samples & Ring Buffer read
        std::vector<float> m2_samples;
        {
            std::lock_guard<std::mutex> lock(mark2_mutex);
            m2_samples.swap(mark2_recent_samples);
        }
        for (float s : m2_samples) {
            wm_mark2_cb[0].add_sample(s);
            wm_mark2_cb[phase].add_sample(s);
        }

        // Read from Mark 2's underlying ring buffer
        std::vector<float> ring_read_buf(512);
        size_t n_read = mark2_mic->ring_buffer().read(std::span<float>(ring_read_buf));
        for (size_t i = 0; i < n_read; ++i) {
            float s = ring_read_buf[i];
            wm_mark2_ring[0].add_sample(s);
            wm_mark2_ring[phase].add_sample(s);
        }

        // Telemetry row every 100ms
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_100ms).count() >= 100) {
            last_100ms = now;
            float r_ch0 = wm_ch0[phase].rms();
            float r_ch1 = wm_ch1[phase].rms();
            float r_avg = wm_mono_avg[phase].rms();
            float r_diff = wm_mono_diff[phase].rms();
            float r_16k_ch0 = wm_resample_ch0[phase].rms();
            float r_m2 = wm_mark2_cb[phase].rms();

            std::cout << std::left
                      << std::setw(8)  << (std::to_string(elapsed_s).substr(0, 4) + "s")
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_ch0
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_ch1
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_avg
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_diff
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_16k_ch0
                      << std::setw(12) << std::fixed << std::setprecision(6) << r_m2
                      << std::setw(18) << render_meter(r_ch0)
                      << (phase == 1 ? "Silence 1 (Noise Floor)" :
                          phase == 2 ? "Speech Normal" :
                          phase == 3 ? "Silence 2" : "Speech Loud")
                      << "\n";
        }
    }

    // Stop streams
    mark2_mic->stop();
    pAudioClient->Stop();
    ma_resampler_uninit(&ch0_resampler, NULL);
    ma_resampler_uninit(&avg_resampler, NULL);

    // ========================================================================
    // FORENSIC REPORT GENERATION
    // ========================================================================
    std::cout << "\n================================================================================\n";
    std::cout << "                  FORENSIC REPORT: STEP 3 - MEASUREMENT RESULTS                 \n";
    std::cout << "================================================================================\n\n";

    auto print_channel_report = [&](const std::string& name, const WindowMetrics wm_arr[5]) {
        float noise_floor = wm_arr[1].rms();
        float speech_norm = wm_arr[2].rms();
        float speech_loud = wm_arr[4].rms();
        float max_speech = std::max(speech_norm, speech_loud);
        float snr_ratio = (noise_floor > 1e-7f) ? (max_speech / noise_floor) : 0.0f;
        float snr_db = (snr_ratio > 0.0f) ? (20.0f * std::log10(snr_ratio)) : 0.0f;

        std::cout << "[" << name << "]\n";
        std::cout << "  Overall RMS:       " << std::fixed << std::setprecision(6) << wm_arr[0].rms() << "\n";
        std::cout << "  Peak:              " << std::fixed << std::setprecision(6) << wm_arr[0].peak << "\n";
        std::cout << "  Min:               " << std::fixed << std::setprecision(6) << wm_arr[0].min_val << "\n";
        std::cout << "  Max:               " << std::fixed << std::setprecision(6) << wm_arr[0].max_val << "\n";
        std::cout << "  Noise Floor (0-3s):" << std::fixed << std::setprecision(6) << noise_floor << "\n";
        std::cout << "  Normal Speech RMS: " << std::fixed << std::setprecision(6) << speech_norm << "\n";
        std::cout << "  Loud Speech RMS:   " << std::fixed << std::setprecision(6) << speech_loud << "\n";
        std::cout << "  Speech/Noise Ratio:" << std::fixed << std::setprecision(2) << snr_ratio << "x ("
                  << snr_db << " dB)\n\n";
    };

    print_channel_report("Channel 0 (Native 48kHz Left / Mark 1 style tap)", wm_ch0);
    print_channel_report("Channel 1 (Native 48kHz Right)", wm_ch1);
    print_channel_report("Mono Downmixed Average ((Ch0 + Ch1) * 0.5)", wm_mono_avg);
    print_channel_report("Channel Difference ((Ch0 - Ch1) * 0.5)", wm_mono_diff);
    print_channel_report("Resampled 16kHz Channel 0 (Mark 1 exact path)", wm_resample_ch0);
    print_channel_report("Resampled 16kHz Downmix Average", wm_resample_avg);
    print_channel_report("Mark 2 Production Miniaudio Capture (WASAPI 16k Mono)", wm_mark2_cb);
    print_channel_report("Mark 2 Ring Buffer Read Stream", wm_mark2_ring);

    // STEP 4 DIAGNOSTIC TABLE
    std::cout << "================================================================================\n";
    std::cout << "    STEP 4 — NATIVE HARDWARE FORMAT MULTI-STAGE DIAGNOSTIC TABLE                \n";
    std::cout << "================================================================================\n";
    std::cout << std::left
              << std::setw(30) << "Stage"
              << std::setw(16) << "Noise RMS (0-3s)"
              << std::setw(16) << "Normal Speech RMS"
              << std::setw(16) << "Loud Speech RMS"
              << "Speech/Noise Ratio\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    auto print_stage_row = [](const std::string& stage_name, const WindowMetrics arr[5]) {
        float nf = arr[1].rms();
        float sp_norm = arr[2].rms();
        float sp_loud = arr[4].rms();
        float max_sp = std::max(sp_norm, sp_loud);
        float ratio = (nf > 1e-7f) ? (max_sp / nf) : 0.0f;
        std::cout << std::left
                  << std::setw(30) << stage_name
                  << std::setw(16) << std::fixed << std::setprecision(6) << nf
                  << std::setw(16) << std::fixed << std::setprecision(6) << sp_norm
                  << std::setw(16) << std::fixed << std::setprecision(6) << sp_loud
                  << std::fixed << std::setprecision(2) << ratio << "x\n";
    };

    print_stage_row("Native 48k Ch0", wm_ch0);
    print_stage_row("Native 48k Ch1", wm_ch1);
    print_stage_row("Native 48k stereo", wm_stereo);
    print_stage_row("48k mono (Avg)", wm_mono_avg);
    print_stage_row("16k mono (Resampled Ch0)", wm_resample_ch0);
    print_stage_row("16k mono (Resampled Avg)", wm_resample_avg);
    print_stage_row("VANI ring buffer", wm_mark2_ring);
    print_stage_row("VAD input", wm_resample_ch0);
    print_stage_row("KWS input", wm_resample_ch0);
    print_stage_row("Whisper input", wm_resample_ch0);
    std::cout << "--------------------------------------------------------------------------------\n\n";

    // STEP 5 CHANNEL ROUTING TEST
    std::cout << "================================================================================\n";
    std::cout << "    STEP 5 — CHANNEL ROUTING COMPARISON TABLE                                   \n";
    std::cout << "================================================================================\n";
    std::cout << std::left
              << std::setw(25) << "Routing Strategy"
              << std::setw(16) << "Speech RMS"
              << std::setw(16) << "Noise Floor"
              << std::setw(16) << "SNR (Linear)"
              << "Contains Real Voice Signal?\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    auto print_routing_row = [](const std::string& routing, const WindowMetrics arr[5]) {
        float nf = arr[1].rms();
        float sp = std::max(arr[2].rms(), arr[4].rms());
        float snr = (nf > 1e-7f) ? (sp / nf) : 0.0f;
        std::string verdict = (snr >= 2.0f && sp > 0.003f) ? "YES (HEALTHY)" :
                              (snr >= 1.5f) ? "WEAK (ATTENUATED)" : "NO (PHASE CANCEL / SUPPRESSED)";
        std::cout << std::left
                  << std::setw(25) << routing
                  << std::setw(16) << std::fixed << std::setprecision(6) << sp
                  << std::setw(16) << std::fixed << std::setprecision(6) << nf
                  << std::setw(16) << std::fixed << std::setprecision(2) << snr
                  << verdict << "\n";
    };

    print_routing_row("Left only (Ch0)", wm_ch0);
    print_routing_row("Right only (Ch1)", wm_ch1);
    print_routing_row("Average ((L+R)*0.5)", wm_mono_avg);
    print_routing_row("Difference ((L-R)*0.5)", wm_mono_diff);
    print_routing_row("Max(abs(L), abs(R))", wm_mono_max);
    std::cout << "--------------------------------------------------------------------------------\n\n";

    // STEP 8 RING BUFFER CHECK
    std::cout << "================================================================================\n";
    std::cout << "    STEP 8 — RING BUFFER CONTINUITY CHECK                                       \n";
    std::cout << "================================================================================\n";
    std::cout << "  Miniaudio Callback RMS:  " << std::fixed << std::setprecision(6) << wm_mark2_cb[0].rms() << "\n";
    std::cout << "  Ring Buffer Read RMS:    " << std::fixed << std::setprecision(6) << wm_mark2_ring[0].rms() << "\n";
    float ring_ratio = (wm_mark2_cb[0].rms() > 1e-7f) ? (wm_mark2_ring[0].rms() / wm_mark2_cb[0].rms()) : 1.0f;
    std::cout << "  Ring Write->Read Ratio:  " << std::fixed << std::setprecision(4) << ring_ratio << "x\n";
    std::cout << "  Ring Buffer Loss:        " << ((std::abs(1.0f - ring_ratio) < 0.05f) ? "NONE (PASS)" : "AMPLITUDE LOSS DETECTED (FAIL)") << "\n\n";

    // ACOUSTIC ENGINES (VAD & KWS) ON RECOVERED CH0 STREAM
    std::cout << "================================================================================\n";
    std::cout << "    ACOUSTIC DETECTION ON RECOVERED CH0 AUDIO STREAM                             \n";
    std::cout << "================================================================================\n";
    std::cout << "  VAD Total Frames:        " << vad_total_frames_count << "\n";
    std::cout << "  VAD Speech Frames:       " << vad_speech_frames_count << "\n";
    std::cout << "  VAD Detection Ratio:     " << (vad_total_frames_count > 0 ? (static_cast<double>(vad_speech_frames_count) / vad_total_frames_count * 100.0) : 0.0) << "%\n";
    std::cout << "  Sherpa KWS Max Score:    " << std::fixed << std::setprecision(4) << kws_max_score << "\n";
    std::cout << "  Sherpa KWS Detections:   " << kws_detections << "\n";
    std::cout << "================================================================================\n";

    // Clean up
    pCaptureClient->Release();
    pAudioClient->Release();
    pDevice->Release();
    pEnumerator->Release();
    CloseHandle(hAudioEvent);
    CoUninitialize();

    return 0;
}
