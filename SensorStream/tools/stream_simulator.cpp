// Симулятор iPhone ARStreamer (протокол v2) для тестов без телефона.
//
// Генерирует известное движение головы (yaw/pitch синусоиды), сырой IMU 100 Гц с bias и шумом,
// и видео с виртуальной камеры, которая смотрит на процедурную сферу-сетку (горизонт красный).
// Видео отправляется с искусственной задержкой, как с реального телефона, — так видно, что
// репроекция в лаунчере держит мир на месте несмотря на задержку видео.
//
//   stream_simulator [host] [port] [--seconds N] [--latency-ms 45] [--size 1280x960] [--fps 60] [--hevc] [--depth] [--room]
//                    [--eyes] [--audio] [--display mono|stereo|lens] [--local-camera] [--display-size 1280x592]
//                    [--save-dir DIR] [--no-warp]
//
// --eyes: модуль айтрекинга — ИК-кадры обоих глаз 60 Гц (пакет EYE), взгляд ходит по восьмёрке ±15°.
// --audio: микрофон — 16 кГц PCM16 (пакет AUDIO), «речь»: слоги на 140 Гц с формантами.
//
// --display: «телефон как экран» — просит у лаунчера готовую картинку (DISPLAY), принимает RETURN_VIDEO,
// декодирует и доворачивает кадр по истинной позе на момент показа — ровно как шейдер iPhone
// (ReturnCompositor ниже — эталон для ARStreamer/ReturnDisplay.metal). Сравнивает с истинным видом
// комнаты: печатает сдвиг картинки в пикселях с доворотом и без него, шлёт ACK, сохраняет кадры (--save-dir).
//
// --room: вместо сферы-сетки — комната 5x6 м с перегородкой, столом и шкафом; «голова» ходит по комнате
// (позиция меняется), LiDAR согласован с видео. Так проверяются позиция по LiDAR, окклюзия и тени.

