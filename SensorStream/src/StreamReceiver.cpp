#include "SensorStream/StreamReceiver.h"
#include "SensorStream/VideoDecoder.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>

namespace sensorstream {

namespace {
constexpr size_t kMaxClients = 8;
constexpr size_t kMaxDecodeQueue = 6;     // больше — значит декодер не успевает: ждём keyframe
constexpr size_t kPoolSize = 4;
constexpr float kGravity = 9.80665f;
} // namespace

StreamReceiver::StreamReceiver() = default;

StreamReceiver::~StreamReceiver()
{
    stop();
}

bool StreamReceiver::openListener(uint16_t port)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);   // все интерфейсы, включая USB-модем iPhone
    addr.sin_port = htons(port);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || ::listen(fd, 4) < 0) {
        std::cerr << "[SensorStream] cannot listen on port " << port << ": " << std::strerror(errno) << std::endl;
        ::close(fd);
        return false;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    m_listeners.push_back(fd);
    std::cout << "[SensorStream] listening on 0.0.0.0:" << port << std::endl;
    return true;
}

bool StreamReceiver::start(const Config& config)
{
    if (m_running) return true;
    m_config = config;

    for (uint16_t port : config.ports) openListener(port);
    if (m_listeners.empty()) return false;

    m_statsWindowStart = pcNowNs();
    m_running = true;
    m_netThread = std::thread(&StreamReceiver::networkLoop, this);
    m_decodeThread = std::thread(&StreamReceiver::decodeLoop, this);
    return true;
}

void StreamReceiver::stop()
{
    if (!m_running) return;
    m_running = false;
    m_queueCv.notify_all();
    if (m_netThread.joinable()) m_netThread.join();
    if (m_decodeThread.joinable()) m_decodeThread.join();
    for (int fd : m_listeners) ::close(fd);
    m_listeners.clear();
    for (auto& c : m_clients) closeClient(c);
    m_clients.clear();
}

VideoFramePtr StreamReceiver::latestVideoFrame() const
{
    std::lock_guard<std::mutex> lock(m_latestMutex);
    return m_latest;
}

StreamStats StreamReceiver::stats() const
{
    std::lock_guard<std::mutex> lock(m_statsMutex);
    return m_stats;
}

// ---------------------------------------------------------------- network

void StreamReceiver::closeClient(Client& c)
{
    if (c.fd >= 0) {
        ::close(c.fd);
        if (c.streaming && m_config.verbose) std::cout << "[SensorStream] iPhone disconnected" << std::endl;
    }
    c.fd = -1;
}

