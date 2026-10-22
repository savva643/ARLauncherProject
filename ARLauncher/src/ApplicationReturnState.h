#pragma once
// Состояние обратного потока (картинка на телефон) — общее для Application.cpp и ApplicationReturn.cpp

#include "Application.h"
#include "SensorStream/ReturnEncoder.h"
#include "SensorStream/Types.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>

struct Application::ReturnState {
    // Запрос телефона (сетевой поток -> главный)
    mutable std::mutex requestMutex;
    sensorstream::DisplayRequest request;
    bool requestChanged = false;

    // Применённый режим (главный поток)
    sensorstream::DisplayRequest active;
    bool on = false;
    bool savedStereo = false;
    bool savedLens = false;
    float savedIpd = 0.063f;
    int savedWidth = 0, savedHeight = 0;
    bool resized = false;
    bool hud = false;                         // ARL_RETURN_HUD=1 — панель статуса и в кадре телефона
    double fixedLeadMs = -1.0;                // ARL_RETURN_LEAD_MS: упреждение вручную (иначе — по ACK телефона)
    float overscan = 1.12f;                   // ARL_RETURN_OVERSCAN: запас под доворот
    std::chrono::steady_clock::time_point lastSubmit{};
    // Кадр между latch и readback
    sensorstream::ReturnFrameMeta meta;
    bool metaValid = false;
    int64_t latchPhoneNs = 0;
    int64_t cameraPhoneNs = 0;

    // Задержка по подтверждениям телефона
    mutable std::mutex statsMutex;
    struct Sent {
        uint32_t id = 0;
        int64_t latchPhoneNs = 0;
        int64_t cameraPhoneNs = 0;
    };
    std::array<Sent, 64> sent{};
    double leadMs = 45.0;                     // от latch до фотонов на телефоне (адаптивно)
    double renderToPhotonMs = 0.0;
    double cameraToPhotonMs = 0.0;
    double decodeMs = 0.0;
    int acks = 0;
    int64_t lastAckPcNs = 0;
};
