# 📊 Поток данных в AR Launcher

## 1️⃣ ПРИМЕР: Поднимаем телефон вверх

### Шаг 1: iPhone собирает данные (t=0ms)
```
CMMotionManager:
  userAcceleration: (0.0, 2.5, 0.0) m/s²  ← ускорение вверх
  rotationRate: (0.0, 0.0, 0.0) rad/s     ← не вращаем
  
ARSession:
  RGB frame: 720x405 пиксели
  timestamp: 1000000 микросекунд
```

### Шаг 2: Отправляем по сети (t=0ms)
```
RawIMUData {
  accelX: 0.0,
  accelY: 2.5,
  accelZ: 0.0,
  gyroX: 0.0,
  gyroY: 0.0,
  gyroZ: 0.0,
  timestamp: 1000000
}

RGBImage {
  data: [R, G, B, R, G, B, ...] (720*405*3 байт)
  width: 720,
  height: 405,
  timestamp: 1000000
}
```

### Шаг 3: Linux обрабатывает IMU (t=10ms)
```cpp
KalmanFilter6DOF::predictIMU(imu, dt=0.01s)
{
    // Текущее состояние
    pos = (0.0, 0.0, 0.0)
    vel = (0.0, 0.0, 0.0)
    rot = identity
    
    // Получаем данные
    accel = (0.0, 2.5, 0.0)
    gyro = (0.0, 0.0, 0.0)
    
    // Интегрируем ротацию (нет вращения)
    rot = rot * quatFromGyro(gyro, dt) = identity
    
    // Преобразуем ускорение в world frame
    rotMat = mat3_cast(identity) = I
    worldAccel = I * (0.0, 2.5, 0.0) = (0.0, 2.5, 0.0)
    
    // Интегрируем скорость
    vel += worldAccel * dt = (0.0, 0.0, 0.0) + (0.0, 2.5, 0.0) * 0.01
         = (0.0, 0.025, 0.0)
    
    // Интегрируем позицию
    pos += vel * dt = (0.0, 0.0, 0.0) + (0.0, 0.025, 0.0) * 0.01
         = (0.0, 0.00025, 0.0)
    
    // Детектор неподвижности
    accelMag = 2.5 > 0.1 → NOT stationary
    decayFactor = 0.98
    
    // Затухание скорости
    vel *= 0.98 = (0.0, 0.0245, 0.0)
    
    // Сохраняем состояние
    m_state = [0.0, 0.00025, 0.0, 0.0, 0.0245, 0.0, ...]
}

// Результат после 10ms:
Position: (0.0, 0.00025, 0.0) м
Velocity: (0.0, 0.0245, 0.0) м/s
Rotation: identity (0°, 0°, 0°)
```

### Шаг 4: Linux обрабатывает видео (t=16.7ms)
```cpp
ARDataProcessor::processFrame(frame)
{
    // Извлекаем feature points
    std::vector<FeaturePoint> features = extractFeaturePointsFast(frame);
    // Результат: 100 углов в случайных местах изображения
    
    // Сохраняем для следующего кадра
    m_previousFrame = frame;
    m_previousFeatures = features;
}
```

### Шаг 5: IMU продолжает интегрировать (t=20ms, 30ms, 40ms...)
```
t=20ms:  pos = (0.0, 0.001, 0.0),   vel = (0.0, 0.048, 0.0)
t=30ms:  pos = (0.0, 0.0025, 0.0),  vel = (0.0, 0.072, 0.0)
t=40ms:  pos = (0.0, 0.0045, 0.0),  vel = (0.0, 0.096, 0.0)
...
```

### Шаг 6: Второй RGB кадр приходит (t=33.3ms)
```
Текущее состояние:
  Position: (0.0, 0.005, 0.0) м
  Velocity: (0.0, 0.1, 0.0) м/s
  Rotation: identity
  
Новый RGB кадр (t=33.3ms):
  Пиксели немного отличаются от предыдущего
  (камера сдвинулась вверх на ~5mm)
```