void StreamReceiver::networkLoop()
{
    std::vector<pollfd> fds;
    std::vector<uint8_t> chunk(1 << 20);

    while (m_running) {
        fds.clear();
        for (int l : m_listeners) fds.push_back({l, POLLIN, 0});
        for (auto& c : m_clients) fds.push_back({c.fd, POLLIN, 0});

        int ready = ::poll(fds.data(), fds.size(), 100);
        if (ready < 0 && errno != EINTR) break;

        size_t idx = 0;
        for (; idx < m_listeners.size(); ++idx) {
            if (!(fds[idx].revents & POLLIN)) continue;
            sockaddr_in peer{};
            socklen_t len = sizeof(peer);
            int fd = ::accept(m_listeners[idx], reinterpret_cast<sockaddr*>(&peer), &len);
            if (fd < 0) continue;
            if (m_clients.size() >= kMaxClients) { ::close(fd); continue; }
            int one = 1;
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
            int rcvbuf = 8 * 1024 * 1024;
            setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
            Client c;
            c.fd = fd;
            c.buffer.reserve(4 << 20);
            m_clients.push_back(std::move(c));
            if (m_config.verbose) {
                char ip[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
                std::cout << "[SensorStream] connection from " << ip << std::endl;
            }
        }

        for (size_t ci = 0; ci < m_clients.size(); ++ci, ++idx) {
            Client& c = m_clients[ci];
            if (idx >= fds.size() || !(fds[idx].revents & (POLLIN | POLLHUP | POLLERR))) continue;
            ssize_t n = ::recv(c.fd, chunk.data(), chunk.size(), 0);
            if (n <= 0) {
                if (c.streaming) {
                    std::lock_guard<std::mutex> lock(m_queueMutex);
                    m_queue.clear();
                    m_waitKeyframe = true;
                    m_resetDecoder = true;
                }
                closeClient(c);
                continue;
            }
            m_bytesRx += static_cast<uint64_t>(n);
            c.buffer.insert(c.buffer.end(), chunk.data(), chunk.data() + n);
            processBuffer(c);
        }

        m_clients.erase(std::remove_if(m_clients.begin(), m_clients.end(),
                                       [](const Client& c) { return c.fd < 0; }),
                        m_clients.end());
        updateStats();
    }
}

void StreamReceiver::processBuffer(Client& c)
{
    while (c.buffer.size() - c.readPos >= kPacketHeaderSize) {
        const uint8_t* h = c.buffer.data() + c.readPos;
        const uint8_t type = h[0];
        const uint32_t size = rdU32BE(h + 9);
        if (size > kMaxPayloadSize) {
            std::cerr << "[SensorStream] corrupt stream (payload " << size << "), dropping connection" << std::endl;
            closeClient(c);
            return;
        }
        if (c.buffer.size() - c.readPos < kPacketHeaderSize + size) break;
        handlePacket(c, type, h + kPacketHeaderSize, size);
        if (c.fd < 0) return;
        c.readPos += kPacketHeaderSize + size;
    }
    // сдвиг буфера, когда прочитана заметная часть
    if (c.readPos > 0 && (c.readPos == c.buffer.size() || c.readPos > (1u << 20))) {
        c.buffer.erase(c.buffer.begin(), c.buffer.begin() + static_cast<long>(c.readPos));
        c.readPos = 0;
    }
}

void StreamReceiver::handlePacket(Client& c, uint8_t type, const uint8_t* p, size_t n)
{
    switch (static_cast<PacketType>(type)) {
    case PacketType::Hello:
        c.streaming = true;
        handleHello(p, n);
        break;
    case PacketType::Video:
        c.streaming = true;
        handleVideo(p, n);
        break;
    case PacketType::Imu:
        c.streaming = true;
        handleImu(p, n);
        break;
    case PacketType::Depth:
        handleDepth(p, n);
        break;
    case PacketType::LegacyJpeg:
        c.streaming = true;
        handleLegacyJpeg(p, n);
        break;
    case PacketType::LegacyImu:
        c.streaming = true;
        handleLegacyImu(p, n);
        break;
    default:
        break;   // ориентация/облака точек старого формата и т.п. — не нужны
    }
}

void StreamReceiver::handleHello(const uint8_t* p, size_t n)
{
    if (n < 16 || std::memcmp(p, "GLSK", 4) != 0) return;
    const uint16_t version = rdU16(p + 4);
    const int64_t phoneNow = rdI64(p + 8);
    m_clock.reset();
    m_clock.addSample(phoneNow, pcNowNs());
    m_protocolV2 = true;
    m_haveAccel = false;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.clear();
        m_waitKeyframe = true;
        m_resetDecoder = true;
    }
    if (m_config.verbose) std::cout << "[SensorStream] iPhone stream started, protocol v" << version << std::endl;
}

void StreamReceiver::handleVideo(const uint8_t* p, size_t n)
{
    if (n < kVideoHeaderSize) return;
    EncodedFrame f;
    f.codec = static_cast<VideoCodec>(p[0]);
    f.flags = p[1];
    f.keyframe = (f.flags & kVideoFlagKeyframe) != 0;
    f.width = rdU16(p + 4);
    f.height = rdU16(p + 6);
    f.ptsNs = rdI64(p + 8);
    f.fx = rdF32(p + 16);
    f.fy = rdF32(p + 20);
    f.cx = rdF32(p + 24);
    f.cy = rdF32(p + 28);
    f.exposureUs = rdU32(p + 32);
    f.data.assign(p + kVideoHeaderSize, p + n);
    m_protocolV2 = true;
    ++m_videoRx;
    queueEncoded(std::move(f));
}

void StreamReceiver::emitGyro(int64_t tNs, const float g[3])
{
    ImuSample s;
    s.tNs = tNs;
    std::memcpy(s.gyro, g, sizeof(s.gyro));
    if (m_haveAccel) {
        std::memcpy(s.accel, m_lastAccel, sizeof(s.accel));
        s.hasAccel = true;
    }
    ++m_imuRx;
    if (m_onImu) m_onImu(s);
}

