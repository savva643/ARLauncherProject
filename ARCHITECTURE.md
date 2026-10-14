# 🏗️ AR Launcher - Полная архитектура системы

## 📱 1. СБОР ДАННЫХ С iPhone (ARStreamer.swift)

### Что берем с iPhone:

```swift
// CMMotionManager - датчики движения
motionManager.startDeviceMotionUpdates() {
    // 1️⃣ ОРИЕНТАЦИЯ (Euler angles)
    euler.pitch, euler.yaw, euler.roll
    
    // 2️⃣ УСКОРЕНИЕ (userAcceleration - БЕЗ гравитации!)
    userAccel.x, userAccel.y, userAccel.z  // в m/s²
    
    // 3️⃣ УГЛОВАЯ СКОРОСТЬ (gyroscope)
    rotationRate.x, rotationRate.y, rotationRate.z  // в rad/s
    
    // 4️⃣ ГРАВИТАЦИЯ (для справки)
    gravity.x, gravity.y, gravity.z  // в m/s²
    
    // 5️⃣ МАГНИТНОЕ ПОЛЕ (для компаса)
    magneticField.x, magneticField.y, magneticField.z
}

// ARSession - видео с камеры
arSession.run(configuration) {
    // RGB видео 720x405 @ 60 FPS
    frame.capturedImage  // CVPixelBuffer
}

// LiDAR сканер (если есть)
lidarSession.run() {
    // 3D облако точек
    depthData.depthMap  // глубина каждого пикселя
}
```

### Частоты:
- **IMU (акселерометр + гироскоп):** ~100 Hz
- **RGB видео:** 60 FPS (16.7 ms между кадрами)
- **LiDAR:** ~30 Hz

### Координатная система iPhone:
```
Camera frame (фиксирована относительно камеры):
  X = right (вправо)
  Y = up (вверх)
  Z = backward (назад от камеры)

IMU frame (фиксирована относительно корпуса):
  X = right
  Y = up
  Z = backward
  
⚠️ ВАЖНО: Камера и IMU имеют РАЗНЫЕ ориентации!
```

---

## 🔄 2. ПЕРЕДАЧА ДАННЫХ НА Linux (NetworkServerSimplified)

### Протокол передачи:

```cpp
struct RawIMUData {
    float accelX, accelY, accelZ;      // m/s² (userAcceleration)
    float gyroX, gyroY, gyroZ;         // rad/s
    uint64_t timestamp;                 // микросекунды
};

struct RGBImage {
    uint8_t* data;                      // RGB пиксели (720x405x3)
    uint32_t width = 720;
    uint32_t height = 405;
    uint64_t timestamp;
};

struct LidarData {
    std::vector<glm::vec3> points3D;   // облако точек
    uint64_t timestamp;
};

// Отправляются по сети (UDP/TCP):
// - IMU: каждые ~10ms (100 Hz)
// - RGB: каждые ~16.7ms (60 FPS)
// - LiDAR: каждые ~33ms (30 Hz)
```

---

## 🖥️ 3. ОБРАБОТКА НА Linux (LensEngine)

### Архитектура обработки:

```
LensEngineCore (главный класс)
│
├─ KalmanFilter6DOF (6DOF tracking)
│  ├─ predictIMU() ← IMU данные (100 Hz)
│  │  ├─ Интегрирование ускорения → скорость
│  │  ├─ Интегрирование скорости → позиция
│  │  ├─ Интегрирование гироскопа → ротация
│  │  ├─ Детектор неподвижности
│  │  └─ Затухание скорости (0.90-0.98)
│  │
│  └─ updateVO() ← Visual Odometry (30 Hz)
│     ├─ Коррекция дрифта
│     └─ Confidence-based fusion
│
├─ ARDataProcessor (обработка видео)
│  ├─ extractFeaturePointsFast()
│  │  └─ goodFeaturesToTrack() → 100 углов
│  │
│  ├─ processVisualOdometryFixed()
│  │  ├─ Optical Flow (отслеживание точек)
│  │  ├─ Essential Matrix (движение камеры)
│  │  ├─ Pose Recovery (R, t)
│  │  ├─ Триангуляция (3D точки)
│  │  └─ SolvePnPRansac (масштабирование)
│  │
│  └─ estimateLightFast()
│     └─ Средняя яркость кадра
│
├─ Lidar3DProcessor (обработка LiDAR)
│  ├─ Анализ облака точек
│  ├─ Поиск пола
│  └─ Поиск препятствий
│
└─ SpatialMappingSystem (пространственная карта)
   ├─ Виртуальные объекты
   └─ Коллизии
```

