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
constexpr size_t kImuHeaderSize = 16;
constexpr size_t kImuRecordSize = 24;
constexpr size_t kDepthHeaderSize = 32;
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
