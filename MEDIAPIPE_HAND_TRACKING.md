# MediaPipe Hand Tracking Integration Guide

## Что нужно для Hand Tracking

### Зависимости

1. **MediaPipe C++ Library**
   - GitHub: https://github.com/google/mediapipe
   - Требует сборку из исходников
   - Зависимости: Bazel, OpenCV, TensorFlow Lite

2. **OpenCV** (для захвата камеры)
   - Уже используется в проекте (но не найден в системе)
   - Установка: `sudo apt install libopencv-dev`

3. **TensorFlow Lite**
   - Для инференса нейросетей MediaPipe
   - Установка: `sudo apt install libtensorflow-lite-dev`

## Установка MediaPipe

### Шаг 1: Установка Bazel

```bash
# Download Bazel
wget https://github.com/bazelbuild/bazel/releases/download/6.1.0/bazel-6.1.0-linux-x86_64
chmod +x bazel-6.1.0-linux-x86_64
sudo mv bazel-6.1.0-linux-x86_64 /usr/local/bin/bazel
```

### Шаг 2: Клонирование MediaPipe

```bash
cd /home/savva/Documents/ARLauncherProject/third_party
git clone https://github.com/google/mediapipe.git
cd mediapipe
```

### Шаг 3: Сборка MediaPipe

```bash
# Сборка Hand Tracking graph
bazel build -c opt mediapipe/graphs/hand_tracking/subgraphs/hand_detection_gpu.pbtxt
bazel build -c opt mediapipe/graphs/hand_tracking/subgraphs/hand_landmark_gpu.pbtxt
```

## Интеграция в проект

### Структура файлов

```
LensEngineSDK/
├── include/
│   └── HandTracker.h          # Новый заголовок
├── src/
│   └── HandTracker.cpp        # Реализация
```

### HandTracker.h

```cpp
#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>

namespace LensEngine {

struct HandLandmark {
    glm::vec3 position;        // 3D позиция
    glm::vec2 screenPosition;  // 2D позиция на экране
    float confidence;          // Уверенность детекции
    int id;                    // ID точки (0-20 для 21 точек руки)
};

struct Hand {
    std::vector<HandLandmark> landmarks;
    glm::vec3 center;
    float confidence;
    bool isLeftHand;
};

class HandTracker {
public:
    HandTracker();
    ~HandTracker();
    
    // Инициализация MediaPipe
    bool initialize();
    
    // Детекция рук на кадре
    std::vector<Hand> detectHands(const cv::Mat& frame);
    
    // Получение последнего результата
    std::vector<Hand> getLastHands() const;
    
private:
    // MediaPipe calculator graph
    void* m_calculatorGraph;  // MediaPipe CalculatorGraph*
    
    // Последний результат
    std::vector<Hand> m_lastHands;
    
    // Конвертация MediaPipe output в наш формат
    Hand convertMediaPipeHand(const void* mediaPipeHand);
};

} // namespace LensEngine
```

### HandTracker.cpp

```cpp
#include "HandTracker.h"
#include <mediapipe/framework/calculator_framework.h>
#include <mediapipe/framework/formats/image_frame.h>
#include <mediapipe/framework/formats/image_frame_opencv.h>

namespace LensEngine {

HandTracker::HandTracker()
    : m_calculatorGraph(nullptr)
{
}

HandTracker::~HandTracker() {
    if (m_calculatorGraph) {
        // Cleanup MediaPipe graph
    }
}

bool HandTracker::initialize() {
    // Инициализация MediaPipe CalculatorGraph
    // Загрузка hand_detection_gpu.pbtxt и hand_landmark_gpu.pbtxt
    
    std::cout << "[HandTracker] Initialized with MediaPipe" << std::endl;
    return true;
}

std::vector<Hand> HandTracker::detectHands(const cv::Mat& frame) {
    std::vector<Hand> hands;
    
    if (!m_calculatorGraph) {
        std::cerr << "[HandTracker] Not initialized" << std::endl;
        return hands;
    }
    
    // Конвертация cv::Mat в MediaPipe ImageFrame
    // Запуск CalculatorGraph
    // Получение результатов
    
    m_lastHands = hands;
    return hands;
}

std::vector<Hand> HandTracker::getLastHands() const {
    return m_lastHands;
}

Hand HandTracker::convertMediaPipeHand(const void* mediaPipeHand) {
    Hand hand;
    // Конвертация MediaPipe NormalizedLandmarkList в наш Hand формат
    return hand;
}

} // namespace LensEngine
```

### Обновление CMakeLists.txt

```cmake
# Добавить в LENSENGINE_SOURCES
src/HandTracker.cpp

# Добавить в LENSENGINE_HEADERS
include/HandTracker.h

# Добавить зависимости MediaPipe
if(MEDIAPIPE_FOUND)
    target_include_directories(LensEngineSDK PRIVATE ${MEDIAPIPE_INCLUDE_DIRS})
    target_link_libraries(LensEngineSDK PRIVATE ${MEDIAPIPE_LIBRARIES})
endif()
```

## Использование в LensEngine

### Интеграция в LensEngine.cpp

```cpp
#include "HandTracker.h"

class LensEngineCore {
private:
    std::unique_ptr<HandTracker> m_handTracker;
    
public:
    bool initialize() {
        // ... существующий код ...
        
        m_handTracker = std::make_unique<HandTracker>();
        m_handTracker->initialize();
        
        return true;
    }
    
    void processRGBData(const uint8_t* data, size_t size, uint32_t width, uint32_t height, uint64_t timestamp) {
        // ... существующий код ...
        
        // Hand tracking
        if (m_handTracker && width <= 640 && height <= 480) {
            cv::Mat frame(height, width, CV_8UC3, const_cast<uint8_t*>(data));
            auto hands = m_handTracker->detectHands(frame);
            
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_hands = hands;
        }
    }
    
    std::vector<Hand> getHands() const {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        return m_hands;
    }
    
private:
    std::vector<Hand> m_hands;
};
```

## Альтернативы MediaPipe

Если MediaPipe слишком сложен для установки:

### 1. OpenCV Hand Tracking

```bash
sudo apt install libopencv-dev
```

Использование OpenCV DNN с предобученной моделью:
- Модель: https://github.com/opencv/opencv_zoo/tree/master/models/hand_tracking

### 2. Simple Hand Detection

Простая детекция кожи + контуров (менее точная, но быстрая):
```cpp
cv::Mat skinMask;
cv::inRange(frame, cv::Scalar(0, 48, 80), cv::Scalar(20, 255, 255), skinMask);
// Найти контуры и определить руки
```

## Рекомендации

1. **Начать с OpenCV DNN** - проще интеграция
2. **MediaPipe** - если нужна максимальная точность
3. **Для AR очков** - использовать GPU ускорение (MediaPipe GPU)

## Текущий статус

- ❌ MediaPipe не установлен
- ❌ OpenCV не найден в системе
- ✅ Структура кода готова
- ⏳ Ожидает установки зависимостей

## Следующие шаги

1. Установить OpenCV: `sudo apt install libopencv-dev`
2. Решить: MediaPipe или OpenCV DNN для hand tracking
3. Реализовать HandTracker
4. Интегрировать в LensEngine
5. Протестировать
