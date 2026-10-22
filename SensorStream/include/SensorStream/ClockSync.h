#pragma once

#include <cstdint>
#include <deque>
#include <mutex>

namespace sensorstream {

/// Оценка смещения часов телефон -> ПК по нижней огибающей (pcRecv - phoneSend).
/// Минимум за окно ≈ смещение + минимальная задержка сети (по USB ~0.2–1 мс).
/// Окно скользящее, поэтому медленный дрейф часов отслеживается.
class ClockSync {
public:
    explicit ClockSync(int64_t windowNs = 4'000'000'000LL) : m_window(windowNs) {}

    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_samples.clear();
        m_offset = 0;
        m_valid = false;
    }

    void addSample(int64_t phoneSendNs, int64_t pcRecvNs)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const int64_t diff = pcRecvNs - phoneSendNs;
        // монотонная очередь минимумов
        while (!m_samples.empty() && m_samples.back().diff >= diff) m_samples.pop_back();
        m_samples.push_back({pcRecvNs, diff});
        while (!m_samples.empty() && pcRecvNs - m_samples.front().pcNs > m_window) m_samples.pop_front();
        m_offset = m_samples.front().diff;
        m_valid = true;
    }

    bool valid() const { std::lock_guard<std::mutex> lock(m_mutex); return m_valid; }
    int64_t offsetNs() const { std::lock_guard<std::mutex> lock(m_mutex); return m_offset; }
    int64_t phoneToPc(int64_t phoneNs) const { return phoneNs + offsetNs(); }
    int64_t pcToPhone(int64_t pcNs) const { return pcNs - offsetNs(); }

private:
    struct Entry { int64_t pcNs; int64_t diff; };
    mutable std::mutex m_mutex;
    std::deque<Entry> m_samples;
    int64_t m_window;
    int64_t m_offset = 0;
    bool m_valid = false;
};

} // namespace sensorstream
