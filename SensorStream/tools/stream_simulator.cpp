// Симулятор iPhone ARStreamer (протокол v2) для тестов без телефона.
//
// Генерирует известное движение головы (yaw/pitch синусоиды), сырой IMU 100 Гц с bias и шумом,
// и видео с виртуальной камеры, которая смотрит на процедурную сферу-сетку (горизонт красный).
// Видео отправляется с искусственной задержкой, как с реального телефона, — так видно, что
// репроекция в лаунчере держит мир на месте несмотря на задержку видео.
//
//   stream_simulator [host] [port] [--seconds N] [--latency-ms 45] [--size 1280x960] [--fps 60] [--hevc] [--depth]

#include "SensorStream/Protocol.h"
#include "SensorStream/Types.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <cstring>
#include <atomic>
#include <deque>
#include <mutex>
#include <iostream>
#include <random>
#include <string>
#include <algorithm>
#include <thread>
#include <vector>

using namespace sensorstream;

namespace {

constexpr double kPi = 3.14159265358979323846;
// Часы «телефона» сдвинуты относительно ПК — проверяем ClockSync
constexpr int64_t kPhoneClockOffsetNs = -7'000'000'000LL;

struct Mat3 {
    double m[3][3];
    static Mat3 identity() { return {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}}; }
    Mat3 operator*(const Mat3& b) const {
        Mat3 r{};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = 0;
                for (int k = 0; k < 3; ++k) r.m[i][j] += m[i][k] * b.m[k][j];
            }
        return r;
    }
    Mat3 transposed() const {
        Mat3 r{};
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) r.m[i][j] = m[j][i];
        return r;
    }
    void apply(const double v[3], double out[3]) const {
        for (int i = 0; i < 3; ++i) out[i] = m[i][0] * v[0] + m[i][1] * v[1] + m[i][2] * v[2];
    }
};

Mat3 rotZ(double a) { double c = std::cos(a), s = std::sin(a); return {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}}; }
Mat3 rotX(double a) { double c = std::cos(a), s = std::sin(a); return {{{1, 0, 0}, {0, c, -s}, {0, s, c}}}; }

// Мир: Z вверх (как CoreMotion), смотрим вперёд по +Y.
// Телефон в шлеме горизонтально: ось x устройства вверх, экран к лицу (z назад), камера вперёд (-z).
Mat3 deviceOrientation(double t)
{
    const Mat3 r0 = {{{0, -1, 0}, {0, 0, -1}, {1, 0, 0}}};   // столбцы: x_d=(0,0,1), y_d=(-1,0,0), z_d=(0,-1,0)
    const double yaw = 35.0 * kPi / 180.0 * std::sin(2 * kPi * 0.20 * t);
    const double pitch = 12.0 * kPi / 180.0 * std::sin(2 * kPi * 0.13 * t);
    return rotZ(yaw) * rotX(pitch) * r0;
}

void bodyRate(double t, double out[3])
{
    const double dt = 1e-4;
    Mat3 a = deviceOrientation(t - dt), b = deviceOrientation(t + dt);
    Mat3 m = a.transposed() * b;
    out[0] = (m.m[2][1] - m.m[1][2]) / (4 * dt);
    out[1] = (m.m[0][2] - m.m[2][0]) / (4 * dt);
    out[2] = (m.m[1][0] - m.m[0][1]) / (4 * dt);
}

int64_t phoneNow() { return pcNowNs() + kPhoneClockOffsetNs; }

class Sender {
public:
    bool connectTo(const std::string& host, uint16_t port)
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &a.sin_addr);
        if (::connect(m_fd, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0) return false;
        int one = 1;
        setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        return true;
    }
    bool send(PacketType type, const std::vector<uint8_t>& payload)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint8_t h[kPacketHeaderSize];
        h[0] = static_cast<uint8_t>(type);
        wrU64BE(h + 1, ++m_seq);
        wrU32BE(h + 9, static_cast<uint32_t>(payload.size()));
        return writeAll(h, sizeof(h)) && writeAll(payload.data(), payload.size());
    }
    ~Sender() { if (m_fd >= 0) ::close(m_fd); }

private:
    bool writeAll(const uint8_t* p, size_t n)
    {
        while (n) {
            ssize_t w = ::send(m_fd, p, n, MSG_NOSIGNAL);
            if (w <= 0) return false;
            p += w;
            n -= size_t(w);
        }
        return true;
    }
    std::mutex m_mutex;
    int m_fd = -1;
    uint64_t m_seq = 0;
};

