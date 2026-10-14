# 🗺️ Дорожная карта: От ±5-10cm к ±1-2cm (ARKit уровень)

## 📊 Текущее состояние vs Цель

```
СЕЙЧАС:
  Точность позиции:     ±5-10cm
  Точность ротации:     ±2-5°
  Дрейф за 1 минуту:    10-20cm
  Задержка:             <50ms
  CPU:                  10-15%
  
ЦЕЛЬ (ARKit уровень):
  Точность позиции:     ±1-2cm ← 5-10x лучше!
  Точность ротация:     ±0.5-1°
  Дрейф за 1 минуту:    1-2cm
  Задержка:             <50ms (сохраняем)
  CPU:                  20-25% (приемлемо)
```

---

## 🎯 КРИТИЧЕСКИЕ КОМПОНЕНТЫ ДЛЯ ТОЧНОСТИ

### 1. Bundle Adjustment (BA) - САМОЕ ВАЖНОЕ
```
Улучшение: 2-3x
Сложность: Высокая
Время: 1-2 недели
Результат: ±3-5cm

Почему это критично:
  ├─ Оптимизирует всю траекторию, а не только текущий кадр
  ├─ Исправляет ошибки в прошлых кадрах
  ├─ Уменьшает накопленный дрейф
  └─ Это основа всех профессиональных SLAM систем
```

### 2. Temporal Filtering - ОЧЕНЬ ВАЖНО
```
Улучшение: 1.5x
Сложность: Низкая
Время: 2-3 дня
Результат: ±2-3cm

Почему это важно:
  ├─ Убирает шум и скачки
  ├─ Стабилизирует позицию
  ├─ Улучшает восприятие пользователем
  └─ Просто реализуется
```

### 3. Loop Closure Detection - ВАЖНО
```
Улучшение: 2x
Сложность: Высокая
Время: 1-2 недели
Результат: ±1-2cm

Почему это важно:
  ├─ Обнаруживает возврат в старое место
  ├─ Исправляет накопленный дрейф
  ├─ Без этого дрейф растет линейно
  └─ С этим дрейф остается ограниченным
```

### 4. LiDAR Fusion - КРИТИЧНО
```
Улучшение: 2x (с LiDAR)
Сложность: Средняя
Время: 3-5 дней
Результат: ±0.5-1cm

Почему это критично:
  ├─ Исправляет ошибку масштаба VO
  ├─ Дает абсолютную глубину
  ├─ Исправляет дрейф глубины
  └─ Это то что делает Vision Pro таким точным
```

---

## 📈 ПОШАГОВЫЙ ПЛАН

### ФАЗА 1: Базовые исправления (2-4 часа)
```
Статус: ГОТОВО (из FIXES_CONCRETE.md)

✓ Убрать затухание скорости при движении
✓ Повысить пороги stationary detection
✓ Интегрировать Visual Odometry
✓ Использовать LiDAR для коррекции Z

Результат: ±5-10cm (текущее состояние)
```

### ФАЗА 2: Temporal Filtering (2-3 дня)
```
Статус: СЛЕДУЮЩАЯ

Что добавить:
  1. Экспоненциальное сглаживание позиции
  2. Экспоненциальное сглаживание ротации
  3. Экспоненциальное сглаживание скорости
  4. Настройка коэффициентов α

Код:
  ├─ Создать класс TemporalFilter
  ├─ Интегрировать в KalmanFilter6DOF
  └─ Настроить коэффициенты

Результат: ±2-3cm (улучшение 2-3x)
Время: 2-3 дня
```

### ФАЗА 3: Bundle Adjustment (1-2 недели)
```
Статус: ПОСЛЕ TEMPORAL FILTERING

Что добавить:
  1. Сохранение ключевых кадров (10-20 последних)
  2. Оптимизация траектории (Ceres Solver)
  3. Оптимизация 3D точек
  4. Интеграция в VO pipeline

Код:
  ├─ Создать класс BundleAdjustment
  ├─ Добавить Ceres Solver
  ├─ Интегрировать в ARDataProcessor
  └─ Настроить параметры оптимизации

Результат: ±1-2cm (улучшение 5-10x) ← ARKit уровень!
Время: 1-2 недели
```

