# 🚀 Быстрая справка - AR Launcher

## 📱 Что берем с iPhone

```swift
// 1. IMU (100 Hz)
userAcceleration: (x, y, z) m/s²     // БЕЗ гравитации!
rotationRate: (x, y, z) rad/s

// 2. RGB видео (60 FPS)
frame.capturedImage: 720x405 пиксели

// 3. LiDAR (30 Hz)
depthData: облако 3D точек
```

---

## 🖥️ Как обрабатываем

```
iPhone → Network → Linux
                    ↓
            LensEngineCore
                    ↓
        ┌───────────┴───────────┐
        ↓                       ↓
   KalmanFilter6DOF      ARDataProcessor
   (100 Hz)              (60 FPS)
   ├─ predictIMU()       ├─ extractFeatures()
   └─ updateVO()         ├─ processVisualOdometry()
                         └─ estimateLight()
```

---

## 🔄 Основной цикл

### 1. IMU обновление (каждые 10ms)
```cpp
KalmanFilter6DOF::predictIMU(imu, dt)
{
    // Интегрируем ускорение → скорость → позиция
    // Интегрируем гироскоп → ротация
    // Затухаем скорость (0.90-0.98)
}
```

### 2. Видео обработка (каждые 16.7ms)
```cpp
ARDataProcessor::processFrame(frame)
{
    // Извлекаем 100 feature points
    // Сохраняем для следующего кадра
}
```

### 3. Visual Odometry (каждые 33ms)
```cpp
ARDataProcessor::processVisualOdometryFixed()
{
    // Optical Flow → Essential Matrix → Pose Recovery
    // Триангуляция → SolvePnPRansac
    // Результат: позиция + ротация + confidence
}
```

### 4. Kalman Filter коррекция (каждые 33ms)
```cpp
KalmanFilter6DOF::updateVO(voPos, voRot, confidence)
{
    // Если телефон неподвижен → отклоняем VO
    // Иначе → применяем коррекцию дрифта
}
```

---

## 📊 Ключевые параметры

| Параметр | Значение | Назначение |
|----------|----------|-----------|
| `VELOCITY_DECAY_STATIONARY` | 0.90 | Затухание когда неподвижен |
| `VELOCITY_DECAY_MOVING` | 0.98 | Затухание когда движется |
| `STATIONARY_ACCEL_THRESHOLD` | 0.1 m/s² | Порог неподвижности |
| `STATIONARY_GYRO_THRESHOLD` | 0.05 rad/s | Порог неподвижности |
| `VO_CONFIDENCE_THRESHOLD` | 0.15 | Минимум для обновления |
| `KALMAN_GAIN_MAX` | 0.3 | Максимум усиления |
| `FEATURE_COUNT` | 100 | Количество точек |

---

## 🎯 Координатные системы

```
iPhone Camera Frame:
  X = right
  Y = up
  Z = backward

↓ (используется везде)

World Frame (ARKit style):
  X = right
  Y = up
  Z = backward

OpenCV (внутри VO):
  X = right
  Y = down
  Z = forward
  
  ↓ (преобразуем)
  
  World.X = OpenCV.X
  World.Y = -OpenCV.Y
  World.Z = -OpenCV.Z
```

---

## 🔍 Диагностика

### Проверить IMU данные:
```
[IMU DIAGNOSTICS]
Samples: 301
📊 ACCELEROMETER:
  Magnitude: 0.01 m/s²
  ✅ STATIONARY (если < 0.5)
  
🔄 GYROSCOPE:
  Mean: [-0.005, -0.006, -0.011]
  ✅ GYRO RANGE OK
```

### Проверить VO:
```
[VO] Features: 95 Confidence: 0.95
[VO] Triangulated: 90 | Inliers: 85 | Depth: 2.50m
```

### Проверить позицию:
```
[POS] X: -0.001 Y: 0.000 Z: -0.000
[ROT] Pitch: 180.0° Roll: 0.0° Yaw: 0.0°
```

---

## ⚙️ Основные классы

### KalmanFilter6DOF
```cpp
void initialize();
void predictIMU(const RawIMUData& imu, double dt);
void updateVO(const glm::vec3& voPos, const glm::quat& voRot, float confidence);
CameraPose getCurrentPose() const;
```

### ARDataProcessor
```cpp
std::vector<FeaturePoint> extractFeaturePoints(const RGBImage &rgbImage);
CameraPose processVisualOdometry(...);
CameraPose processVisualOdometryFixed(...);
void processIMUData(const RawIMUData &imuData);
```

