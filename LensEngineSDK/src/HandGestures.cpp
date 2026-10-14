#include "HandGestures.h"
#include <cmath>
#include <algorithm>

#ifdef LENSENGINE_USE_OPENCV

namespace LensEngine {

HandGestures::HandGestures() {
}

HandGestures::~HandGestures() {
}

float HandGestures::distance(const glm::vec3& p1, const glm::vec3& p2) {
    return glm::length(p1 - p2);
}

float HandGestures::angle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    glm::vec3 v1 = p1 - p2;
    glm::vec3 v2 = p3 - p2;
    float dot = glm::dot(v1, v2);
    float len1 = glm::length(v1);
    float len2 = glm::length(v2);
    if (len1 < 0.001f || len2 < 0.001f) return 0.0f;
    return std::acos(dot / (len1 * len2));
}

bool HandGestures::isFingerExtended(const Hand& hand, int fingerIndex) {
    // fingerIndex: 0=thumb, 1=index, 2=middle, 3=ring, 4=pinky
    // Landmarks: thumb=4,8,12,16,20 (tips)
    
    if (hand.landmarks.size() < 21) return false;
    
    int tipIndex, pipIndex, mcpIndex;
    
    switch (fingerIndex) {
        case 0: // thumb
            tipIndex = 4; pipIndex = 3; mcpIndex = 2;
            break;
        case 1: // index
            tipIndex = 8; pipIndex = 6; mcpIndex = 5;
            break;
        case 2: // middle
            tipIndex = 12; pipIndex = 10; mcpIndex = 9;
            break;
        case 3: // ring
            tipIndex = 16; pipIndex = 14; mcpIndex = 13;
            break;
        case 4: // pinky
            tipIndex = 20; pipIndex = 18; mcpIndex = 17;
            break;
        default:
            return false;
    }
    
    glm::vec3 tip = hand.landmarks[tipIndex].position;
    glm::vec3 pip = hand.landmarks[pipIndex].position;
    glm::vec3 mcp = hand.landmarks[mcpIndex].position;
    
    // Палец вытянут если tip дальше от wrist чем pip
    float wristToTip = distance(hand.landmarks[0].position, tip);
    float wristToPip = distance(hand.landmarks[0].position, pip);
    
    return wristToTip > wristToPip * 1.1f;
}

bool HandGestures::isPinch(const Hand& hand, float threshold) {
    if (hand.landmarks.size() < 21) return false;
    
    // Пинч: большой (4) и указательный (8) пальцы близко
    float dist = distance(hand.landmarks[4].position, hand.landmarks[8].position);
    
    // Нормализуем относительно размера ладони
    float palmSize = distance(hand.landmarks[0].position, hand.landmarks[9].position);
    
    return (dist / palmSize) < threshold;
}

bool HandGestures::isOpenPalm(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Открытая ладонь: все пальцы вытянуты
    int extendedCount = 0;
    for (int i = 0; i < 5; ++i) {
        if (isFingerExtended(hand, i)) extendedCount++;
    }
    
    return extendedCount >= 4;
}

bool HandGestures::isFist(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Кулак: все пальцы согнуты (кроме большого)
    int extendedCount = 0;
    for (int i = 1; i < 5; ++i) { // кроме большого
        if (isFingerExtended(hand, i)) extendedCount++;
    }
    
    return extendedCount <= 1;
}

bool HandGestures::isPointing(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Указание: только указательный палец вытянут
    bool indexExtended = isFingerExtended(hand, 1);
    bool middleExtended = isFingerExtended(hand, 2);
    bool ringExtended = isFingerExtended(hand, 3);
    bool pinkyExtended = isFingerExtended(hand, 4);
    
    return indexExtended && !middleExtended && !ringExtended && !pinkyExtended;
}

bool HandGestures::isThumbsUp(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Большой палец вверх: большой вытянут, другие согнуты
    bool thumbExtended = isFingerExtended(hand, 0);
    bool othersExtended = isFingerExtended(hand, 1) || isFingerExtended(hand, 2) || 
                          isFingerExtended(hand, 3) || isFingerExtended(hand, 4);
    
    // Проверяем что большой палец выше остальных
    if (!thumbExtended || othersExtended) return false;
    
    float thumbY = hand.landmarks[4].position.y;
    float wristY = hand.landmarks[0].position.y;
    
    // Большой палец выше запястья
    return thumbY < wristY;
}

bool HandGestures::isThumbsDown(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Большой палец вниз: большой вытянут, другие согнуты
    bool thumbExtended = isFingerExtended(hand, 0);
    bool othersExtended = isFingerExtended(hand, 1) || isFingerExtended(hand, 2) || 
                          isFingerExtended(hand, 3) || isFingerExtended(hand, 4);
    
    if (!thumbExtended || othersExtended) return false;
    
    float thumbY = hand.landmarks[4].position.y;
    float wristY = hand.landmarks[0].position.y;
    
    // Большой палец ниже запястья
    return thumbY > wristY;
}

bool HandGestures::isVictory(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Жест "V": указательный и средний вытянуты, остальные согнуты
    bool indexExtended = isFingerExtended(hand, 1);
    bool middleExtended = isFingerExtended(hand, 2);
    bool ringExtended = isFingerExtended(hand, 3);
    bool pinkyExtended = isFingerExtended(hand, 4);
    
    return indexExtended && middleExtended && !ringExtended && !pinkyExtended;
}

bool HandGestures::isOK(const Hand& hand) {
    if (hand.landmarks.size() < 21) return false;
    
    // Жест "OK": большой и указательный образуют круг
    if (!isPinch(hand, 0.08f)) return false;
    
    // Остальные пальцы вытянуты
    bool middleExtended = isFingerExtended(hand, 2);
    bool ringExtended = isFingerExtended(hand, 3);
    bool pinkyExtended = isFingerExtended(hand, 4);
    
    return middleExtended && ringExtended && pinkyExtended;
}

GestureResult HandGestures::recognizeGesture(const Hand& hand) {
    GestureResult result;
    result.type = GestureType::NONE;
    result.confidence = 0.0f;
    result.name = "None";
    result.position = hand.screenCenter;
    
    if (hand.landmarks.size() < 21) {
        return result;
    }
    
    // Проверяем жесты в приоритетном порядке
    if (isPinch(hand)) {
        result.type = GestureType::PINCH;
        result.confidence = 0.9f;
        result.name = "Pinch";
    } else if (isOK(hand)) {
        result.type = GestureType::OK;
        result.confidence = 0.85f;
        result.name = "OK";
    } else if (isVictory(hand)) {
        result.type = GestureType::VICTORY;
        result.confidence = 0.9f;
        result.name = "Victory";
    } else if (isThumbsUp(hand)) {
        result.type = GestureType::THUMBS_UP;
        result.confidence = 0.85f;
        result.name = "Thumbs Up";
    } else if (isThumbsDown(hand)) {
        result.type = GestureType::THUMBS_DOWN;
        result.confidence = 0.85f;
        result.name = "Thumbs Down";
    } else if (isPointing(hand)) {
        result.type = GestureType::POINTING;
        result.confidence = 0.8f;
        result.name = "Pointing";
    } else if (isFist(hand)) {
        result.type = GestureType::FIST;
        result.confidence = 0.85f;
        result.name = "Fist";
    } else if (isOpenPalm(hand)) {
        result.type = GestureType::OPEN_PALM;
        result.confidence = 0.9f;
        result.name = "Open Palm";
    } else {
        result.type = GestureType::UNKNOWN;
        result.confidence = 0.5f;
        result.name = "Unknown";
    }
    
    return result;
}

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