#include "SensorStream/Protocol.h"
#include "SensorStream/Types.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
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
#include <fstream>
#include <memory>
#include <cerrno>

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
    static Mat3 fromQuat(double x, double y, double z, double w) {
        return {{{1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
                 {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
                 {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)}}};
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
bool g_room = false;   // --room: взгляд чуть вниз (видно пол и тени), повороты медленнее
double g_headSpeed = 1.0;   // --head-speed: во сколько раз быстрее поворачивать голову

Mat3 deviceOrientation(double t)
{
    const Mat3 r0 = {{{0, -1, 0}, {0, 0, -1}, {1, 0, 0}}};   // столбцы: x_d=(0,0,1), y_d=(-1,0,0), z_d=(0,-1,0)
    const double k = g_headSpeed;
    const double yaw = (g_room ? 25.0 * std::sin(2 * kPi * 0.07 * k * t) : 35.0 * std::sin(2 * kPi * 0.20 * k * t)) * kPi / 180.0;
    const double pitch = (g_room ? -22.0 + 14.0 * std::sin(2 * kPi * 0.05 * k * t) : 12.0 * std::sin(2 * kPi * 0.13 * k * t)) * kPi / 180.0;
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

// ---------------- комната (режим --room) ----------------
struct Box {
    double lo[3], hi[3];
    uint8_t y, u, v;     // цвет поверхности (YUV full range)
};
// Мир: Z вверх, пол z = 0. Старт головы — (0, -0.5, 1.55), взгляд по +Y.
const Box kRoom{{-2.5, -2.0, 0.0}, {2.5, 4.0, 2.6}, 0, 0, 0};
const Box kSolids[] = {
    {{-0.3, 1.0, 0.0}, {0.9, 1.25, 2.0}, 170, 105, 165},     // перегородка (терракота)
    {{-1.6, 1.5, 0.0}, {-0.8, 2.3, 0.75}, 95, 115, 150},     // стол (дерево)
    {{1.6, -1.5, 0.0}, {2.5, -0.9, 1.9}, 120, 150, 110},     // шкаф (серо-синий)
    {{-2.5, -0.5, 0.0}, {-2.1, 0.7, 1.1}, 140, 128, 128},    // комод
};
const double kLampPos[3] = {0.2, 1.8, 2.59};

bool slab(double o, double d, double lo, double hi, double& tmin, double& tmax, int axis, int& enterAxis, int& exitAxis)
{
    if (std::fabs(d) < 1e-12) return o >= lo && o <= hi;
    double t0 = (lo - o) / d, t1 = (hi - o) / d;
    if (t0 > t1) std::swap(t0, t1);
    if (t0 > tmin) { tmin = t0; enterAxis = axis; }
    if (t1 < tmax) { tmax = t1; exitAxis = axis; }
    return tmin <= tmax;
}

struct Hit {
    double t = 1e9;
    int box = -1;      // -1 — стены комнаты
    int axis = 2;      // ось нормали
};

Hit raycastRoom(const double o[3], const double d[3])
{
    Hit h;
    {
        double tmin = -1e9, tmax = 1e9;
        int ea = 0, xa = 0;
        bool ok = true;
        for (int a = 0; a < 3 && ok; ++a) ok = slab(o[a], d[a], kRoom.lo[a], kRoom.hi[a], tmin, tmax, a, ea, xa);
        if (ok && tmax > 0) { h.t = tmax; h.axis = xa; }
    }
    for (int b = 0; b < int(sizeof(kSolids) / sizeof(kSolids[0])); ++b) {
        double tmin = 0.0, tmax = 1e9;
        int ea = 0, xa = 0;
        bool ok = true;
        for (int a = 0; a < 3 && ok; ++a) ok = slab(o[a], d[a], kSolids[b].lo[a], kSolids[b].hi[a], tmin, tmax, a, ea, xa);
        if (ok && tmin > 1e-6 && tmin < h.t) { h.t = tmin; h.box = b; h.axis = ea; }
    }
    return h;
}

// Цвет точки комнаты: материал + простое освещение от лампы на потолке
void shadeRoom(const double o[3], const double d[3], const Hit& h, uint8_t& Y, uint8_t& U, uint8_t& V)
{
    const double p[3] = {o[0] + d[0] * h.t, o[1] + d[1] * h.t, o[2] + d[2] * h.t};
    double base = 150;
    int u = 128, v = 128;
    if (h.box == 0 && h.axis == 1 && p[1] < 1.001) {
        // Метка ArUco DICT_4X4_50 id 3 (30 см) на перегородке: проверка якорей (ARL_MARKER_SIZE_CM=30)
        static const char* kMarker[6] = {"000000", "010010", "010010", "001000", "001100", "000000"};
        const double x0 = 0.30, zTop = 1.50, cell = 0.05;
        const double mx = (p[0] - x0) / cell, mz = (zTop - p[2]) / cell;
        if (mx >= -1 && mx < 7 && mz >= -1 && mz < 7) {
            const bool inside = mx >= 0 && mx < 6 && mz >= 0 && mz < 6;
            const bool white = !inside || kMarker[int(mz)][int(mx)] == '1';
            Y = white ? 235 : 20;
            U = V = 128;
            return;
        }
    }
    if (h.box >= 0) {
        base = kSolids[h.box].y;
        u = kSolids[h.box].u;
        v = kSolids[h.box].v;
    } else if (h.axis == 2 && p[2] < 0.01) {
        // пол: доски
        const bool plank = int(std::floor(p[0] / 0.2)) & 1;
        const double grain = 8.0 * std::sin(p[1] * 9.0 + p[0] * 3.0);
        base = (plank ? 118 : 104) + grain;
        u = 112;
        v = 150;
    } else if (h.axis == 2) {
        // потолок + лампа (квадрат 40 см)
        if (std::fabs(p[0] - kLampPos[0]) < 0.2 && std::fabs(p[1] - kLampPos[1]) < 0.2) {
            Y = 254; U = 124; V = 132;
            return;
        }
        base = 205;
    } else {
        // стены: светлая штукатурка + плинтус + картина на дальней стене
        base = p[2] < 0.1 ? 90 : 185;
        u = 122;
        v = 134;
        if (h.axis == 1 && p[1] > 3.99 && std::fabs(p[0] - 0.5) < 0.6 && std::fabs(p[2] - 1.5) < 0.4) {
            base = 80; u = 160; v = 100;   // синяя картина
        }
        // текстура для глаз (и для VIO)
        base += 10.0 * std::sin(p[0] * 13.0 + p[2] * 7.0) * std::sin(p[1] * 11.0 - p[2] * 5.0);
    }
    // освещение: лампа сверху (косинус к нормали + затухание) + рассеянный свет
    double n[3] = {0, 0, 0};
    n[h.axis] = (h.box >= 0) ? (d[h.axis] > 0 ? -1 : 1) : (d[h.axis] > 0 ? -1 : 1);
    double l[3] = {kLampPos[0] - p[0], kLampPos[1] - p[1], kLampPos[2] - p[2]};
    const double dist = std::sqrt(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]);
    const double ndl = std::max(0.0, (n[0] * l[0] + n[1] * l[1] + n[2] * l[2]) / dist);
    const double light = 0.55 + 0.75 * ndl / (0.4 + 0.15 * dist * dist);
    Y = uint8_t(std::clamp(base * light, 0.0, 250.0));
    U = uint8_t(u);
    V = uint8_t(v);
}

double roomPos(double t, int axis)
{
    // Шаги по комнате: влево-вправо 0.7 м (20 с), вперёд-назад 0.6 м (40 с), лёгкое покачивание
    if (axis == 0) return 0.7 * std::sin(2 * kPi * t / 20.0);
    if (axis == 1) return -0.5 + 0.3 * (1.0 - std::cos(2 * kPi * t / 40.0));
    return 1.55 + 0.02 * std::sin(2 * kPi * 0.5 * t);
}

// ИК-кадр глаза (как в тесте EyeTracker): веки, радужка, зрачок, блики диодов, шум
void renderEye(std::vector<uint8_t>& img, int w, int h, double yawDeg, double pitchDeg, int eye, std::mt19937& rng)
{
    std::normal_distribution<float> noise(0.0f, 4.0f);
    img.resize(size_t(w) * h);
    const double R = 0.33 * w, yaw = yawDeg * kPi / 180.0, pitch = pitchDeg * kPi / 180.0;
    const double ex = 0.5 * w + (eye ? 3.0 : -3.0), ey = 0.52 * h;
    const double px = ex - R * std::sin(yaw) * std::cos(pitch), py = ey - R * std::sin(pitch) * 0.9;
    const double iris = 0.12 * w, pupil = 0.048 * w;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const double dx = (x - ex) / (0.42 * w), dy = (y - ey) / (0.30 * h);
            const bool open = dx * dx + dy * dy < 1.0;
            double v = open ? 205.0 : 125.0;
            const double r = std::hypot(x - px, (y - py) * 1.05);
            if (open && r < iris) v = 85.0;
            if (r < pupil) v = 18.0;
            for (double gx : {-0.03 * w, 0.03 * w})
                if (std::hypot(x - (ex + gx + (px - ex) * 0.5), y - (ey + 0.02 * h + (py - ey) * 0.5)) < 2.2) v = 250.0;
            img[size_t(y) * w + x] = uint8_t(std::clamp(v + noise(rng), 0.0, 255.0));
        }
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
    int fd() const { return m_fd; }

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

// ---------------- «телефон как экран»: эталон компоновщика iPhone (ReturnDisplay.metal) ----------------

struct Quat {
    double x = 0, y = 0, z = 0, w = 1;
    Mat3 mat() const { return Mat3::fromQuat(x, y, z, w); }
};

/// Линзы VR-очков: линза-лупа даёт подушку — угол глаза растёт быстрее экранной координаты:
/// t = s (1 + k1 s^2 + k2 s^4), s — точка экрана относительно центра линзы в tan (мм / расстояние до линзы).
/// Поэтому кадр на экране рисуется «бочкой», и линза его выпрямляет. Экран и линзы — в миллиметрах.
struct LensModel {
    double screenW = 141.2, screenH = 65.3;     // мм (iPhone 15 Pro: 2556x1179 @ 460 ppi)
    double interLens = 63.9;                    // мм между центрами линз
    double screenToLens = 39.0;                 // мм
    double centerOffsetY = 0.0;                 // мм, центр линз ниже(+) центра экрана
    double k1 = 0.34, k2 = 0.55;                // Cardboard v2
    double chroma = 0.012;
    double maxTan = 1.28;                       // ~52° от оси

    /// tan угла глаза по экранной точке в мм (x вправо, y вниз), глаз 0/1; false — за краем линзы
    bool eyeTan(int eye, double mmX, double mmY, double out[2]) const
    {
        const double cx = screenW * 0.5 + (eye == 0 ? -0.5 : 0.5) * interLens;
        const double cy = screenH * 0.5 + centerOffsetY;
        const double dx = (mmX - cx) / screenToLens, dy = -(mmY - cy) / screenToLens;
        const double r2 = dx * dx + dy * dy;
        const double f = 1.0 + k1 * r2 + k2 * r2 * r2;
        out[0] = dx * f;
        out[1] = dy * f;
        return r2 * f * f <= maxTan * maxTan;
    }
    /// Полуугол, который глаз видит через свою половину экрана (симметрично — так проще рендеру ПК)
    void requestTan(double& tanX, double& tanY) const
    {
        double t[2];
        tanX = tanY = 0;
        for (double x : {0.0, screenW * 0.5})
            for (double y : {0.0, screenH * 0.5, screenH}) {
                eyeTan(0, x, y, t);
                tanX = std::max(tanX, std::fabs(t[0]));
                tanY = std::max(tanY, std::fabs(t[1]));
            }
        tanX = std::min(tanX, maxTan);
        tanY = std::min(tanY, maxTan);
    }
};

struct ReturnHeader {
    uint8_t codec = 1, flags = 0, layout = 1, mode = 1;
    int width = 0, height = 0;
    uint32_t frameId = 0;
    int64_t tRef = 0, tTarget = 0, tCamera = 0, tSend = 0;
    Quat predFromRef, eyeFromDevice, cameraFromDevice;
    double bias[3] = {0, 0, 0};
    double overscan = 1, tanX = 1, tanY = 1, ipd = 0.063;
};

ReturnHeader parseReturnHeader(const uint8_t* h)
{
    ReturnHeader r;
    r.codec = h[0]; r.flags = h[1]; r.layout = h[2]; r.mode = h[3];
    r.width = rdU16(h + 4); r.height = rdU16(h + 6);
    r.frameId = rdU32(h + 8);
    r.tRef = rdI64(h + 16); r.tTarget = rdI64(h + 24); r.tCamera = rdI64(h + 32); r.tSend = rdI64(h + 40);
    auto q = [&](int off) { return Quat{rdF32(h + off), rdF32(h + off + 4), rdF32(h + off + 8), rdF32(h + off + 12)}; };
    r.predFromRef = q(48); r.eyeFromDevice = q(64); r.cameraFromDevice = q(80);
    for (int i = 0; i < 3; ++i) r.bias[i] = rdF32(h + 96 + 4 * i);
    r.overscan = rdF32(h + 108); r.tanX = rdF32(h + 112); r.tanY = rdF32(h + 116); r.ipd = rdF32(h + 120);
    return r;
}

/// Последний кадр «камеры телефона» (для режима «своя камера»)
struct LocalCamera {
    std::mutex mutex;
    std::vector<uint8_t> y, u, v;
    int width = 0, height = 0;
    int64_t tPhone = 0;
    double fx = 0, fy = 0, cx = 0, cy = 0;
};

void yuvToRgb(double Y, double U, double V, double rgb[3])
{
    // BT.709 full range
    const double cb = U - 128, cr = V - 128;
    rgb[0] = std::clamp(Y + 1.5748 * cr, 0.0, 255.0);
    rgb[1] = std::clamp(Y - 0.1873 * cb - 0.4681 * cr, 0.0, 255.0);
    rgb[2] = std::clamp(Y + 1.8556 * cb, 0.0, 255.0);
}

class ReturnClient {
public:
    struct Options {
        int mode = 1;
        bool localCamera = false;
        bool noWarp = false;
        int width = 1280, height = 592;
        std::string saveDir;
    };

    ReturnClient(Sender& net, int64_t startPc, const Options& o, LocalCamera& cam) : m_net(net), m_start(startPc), m_opt(o), m_cam(cam)
    {
        const AVCodec* dec = avcodec_find_decoder(AV_CODEC_ID_H264);
        m_dec = avcodec_alloc_context3(dec);
        m_dec->flags |= AV_CODEC_FLAG_LOW_DELAY;
        m_dec->thread_count = 1;
        avcodec_open2(m_dec, dec, nullptr);
        m_frame = av_frame_alloc();
        m_pkt = av_packet_alloc();
        // Экран «телефона»: размер запроса; физический размер — как у iPhone 15 Pro, но в пропорции запроса
        m_lens.screenH = m_lens.screenW * o.height / o.width;
    }
    ~ReturnClient()
    {
        if (m_worker.joinable()) m_worker.join();
        av_frame_free(&m_frame);
        av_packet_free(&m_pkt);
        avcodec_free_context(&m_dec);
        if (m_sws) sws_freeContext(m_sws);
    }

    std::vector<uint8_t> displayRequest() const
    {
        std::vector<uint8_t> p(kDisplayRequestSize, 0);
        p[0] = uint8_t(m_opt.mode);
        p[1] = m_opt.localCamera ? kDisplayFlagLocalCamera : 0;
        wrU16(p.data() + 2, uint16_t(m_opt.width));
        wrU16(p.data() + 4, uint16_t(m_opt.height));
        wrU16(p.data() + 6, 60);
        wrU32(p.data() + 8, 10000);
        double tx = 0, ty = 0;
        if (m_opt.mode == 3) m_lens.requestTan(tx, ty);
        wrF32(p.data() + 12, float(tx));
        wrF32(p.data() + 16, float(ty));
        wrF32(p.data() + 20, 0.063f);
        wrF32(p.data() + 24, 60.0f);
        wrI64(p.data() + 32, phoneNow());
        return p;
    }

    /// Пакет RETURN_VIDEO: декодировать, «показать» (доворот по истинной позе), сравнить с истиной, ACK
    void handle(const uint8_t* p, size_t n)
    {
        if (n < kReturnVideoHeaderSize) return;
        const int64_t tRecv = phoneNow();
        const ReturnHeader h = parseReturnHeader(p);
        m_pkt->data = const_cast<uint8_t*>(p + kReturnVideoHeaderSize);
        m_pkt->size = int(n - kReturnVideoHeaderSize);
        if (avcodec_send_packet(m_dec, m_pkt) < 0) { sendAck(h.frameId, true, tRecv, 0, 0); return; }
        if (avcodec_receive_frame(m_dec, m_frame) < 0) return;
        const int64_t tDecoded = phoneNow();
        // Показ: следующий vsync 60 Гц + полразвёртки
        const int64_t period = 16'666'667;
        const int64_t tDisplay = ((tDecoded / period) + 1) * period + period / 2;
        sendAck(h.frameId, false, tRecv, tDisplay, uint32_t((tDecoded - tRecv) / 1000));
        ++m_frames;
        m_latencySum += double(tDisplay - h.tTarget) * 1e-6;
        if (h.tCamera) m_photonSum += double(tDisplay - h.tCamera) * 1e-6, ++m_photonCount;

        if (m_frames < 30 || m_busy) return;   // сравнение с истиной — в своём потоке, пока он свободен
        toRgb();
        if (m_worker.joinable()) m_worker.join();
        m_busy = true;
        m_worker = std::thread([this, h, tDisplay] { compare(h, tDisplay); m_busy = false; });
    }

    void compare(const ReturnHeader& h, int64_t tDisplay)
    {
        std::vector<uint8_t> warped, plain, truth;
        compose(h, tDisplay, false, warped);
        compose(h, tDisplay, true, plain);
        renderTruth(h, tDisplay, truth);
        const double degPerPx = 2.0 * std::atan(displayTanX(h)) * 180.0 / kPi / eyeWidth();
        const double e1 = shiftPx(warped, truth), e2 = shiftPx(plain, truth);
        // Ошибка самой картинки ПК (без доворота против истины на момент, под который она нарисована)
        std::vector<uint8_t> truthAtTarget;
        renderTruth(h, h.tTarget, truthAtTarget);
        const double e0 = shiftPx(plain, truthAtTarget);
        m_errContent += e0 * degPerPx;
        m_errWarp += e1 * degPerPx;
        m_errPlain += e2 * degPerPx;
        ++m_compared;
        std::printf("[display] frame %u: %s %dx%d%s, target-display %.1f ms, image error vs truth: warp %.2f deg, no warp %.2f deg (PC image itself %.2f deg)\n",
                    h.frameId, h.layout == 2 ? "SBS" : "mono", h.width, h.height, (h.flags & kReturnFlagAlpha) ? " +alpha" : "",
                    double(tDisplay - h.tTarget) * 1e-6, e1 * degPerPx, e2 * degPerPx, e0 * degPerPx);
        if (!m_opt.saveDir.empty() && m_saved < 3) {
            const std::string base = m_opt.saveDir + "/display_" + std::to_string(m_saved);
            savePpm(base + "_phone.ppm", m_opt.noWarp ? plain : warped);
            savePpm(base + "_truth.ppm", truth);
            savePpm(base + "_received.ppm", m_rgb, m_rgbW, m_rgbH);
            ++m_saved;
        }
    }

    void summary()
    {
        if (m_worker.joinable()) m_worker.join();
        if (!m_frames) {
            std::printf("[display] no frames received from launcher\n");
            return;
        }
        std::printf("[display] %llu frames, render-target->display %.1f ms avg", (unsigned long long)m_frames, m_latencySum / double(m_frames));
        if (m_photonCount) std::printf(", camera->display %.1f ms", m_photonSum / double(m_photonCount));
        if (m_compared)
            std::printf(", image error vs truth: warp %.2f deg, no warp %.2f deg (PC image itself %.2f deg)", m_errWarp / m_compared,
                        m_errPlain / m_compared, m_errContent / m_compared);
        std::printf("\n");
    }

private:
    double simTime(int64_t tPhone) const { return double(tPhone - kPhoneClockOffsetNs - m_start) * 1e-9; }
    int eyeWidth() const { return m_opt.mode >= 2 ? m_opt.width / 2 : m_opt.width; }
    double displayTanX(const ReturnHeader& h) const
    {
        if (m_opt.mode == 3) { double tx, ty; m_lens.requestTan(tx, ty); return tx; }
        return h.tanX / h.overscan;
    }

    void sendAck(uint32_t id, bool needKey, int64_t tRecv, int64_t tDisplay, uint32_t decodeUs)
    {
        std::vector<uint8_t> a(kReturnAckSize, 0);
        wrU32(a.data(), id);
        a[4] = needKey ? kAckFlagNeedKeyframe : 0;
        wrI64(a.data() + 8, tRecv);
        wrI64(a.data() + 16, tDisplay);
        wrU32(a.data() + 24, decodeUs);
        m_net.send(PacketType::ReturnAck, a);
    }

    void toRgb()
    {
        m_rgbW = m_frame->width;
        m_rgbH = m_frame->height;
        m_sws = sws_getCachedContext(m_sws, m_rgbW, m_rgbH, AVPixelFormat(m_frame->format), m_rgbW, m_rgbH, AV_PIX_FMT_RGB24,
                                     SWS_POINT, nullptr, nullptr, nullptr);
        const int* coef = sws_getCoefficients(SWS_CS_ITU709);
        sws_setColorspaceDetails(m_sws, coef, 1, coef, 1, 0, 1 << 16, 1 << 16);
        m_rgb.resize(size_t(m_rgbW) * m_rgbH * 3);
        uint8_t* dst[1] = {m_rgb.data()};
        const int dstStride[1] = {m_rgbW * 3};
        sws_scale(m_sws, m_frame->data, m_frame->linesize, 0, m_rgbH, dst, dstStride);
    }

    /// Луч экранного пикселя в осях экрана (x вправо, y вверх, z к зрителю); false — чёрное (за линзой)
    bool screenRay(const ReturnHeader& h, int px, int py, int& eye, double ray[3], double tanXd, double tanYd) const
    {
        const int ew = eyeWidth();
        eye = m_opt.mode >= 2 && px >= ew ? 1 : 0;
        if (m_opt.mode == 3) {
            const double mmX = (px + 0.5) * m_lens.screenW / m_opt.width, mmY = (py + 0.5) * m_lens.screenH / m_opt.height;
            double t[2];
            if (!m_lens.eyeTan(eye, mmX, mmY, t)) return false;
            ray[0] = t[0]; ray[1] = t[1]; ray[2] = -1;
            return true;
        }
        const double u = (px - eye * ew + 0.5) / ew, v = (py + 0.5) / m_opt.height;
        ray[0] = (2 * u - 1) * tanXd;
        ray[1] = (1 - 2 * v) * tanYd;
        ray[2] = -1;
        (void)h;
        return true;
    }

    void displayTans(const ReturnHeader& h, double& tx, double& ty) const
    {
        // Показ: отрендеренный FOV без запаса, вписанный в пропорции глаза на экране
        tx = h.tanX / h.overscan;
        ty = h.tanY / h.overscan;
        const double va = double(eyeWidth()) / m_opt.height;
        tx = std::min(tx, ty * va);
        ty = tx / va;
    }

    /// Компоновщик телефона (то же, что фрагментный шейдер ReturnDisplay.metal)
    void compose(const ReturnHeader& h, int64_t tDisplay, bool noWarp, std::vector<uint8_t>& out)
    {
        const int W = m_opt.width, H = m_opt.height;
        out.assign(size_t(W) * H * 3, 0);
        double tx, ty;
        displayTans(h, tx, ty);
        const Mat3 eyeFromDevice = h.eyeFromDevice.mat();
        const Mat3 deviceFromScreen = eyeFromDevice.transposed();   // интерфейс телефона повёрнут как крепление
        // M = P^-1 * A: луч устройства «сейчас» -> оси устройства, под которые нарисован кадр
        const Mat3 A = deviceOrientation(simTime(h.tRef)).transposed() * deviceOrientation(simTime(tDisplay));
        const Mat3 M = noWarp ? Mat3::identity() : h.predFromRef.mat().transposed() * A;
        const Mat3 toRendered = eyeFromDevice * M * deviceFromScreen;
        const bool alpha = (h.flags & kReturnFlagAlpha) != 0;
        const int colorH = alpha ? h.height / 2 : h.height;
        const int texEyeW = h.layout == 2 ? h.width / 2 : h.width;

        // Своя камера: луч -> камера на момент её кадра
        LocalCamera cam;
        Mat3 toCamera = Mat3::identity();
        if (m_opt.localCamera) {
            std::lock_guard<std::mutex> lock(m_cam.mutex);
            cam.y = m_cam.y; cam.u = m_cam.u; cam.v = m_cam.v;
            cam.width = m_cam.width; cam.height = m_cam.height; cam.tPhone = m_cam.tPhone;
            cam.fx = m_cam.fx; cam.fy = m_cam.fy; cam.cx = m_cam.cx; cam.cy = m_cam.cy;
            const Mat3 C = deviceOrientation(simTime(cam.tPhone)).transposed() * deviceOrientation(simTime(tDisplay));
            toCamera = h.cameraFromDevice.mat() * C * deviceFromScreen;
        }

        for (int py = 0; py < H; ++py)
            for (int px = 0; px < W; ++px) {
                int eye;
                double s[3];
                if (!screenRay(h, px, py, eye, s, tx, ty)) continue;
                double r[3];
                toRendered.apply(s, r);
                double rgb[3] = {0, 0, 0};
                double a = 1.0;
                if (r[2] < -1e-6) {
                    const double x = r[0] / -r[2], y = r[1] / -r[2];
                    const double u = 0.5 + 0.5 * x / h.tanX, v = 0.5 - 0.5 * y / h.tanY;
                    if (u >= 0 && u < 1 && v >= 0 && v < 1) {
                        const int sx = std::min(texEyeW - 1, int(u * texEyeW)) + (h.layout == 2 ? eye * texEyeW : 0);
                        const int sy = std::min(colorH - 1, int(v * colorH));
                        const uint8_t* c = &m_rgb[(size_t(sy) * m_rgbW + sx) * 3];
                        rgb[0] = c[0]; rgb[1] = c[1]; rgb[2] = c[2];
                        if (alpha) a = m_rgb[(size_t(sy + colorH) * m_rgbW + sx) * 3 + 1] / 255.0;   // яркость = G серого
                    } else {
                        a = 0.0;
                    }
                } else {
                    a = 0.0;
                }
                if (m_opt.localCamera && cam.width > 0) {
                    double c[3];
                    toCamera.apply(s, c);
                    double bg[3] = {0, 0, 0};
                    if (c[2] > 1e-6) {
                        const int cxp = int(cam.fx * c[0] / c[2] + cam.cx), cyp = int(cam.fy * c[1] / c[2] + cam.cy);
                        if (cxp >= 0 && cxp < cam.width && cyp >= 0 && cyp < cam.height)
                            yuvToRgb(cam.y[size_t(cyp) * cam.width + cxp], cam.u[size_t(cyp / 2) * (cam.width / 2) + cxp / 2],
                                     cam.v[size_t(cyp / 2) * (cam.width / 2) + cxp / 2], bg);
                    }
                    for (int k = 0; k < 3; ++k) rgb[k] = rgb[k] + bg[k] * (1.0 - a);   // цвет кадра — premultiplied
                }
                uint8_t* o = &out[(size_t(py) * W + px) * 3];
                for (int k = 0; k < 3; ++k) o[k] = uint8_t(std::clamp(rgb[k], 0.0, 255.0));
            }
    }

    /// Истина: комната из истинной позы на момент показа (луч -> устройство -> мир)
    void renderTruth(const ReturnHeader& h, int64_t tDisplay, std::vector<uint8_t>& out) const
    {
        const int W = m_opt.width, H = m_opt.height;
        out.assign(size_t(W) * H * 3, 0);
        double tx, ty;
        displayTans(h, tx, ty);
        const double t = simTime(tDisplay);
        const Mat3 worldFromScreen = deviceOrientation(t) * h.eyeFromDevice.mat().transposed();
        const unsigned workers = std::max(2u, std::thread::hardware_concurrency());
        std::vector<std::thread> pool;
        for (unsigned w = 0; w < workers; ++w)
            pool.emplace_back([&, w] {
                for (int py = int(w); py < H; py += int(workers))
                    for (int px = 0; px < W; ++px) {
                        int eye;
                        double s[3];
                        if (!screenRay(h, px, py, eye, s, tx, ty)) continue;
                        double d[3];
                        worldFromScreen.apply(s, d);
                        double o[3] = {roomPos(t, 0), roomPos(t, 1), roomPos(t, 2)};
                        if (m_opt.mode >= 2) {
                            // глаз смещён на ±IPD/2 вдоль оси x экрана
                            const double side = (eye == 0 ? -0.5 : 0.5) * h.ipd;
                            const double xs[3] = {side, 0, 0};
                            double xw[3];
                            worldFromScreen.apply(xs, xw);
                            for (int k = 0; k < 3; ++k) o[k] += xw[k];
                        }
                        uint8_t Y, U, V;
                        if (g_room) shadeRoom(o, d, raycastRoom(o, d), Y, U, V);
                        else { Y = 128; U = V = 128; }
                        double rgb[3];
                        yuvToRgb(Y, U, V, rgb);
                        uint8_t* p = &out[(size_t(py) * W + px) * 3];
                        for (int k = 0; k < 3; ++k) p[k] = uint8_t(rgb[k]);
                    }
            });
        for (auto& th : pool) th.join();
    }

    /// Глобальный сдвиг картинки относительно истины (пиксели), перебор по яркости в центре левого глаза
    double shiftPx(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) const
    {
        const int W = m_opt.width, H = m_opt.height, ew = eyeWidth();
        auto lum = [&](const std::vector<uint8_t>& img, int x, int y) {
            const uint8_t* p = &img[(size_t(y) * W + x) * 3];
            return 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2];
        };
        const int R = 40, step = 3;
        double best = 1e18;
        int bx = 0, by = 0;
        for (int dy = -R; dy <= R; dy += 1)
            for (int dx = -R; dx <= R; dx += 1) {
                double sum = 0;
                int count = 0;
                for (int y = H / 4; y < 3 * H / 4; y += step)
                    for (int x = ew / 4; x < 3 * ew / 4; x += step) {
                        const int xs = x + dx, ys = y + dy;
                        if (xs < 0 || ys < 0 || xs >= ew || ys >= H) continue;
                        sum += std::fabs(lum(a, xs, ys) - lum(b, x, y));
                        ++count;
                    }
                if (count && sum / count < best) { best = sum / count; bx = dx; by = dy; }
            }
        return std::hypot(double(bx), double(by));
    }

    void savePpm(const std::string& path, const std::vector<uint8_t>& rgb, int w = 0, int h = 0) const
    {
        if (!w) { w = m_opt.width; h = m_opt.height; }
        std::ofstream f(path, std::ios::binary);
        f << "P6\n" << w << " " << h << "\n255\n";
        f.write(reinterpret_cast<const char*>(rgb.data()), std::streamsize(rgb.size()));
    }

    Sender& m_net;
    int64_t m_start;
    Options m_opt;
    LocalCamera& m_cam;
    LensModel m_lens;
    AVCodecContext* m_dec = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_pkt = nullptr;
    SwsContext* m_sws = nullptr;
    std::vector<uint8_t> m_rgb;
    int m_rgbW = 0, m_rgbH = 0;
    uint64_t m_frames = 0;
    double m_latencySum = 0, m_photonSum = 0;
    uint64_t m_photonCount = 0;
    double m_errWarp = 0, m_errPlain = 0, m_errContent = 0;
    int m_compared = 0, m_saved = 0;
    std::thread m_worker;
    std::atomic<bool> m_busy{false};
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
    bool room = false;
    bool eyes = false;
    bool audio = false;
    ReturnClient::Options display;
    display.mode = 0;

    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seconds" && i + 1 < argc) seconds = std::stod(argv[++i]);
        else if (a == "--latency-ms" && i + 1 < argc) latencyMs = std::stod(argv[++i]);
        else if (a == "--fps" && i + 1 < argc) fps = std::stoi(argv[++i]);
        else if (a == "--hevc") hevc = true;
        else if (a == "--depth") depth = true;
        else if (a == "--room") room = g_room = true;
        else if (a == "--eyes") eyes = true;
        else if (a == "--audio") audio = true;
        else if (a == "--size" && i + 1 < argc) std::sscanf(argv[++i], "%dx%d", &width, &height);
        else if (a == "--display" && i + 1 < argc) {
            const std::string m = argv[++i];
            display.mode = m == "stereo" ? 2 : m == "lens" ? 3 : 1;
        }
        else if (a == "--local-camera") display.localCamera = true;
        else if (a == "--head-speed" && i + 1 < argc) g_headSpeed = std::stod(argv[++i]);
        else if (a == "--no-warp") display.noWarp = true;
        else if (a == "--display-size" && i + 1 < argc) std::sscanf(argv[++i], "%dx%d", &display.width, &display.height);
        else if (a == "--save-dir" && i + 1 < argc) display.saveDir = argv[++i];
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

    const int64_t start = pcNowNs();
    LocalCamera localCamera;
    std::unique_ptr<ReturnClient> returnClient;
    std::thread returnThread;
    std::atomic<bool> receiving{true};
    if (display.mode) {
        returnClient = std::make_unique<ReturnClient>(net, start, display, localCamera);
        net.send(PacketType::DisplayRequest, returnClient->displayRequest());
        // Приём RETURN_VIDEO от лаунчера по тому же соединению
        returnThread = std::thread([&] {
            timeval tv{0, 200000};
            setsockopt(net.fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            std::vector<uint8_t> buf;
            std::vector<uint8_t> chunk(1 << 20);
            while (receiving) {
                const ssize_t n = ::recv(net.fd(), chunk.data(), chunk.size(), 0);
                if (n <= 0) {
                    if (n < 0 && (errno == EAGAIN || errno == EINTR)) continue;
                    break;
                }
                buf.insert(buf.end(), chunk.begin(), chunk.begin() + n);
                size_t pos = 0;
                while (buf.size() - pos >= kPacketHeaderSize) {
                    const uint32_t size = rdU32BE(buf.data() + pos + 9);
                    if (buf.size() - pos < kPacketHeaderSize + size) break;
                    if (buf[pos] == uint8_t(PacketType::ReturnVideo))
                        returnClient->handle(buf.data() + pos + kPacketHeaderSize, size);
                    pos += kPacketHeaderSize + size;
                }
                buf.erase(buf.begin(), buf.begin() + long(pos));
            }
        });
    }

    std::mt19937 rng(42);
    std::normal_distribution<float> gyroNoise(0.0f, 0.002f);
    std::normal_distribution<float> accNoise(0.0f, 0.02f);
    const float gyroBias[3] = {0.012f, -0.006f, 0.009f};

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
            double gWorld[3] = {0, 0, 9.80665};
            if (room) {
                // + ускорение головы (вторая производная позиции)
                const double e = 1e-3;
                for (int a = 0; a < 3; ++a)
                    gWorld[a] += (roomPos(t + e, a) - 2 * roomPos(t, a) + roomPos(t - e, a)) / (e * e);
            }
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
    auto renderRoomRows = [&](const Mat3& rwc, const double o[3], int v0, int v1) {
        for (int v = v0; v < v1; ++v) {
            uint8_t* yRow = frame->data[0] + size_t(v) * frame->linesize[0];
            uint8_t* uRow = frame->data[1] + size_t(v / 2) * frame->linesize[1];
            uint8_t* vRow = frame->data[2] + size_t(v / 2) * frame->linesize[2];
            for (int u = 0; u < width; ++u) {
                const double rc[3] = {(u - cx) / f, (v - cy) / f, 1.0};
                double rw[3];
                rwc.apply(rc, rw);
                uint8_t Y, U, V;
                shadeRoom(o, rw, raycastRoom(o, rw), Y, U, V);
                yRow[u] = Y;
                if (!(v & 1) && !(u & 1)) {
                    uRow[u / 2] = U;
                    vRow[u / 2] = V;
                }
            }
        }
    };
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
        // LiDAR выровнен с кадром камеры: те же интринсики в масштабе 256 по ширине
        const float dfx = room ? float(f * dw / width) : 211.0f, dcx = dw * 0.5f, dcy = room ? float(cy * dw / width) : dh * 0.5f;
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
            const double tSec = double(next - start) * 1e-9;
            const Mat3 rdcD = {{{0, -1, 0}, {-1, 0, 0}, {0, 0, -1}}};
            const Mat3 rwcD = deviceOrientation(tSec) * rdcD;
            const double o[3] = {roomPos(tSec, 0), roomPos(tSec, 1), roomPos(tSec, 2)};
            for (int v = 0; v < dh; ++v)
                for (int u = 0; u < dw; ++u) {
                    const float x = (u - dcx) / dfx, y = (v - dcy) / dfx;
                    float z = 2.5f / std::sqrt(1 + x * x + y * y);   // расстояние вдоль оси
                    if (room) {
                        const double rc[3] = {x, y, 1.0};
                        double rw[3];
                        rwcD.apply(rc, rw);
                        const Hit h = raycastRoom(o, rw);   // rc.z = 1 -> t и есть глубина по оси
                        z = h.t < 6.0 ? float(h.t) : 0.0f;
                    }
                    // float -> half (норм. числа в диапазоне 0.1..10 м)
                    uint32_t bits;
                    std::memcpy(&bits, &z, 4);
                    const uint16_t half = z <= 0.0f ? 0 : uint16_t(((bits >> 16) & 0x8000) | ((((bits >> 23) & 0xFF) - 112) << 10) | ((bits >> 13) & 0x3FF));
                    wrU16(d + 2 * (size_t(v) * dw + u), half);
                }
            if (!net.send(PacketType::Depth, p)) { ok = false; break; }
            next += 33'333'333;
            const int64_t sleepNs = next - pcNowNs();
            if (sleepNs > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    });

    // --- модуль айтрекинга: 60 Гц, оба глаза
    std::thread eyeThread([&] {
        if (!eyes) return;
        std::mt19937 erng(17);
        const int ew = 160, eh = 120;
        std::vector<uint8_t> img;
        int64_t next = start;
        while (ok && pcNowNs() < endNs) {
            const double t = double(next - start) * 1e-9;
            const double yaw = 15.0 * std::sin(2 * kPi * 0.25 * t), pitch = 8.0 * std::sin(2 * kPi * 0.5 * t);
            for (int eye = 0; eye < 2; ++eye) {
                renderEye(img, ew, eh, yaw, pitch, eye, erng);
                std::vector<uint8_t> p(kEyeHeaderSize + img.size(), 0);
                wrI64(p.data(), next + kPhoneClockOffsetNs);
                p[8] = uint8_t(eye);
                p[9] = kEyeFormatGray8;
                wrU16(p.data() + 10, ew);
                wrU16(p.data() + 12, eh);
                std::memcpy(p.data() + kEyeHeaderSize, img.data(), img.size());
                if (!net.send(PacketType::EyeImage, p)) { ok = false; break; }
            }
            next += 16'666'667;
            const int64_t sleepNs = next - pcNowNs();
            if (sleepNs > 0) std::this_thread::sleep_for(std::chrono::nanoseconds(sleepNs));
        }
    });

    // --- микрофон: 16 кГц, пачки по 20 мс
    std::thread audioThread([&] {
        if (!audio) return;
        const int rate = 16000, chunk = 320;
        int64_t next = start;
        uint64_t n = 0;
        while (ok && pcNowNs() < endNs) {
            std::vector<uint8_t> p(kAudioHeaderSize + chunk * 2, 0);
            wrI64(p.data(), next + kPhoneClockOffsetNs);
            wrU32(p.data() + 8, rate);
            p[12] = 1;
            p[13] = kAudioFormatPcm16;
            for (int i = 0; i < chunk; ++i, ++n) {
                const double t = double(n) / rate;
                // слоги 4 в секунду, паузы между фразами
                const double syll = std::max(0.0, std::sin(2 * kPi * 4.0 * t));
                const double phrase = std::fmod(t, 3.0) < 2.0 ? 1.0 : 0.0;
                double v = 0.0;
                for (int k = 1; k <= 12; ++k) {
                    const double f = 140.0 * k;
                    const double formant = std::exp(-std::pow((f - 700.0) / 250.0, 2)) + 0.6 * std::exp(-std::pow((f - 1200.0) / 300.0, 2));
                    v += formant * std::sin(2 * kPi * f * t);
                }
                const int16_t s16 = int16_t(std::clamp(v * syll * phrase * 6000.0, -32767.0, 32767.0));
                wrU16(p.data() + kAudioHeaderSize + 2 * i, uint16_t(s16));
            }
            if (!net.send(PacketType::Audio, p)) { ok = false; break; }
            next += 20'000'000;
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
            const double o[3] = {roomPos(t, 0), roomPos(t, 1), roomPos(t, 2)};
            for (int v0 = 0; v0 < height; v0 += rowsPer) {
                if (room) pool.emplace_back(renderRoomRows, rwc, o, v0, std::min(height, v0 + rowsPer));
                else pool.emplace_back(renderRows, rwc, v0, std::min(height, v0 + rowsPer));
            }
            for (auto& th : pool) th.join();

            frame->pts = nextFrame + kPhoneClockOffsetNs;
            if (display.localCamera) {
                // «Камера телефона» для режима, где фон показывает сам телефон
                std::lock_guard<std::mutex> lock(localCamera.mutex);
                localCamera.width = width;
                localCamera.height = height;
                localCamera.y.resize(size_t(width) * height);
                localCamera.u.resize(size_t(width / 2) * (height / 2));
                localCamera.v.resize(size_t(width / 2) * (height / 2));
                for (int r = 0; r < height; ++r)
                    std::memcpy(&localCamera.y[size_t(r) * width], frame->data[0] + size_t(r) * frame->linesize[0], size_t(width));
                for (int r = 0; r < height / 2; ++r) {
                    std::memcpy(&localCamera.u[size_t(r) * (width / 2)], frame->data[1] + size_t(r) * frame->linesize[1], size_t(width / 2));
                    std::memcpy(&localCamera.v[size_t(r) * (width / 2)], frame->data[2] + size_t(r) * frame->linesize[2], size_t(width / 2));
                }
                localCamera.tPhone = frame->pts;
                localCamera.fx = localCamera.fy = f;
                localCamera.cx = cx;
                localCamera.cy = cy;
            }
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
    eyeThread.join();
    audioThread.join();
    receiving = false;
    if (returnThread.joinable()) returnThread.join();
    if (returnClient) returnClient->summary();

    std::cout << "done, frames " << frames << std::endl;
    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&enc);
    return 0;
}
