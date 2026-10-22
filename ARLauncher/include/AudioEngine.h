#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

/**
 * @brief Пространственный звук GlaskiOS (OpenAL Soft + HRTF).
 *
 * Звук привязан к точкам в мире рендера (Y вверх, взгляд в начале по -Z, метры). Слушатель — голова
 * (поза из трекинга, с предсказанием), поэтому при повороте головы источник остаётся на месте.
 * HRTF даёт направление (в т.ч. спереди/сзади/сверху) в обычных наушниках. Если реальная стена
 * (LiDAR) между головой и источником — звук глуше (фильтр низких частот) и тише.
 *
 * Без OpenAL (не найден при сборке) или без аудиоустройства — всё молча выключено.
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool initialize(bool hrtf = true);
    void shutdown();
    bool active() const { return m_active; }
    bool hrtfActive() const { return m_hrtf; }
    const std::string& deviceName() const { return m_deviceName; }

    /// Источник из моно PCM16; возвращает id (>= 0) или -1
    int addSource(const std::vector<int16_t>& mono, int sampleRate, bool loop, float gain);
    void setSourcePosition(int id, const glm::vec3& position);
    /// 0 — прямая видимость, 1 — за стеной (глухо и тише)
    void setSourceOcclusion(int id, float occlusion);
    void setMuted(bool muted);
    bool muted() const { return m_muted; }

    /// Поток без пространства (микрофон в наушники): сэмплы моно PCM16; задержка ограничена ~120 мс
    void queueStream(const int16_t* samples, size_t count, int sampleRate);
    void setStreamEnabled(bool enabled);
    bool streamEnabled() const { return m_streamEnabled; }

    /// Голова: позиция и R_world_from_eye (оси глаза: x вправо, y вверх, взгляд -z)
    void setListener(const glm::vec3& position, const glm::quat& orientation);

    // Процедурные звуки для демо-сцены (без файлов)
    static std::vector<int16_t> makeHum(int sampleRate, float seconds);
    static std::vector<int16_t> makeChime(int sampleRate, float periodSeconds);

private:
    struct Source {
        unsigned int source = 0;
        unsigned int buffer = 0;
        unsigned int filter = 0;
        float gain = 1.0f;
    };
    bool m_active = false;
    bool m_hrtf = false;
    bool m_efx = false;
    bool m_muted = false;
    std::string m_deviceName;
    void* m_device = nullptr;
    void* m_context = nullptr;
    std::vector<Source> m_sources;
    std::mutex m_streamMutex;
    bool m_streamEnabled = false;
    unsigned int m_streamSource = 0;
    std::vector<unsigned int> m_streamFree;
};

/// Запись WAV (моно PCM16) — для микрофона
class WavWriter {
public:
    ~WavWriter() { close(); }
    bool open(const std::string& path, int sampleRate);
    void write(const int16_t* samples, size_t count);
    void close();
    bool isOpen() const { return m_file != nullptr; }
    const std::string& path() const { return m_path; }

private:
    std::FILE* m_file = nullptr;
    std::string m_path;
    uint32_t m_samples = 0;
    int m_rate = 16000;
};

#endif // AUDIOENGINE_H