struct PendingVideo {
    int64_t sendAtPc;
    std::vector<uint8_t> payload;
};

} // namespace

int main(int argc, char** argv)
{
    std::string host = "127.0.0.1";
    uint16_t port = 9001;
    double seconds = 30;
    double latencyMs = 45;
    int width = 1280, height = 960, fps = 60;
    bool hevc = false;
    bool depth = false;

    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seconds" && i + 1 < argc) seconds = std::stod(argv[++i]);
        else if (a == "--latency-ms" && i + 1 < argc) latencyMs = std::stod(argv[++i]);
        else if (a == "--fps" && i + 1 < argc) fps = std::stoi(argv[++i]);
        else if (a == "--hevc") hevc = true;
        else if (a == "--depth") depth = true;
        else if (a == "--size" && i + 1 < argc) std::sscanf(argv[++i], "%dx%d", &width, &height);
        else if (positional == 0) { host = a; ++positional; }
        else if (positional == 1) { port = uint16_t(std::stoi(a)); ++positional; }
    }

    Sender net;
    if (!net.connectTo(host, port)) {
        std::cerr << "cannot connect to " << host << ":" << port << std::endl;
        return 1;
    }
    std::cout << "connected to " << host << ":" << port << " " << width << "x" << height << "@" << fps
              << " latency " << latencyMs << " ms" << std::endl;

    // --- encoder
    const AVCodec* codec = nullptr;
    if (hevc) {
        codec = avcodec_find_encoder_by_name("libx265");
        if (!codec) codec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    } else {
        codec = avcodec_find_encoder_by_name("libx264");
        if (!codec) codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        std::cerr << "no encoder" << std::endl;
        return 1;
    }
    AVCodecContext* enc = avcodec_alloc_context3(codec);
    enc->width = width;
    enc->height = height;
    enc->time_base = {1, 1'000'000'000};
    enc->framerate = {fps, 1};
    enc->gop_size = fps;
    enc->max_b_frames = 0;
    enc->pix_fmt = AV_PIX_FMT_YUV420P;
    enc->color_range = AVCOL_RANGE_JPEG;
    enc->colorspace = AVCOL_SPC_BT709;
    enc->bit_rate = 12'000'000;
    av_opt_set(enc->priv_data, "preset", "ultrafast", 0);
    av_opt_set(enc->priv_data, "tune", "zerolatency", 0);
    if (avcodec_open2(enc, codec, nullptr) < 0) {
        std::cerr << "encoder open failed" << std::endl;
        return 1;
    }
    std::cout << "encoder: " << codec->name << std::endl;

    AVFrame* frame = av_frame_alloc();
    frame->format = enc->pix_fmt;
    frame->width = width;
    frame->height = height;
    av_frame_get_buffer(frame, 32);
    AVPacket* pkt = av_packet_alloc();

    // Интринсики виртуальной камеры: HFOV ~ 70°
    const double f = width * 0.5 / std::tan(35.0 * kPi / 180.0);
    const double cx = width * 0.5, cy = height * 0.5;

    // hello
    {
        std::vector<uint8_t> p(16, 0);
        std::memcpy(p.data(), "GLSK", 4);
        wrU16(p.data() + 4, 2);
        wrI64(p.data() + 8, phoneNow());
        net.send(PacketType::Hello, p);
    }

    std::mt19937 rng(42);
    std::normal_distribution<float> gyroNoise(0.0f, 0.002f);
    std::normal_distribution<float> accNoise(0.0f, 0.02f);
    const float gyroBias[3] = {0.012f, -0.006f, 0.009f};

    const int64_t start = pcNowNs();
    const int64_t endNs = start + int64_t(seconds * 1e9);
    std::atomic<bool> ok{true};

    // --- IMU 100 Гц в отдельном потоке: gyro + accel отдельными записями, как на iPhone
    std::thread imuThread([&] {
        const int64_t imuPeriod = 10'000'000;
        int64_t nextImu = start;
        while (ok && pcNowNs() < endNs) {
            const double t = double(nextImu - start) * 1e-9;
            double w[3];
            bodyRate(t, w);
            Mat3 r = deviceOrientation(t);
            const double gWorld[3] = {0, 0, 9.80665};
            double aDev[3];
            r.transposed().apply(gWorld, aDev);
            const int64_t tPhone = nextImu + kPhoneClockOffsetNs;

            std::vector<uint8_t> p(kImuHeaderSize + 2 * kImuRecordSize, 0);
            wrU16(p.data(), 2);
            wrI64(p.data() + 8, phoneNow());
            uint8_t* e = p.data() + kImuHeaderSize;
            wrI64(e, tPhone);
            e[8] = uint8_t(ImuKind::Accel);
            for (int i = 0; i < 3; ++i) wrF32(e + 12 + 4 * i, float(aDev[i]) + accNoise(rng));
            e += kImuRecordSize;
            wrI64(e, tPhone);
            e[8] = uint8_t(ImuKind::Gyro);
            for (int i = 0; i < 3; ++i) wrF32(e + 12 + 4 * i, float(w[i]) + gyroBias[i] + gyroNoise(rng));
            if (!net.send(PacketType::Imu, p)) { ok = false; break; }
            nextImu += imuPeriod;
            const int64_t sleepNs = nextImu - pcNowNs();
            if (sleepNs > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    });

    const unsigned workers = std::max(2u, std::thread::hardware_concurrency());
    auto renderRows = [&](const Mat3& rwc, int v0, int v1) {
        for (int v = v0; v < v1; ++v) {
            uint8_t* yRow = frame->data[0] + size_t(v) * frame->linesize[0];
            for (int u = 0; u < width; ++u) {
                const double rc[3] = {(u - cx) / f, (v - cy) / f, 1.0};
                double rw[3];
                rwc.apply(rc, rw);
                const double n = std::sqrt(rw[0] * rw[0] + rw[1] * rw[1] + rw[2] * rw[2]);
                const double lon = std::atan2(rw[1], rw[0]) * 180.0 / kPi;
                const double lat = std::asin(rw[2] / n) * 180.0 / kPi;
                const bool line = std::fabs(std::remainder(lon, 10.0)) < 0.25 || std::fabs(std::remainder(lat, 10.0)) < 0.25;
                const bool checker = (int(std::floor(lon / 10.0)) + int(std::floor(lat / 10.0))) & 1;
                yRow[u] = line ? 235 : (checker ? 150 : 70);
            }
            if (v & 1) continue;
            const int cv = v / 2;
            uint8_t* uRow = frame->data[1] + size_t(cv) * frame->linesize[1];
            uint8_t* vRow = frame->data[2] + size_t(cv) * frame->linesize[2];
            for (int u = 0; u < width / 2; ++u) {
                const double rc[3] = {(2 * u - cx) / f, (v - cy) / f, 1.0};
                double rw[3];
                rwc.apply(rc, rw);
                const double n = std::sqrt(rw[0] * rw[0] + rw[1] * rw[1] + rw[2] * rw[2]);
                const double lat = std::asin(rw[2] / n) * 180.0 / kPi;
                const double lon = std::atan2(rw[1], rw[0]) * 180.0 / kPi;
                if (std::fabs(lat) < 0.6) { uRow[u] = 90; vRow[u] = 240; }        // горизонт — красный
                else if (lat > 0) { uRow[u] = 160; vRow[u] = 110; }               // небо — голубоватое
                else { uRow[u] = lon > 0 ? 100 : 120; vRow[u] = 140; }            // пол — тёплый
            }
        }
    };

    // --- LiDAR 30 Гц: 256x192 float16, «комната» — сфера радиусом 2.5 м
    std::thread depthThread([&] {
        if (!depth) return;
        const int dw = 256, dh = 192;
        const float dfx = 211.0f, dcx = dw * 0.5f, dcy = dh * 0.5f;
        int64_t next = start;
        while (ok && pcNowNs() < endNs) {
            std::vector<uint8_t> p(kDepthHeaderSize + size_t(dw) * dh * 2, 0);
            wrI64(p.data(), next + kPhoneClockOffsetNs);
            wrU16(p.data() + 8, dw);
            wrU16(p.data() + 10, dh);
            wrF32(p.data() + 16, dfx);
            wrF32(p.data() + 20, dfx);
            wrF32(p.data() + 24, dcx);
            wrF32(p.data() + 28, dcy);
            uint8_t* d = p.data() + kDepthHeaderSize;
            for (int v = 0; v < dh; ++v)
                for (int u = 0; u < dw; ++u) {
                    const float x = (u - dcx) / dfx, y = (v - dcy) / dfx;
                    const float z = 2.5f / std::sqrt(1 + x * x + y * y);   // расстояние вдоль оси
                    // float -> half (норм. числа в диапазоне 0.1..10 м)
                    uint32_t bits;
                    std::memcpy(&bits, &z, 4);
                    const uint16_t half = uint16_t(((bits >> 16) & 0x8000) | ((((bits >> 23) & 0xFF) - 112) << 10) | ((bits >> 13) & 0x3FF));
                    wrU16(d + 2 * (size_t(v) * dw + u), half);
                }
            if (!net.send(PacketType::Depth, p)) { ok = false; break; }
            next += 33'333'333;
            const int64_t sleepNs = next - pcNowNs();
            if (sleepNs > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    });

    const int64_t framePeriod = 1'000'000'000LL / fps;
    int64_t nextFrame = start;
    std::deque<PendingVideo> videoQueue;
    uint64_t frames = 0;

    while (ok && pcNowNs() < endNs) {
        const int64_t now = pcNowNs();

        // --- кадр камеры
        if (now >= nextFrame) {
            const double t = double(nextFrame - start) * 1e-9;
            Mat3 rwd = deviceOrientation(t);
            // камера (OpenCV) в осях устройства: x_c = -y_d, y_c = -x_d, z_c = -z_d
            const Mat3 rdc = {{{0, -1, 0}, {-1, 0, 0}, {0, 0, -1}}};
            Mat3 rwc = rwd * rdc;

            std::vector<std::thread> pool;
            const int rowsPer = ((height + int(workers) - 1) / int(workers) + 1) & ~1;
            for (int v0 = 0; v0 < height; v0 += rowsPer)
                pool.emplace_back(renderRows, rwc, v0, std::min(height, v0 + rowsPer));
            for (auto& th : pool) th.join();

            frame->pts = nextFrame + kPhoneClockOffsetNs;
            if (avcodec_send_frame(enc, frame) == 0) {
                while (avcodec_receive_packet(enc, pkt) == 0) {
                    std::vector<uint8_t> p(kVideoHeaderSize + size_t(pkt->size), 0);
                    p[0] = uint8_t(hevc ? VideoCodec::HEVC : VideoCodec::H264);
                    p[1] = uint8_t(((pkt->flags & AV_PKT_FLAG_KEY) ? kVideoFlagKeyframe : 0) | kVideoFlagColorInfo | kVideoFlagFullRange | kVideoFlagBT709);
                    wrU16(p.data() + 4, uint16_t(width));
                    wrU16(p.data() + 6, uint16_t(height));
                    wrI64(p.data() + 8, pkt->pts);
                    wrF32(p.data() + 16, float(f));
                    wrF32(p.data() + 20, float(f));
                    wrF32(p.data() + 24, float(cx));
                    wrF32(p.data() + 28, float(cy));
                    wrU32(p.data() + 32, 4000);
                    std::memcpy(p.data() + kVideoHeaderSize, pkt->data, size_t(pkt->size));
                    const int64_t capturePc = pkt->pts - kPhoneClockOffsetNs;
                    videoQueue.push_back({capturePc + int64_t(latencyMs * 1e6), std::move(p)});
                    av_packet_unref(pkt);
                }
            }
            ++frames;
            nextFrame += framePeriod;
            // если рендер не успевает — пропускаем кадры, а не копим отставание
            while (nextFrame < pcNowNs() - framePeriod) nextFrame += framePeriod;
        }

        // --- отправка видео с задержкой «камера → сеть»
        while (!videoQueue.empty() && pcNowNs() >= videoQueue.front().sendAtPc) {
            auto& pv = videoQueue.front();
            wrI64(pv.payload.data() + 40, phoneNow());
            if (!net.send(PacketType::Video, pv.payload)) { ok = false; break; }
            videoQueue.pop_front();
        }

        int64_t next = nextFrame;
        if (!videoQueue.empty()) next = std::min(next, videoQueue.front().sendAtPc);
        const int64_t sleepNs = next - pcNowNs();
        if (sleepNs > 200'000) std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs - 100'000));
    }
    ok = false;
    imuThread.join();
    depthThread.join();

    std::cout << "done, frames " << frames << std::endl;
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&enc);
    return 0;
}