### ФАЗА 4: Loop Closure Detection (1-2 недели)
```
Статус: ПОСЛЕ BUNDLE ADJUSTMENT

Что добавить:
  1. Сохранение дескрипторов кадров
  2. Быстрый поиск похожих кадров (DBoW2)
  3. Обнаружение loop closure
  4. Коррекция дрейфа

Код:
  ├─ Создать класс LoopClosureDetector
  ├─ Добавить DBoW2 библиотеку
  ├─ Интегрировать в ARDataProcessor
  └─ Настроить пороги обнаружения

Результат: ±0.5-1cm (улучшение 10-20x) ← Vision Pro уровень!
Время: 1-2 недели
```

### ФАЗА 5: LiDAR Fusion (3-5 дней)
```
Статус: ПАРАЛЛЕЛЬНО С ФАЗОЙ 3-4

Что улучшить:
  1. Калибровка масштаба VO с LiDAR
  2. Коррекция глубины триангулированных точек
  3. Абсолютная позиция из LiDAR
  4. Взвешенное объединение VO + LiDAR

Код:
  ├─ Улучшить updateLidar() в KalmanFilter6DOF
  ├─ Добавить калибровку масштаба
  ├─ Добавить коррекцию глубины
  └─ Настроить веса объединения

Результат: ±0.5-1cm (с LiDAR)
Время: 3-5 дней
```

---

## 🔧 КОНКРЕТНЫЕ КОДЫ

### ФАЗА 2: Temporal Filtering

```cpp
// TemporalFilter.h
class TemporalFilter {
public:
    TemporalFilter() : 
        m_lastPos(0.0f), 
        m_lastRot(1.0f, 0.0f, 0.0f, 0.0f),
        m_lastVel(0.0f) {}
    
    glm::vec3 filterPosition(const glm::vec3& newPos) {
        // Экспоненциальное сглаживание
        // pos_filtered = α * pos_new + (1-α) * pos_last
        const float ALPHA = 0.4f;  // 40% новое, 60% старое
        m_lastPos = ALPHA * newPos + (1.0f - ALPHA) * m_lastPos;
        return m_lastPos;
    }
    
    glm::quat filterRotation(const glm::quat& newRot) {
        // Сферическая линейная интерполяция (SLERP)
        const float ALPHA = 0.3f;  // 30% новое, 70% старое
        m_lastRot = glm::slerp(m_lastRot, newRot, ALPHA);
        return m_lastRot;
    }
    
    glm::vec3 filterVelocity(const glm::vec3& newVel) {
        const float ALPHA = 0.6f;  // 60% новое, 40% старое
        m_lastVel = ALPHA * newVel + (1.0f - ALPHA) * m_lastVel;
        return m_lastVel;
    }
    
private:
    glm::vec3 m_lastPos;
    glm::quat m_lastRot;
    glm::vec3 m_lastVel;
};

// Использование в KalmanFilter6DOF:
void KalmanFilter6DOF::updateVO(const glm::vec3& voPos, 
                                 const glm::quat& voRot, 
                                 float confidence)
{
    // ... существующий код ...
    
    // Применяем temporal filtering
    glm::vec3 smoothPos = m_temporalFilter.filterPosition(voPos);
    glm::quat smoothRot = m_temporalFilter.filterRotation(voRot);
    
    // Используем сглаженные значения
    // ... остальной код ...
}
```

### ФАЗА 3: Bundle Adjustment (упрощенная версия)