### LensEngineCore
```cpp
bool initialize();
void processRGBData(const uint8_t* data, ...);
void processIMUData(const RawIMUData& imuData);
void processLidarData(const std::vector<glm::vec3>& points);
CameraPose getCurrentPose() const;
```

---

## 🚨 Частые проблемы

### ❌ Дрифт позиции (X постоянно уменьшается)
**Причина:** VO дает неправильные данные когда телефон неподвижен  
**Решение:** Используется детектор неподвижности + отклонение VO

### ❌ Скачки позиции
**Причина:** Большая инновация от VO  
**Решение:** Проверка `innovationMag > 0.5f` → отклонить

### ❌ Неправильная ротация
**Причина:** Неправильное преобразование координат  
**Решение:** Используется правильное преобразование (Y и Z инвертируются)

### ❌ Задержка обновления
**Причина:** Асинхронная обработка видео  
**Решение:** IMU обновляется каждые 10ms, VO каждые 33ms

---

## 📈 Производительность

```
Целевые параметры:
  ├─ IMU обработка: < 1ms
  ├─ VO обработка: < 30ms
  ├─ Kalman Filter: < 1ms
  └─ Общая задержка: < 50ms

Текущие результаты:
  ├─ Video FPS: 60
  ├─ Vulkan FPS: 60
  └─ DeltaTime: 16-17ms
```

---

## 🔗 Файлы проекта

```
ARLauncherProject/
├─ ARStreamer/
│  └─ ARStreamer.swift          ← Сбор данных с iPhone
│
├─ LensEngineSDK/
│  ├─ include/
│  │  ├─ KalmanFilter6DOF.h     ← 6DOF Kalman Filter
│  │  ├─ ARDataProcessor.h      ← Обработка видео
│  │  ├─ LensEngine.h           ← Главный класс
│  │  └─ IMUDiagnostics.h       ← Диагностика IMU
│  │
│  └─ src/
│     ├─ KalmanFilter6DOF.cpp
│     ├─ ARDataProcessor.cpp
│     ├─ LensEngine.cpp
│     ├─ VisualOdometryTracker.cpp
│     └─ IMUDiagnostics.cpp
│
├─ SensorConnector/
│  └─ NetworkServerSimplified.h ← Передача данных
│
└─ ARLauncher/
   └─ Application.cpp           ← Главное приложение
```

---

## 🎓 Как это работает в двух словах

```
1. iPhone отправляет IMU (100 Hz) + RGB (60 FPS)
2. Linux получает и обрабатывает:
   - IMU → интегрируем в позицию/ротацию
   - RGB → извлекаем feature points
3. Каждые 33ms:
   - Вычисляем Visual Odometry
   - Применяем коррекцию дрифта в Kalman Filter
4. Результат: стабильная 6DOF позиция без дрифта
```

---

## 📚 Используемые библиотеки

```cpp
// Компьютерное зрение
#include <opencv2/opencv.hpp>

// Математика
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// STL
#include <vector>
#include <array>
#include <memory>
#include <future>
```

---

## 🔐 Важные константы

```cpp
// Kalman Filter
const float VELOCITY_DECAY_STATIONARY = 0.90f;
const float VELOCITY_DECAY_MOVING = 0.98f;
const float STATIONARY_ACCEL_THRESHOLD = 0.1f;
const float STATIONARY_GYRO_THRESHOLD = 0.05f;

// Visual Odometry
const float VO_CONFIDENCE_THRESHOLD = 0.15f;
const float VO_INNOVATION_MAX = 0.5f;
const float KALMAN_GAIN_MAX = 0.3f;

// Feature Detection
const int FEATURE_COUNT = 100;
const float FEATURE_QUALITY = 0.01f;
const float FEATURE_MIN_DISTANCE = 10.0f;

// Optical Flow
const cv::Size OPTICAL_FLOW_WINDOW = cv::Size(15, 15);
const int OPTICAL_FLOW_LEVELS = 2;
const float OPTICAL_FLOW_ERROR_THRESHOLD = 30.0f;

// SolvePnP
const int SOLVEPNP_ITERATIONS = 200;
const float SOLVEPNP_REPROJECTION_ERROR = 4.0f;
const float SOLVEPNP_CONFIDENCE = 0.999f;
```

---

**Версия:** 1.0  
**Дата:** 2026-06-10  
**Статус:** ✅ Полностью функциональна