---

## 📊 4. ДЕТАЛЬНЫЙ PIPELINE ОБРАБОТКИ

### 4.1 IMU Pipeline (100 Hz)

```cpp
void KalmanFilter6DOF::predictIMU(const RawIMUData& imu, double dt)
{
    // 1️⃣ ПОЛУЧАЕМ ДАННЫЕ
    glm::vec3 accel(imu.accelX, imu.accelY, imu.accelZ);  // m/s²
    glm::vec3 gyro(imu.gyroX, imu.gyroY, imu.gyroZ);      // rad/s
    
    // 2️⃣ ВЫЧИТАЕМ BIAS (смещение датчиков)
    accel -= m_accelBias;
    gyro -= m_gyroBias;
    
    // 3️⃣ ФИЛЬТРУЕМ ШУМ (низкопроходный фильтр)
    filteredAccel = accel * 0.7 + lastAccel * 0.3;
    filteredGyro = gyro * 0.7 + lastGyro * 0.3;
    
    // 4️⃣ ИНТЕГРИРУЕМ РОТАЦИЮ (гироскоп)
    glm::quat deltaRot = quatFromGyro(filteredGyro, dt);
    rotation = quatMultiply(rotation, deltaRot);
    rotation = quatNormalize(rotation);
    
    // 5️⃣ ПРЕОБРАЗУЕМ УСКОРЕНИЕ В WORLD FRAME
    glm::mat3 rotMat = glm::mat3_cast(rotation);
    glm::vec3 worldAccel = rotMat * filteredAccel;
    
    // 6️⃣ ИНТЕГРИРУЕМ СКОРОСТЬ И ПОЗИЦИЮ
    velocity += worldAccel * dt;
    position += velocity * dt;
    
    // 7️⃣ ДЕТЕКТОР НЕПОДВИЖНОСТИ
    bool isStationary = (accelMag < 0.1f && gyroMag < 0.05f);
    
    // 8️⃣ ЗАТУХАНИЕ СКОРОСТИ (предотвращает дрифт)
    float decayFactor = isStationary ? 0.90f : 0.98f;
    velocity *= decayFactor;
    
    // 9️⃣ СОХРАНЯЕМ СОСТОЯНИЕ
    m_state = [x, y, z, vx, vy, vz, qx, qy, qz, qw]
}
```

**Результат:** Позиция и ротация обновляются каждые 10ms

---

### 4.2 Visual Odometry Pipeline (30 Hz)

