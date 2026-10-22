#include "AudioEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

#ifdef USE_OPENAL
#define AL_ALEXT_PROTOTYPES
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/efx.h>
#endif

namespace {
constexpr float kTwoPi = 6.28318530718f;
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine()
{
    shutdown();
}

bool AudioEngine::initialize(bool hrtf)
{
#ifdef USE_OPENAL
    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device) {
        std::cout << "[AUDIO] no audio device — spatial audio off" << std::endl;
        return false;
    }
    std::vector<ALCint> attrs;
    if (alcIsExtensionPresent(device, "ALC_SOFT_HRTF")) {
        attrs = {ALC_HRTF_SOFT, hrtf ? ALC_TRUE : ALC_FALSE, 0};
    }
    ALCcontext* context = alcCreateContext(device, attrs.empty() ? nullptr : attrs.data());
    if (!context || !alcMakeContextCurrent(context)) {
        if (context) alcDestroyContext(context);
        alcCloseDevice(device);
        std::cout << "[AUDIO] cannot create OpenAL context — spatial audio off" << std::endl;
        return false;
    }
    m_device = device;
    m_context = context;
    if (const ALCchar* name = alcGetString(device, ALC_DEVICE_SPECIFIER)) m_deviceName = name;
    if (alcIsExtensionPresent(device, "ALC_SOFT_HRTF")) {
        ALCint status = 0;
        alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &status);
        m_hrtf = status == ALC_TRUE;
    }
    m_efx = alcIsExtensionPresent(device, "ALC_EXT_EFX") == ALC_TRUE;
    // Затухание как в реальности: 1/r от 0.5 м, но не тише 1/8 (иначе далёкое пропадает совсем)
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    m_active = true;
    std::cout << "[AUDIO] OpenAL '" << m_deviceName << "', HRTF " << (m_hrtf ? "on" : "off (stereo panning)")
              << ", occlusion filter " << (m_efx ? "on" : "off") << std::endl;
    return true;
#else
    (void)hrtf;
    std::cout << "[AUDIO] built without OpenAL (sudo apt install libopenal-dev) — spatial audio off" << std::endl;
    return false;
#endif
}

void AudioEngine::shutdown()
{
#ifdef USE_OPENAL
    if (!m_active) return;
    setStreamEnabled(false);
    for (auto& s : m_sources) {
        alSourceStop(s.source);
        alDeleteSources(1, &s.source);
        alDeleteBuffers(1, &s.buffer);
        if (s.filter) alDeleteFilters(1, &s.filter);
    }
    m_sources.clear();
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(static_cast<ALCcontext*>(m_context));
    alcCloseDevice(static_cast<ALCdevice*>(m_device));
    m_context = m_device = nullptr;
    m_active = false;
#endif
}

