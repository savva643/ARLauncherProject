#include "Application.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace {
std::atomic<bool> g_stop{false};
void onSignal(int) { g_stop = true; }   // Ctrl+C / kill: корректный выход (карта мира сохраняется)
}

int main(int argc, char* argv[]) {
    Application app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize ARLauncher" << std::endl;
        return -1;
    }
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    app.setStopFlag(&g_stop);
    
    std::cout << "[RUN] ARLauncher started" << std::endl;
    std::cout << "📡 Waiting for sensor data..." << std::endl;
    
    app.run();
    app.shutdown();
    
    return 0;
}