```cpp
CameraPose ARDataProcessor::processVisualOdometryFixed(
    const RGBImage &currentFrame,
    const RGBImage &previousFrame,
    const std::vector<FeaturePoint> &previousFeatures)
{
    // 1️⃣ ПОДГОТОВКА ИЗОБРАЖЕНИЙ
    cv::Mat prevGray = cvtColor(previousFrame, RGB2GRAY);
    cv::Mat currGray = cvtColor(currentFrame, RGB2GRAY);
    
    // 2️⃣ OPTICAL FLOW - отслеживаем предыдущие точки
    std::vector<cv::Point2f> prevPoints = previousFeatures;
    std::vector<cv::Point2f> currPoints;
    std::vector<uchar> status;
    
    cv::calcOpticalFlowPyrLK(
        prevGray, currGray,
        prevPoints, currPoints,
        status, err,
        cv::Size(15, 15), 2,  // размер окна, уровни пирамиды
        cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 20, 0.01)
    );
    
    // Фильтруем хорошие отслеженные точки
    std::vector<cv::Point2f> goodPrevPoints, goodCurrPoints;
    for (size_t i = 0; i < status.size(); ++i) {
        if (status[i] && err[i] < 30.0f) {
            // Проверяем что точка осталась в кадре
            if (x > 5 && x < width-5 && y > 5 && y < height-5) {
                goodPrevPoints.push_back(prevPoints[i]);
                goodCurrPoints.push_back(currPoints[i]);
            }
        }
    }
    
    if (goodPrevPoints.size() < 8) {
        return fallback_pose;  // Недостаточно точек
    }
    
    // 3️⃣ ESSENTIAL MATRIX - вычисляем движение камеры
    cv::Mat cameraMatrix = (cv::Mat_<double>(3,3) <<
        fx, 0.0, cx,
        0.0, fy, cy,
        0.0, 0.0, 1.0);
    
    cv::Mat essentialMatrix = cv::findEssentialMat(
        goodPrevPoints, goodCurrPoints,
        cameraMatrix,
        cv::RANSAC, 0.999, 1.0
    );
    
    // 4️⃣ POSE RECOVERY - извлекаем R (rotation) и t (translation)
    cv::Mat R, t;
    int inliers = cv::recoverPose(
        essentialMatrix,
        goodPrevPoints, goodCurrPoints,
        cameraMatrix, R, t
    );
    
    if (inliers < 8) {
        return fallback_pose;
    }
    
    // 5️⃣ ТРИАНГУЛЯЦИЯ - восстанавливаем 3D точки
    cv::Mat P1 = cameraMatrix * cv::Mat::eye(3, 4, CV_64F);
    cv::Mat P2 = cameraMatrix * [R | t];  // матрица проекции 2-го кадра
    
    cv::Mat points4D;
    cv::triangulatePoints(P1, P2, goodPrevPoints, goodCurrPoints, points4D);
    
    // Конвертируем в 3D (нормализуем по w)
    std::vector<cv::Point3f> triangulatedPoints;
    for (int i = 0; i < points4D.cols; ++i) {
        float w = points4D.at<float>(3, i);
        if (w > 0.001f) {
            cv::Point3f pt(
                points4D.at<float>(0, i) / w,
                points4D.at<float>(1, i) / w,
                points4D.at<float>(2, i) / w
            );
            
            // Фильтруем по глубине (0.1m - 100m)
            if (pt.z > 0.1f && pt.z < 100.0f) {
                triangulatedPoints.push_back(pt);
            }
        }
    }
    
    if (triangulatedPoints.size() < 8) {
        return fallback_pose;
    }
    
    // 6️⃣ SOLVEPNPRANSAC - точное определение позы с масштабом
    std::vector<cv::Point2f> imagePointsForPnP = goodCurrPoints;
    
    cv::Mat rvec, tvec;
    cv::Mat inliersMask;
    
    bool success = cv::solvePnPRansac(
        triangulatedPoints,           // 3D точки из триангуляции
        imagePointsForPnP,            // 2D точки в текущем кадре
        cameraMatrix,
        cv::Mat::zeros(4, 1, CV_64F),
        rvec, tvec,
        false,                        // useExtrinsicGuess
        200,                          // iterationsCount
        4.0f,                         // reprojectionError (пиксели)
        0.999,                        // confidence
        inliersMask,
        cv::SOLVEPNP_EPNP
    );
    
    if (!success || inliersMask.empty()) {
        return fallback_pose;
    }
    
    // 7️⃣ КОНВЕРТИРУЕМ РЕЗУЛЬТАТЫ
    // Rotation vector → Rotation matrix
    cv::Mat rotationMatrix;
    cv::Rodrigues(rvec, rotationMatrix);
    glm::mat3 rotMat = cv_to_glm(rotationMatrix);
    glm::quat rotation = glm::quat_cast(rotMat);
    
    // Translation (преобразуем координаты)
    glm::vec3 translation(
        tvec.at<double>(0),
        -tvec.at<double>(1),  // инвертируем Y
        -tvec.at<double>(2)   // инвертируем Z
    );
    
    // 8️⃣ МАСШТАБИРОВАНИЕ (используем среднюю глубину)
    float avgDepth = 0.0f;
    for (const auto& pt : triangulatedPoints) {
        avgDepth += pt.z;
    }
    avgDepth /= triangulatedPoints.size();
    
    float depthScale = avgDepth / 1.0f;
    translation *= depthScale;
    
    // 9️⃣ ВЫЧИСЛЯЕМ CONFIDENCE (на основе количества inliers)
    int inlierCount = cv::countNonZero(inliersMask);
    float confidence = std::min(1.0f, (float)inlierCount / 30.0f);
    
    // 🔟 ИНТЕГРИРУЕМ С IMU ПОЗИЦИЕЙ
    glm::vec3 currentIMUPos = m_sensorFusion->getCurrentPose().position;
    glm::vec3 finalPosition = currentIMUPos + translation;
    
    return CameraPose {
        position: finalPosition,
        rotation: rotation,
        confidence: confidence
    };
}
```