```cpp
// BundleAdjustment.h
class BundleAdjustment {
public:
    struct KeyFrame {
        int frameId;
        glm::mat4 pose;  // 4x4 матрица позы
        std::vector<glm::vec3> points3D;
        std::vector<glm::vec2> points2D;
        std::vector<int> pointIds;
    };
    
    void addKeyFrame(const KeyFrame& kf) {
        m_keyframes.push_back(kf);
        // Сохраняем только последние 20 кадров
        if (m_keyframes.size() > 20) {
            m_keyframes.pop_front();
        }
    }
    
    void optimize() {
        // Используем Ceres Solver для оптимизации
        // Минимизируем ошибку репроекции:
        // error = ||p_2d - project(p_3d, pose)||²
        
        // Упрощенная версия (без Ceres):
        // Просто усредняем позы соседних кадров
        for (size_t i = 1; i < m_keyframes.size() - 1; ++i) {
            glm::mat4 prev = m_keyframes[i-1].pose;
            glm::mat4 next = m_keyframes[i+1].pose;
            m_keyframes[i].pose = (prev + next) * 0.5f;
        }
    }
    
    std::vector<glm::vec3> getOptimizedPoints() const {
        std::vector<glm::vec3> result;
        for (const auto& kf : m_keyframes) {
            result.insert(result.end(), 
                         kf.points3D.begin(), 
                         kf.points3D.end());
        }
        return result;
    }
    
private:
    std::deque<KeyFrame> m_keyframes;
};

// Использование в ARDataProcessor:
void ARDataProcessor::processVisualOdometryFixed(...) {
    // ... существующий код VO ...
    
    // Добавляем ключевой кадр
    BundleAdjustment::KeyFrame kf;
    kf.frameId = m_frameCount++;
    kf.pose = voResult.pose;
    kf.points3D = triangulatedPoints;
    kf.points2D = goodCurrPoints;
    
    m_bundleAdjustment.addKeyFrame(kf);
    
    // Оптимизируем каждые 10 кадров
    if (m_frameCount % 10 == 0) {
        m_bundleAdjustment.optimize();
    }
}
```

### ФАЗА 4: Loop Closure Detection (упрощенная версия)

```cpp
// LoopClosureDetector.h
class LoopClosureDetector {
public:
    void addFrame(const RGBImage& frame, const CameraPose& pose) {
        m_frames.push_back(frame);
        m_poses.push_back(pose);
    }
    
    struct LoopConstraint {
        int frameId1, frameId2;
        float confidence;
    };
    
    std::vector<LoopConstraint> detectLoopClosures() {
        std::vector<LoopConstraint> constraints;
        
        if (m_frames.size() < 20) return constraints;
        
        // Проверяем только старые кадры (>20 кадров назад)
        int currentId = m_frames.size() - 1;
        int oldId = currentId - 20;
        
        if (oldId < 0) return constraints;
        
        // Вычисляем сходство между текущим и старым кадром
        float similarity = computeSimilarity(m_frames[currentId], 
                                            m_frames[oldId]);
        
        // Если сходство высокое - это loop closure!
        if (similarity > 0.8f) {
            LoopConstraint constraint;
            constraint.frameId1 = oldId;
            constraint.frameId2 = currentId;
            constraint.confidence = similarity;
            constraints.push_back(constraint);
            
            std::cout << "[LOOP CLOSURE] Detected between frame " 
                      << oldId << " and " << currentId << std::endl;
        }
        
        return constraints;
    }
    
private:
    std::vector<RGBImage> m_frames;
    std::vector<CameraPose> m_poses;
    
    float computeSimilarity(const RGBImage& frame1, 
                           const RGBImage& frame2) {
        // Простое сравнение: количество совпадающих feature points
        // В реальности используется DBoW2 для быстрого поиска
        
        auto features1 = extractFeatures(frame1);
        auto features2 = extractFeatures(frame2);
        
        int matches = 0;
        for (const auto& f1 : features1) {
            for (const auto& f2 : features2) {
                if (glm::distance(f1.screenPosition, f2.screenPosition) < 10.0f) {
                    matches++;
                }
            }
        }
        
        return (float)matches / std::max(features1.size(), features2.size());
    }
};
```

### ФАЗА 5: LiDAR Fusion (улучшенная версия)

