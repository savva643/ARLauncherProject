#pragma once
// GlaskiOS sensor stream protocol v2 (iPhone ARStreamer -> PC).
// Описание: docs/PROTOCOL_V2.md. Должно совпадать с ARStreamer/StreamProtocol.swift.
//
// Пакет: [type u8][sequence u64 BE][payloadSize u32 BE][payload]
// Поля payload — little-endian. Время — наносекунды host clock iPhone.

#include <cstdint>
#include <cstring>

namespace sensorstream {

enum class PacketType : uint8_t {
    // v2
    Hello = 0x20,
    Video = 0x21,
    Imu = 0x22,
    Depth = 0x23,
    Audio = 0x24,    // микрофон: PCM16
    EyeImage = 0x25, // ИК-камера глаза (модуль айтрекинга шлема): GRAY8
    DisplayRequest = 0x26, // телефон -> ПК: «верни картинку лаунчера» (режим, размер, FOV)
    ReturnAck = 0x27,      // телефон -> ПК: кадр показан (задержка, запрос keyframe)
    // ПК -> телефон
    ReturnVideo = 0x30,    // готовый кадр лаунчера (H.264/HEVC) + поза для доворота на телефоне
    // legacy (старый ARStreamer)
    LegacyJpeg = 0x01,
    LegacyLidarDepth = 0x02,
    LegacyImu = 0x03,
    LegacyOrientation = 0x0A,
};

enum class VideoCodec : uint8_t {
    H264 = 1,
    HEVC = 2,
    JPEG = 3,
};

enum class ImuKind : uint8_t {
    Gyro = 0,   // rad/s
    Accel = 1,  // m/s^2, specific force (в покое вверх = +9.81)
    Mag = 2,    // uT
};

constexpr size_t kPacketHeaderSize = 13;
constexpr size_t kVideoHeaderSize = 48;
// Флаги VIDEO (байт 1)
constexpr uint8_t kVideoFlagKeyframe = 1u << 0;
constexpr uint8_t kVideoFlagColorInfo = 1u << 1;   // биты 2–3 валидны
constexpr uint8_t kVideoFlagFullRange = 1u << 2;   // YUV 0..255 (иначе 16..235)
constexpr uint8_t kVideoFlagBT709 = 1u << 3;       // матрица BT.709 (иначе BT.601)
constexpr size_t kImuHeaderSize = 16;
constexpr size_t kImuRecordSize = 24;
constexpr size_t kDepthHeaderSize = 32;
// AUDIO: [tNs i64 — первый сэмпл][sampleRate u32][channels u8][format u8 = 1 (PCM16 LE)][reserved u16] + сэмплы
constexpr size_t kAudioHeaderSize = 16;
constexpr uint8_t kAudioFormatPcm16 = 1;
// EYE: [tNs i64][eye u8: 0 левый, 1 правый][format u8 = 1 (GRAY8)][width u16][height u16][reserved u16] + пиксели
constexpr size_t kEyeHeaderSize = 16;
constexpr uint8_t kEyeFormatGray8 = 1;
// ---- Обратный поток: картинка лаунчера на экран телефона (docs/PROTOCOL_V2.md, «Возврат картинки»)
enum class ReturnMode : uint8_t {
    Off = 0,
    Mono = 1,        // один вид на весь экран (как «окно» в AR)
    Stereo = 2,      // два глаза side-by-side без линз
    StereoLens = 3,  // два глаза + коррекция линз VR-очков (делает телефон, после доворота)
};
// DISPLAY (телефон -> ПК), 40 байт:
// [mode u8][flags u8: бит0 своя камера телефона (ПК шлёт только объекты + альфа), бит1 HEVC]
// [width u16][height u16][fps u16][bitrate kbit/s u32][tanX f32][tanY f32 — полуугол глаза, 0 = решает ПК]
// [ipd м f32][частота экрана Гц f32][reserved u32][время телефона i64]
constexpr size_t kDisplayRequestSize = 40;
constexpr uint8_t kDisplayFlagLocalCamera = 1u << 0;
constexpr uint8_t kDisplayFlagHevc = 1u << 1;
// ACK (телефон -> ПК), 32 байта:
// [frameId u32][flags u8: бит0 нужен keyframe][3 reserved][tReceived i64][tDisplay i64][decodeUs u32][reserved u32]
constexpr size_t kReturnAckSize = 32;
constexpr uint8_t kAckFlagNeedKeyframe = 1u << 0;
// RETURN_VIDEO (ПК -> телефон): 128 байт заголовка + Annex-B
//   0 codec u8 | 1 flags u8 (бит0 keyframe, бит1 альфа снизу, бит2 full range, бит3 BT.709) | 2 layout u8 (1 моно, 2 SBS)
//   3 mode u8 | 4 width u16 | 6 height u16 (всё изображение, с альфой) | 8 frameId u32 | 12 reserved u32
//  16 tRef i64 — время IMU, от которого предсказана поза | 24 tTarget i64 — на когда предсказана
//  32 tCamera i64 — PTS кадра камеры в фоне (0 — нет) | 40 tSend i64 — отправка (часы телефона, 0 — неизвестно)
//  48 qPredFromRef f32x4 (x,y,z,w) — поворот устройства tRef -> tTarget (оси устройства)
//  64 eyeFromDevice f32x4 | 80 cameraFromDevice f32x4 (камера OpenCV)
//  96 gyroBias f32x3 рад/с | 108 overscan f32 (рендер шире показа во столько раз по tan)
// 112 tanX f32, 116 tanY f32 — полууглы отрендеренного глаза | 120 ipd f32 м | 124 reserved
constexpr size_t kReturnVideoHeaderSize = 128;
constexpr uint8_t kReturnFlagKeyframe = 1u << 0;
constexpr uint8_t kReturnFlagAlpha = 1u << 1;
constexpr uint8_t kReturnFlagFullRange = 1u << 2;
constexpr uint8_t kReturnFlagBT709 = 1u << 3;
constexpr uint32_t kMaxPayloadSize = 64u * 1024u * 1024u;

// --- little-endian readers (x86/ARM хосты little-endian, но не полагаемся на выравнивание)
inline uint16_t rdU16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }
inline uint32_t rdU32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline int64_t rdI64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i) v = (v << 8) | p[i];
    return static_cast<int64_t>(v);
}
inline float rdF32(const uint8_t* p) {
    uint32_t u = rdU32(p);
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}
inline double rdF64(const uint8_t* p) {
    uint64_t u = static_cast<uint64_t>(rdI64(p));
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
inline uint32_t rdU32BE(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
inline uint64_t rdU64BE(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}

// --- writers (для симулятора / тестов)
inline void wrU16(uint8_t* p, uint16_t v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
inline void wrU32(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); }
inline void wrI64(uint8_t* p, int64_t v) {
    uint64_t u = static_cast<uint64_t>(v);
    for (int i = 0; i < 8; ++i) p[i] = uint8_t(u >> (8 * i));
}
inline void wrF32(uint8_t* p, float f) { uint32_t u; std::memcpy(&u, &f, 4); wrU32(p, u); }
inline void wrU32BE(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = uint8_t(v >> (24 - 8 * i)); }
inline void wrU64BE(uint8_t* p, uint64_t v) { for (int i = 0; i < 8; ++i) p[i] = uint8_t(v >> (56 - 8 * i)); }

// IEEE 754 half -> float
inline float halfToFloat(uint16_t h) {
    uint32_t sign = uint32_t(h & 0x8000) << 16;
    uint32_t exp = (h >> 10) & 0x1F;
    uint32_t mant = h & 0x3FF;
    uint32_t bits;
    if (exp == 0) {
        if (mant == 0) {
            bits = sign;
        } else {
            exp = 127 - 15 + 1;
            while ((mant & 0x400) == 0) { mant <<= 1; --exp; }
            mant &= 0x3FF;
            bits = sign | (exp << 23) | (mant << 13);
        }
    } else if (exp == 31) {
        bits = sign | 0x7F800000u | (mant << 13);
    } else {
        bits = sign | ((exp - 15 + 127) << 23) | (mant << 13);
    }
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}

} // namespace sensorstream