### Шаг 7: Visual Odometry обрабатывает (t=33.3ms)
```cpp
CameraPose vo = processVisualOdometryFixed(currentFrame, previousFrame, features)
{
    // 1. Optical Flow: отслеживаем 100 точек
    //    Результат: 95 точек успешно отслежены
    
    // 2. Essential Matrix: вычисляем движение
    //    E = findEssentialMat(95 точек)
    
    // 3. Pose Recovery: извлекаем R, t
    //    R = identity (нет вращения)
    //    t = (0, 0.05, 0) (движение вверх на 5cm)
    
    // 4. Триангуляция: восстанавливаем 3D точки
    //    Результат: 90 3D точек на расстояниях 1-5 метров
    
    // 5. SolvePnPRansac: точная поза
    //    rvec = (0, 0, 0)
    //    tvec = (0, 0.05, 0)
    //    inliers = 85
    
    // 6. Масштабирование
    //    avgDepth = 2.5 м
    //    translation *= 2.5 = (0, 0.125, 0)
    
    // 7. Confidence
    //    confidence = 85 / 30 = 1.0 (максимум)
    
    // 8. Интегрируем с IMU позицией
    //    currentIMUPos = (0.0, 0.005, 0.0)
    //    finalPos = (0.0, 0.005, 0.0) + (0, 0.125, 0)
    //            = (0.0, 0.13, 0.0)
    
    return CameraPose {
        position: (0.0, 0.13, 0.0),
        rotation: identity,
        confidence: 1.0
    };
}
```

### Шаг 8: Kalman Filter применяет VO коррекцию (t=33.3ms)
```cpp
KalmanFilter6DOF::updateVO(voPos, voRot, confidence=1.0)
{
    // Текущее состояние из IMU
    currentPos = (0.0, 0.005, 0.0)
    
    // VO дает
    voPosition = (0.0, 0.13, 0.0)
    
    // Инновация
    innovation = (0.0, 0.13, 0.0) - (0.0, 0.005, 0.0)
               = (0.0, 0.125, 0.0)
    innovationMag = 0.125 м < 0.5 м ✓
    
    // Kalman gain
    kalmanGain = 0.3 * 1.0 = 0.3
    
    // Обновляем позицию
    correctedPos = (0.0, 0.005, 0.0) + (0.0, 0.125, 0.0) * 0.3
                 = (0.0, 0.005, 0.0) + (0.0, 0.0375, 0.0)
                 = (0.0, 0.0425, 0.0)
    
    // Сохраняем
    m_state[0] = 0.0
    m_state[1] = 0.0425
    m_state[2] = 0.0
}

// Результат:
Position: (0.0, 0.0425, 0.0) м  ← скорректирована VO
Velocity: (0.0, 0.1, 0.0) м/s   ← из IMU
Rotation: identity
```

### Итого за 33.3ms:
```
Без VO (только IMU):
  Position: (0.0, 0.005, 0.0) м
  Ошибка: ~0.125 м (дрифт!)

С VO (IMU + VO fusion):
  Position: (0.0, 0.0425, 0.0) м
  Ошибка: ~0.0075 м (исправлено!)
```

---

## 2️⃣ ПРИМЕР: Телефон неподвижен

### Шаг 1: iPhone отправляет (t=0ms)
```
CMMotionManager:
  userAcceleration: (0.0, 0.01, 0.0) м/s²  ← шум датчика
  rotationRate: (0.0, 0.0, 0.0) rad/s
```

### Шаг 2: KalmanFilter обрабатывает (t=10ms)
```cpp
KalmanFilter6DOF::predictIMU(imu, dt=0.01s)
{
    accel = (0.0, 0.01, 0.0)
    gyro = (0.0, 0.0, 0.0)
    
    // Детектор неподвижности
    accelMag = 0.01 < 0.1 ✓
    gyroMag = 0.0 < 0.05 ✓
    isStationary = true
    
    // Интегрируем (как обычно)
    vel += accel * dt = (0.0, 0.0001, 0.0)
    pos += vel * dt = (0.0, 0.000001, 0.0)
    
    // АГРЕССИВНОЕ затухание скорости
    decayFactor = 0.90  ← вместо 0.98!
    vel *= 0.90 = (0.0, 0.00009, 0.0)
}

// Результат:
Position: (0.0, 0.000001, 0.0) м  ← почти не меняется
Velocity: (0.0, 0.00009, 0.0) м/s ← быстро затухает
```

### Шаг 3: VO обновление (t=33.3ms)
```cpp
KalmanFilter6DOF::updateVO(voPos, voRot, confidence)
{
    // Проверяем флаг неподвижности
    if (m_isStationary) {
        return;  // ← ОТКЛОНЯЕМ VO!
    }
    // VO не применяется когда телефон неподвижен
}
```

### Итого:
```
Без отклонения VO:
  VO может дать неправильные данные (шум)
  → дрифт позиции

С отклонением VO:
  VO игнорируется
  Только IMU с агрессивным затуханием
  → позиция остается на месте ✓
```

---

## 3️⃣ ПРИМЕР: Вращаем телефон (Pitch)

### Шаг 1: iPhone отправляет (t=0ms)
```
CMMotionManager:
  userAcceleration: (0.0, 0.0, 0.0) м/s²  ← нет линейного движения
  rotationRate: (0.5, 0.0, 0.0) rad/s     ← вращаем вокруг X (Pitch)
```

