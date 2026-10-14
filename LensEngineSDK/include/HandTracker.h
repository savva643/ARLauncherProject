#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

#ifdef LENSENGINE_USE_OPENCV

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
    glm::vec2 screenCenter;
    float confidence;
    bool isLeftHand;
    cv::Rect boundingBox;
};

class HandTracker {
public:
    HandTracker();
    ~HandTracker();
    
    // Инициализация с путями к ONNX моделям
    bool initialize(const std::string& palmModelPath, const std::string& handposeModelPath);
    
    // Детекция рук на кадре (OpenCV DNN с ONNX моделями)
    std::vector<Hand> detectHands(const cv::Mat& frame);
    
    // Получение последнего результата
    std::vector<Hand> getLastHands() const;
    
private:
    // Детекция ладоней
    std::vector<cv::Rect> detectPalms(const cv::Mat& frame);
    
    // Оценка landmarks для каждой ладони
    std::vector<HandLandmark> estimateHandLandmarks(const cv::Mat& frame, const cv::Rect& palmBbox);
    
    // Предобработка кадра для нейросети
    cv::Mat preprocessFrame(const cv::Mat& frame, int targetSize);
    
    // Конвертация landmarks из нейросети в наш формат
    Hand convertToHand(const std::vector<cv::Point2f>& landmarks, const cv::Rect& bbox, float confidence);
    
    // Fallback методы для детекции кожи
    cv::Mat detectSkin(const cv::Mat& frame);
    std::vector<Hand> detectHandsSkinDetection(const cv::Mat& frame);
    std::vector<HandLandmark> estimateLandmarksSimple(const cv::Mat& handRegion, const cv::Rect& bbox);
    
    // OpenCV DNN сети
    cv::dnn::Net m_palmDetector;
    cv::dnn::Net m_handposeEstimator;
    
    // Последний результат
    std::vector<Hand> m_lastHands;
    
    // Размеры входа для нейросетей
    int m_palmInputSize;
    int m_handposeInputSize;
    
    // Порог уверенности
    float m_confidenceThreshold;
    
    bool m_initialized;
    bool m_useSkinDetection; // Fallback на детекцию кожи
    
    // Параметры для детекции кожи (fallback)
    cv::Scalar m_skinLower;
    cv::Scalar m_skinUpper;
    int m_minHandSize;
    int m_maxHandSize;
};

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