**Результат:** Обновление позиции и ротации каждые 33ms (30 FPS)

---

### 4.3 Kalman Filter Fusion (100 Hz)

```cpp
void KalmanFilter6DOF::updateVO(
    const glm::vec3& voPosition,
    const glm::quat& voRotation,
    float confidence)
{
    // 1️⃣ ПРОВЕРЯЕМ ЧТО ТЕЛЕФОН НЕ НЕПОДВИЖЕН
    if (m_isStationary) {
        return;  // VO ненадежна когда нет движения
    }
    
    // 2️⃣ ВЫЧИСЛЯЕМ ИННОВАЦИЮ (разница между VO и фильтром)
    glm::vec3 currentPos(m_state[0], m_state[1], m_state[2]);
    glm::vec3 innovation = voPosition - currentPos;
    float innovationMag = glm::length(innovation);
    
    // 3️⃣ ОТКЛОНЯЕМ СЛИШКОМ БОЛЬШИЕ ИННОВАЦИИ
    if (innovationMag > 0.5f) {
        return;  // Ошибка отслеживания
    }
    
    // 4️⃣ ВЫЧИСЛЯЕМ KALMAN GAIN (зависит от confidence)
    float kalmanGain = 0.3f * confidence;  // 0-0.3
    
    // 5️⃣ ОБНОВЛЯЕМ ПОЗИЦИЮ
    glm::vec3 correctedPos = currentPos + innovation * kalmanGain;
    
    // 6️⃣ ОБНОВЛЯЕМ РОТАЦИЮ (более консервативно)
    glm::quat currentRot(m_state[9], m_state[6], m_state[7], m_state[8]);
    glm::quat correctedRot = glm::slerp(currentRot, voRotation, kalmanGain * 0.5f);
    correctedRot = quatNormalize(correctedRot);
    
    // 7️⃣ СОХРАНЯЕМ ОБНОВЛЕННОЕ СОСТОЯНИЕ
    m_state[0] = correctedPos.x;
    m_state[1] = correctedPos.y;
    m_state[2] = correctedPos.z;
    m_state[6] = correctedRot.x;
    m_state[7] = correctedRot.y;
    m_state[8] = correctedRot.z;
    m_state[9] = correctedRot.w;
}
```

**Результат:** Коррекция дрифта, стабильная позиция и ротация

---

## 🎯 5. КООРДИНАТНЫЕ СИСТЕМЫ

### Преобразования:

```
iPhone Camera Frame:
  X = right
  Y = up
  Z = backward

↓ (используется везде в коде)

World Frame (ARKit style):
  X = right (вправо)
  Y = up (вверх)
  Z = backward (назад)

OpenCV Camera Frame (внутри VO):
  X = right
  Y = down
  Z = forward

↓ (преобразуем в World Frame)

  World.X = OpenCV.X
  World.Y = -OpenCV.Y
  World.Z = -OpenCV.Z
```

---

## 📈 6. ЧАСТОТЫ И СИНХРОНИЗАЦИЯ

```
iPhone (100 Hz)
  ├─ IMU: 100 Hz (10ms)
  ├─ RGB: 60 FPS (16.7ms)
  └─ LiDAR: 30 Hz (33ms)
       ↓
    Network
       ↓
Linux (LensEngine)
  ├─ KalmanFilter6DOF.predictIMU(): 100 Hz (10ms)
  │  └─ Обновляет позицию/ротацию
  │
  ├─ ARDataProcessor.processFrame(): 60 FPS (16.7ms)
  │  ├─ Извлекает feature points
  │  └─ Обновляет tracked features
  │
  ├─ ARDataProcessor.processVisualOdometryFixed(): 30 FPS (33ms)
  │  └─ Вычисляет VO pose
  │
  └─ KalmanFilter6DOF.updateVO(): 30 FPS (33ms)
     └─ Коррирует дрифт
```