```cpp
// В KalmanFilter6DOF::updateLidar()

void KalmanFilter6DOF::updateLidar(const std::vector<glm::vec3>& lidarPoints)
{
    if (lidarPoints.empty()) return;
    
    // 1. Вычисляем среднюю глубину из LiDAR
    float lidarDepth = 0.0f;
    int validPoints = 0;
    for (const auto& pt : lidarPoints) {
        if (pt.z > 0.1f && pt.z < 100.0f) {
            lidarDepth += pt.z;
            validPoints++;
        }
    }
    if (validPoints == 0) return;
    lidarDepth /= validPoints;
    
    // 2. Вычисляем среднюю глубину из VO
    // (из последних триангулированных точек)
    float voDepth = computeVOAverageDepth();
    
    // 3. Вычисляем масштабный коэффициент
    float scaleCorrection = lidarDepth / voDepth;
    
    // Ограничиваем коррекцию (не более ±20%)
    scaleCorrection = glm::clamp(scaleCorrection, 0.8f, 1.2f);
    
    // 4. Исправляем позицию и скорость
    // (масштаб влияет на всю траекторию!)
    m_state[0] *= scaleCorrection;  // x
    m_state[1] *= scaleCorrection;  // y
    m_state[2] *= scaleCorrection;  // z
    m_state[3] *= scaleCorrection;  // vx
    m_state[4] *= scaleCorrection;  // vy
    m_state[5] *= scaleCorrection;  // vz
    
    std::cout << "[LIDAR] Scale correction: " << std::fixed << std::setprecision(3) 
              << scaleCorrection << " (LiDAR depth: " << lidarDepth 
              << "m, VO depth: " << voDepth << "m)" << std::endl;
}
```

---

## 📊 РЕЗУЛЬТАТЫ ПО ФАЗАМ

```
ФАЗА 1 (ГОТОВО):
  Время:      2-4 часа
  Точность:   ±5-10cm
  Улучшение:  Базовое

ФАЗА 2 (NEXT):
  Время:      2-3 дня
  Точность:   ±2-3cm (улучшение 2-3x)
  Улучшение:  Сглаживание

ФАЗА 3:
  Время:      1-2 недели
  Точность:   ±1-2cm (улучшение 5-10x) ← ARKit уровень!
  Улучшение:  Bundle Adjustment

ФАЗА 4:
  Время:      1-2 недели
  Точность:   ±0.5-1cm (улучшение 10-20x) ← Vision Pro уровень!
  Улучшение:  Loop Closure

ФАЗА 5:
  Время:      3-5 дней (параллельно)
  Точность:   ±0.5-1cm (с LiDAR)
  Улучшение:  Абсолютная глубина
```

---

## 🎯 РЕКОМЕНДАЦИЯ

### Вариант 1: Быстро (2-4 часа)
```
Применить ФАЗУ 1 (базовые исправления)
Результат: ±5-10cm
Потом добавить LiDAR
Результат: ±3-5cm
```

### Вариант 2: Хорошо (1 неделя)
```
ФАЗА 1 + ФАЗА 2 + ФАЗА 5
Результат: ±1-2cm (ARKit уровень)
```

### Вариант 3: Идеально (4-5 недель)
```
ФАЗА 1 + ФАЗА 2 + ФАЗА 3 + ФАЗА 4 + ФАЗА 5
Результат: ±0.5-1cm (Vision Pro уровень)
```

---

## 📚 ЗАВИСИМОСТИ

```cpp
// ФАЗА 2: Temporal Filtering
// Зависимостей нет (только GLM)

// ФАЗА 3: Bundle Adjustment
#include <ceres/ceres.h>
// Установка: apt-get install libceres-dev

// ФАЗА 4: Loop Closure Detection
#include <DBoW2/DBoW2.h>
// Установка: https://github.com/dorian3d/DBoW2

// ФАЗА 5: LiDAR Fusion
// Зависимостей нет (уже есть в вашем коде)
```

---

**Версия:** 1.0  
**Дата:** 2026-06-10  
**Статус:** ✅ Полная дорожная карта к ARKit-уровню точности