int AudioEngine::addSource(const std::vector<int16_t>& mono, int sampleRate, bool loop, float gain)
{
#ifdef USE_OPENAL
    if (!m_active || mono.empty()) return -1;
    Source s;
    s.gain = gain;
    alGenBuffers(1, &s.buffer);
    alBufferData(s.buffer, AL_FORMAT_MONO16, mono.data(), ALsizei(mono.size() * sizeof(int16_t)), sampleRate);
    alGenSources(1, &s.source);
    alSourcei(s.source, AL_BUFFER, ALint(s.buffer));
    alSourcei(s.source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcef(s.source, AL_GAIN, m_muted ? 0.0f : gain);
    alSourcef(s.source, AL_REFERENCE_DISTANCE, 0.5f);
    alSourcef(s.source, AL_MAX_DISTANCE, 4.0f);
    alSourcef(s.source, AL_ROLLOFF_FACTOR, 1.0f);
    if (m_efx) {
        alGenFilters(1, &s.filter);
        alFilteri(s.filter, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
        alFilterf(s.filter, AL_LOWPASS_GAIN, 1.0f);
        alFilterf(s.filter, AL_LOWPASS_GAINHF, 1.0f);
        alSourcei(s.source, AL_DIRECT_FILTER, ALint(s.filter));
    }
    alSourcePlay(s.source);
    m_sources.push_back(s);
    return int(m_sources.size()) - 1;
#else
    (void)mono; (void)sampleRate; (void)loop; (void)gain;
    return -1;
#endif
}

void AudioEngine::setSourcePosition(int id, const glm::vec3& p)
{
#ifdef USE_OPENAL
    if (!m_active || id < 0 || id >= int(m_sources.size())) return;
    alSource3f(m_sources[size_t(id)].source, AL_POSITION, p.x, p.y, p.z);
#else
    (void)id; (void)p;
#endif
}

void AudioEngine::setSourceOcclusion(int id, float occlusion)
{
#ifdef USE_OPENAL
    if (!m_active || id < 0 || id >= int(m_sources.size())) return;
    Source& s = m_sources[size_t(id)];
    occlusion = std::clamp(occlusion, 0.0f, 1.0f);
    if (s.filter) {
        // За стеной: высокие частоты почти пропадают, общий уровень — вдвое ниже
        alFilterf(s.filter, AL_LOWPASS_GAIN, 1.0f - 0.5f * occlusion);
        alFilterf(s.filter, AL_LOWPASS_GAINHF, 1.0f - 0.9f * occlusion);
        alSourcei(s.source, AL_DIRECT_FILTER, ALint(s.filter));
    } else {
        alSourcef(s.source, AL_GAIN, m_muted ? 0.0f : s.gain * (1.0f - 0.6f * occlusion));
    }
#else
    (void)id; (void)occlusion;
#endif
}

void AudioEngine::setMuted(bool muted)
{
    m_muted = muted;
#ifdef USE_OPENAL
    if (!m_active) return;
    for (auto& s : m_sources) alSourcef(s.source, AL_GAIN, muted ? 0.0f : s.gain);
#endif
}

void AudioEngine::setStreamEnabled(bool enabled)
{
#ifdef USE_OPENAL
    std::lock_guard<std::mutex> lock(m_streamMutex);
    if (!m_active || enabled == m_streamEnabled) return;
    if (enabled) {
        alGenSources(1, &m_streamSource);
        alSourcei(m_streamSource, AL_SOURCE_RELATIVE, AL_TRUE);   // в голове, без пространства
        alSource3f(m_streamSource, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSourcef(m_streamSource, AL_ROLLOFF_FACTOR, 0.0f);
        m_streamFree.resize(8);
        alGenBuffers(8, m_streamFree.data());
    } else {
        alSourceStop(m_streamSource);
        ALint queued = 0;
        alGetSourcei(m_streamSource, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0) {
            ALuint b = 0;
            alSourceUnqueueBuffers(m_streamSource, 1, &b);
            m_streamFree.push_back(b);
        }
        alDeleteSources(1, &m_streamSource);
        alDeleteBuffers(ALsizei(m_streamFree.size()), m_streamFree.data());
        m_streamFree.clear();
        m_streamSource = 0;
    }
    m_streamEnabled = enabled;
#else
    (void)enabled;
#endif
}

void AudioEngine::queueStream(const int16_t* samples, size_t count, int sampleRate)
{
#ifdef USE_OPENAL
    std::lock_guard<std::mutex> lock(m_streamMutex);
    if (!m_active || !m_streamEnabled || !samples || count == 0) return;
    ALint processed = 0;
    alGetSourcei(m_streamSource, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0) {
        ALuint b = 0;
        alSourceUnqueueBuffers(m_streamSource, 1, &b);
        m_streamFree.push_back(b);
    }
    if (m_streamFree.empty()) return;   // отстаём — выбрасываем пачку, задержка не копится
    const ALuint b = m_streamFree.back();
    m_streamFree.pop_back();
    alBufferData(b, AL_FORMAT_MONO16, samples, ALsizei(count * sizeof(int16_t)), sampleRate);
    alSourceQueueBuffers(m_streamSource, 1, &b);
    ALint state = 0, queued = 0;
    alGetSourcei(m_streamSource, AL_SOURCE_STATE, &state);
    alGetSourcei(m_streamSource, AL_BUFFERS_QUEUED, &queued);
    if (state != AL_PLAYING && queued >= 2) alSourcePlay(m_streamSource);
#else
    (void)samples; (void)count; (void)sampleRate;
#endif
}

void AudioEngine::setListener(const glm::vec3& position, const glm::quat& orientation)
{
#ifdef USE_OPENAL
    if (!m_active) return;
    const glm::vec3 at = orientation * glm::vec3(0.0f, 0.0f, -1.0f);
    const glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);
    const ALfloat ori[6] = {at.x, at.y, at.z, up.x, up.y, up.z};
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListenerfv(AL_ORIENTATION, ori);
#else
    (void)position; (void)orientation;
#endif
}

std::vector<int16_t> AudioEngine::makeHum(int sampleRate, float seconds)
{
    // Тёплый гул: 110 + 165 + 220 Гц с медленной пульсацией (целое число периодов — петля без щелчка)
    const size_t n = size_t(sampleRate * seconds);
    std::vector<int16_t> out(n);
    for (size_t i = 0; i < n; ++i) {
        const float t = float(i) / float(sampleRate);
        const float am = 0.75f + 0.25f * std::sin(kTwoPi * 0.5f * t);
        const float v = 0.45f * std::sin(kTwoPi * 110.0f * t) + 0.25f * std::sin(kTwoPi * 165.0f * t) +
                        0.15f * std::sin(kTwoPi * 220.0f * t) + 0.08f * std::sin(kTwoPi * 880.0f * t);
        out[i] = int16_t(std::clamp(v * am, -1.0f, 1.0f) * 20000.0f);
    }
    return out;
}

std::vector<int16_t> AudioEngine::makeChime(int sampleRate, float periodSeconds)
{
    // Колокольчик раз в periodSeconds: несколько негармоничных обертонов с затуханием
    const size_t n = size_t(sampleRate * periodSeconds);
    std::vector<int16_t> out(n);
    const float partials[4] = {880.0f, 1320.0f * 1.01f, 2093.0f, 2637.0f * 0.99f};
    const float amps[4] = {0.5f, 0.25f, 0.15f, 0.1f};
    for (size_t i = 0; i < n; ++i) {
        const float t = float(i) / float(sampleRate);
        float v = 0.0f;
        for (int k = 0; k < 4; ++k) v += amps[k] * std::exp(-t * (2.5f + 1.5f * k)) * std::sin(kTwoPi * partials[k] * t);
        const float attack = std::min(1.0f, t / 0.003f);
        out[i] = int16_t(std::clamp(v * attack, -1.0f, 1.0f) * 24000.0f);
    }
    return out;
}

bool WavWriter::open(const std::string& path, int sampleRate)
{
    close();
    m_file = std::fopen(path.c_str(), "wb");
    if (!m_file) return false;
    m_path = path;
    m_rate = sampleRate;
    m_samples = 0;
    uint8_t header[44] = {};
    std::fwrite(header, 1, sizeof(header), m_file);   // заполняется при закрытии
    return true;
}

void WavWriter::write(const int16_t* samples, size_t count)
{
    if (!m_file) return;
    std::fwrite(samples, sizeof(int16_t), count, m_file);
    m_samples += uint32_t(count);
}

void WavWriter::close()
{
    if (!m_file) return;
    auto u32 = [](uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); };
    auto u16 = [](uint8_t* p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); };
    uint8_t h[44];
    std::memcpy(h, "RIFF", 4);
    u32(h + 4, 36 + m_samples * 2);
    std::memcpy(h + 8, "WAVEfmt ", 8);
    u32(h + 16, 16);
    u16(h + 20, 1);
    u16(h + 22, 1);
    u32(h + 24, uint32_t(m_rate));
    u32(h + 28, uint32_t(m_rate) * 2);
    u16(h + 32, 2);
    u16(h + 34, 16);
    std::memcpy(h + 36, "data", 4);
    u32(h + 40, m_samples * 2);
    std::fseek(m_file, 0, SEEK_SET);
    std::fwrite(h, 1, sizeof(h), m_file);
    std::fclose(m_file);
    m_file = nullptr;
}
