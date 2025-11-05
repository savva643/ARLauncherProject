#include "Application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Application app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize ARLauncher" << std::endl;
        return -1;
    }
    
    std::cout << "🚀 ARLauncher started" << std::endl;
    std::cout << "📡 Waiting for sensor data..." << std::endl;
    
    app.run();
    app.shutdown();
    
    return 0;
}

