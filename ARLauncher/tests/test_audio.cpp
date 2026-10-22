// Пространственный звук: OpenAL Soft пишет вывод в WAV (без звуковой карты).
// Источник слева — громче левый канал, справа — правый; повернули голову на 180° — каналы меняются;
// «за стеной» — глуше (меньше высоких частот).

#include "AudioEngine.h"

#include <glm/gtc/quaternion.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

namespace {

struct Rms {
    double left = 0, right = 0, hfLeft = 0;
};

// Свой конфиг OpenAL: вывод в файл, стерео int16, HRTF
void configure(const std::string& wav)
{
    const std::string conf = wav + ".conf";
    std::ofstream c(conf);
    c << "[general]\ndrivers = wave\nchannels = stereo\nsample-type = int16\nfrequency = 48000\nhrtf = true\n"
      << "[wave]\nfile = " << wav << "\n";
    c.close();
    setenv("ALSOFT_CONF", conf.c_str(), 1);
    setenv("ALSOFT_DRIVERS", "wave", 1);
}

Rms analyze(const std::string& wav)
{
    Rms r;
    std::ifstream f(wav, std::ios::binary);
    std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (data.size() < 44 + 4) return r;
    const int16_t* s = reinterpret_cast<const int16_t*>(data.data() + 44);
    const size_t frames = (data.size() - 44) / 4;
    double prev = 0;
    size_t n = 0;
    for (size_t i = frames / 4; i < frames; ++i) {   // без первых 25% (разгон)
        const double l = s[2 * i] / 32768.0, rr = s[2 * i + 1] / 32768.0;
        r.left += l * l;
        r.right += rr * rr;
        r.hfLeft += (l - prev) * (l - prev);   // разность соседних отсчётов ~ высокие частоты
        prev = l;
        ++n;
    }
    if (n) {
        r.left = std::sqrt(r.left / n);
        r.right = std::sqrt(r.right / n);
        r.hfLeft = std::sqrt(r.hfLeft / n);
    }
    return r;
}

// OpenAL читает конфиг один раз на процесс — каждый случай в своём процессе
Rms run(const std::string& wav, const glm::vec3& source, const glm::quat& head, float occlusion)
{
    const pid_t pid = fork();
    if (pid == 0) {
        configure(wav);
        AudioEngine audio;
        if (!audio.initialize(true)) _exit(1);
        const int id = audio.addSource(AudioEngine::makeHum(48000, 2.0f), 48000, true, 0.8f);
        audio.setListener(glm::vec3(0.0f), head);
        audio.setSourcePosition(id, source);
        audio.setSourceOcclusion(id, occlusion);
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        const bool hrtf = audio.hrtfActive();
        audio.shutdown();
        _exit(hrtf ? 0 : 2);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) == 1) return {};
    if (WEXITSTATUS(status) == 2) std::printf("  (HRTF unavailable — plain stereo panning)\n");
    return analyze(wav);
}

} // namespace

int main(int argc, char** argv)
{
    const std::string dir = argc > 1 ? argv[1] : "/tmp";
    const glm::quat forward(1, 0, 0, 0);
    const glm::quat turned = glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0));

    const Rms left = run(dir + "/audio_left.wav", glm::vec3(-1.5f, 0, -0.3f), forward, 0.0f);
    const Rms right = run(dir + "/audio_right.wav", glm::vec3(1.5f, 0, -0.3f), forward, 0.0f);
    const Rms leftTurned = run(dir + "/audio_left_turned.wav", glm::vec3(-1.5f, 0, -0.3f), turned, 0.0f);
    const Rms leftWall = run(dir + "/audio_left_wall.wav", glm::vec3(-1.5f, 0, -0.3f), forward, 1.0f);

    std::printf("source left:            L %.3f  R %.3f\n", left.left, left.right);
    std::printf("source right:           L %.3f  R %.3f\n", right.left, right.right);
    std::printf("source left, head 180°: L %.3f  R %.3f\n", leftTurned.left, leftTurned.right);
    std::printf("source left, behind wall: L %.3f (HF %.4f vs %.4f in the open)\n", leftWall.left, leftWall.hfLeft, left.hfLeft);

    const bool ok = left.left > 1e-3 && left.left > 1.5 * left.right && right.right > 1.5 * right.left &&
                    leftTurned.right > 1.5 * leftTurned.left && leftWall.hfLeft < 0.5 * left.hfLeft &&
                    leftWall.left < left.left;
    std::printf("%s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