void StreamReceiver::handleImu(const uint8_t* p, size_t n)
{
    if (n < kImuHeaderSize) return;
    const uint16_t count = rdU16(p);
    const int64_t sendNs = rdI64(p + 8);
    m_clock.addSample(sendNs, pcNowNs());
    m_protocolV2 = true;

    if (n < kImuHeaderSize + size_t(count) * kImuRecordSize) return;
    const uint8_t* r = p + kImuHeaderSize;

    // Аксель обновляем до гироскопа той же пачки, чтобы каждый gyro-сэмпл шёл с самым свежим accel
    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t* e = r + size_t(i) * kImuRecordSize;
        if (static_cast<ImuKind>(e[8]) == ImuKind::Accel) {
            m_lastAccel[0] = rdF32(e + 12);
            m_lastAccel[1] = rdF32(e + 16);
            m_lastAccel[2] = rdF32(e + 20);
            m_haveAccel = true;
        }
    }
    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t* e = r + size_t(i) * kImuRecordSize;
        const int64_t t = rdI64(e);
        const float v[3] = {rdF32(e + 12), rdF32(e + 16), rdF32(e + 20)};
        switch (static_cast<ImuKind>(e[8])) {
        case ImuKind::Gyro:
            emitGyro(t, v);
            break;
        case ImuKind::Mag:
            if (m_onMag) {
                MagSample m;
                m.tNs = t;
                std::memcpy(m.field, v, sizeof(v));
                m_onMag(m);
            }
            break;
        default:
            break;
        }
    }
}

void StreamReceiver::handleDepth(const uint8_t* p, size_t n)
{
    if (n < kDepthHeaderSize || !m_onDepth) return;
    DepthFrame d;
    d.tNs = rdI64(p);
    d.width = rdU16(p + 8);
    d.height = rdU16(p + 10);
    const uint8_t format = p[12];
    d.fx = rdF32(p + 16);
    d.fy = rdF32(p + 20);
    d.cx = rdF32(p + 24);
    d.cy = rdF32(p + 28);
    const size_t count = size_t(d.width) * d.height;
    const uint8_t* data = p + kDepthHeaderSize;
    const size_t bytes = n - kDepthHeaderSize;
    d.depth.resize(count);
    if (format == 0 && bytes >= count * 2) {
        for (size_t i = 0; i < count; ++i) d.depth[i] = halfToFloat(rdU16(data + 2 * i));
    } else if (format == 1 && bytes >= count * 4) {
        for (size_t i = 0; i < count; ++i) d.depth[i] = rdF32(data + 4 * i);
    } else {
        return;
    }
    m_onDepth(d);
}

// Совместимость со старым ARStreamer: JPEG без метаданных
void StreamReceiver::handleLegacyJpeg(const uint8_t* p, size_t n)
{
    if (n < 4 || p[0] != 0xFF || p[1] != 0xD8) return;
    const int64_t now = pcNowNs();
    if (!m_protocolV2) m_clock.addSample(now, now);
    EncodedFrame f;
    f.codec = VideoCodec::JPEG;
    f.keyframe = true;
    f.ptsNs = now;
    f.data.assign(p, p + n);
    ++m_videoRx;
    queueEncoded(std::move(f));
}

// Совместимость: старый пакет 0x03 (fused CMDeviceMotion: userAccel, rotationRate, gravity, mag в g)
void StreamReceiver::handleLegacyImu(const uint8_t* p, size_t n)
{
    if (n < 104 || m_protocolV2) return;
    const int64_t now = pcNowNs();
    m_clock.addSample(now, now);
    double v[12];
    for (int i = 0; i < 12; ++i) v[i] = rdF64(p + 8 + 8 * i);
    for (int i = 0; i < 3; ++i) m_lastAccel[i] = float(-(v[i] + v[6 + i]) * kGravity);
    m_haveAccel = true;
    const float g[3] = {float(v[3]), float(v[4]), float(v[5])};
    emitGyro(now, g);
}

// ---------------------------------------------------------------- decode

void StreamReceiver::queueEncoded(EncodedFrame&& f)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_waitKeyframe && !f.keyframe) {
        ++m_droppedDecoder;
        return;
    }
    if (m_queue.size() >= kMaxDecodeQueue) {
        // Декодер отстаёт: выкидываем всё и ждём следующий keyframe — задержка не растёт
        m_droppedDecoder += m_queue.size() + 1;
        m_queue.clear();
        m_waitKeyframe = true;
        if (!f.keyframe) return;
    }
    if (f.keyframe) m_waitKeyframe = false;
    m_queue.push_back(std::move(f));
    m_queueCv.notify_one();
}

std::shared_ptr<VideoFrame> StreamReceiver::acquirePoolFrame()
{
    for (auto& f : m_pool) {
        if (f.use_count() == 1) return f;   // никто (рендер) его больше не держит
    }
    auto f = std::make_shared<VideoFrame>();
    if (m_pool.size() < kPoolSize * 2) m_pool.push_back(f);
    return f;
}

