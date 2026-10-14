#pragma once

#ifdef LENSENGINE_USE_OPENCV

#include "HandTracker.h"
#include <string>
#include <vector>

namespace LensEngine {

// Типы жестов
enum class GestureType {
    NONE,
    PINCH,          // Пинч (большой и указательный палец вместе)
    OPEN_PALM,      // Открытая ладонь
    FIST,           // Кулак
    POINTING,       // Указание
    THUMBS_UP,      // Большой палец вверх
    THUMBS_DOWN,    // Большой палец вниз
    VICTORY,        // Жест "V"
    OK,             // Жест "OK"
    UNKNOWN
};

// Результат распознавания жеста
struct GestureResult {
    GestureType type;
    float confidence;      // Уверенность (0-1)
    std::string name;      // Название жеста
    glm::vec2 position;    // Позиция жеста на экране
};

class HandGestures {
public:
    HandGestures();
    ~HandGestures();

    // Распознать жест на основе landmarks
    GestureResult recognizeGesture(const Hand& hand);

    // Проверка конкретных жестов
    bool isPinch(const Hand& hand, float threshold = 0.05f);
    bool isOpenPalm(const Hand& hand);
    bool isFist(const Hand& hand);
    bool isPointing(const Hand& hand);
    bool isThumbsUp(const Hand& hand);
    bool isThumbsDown(const Hand& hand);
    bool isVictory(const Hand& hand);
    bool isOK(const Hand& hand);

private:
    // Вспомогательные функции
    float distance(const glm::vec3& p1, const glm::vec3& p2);
    float angle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
    bool isFingerExtended(const Hand& hand, int fingerIndex);
    
    // Индексы landmarks MediaPipe
    // 0: wrist
    // 1: thumb_cmc
    // 2: thumb_mcp
    // 3: thumb_ip
    // 4: thumb_tip
    // 5: index_mcp
    // 6: index_pip
    // 7: index_dip
    // 8: index_tip
    // 9: middle_mcp
    // 10: middle_pip
    // 11: middle_dip
    // 12: middle_tip
    // 13: ring_mcp
    // 14: ring_pip
    // 15: ring_dip
    // 16: ring_tip
    // 17: pinky_mcp
    // 18: pinky_pip
    // 19: pinky_dip
    // 20: pinky_tip
};

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