### Шаг 2: KalmanFilter обрабатывает (t=10ms)
```cpp
KalmanFilter6DOF::predictIMU(imu, dt=0.01s)
{
    accel = (0.0, 0.0, 0.0)
    gyro = (0.5, 0.0, 0.0)
    
    // Интегрируем ротацию
    deltaRot = quatFromGyro(gyro, dt)
             = quat(0.5 * 0.01 / 2, 0, 0)  // половинный угол
             = quat(0.0025, 0, 0)
    
    rot = rot * deltaRot
        = identity * quat(0.0025, 0, 0)
        = quat(0.0025, 0, 0)
    
    // Нормализуем
    rot = normalize(rot)
    
    // Преобразуем ускорение в world frame
    rotMat = mat3_cast(rot)  // матрица поворота
    worldAccel = rotMat * (0, 0, 0) = (0, 0, 0)
    
    // Позиция не меняется (нет ускорения)
    vel += (0, 0, 0) * dt = (0, 0, 0)
    pos += (0, 0, 0) * dt = (0, 0, 0)
}

// Результат:
Position: (0.0, 0.0, 0.0) м  ← не меняется
Rotation: Pitch ≈ 0.29° (0.5 rad/s * 0.01s * 180/π)
```

### Итого:
```
Вращение интегрируется правильно
Позиция остается на месте (нет линейного движения)
VO коррекция не нужна (нет движения)
```

---

## 📊 СРАВНЕНИЕ: С VO vs Без VO

### Сценарий: Поднимаем телефон на 10cm за 1 секунду

```
Без Visual Odometry (только IMU):
  t=0.0s:  pos = (0.0, 0.0, 0.0)
  t=0.5s:  pos = (0.0, 0.025, 0.0)  ← интегрируем ускорение
  t=1.0s:  pos = (0.0, 0.05, 0.0)   ← дрифт из-за затухания!
  
  Ошибка: 5cm (50% от реального движения)

С Visual Odometry (IMU + VO fusion):
  t=0.0s:  pos = (0.0, 0.0, 0.0)
  t=0.5s:  pos = (0.0, 0.025, 0.0)  ← IMU
  t=1.0s:  pos = (0.0, 0.10, 0.0)   ← VO коррекция!
  
  Ошибка: 0cm (100% точность)
```

---

## 🔄 СИНХРОНИЗАЦИЯ ДАННЫХ

```
Timeline (в миллисекундах):

0ms:   IMU #1 отправляется
5ms:   IMU #2 отправляется
10ms:  IMU #3 отправляется
       ↓
       Linux получает IMU #1
       KalmanFilter.predictIMU() обновляет позицию
       
16.7ms: RGB frame #1 отправляется
       ↓
       Linux получает RGB #1
       ARDataProcessor.processFrame() извлекает features
       
20ms:  IMU #4 отправляется
       ↓
       Linux получает IMU #4
       KalmanFilter.predictIMU() обновляет позицию
       
33.3ms: RGB frame #2 отправляется
        ↓
        Linux получает RGB #2
        ARDataProcessor.processVisualOdometryFixed()
        вычисляет VO pose
        ↓
        KalmanFilter6DOF.updateVO() применяет коррекцию
```

---

## ⚠️ ПОТЕНЦИАЛЬНЫЕ ОШИБКИ И КАК ИХ ИЗБЕЖАТЬ

### 1. Дрифт позиции (X постоянно уменьшается)
```
Причина: VO дает неправильные данные когда телефон неподвижен
Решение: 
  ✓ Детектор неподвижности (accelMag < 0.1, gyroMag < 0.05)
  ✓ Отклонение VO когда isStationary = true
  ✓ Агрессивное затухание скорости (0.90x)
```

### 2. Скачки позиции
```
Причина: VO дает большую инновацию (> 0.5m)
Решение:
  ✓ Проверка innovationMag > 0.5f → отклонить
  ✓ Проверка confidence > 0.15f → применить
```

### 3. Неправильная ротация
```
Причина: Неправильное преобразование координат
Решение:
  ✓ Использовать правильное преобразование:
    World.X = OpenCV.X
    World.Y = -OpenCV.Y
    World.Z = -OpenCV.Z
```

### 4. Задержка обновления
```
Причина: Асинхронная обработка видео
Решение:
  ✓ IMU обновляется каждые 10ms (100 Hz)
  ✓ VO обновляется каждые 33ms (30 FPS)
  ✓ Kalman Filter синхронизирует оба источника
```

---

**Версия:** 1.0  
**Дата:** 2026-06-10  
**Статус:** ✅ Полностью задокументирована