---

## 🔍 7. ДИАГНОСТИКА (IMUDiagnostics)

```cpp
// Каждые 100 samples выводим статистику:

📊 ACCELEROMETER (userAcceleration - без гравитации) (m/s²):
  Min: [-0.10, -0.03, -0.06]
  Max: [0.19, 0.03, 0.07]
  Mean: [-0.01, -0.00, 0.00]
  Magnitude: 0.01 m/s²
  ✅ STATIONARY (телефон неподвижен)

🔄 GYROSCOPE (rad/s):
  Min: [-0.213, -0.378, -0.481]
  Max: [0.255, 0.408, 0.290]
  Mean: [-0.008, -0.008, -0.016]
  ✅ GYRO RANGE OK (stationary)
```

---

## 🚀 8. ПРОИЗВОДИТЕЛЬНОСТЬ

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

## 📋 9. ОСНОВНЫЕ КОМПОНЕНТЫ И ИХ РОЛИ

| Компонент | Роль | Частота | Задержка |
|-----------|------|---------|----------|
| **KalmanFilter6DOF** | Fusion IMU + VO | 100 Hz | < 1ms |
| **ARDataProcessor** | Обработка видео | 60 FPS | < 30ms |
| **VisualOdometryTracker** | VO pipeline | 30 FPS | < 30ms |
| **Lidar3DProcessor** | Анализ облака точек | 30 Hz | < 30ms |
| **SpatialMappingSystem** | Пространственная карта | 30 Hz | < 10ms |
| **IMUDiagnostics** | Мониторинг датчиков | 100 Hz | < 1ms |

---

## 🔐 10. КЛЮЧЕВЫЕ ПАРАМЕТРЫ

```cpp
// Kalman Filter
const float VELOCITY_DECAY_STATIONARY = 0.90f;  // 10% затухание
const float VELOCITY_DECAY_MOVING = 0.98f;      // 2% затухание
const float STATIONARY_ACCEL_THRESHOLD = 0.1f;  // m/s²
const float STATIONARY_GYRO_THRESHOLD = 0.05f;  // rad/s

// Visual Odometry
const float VO_CONFIDENCE_THRESHOLD = 0.15f;    // Минимум для обновления
const float VO_INNOVATION_MAX = 0.5f;           // Максимум инновации (м)
const float KALMAN_GAIN_MAX = 0.3f;             // Максимум усиления

// Feature Detection
const int FEATURE_COUNT = 100;                  // Количество точек
const float FEATURE_QUALITY = 0.01f;            // Качество углов
const float FEATURE_MIN_DISTANCE = 10.0f;       // Минимум расстояния

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

## 📚 11. ИСПОЛЬЗУЕМЫЕ БИБЛИОТЕКИ

```cpp
// OpenCV (компьютерное зрение)
#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>        // Feature detection
#include <opencv2/calib3d.hpp>           // Calibration & 3D geometry
#include <opencv2/imgproc.hpp>           // Image processing

// GLM (математика)
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>        // Quaternions
#include <glm/gtc/matrix_transform.hpp>  // Matrix operations

// STL
#include <vector>
#include <array>
#include <memory>
#include <future>
#include <chrono>
```

---

## 🎓 ИТОГО: Как работает система

```
1. iPhone отправляет:
   - IMU (100 Hz): accel, gyro
   - RGB (60 FPS): видео 720x405
   - LiDAR (30 Hz): облако точек

2. Linux получает и обрабатывает:
   - KalmanFilter6DOF.predictIMU() → интегрирует IMU
   - ARDataProcessor.processFrame() → извлекает feature points
   - ARDataProcessor.processVisualOdometryFixed() → вычисляет VO
   - KalmanFilter6DOF.updateVO() → корректирует дрифт

3. Результат:
   - Стабильная 6DOF позиция (X, Y, Z)
   - Стабильная ротация (Pitch, Roll, Yaw)
   - Без дрифта благодаря VO коррекции
   - Работает при любой ориентации телефона
```

---

**Версия:** 1.0  
**Дата:** 2026-06-10  
**Статус:** ✅ Полностью функциональна