void StreamReceiver::decodeLoop()
{
    VideoDecoder decoder;
    while (m_running) {
        EncodedFrame f;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait_for(lock, std::chrono::milliseconds(100),
                               [this] { return !m_queue.empty() || !m_running; });
            if (!m_running) break;
            if (m_queue.empty()) continue;
            f = std::move(m_queue.front());
            m_queue.pop_front();
        }
        if (m_resetDecoder.exchange(false)) decoder.reset();

        auto frame = acquirePoolFrame();
        const int64_t t0 = pcNowNs();
        if (!decoder.decode(f.codec, f.data.data(), f.data.size(), f.ptsNs, *frame)) continue;
        const int64_t t1 = pcNowNs();

        // Интринсики приходят для разрешения кадра на телефоне; если фолбэк JPEG без них — оценка 60° HFOV
        if (f.fx > 0) {
            const float sx = f.width > 0 ? float(frame->width) / float(f.width) : 1.0f;
            const float sy = f.height > 0 ? float(frame->height) / float(f.height) : 1.0f;
            frame->fx = f.fx * sx;
            frame->fy = f.fy * sy;
            frame->cx = f.cx * sx;
            frame->cy = f.cy * sy;
        } else {
            const float fpx = float(frame->width) * 0.5f / std::tan(30.0f * 3.14159265f / 180.0f);
            frame->fx = frame->fy = fpx;
            frame->cx = frame->width * 0.5f;
            frame->cy = frame->height * 0.5f;
        }
        frame->ptsNs = f.ptsNs;
        frame->pcPtsNs = m_clock.phoneToPc(f.ptsNs);
        frame->decodedPcNs = t1;
        frame->exposureUs = f.exposureUs;
        // Цвет: телефон знает точно (420f + матрица буфера камеры) — важнее VUI потока
        if (f.flags & kVideoFlagColorInfo) {
            frame->fullRange = (f.flags & kVideoFlagFullRange) != 0;
            frame->bt709 = (f.flags & kVideoFlagBT709) != 0;
        }
        frame->index = ++m_decodedIndex;

        m_decodeNsSum += (t1 - t0);
        m_latencyNsSum += (t1 - frame->pcPtsNs);
        ++m_videoDecoded;
        if (m_decoderName != decoder.name()) {
            std::lock_guard<std::mutex> lock(m_statsMutex);
            m_decoderName = decoder.name();
        }

        VideoFramePtr published = frame;
        {
            std::lock_guard<std::mutex> lock(m_latestMutex);
            m_latest = published;
        }
        if (m_onVideo) m_onVideo(published);
    }
}

// ---------------------------------------------------------------- stats

void StreamReceiver::updateStats()
{
    const int64_t now = pcNowNs();
    const int64_t elapsed = now - m_statsWindowStart;
    if (elapsed < 1'000'000'000LL) return;
    const double sec = double(elapsed) * 1e-9;
    m_statsWindowStart = now;

    const uint64_t bytes = m_bytesRx.exchange(0);
    const uint64_t vrx = m_videoRx.exchange(0);
    const uint64_t vdec = m_videoDecoded.exchange(0);
    const uint64_t imu = m_imuRx.exchange(0);
    const int64_t decNs = m_decodeNsSum.exchange(0);
    const int64_t latNs = m_latencyNsSum.exchange(0);

    bool connected = false;
    for (const auto& c : m_clients) connected |= (c.fd >= 0 && c.streaming);

    StreamStats s;
    s.clientConnected = connected;
    s.protocolV2 = m_protocolV2;
    s.videoFpsReceived = double(vrx) / sec;
    s.videoFpsDecoded = double(vdec) / sec;
    s.imuRateHz = double(imu) / sec;
    s.megabitsPerSecond = double(bytes) * 8.0 / 1e6 / sec;
    s.decodeMsAvg = vdec ? double(decNs) / double(vdec) * 1e-6 : 0;
    s.captureToDecodedMsAvg = vdec ? double(latNs) / double(vdec) * 1e-6 : 0;
    s.clockOffsetMs = double(m_clock.offsetNs()) * 1e-6;
    s.framesDroppedDecoder = m_droppedDecoder.load();
    {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        s.decoderName = m_decoderName;
        m_stats = s;
    }

    if (m_config.verbose && connected) {
        std::cout << std::fixed << std::setprecision(1)
                  << "[SensorStream] video rx " << s.videoFpsReceived << " fps, decoded " << s.videoFpsDecoded
                  << " fps, IMU " << s.imuRateHz << " Hz, " << s.megabitsPerSecond << " Mbit/s, decode "
                  << s.decodeMsAvg << " ms, capture->decoded " << s.captureToDecodedMsAvg << " ms, dropped "
                  << s.framesDroppedDecoder << std::endl;
    }
}

} // namespace sensorstream
