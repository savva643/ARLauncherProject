#include "HandTracker.h"
#include <iostream>
#include <algorithm>

#ifdef LENSENGINE_USE_OPENCV

namespace LensEngine {

HandTracker::HandTracker()
    : m_palmInputSize(192)
    , m_handposeInputSize(224)
    , m_confidenceThreshold(0.5f)
    , m_initialized(false)
    , m_useSkinDetection(false)
    , m_skinLower(cv::Scalar(0, 48, 80))
    , m_skinUpper(cv::Scalar(20, 255, 255))
    , m_minHandSize(100)
    , m_maxHandSize(5000)
{
}

HandTracker::~HandTracker() {
}

bool HandTracker::initialize(const std::string& palmModelPath, const std::string& handposeModelPath) {
    try {
        // Попытка загрузки ONNX моделей
        m_palmDetector = cv::dnn::readNetFromONNX(palmModelPath);
        m_handposeEstimator = cv::dnn::readNetFromONNX(handposeModelPath);
        
        if (!m_palmDetector.empty() && !m_handposeEstimator.empty()) {
            // Установка предпочтительного бэкенда (CPU)
            m_palmDetector.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_palmDetector.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            m_handposeEstimator.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_handposeEstimator.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            
            m_initialized = true;
            std::cout << "[HandTracker] Initialized with OpenCV DNN ONNX models" << std::endl;
            return true;
        }
    } catch (const cv::Exception& e) {
        std::cerr << "[HandTracker] OpenCV DNN failed: " << e.what() << std::endl;
        std::cerr << "[HandTracker] Falling back to skin detection" << std::endl;
    }
    
    // Fallback на простую детекцию кожи
    m_initialized = true;
    m_useSkinDetection = true;
    std::cout << "[HandTracker] Initialized with skin detection fallback" << std::endl;
    return true;
}

cv::Mat HandTracker::preprocessFrame(const cv::Mat& frame, int targetSize) {
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(targetSize, targetSize));
    
    // Нормализация (0-1)
    cv::Mat floatMat;
    resized.convertTo(floatMat, CV_32F, 1.0 / 255.0);
    
    // Конвертация BGR в RGB
    cv::Mat rgb;
    cv::cvtColor(floatMat, rgb, cv::COLOR_BGR2RGB);
    
    return rgb;
}

std::vector<cv::Rect> HandTracker::detectPalms(const cv::Mat& frame) {
    std::vector<cv::Rect> palms;
    
    if (!m_initialized || frame.empty()) {
        return palms;
    }
    
    try {
        // Предобработка
        cv::Mat inputBlob = preprocessFrame(frame, m_palmInputSize);
        
        // Создание blob
        cv::Mat blob = cv::dnn::blobFromImage(inputBlob, 1.0, cv::Size(m_palmInputSize, m_palmInputSize), cv::Scalar(0, 0, 0), true, false);
        
        // Запуск детекции
        m_palmDetector.setInput(blob);
        cv::Mat output = m_palmDetector.forward();
        
        // Парсинг результатов (формат зависит от модели)
        // Для MediaPipe palm detection: [batch, num_detections, 18]
        // 18 = [x, y, w, h, score, 7 keypoints * 2]
        
        int numDetections = output.size[1];
        
        for (int i = 0; i < numDetections; ++i) {
            float score = output.at<float>(0, i, 4);
            
            if (score > m_confidenceThreshold) {
                float x = output.at<float>(0, i, 0) * frame.cols;
                float y = output.at<float>(0, i, 1) * frame.rows;
                float w = output.at<float>(0, i, 2) * frame.cols;
                float h = output.at<float>(0, i, 3) * frame.rows;
                
                cv::Rect palm(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
                
                // Проверка границ
                palm &= cv::Rect(0, 0, frame.cols, frame.rows);
                
                if (palm.area() > 100) {
                    palms.push_back(palm);
                }
            }
        }
        
        std::cout << "[HandTracker] Detected " << palms.size() << " palms" << std::endl;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[HandTracker] Exception in palm detection: " << e.what() << std::endl;
    }
    
    return palms;
}

std::vector<HandLandmark> HandTracker::estimateHandLandmarks(const cv::Mat& frame, const cv::Rect& palmBbox) {
    std::vector<HandLandmark> landmarks;
    
    if (!m_initialized || frame.empty()) {
        return landmarks;
    }
    
    try {
        // Вырезаем область ладони
        cv::Mat handRegion = frame(palmBbox);
        
        // Предобработка
        cv::Mat inputBlob = preprocessFrame(handRegion, m_handposeInputSize);
        
        // Создание blob
        cv::Mat blob = cv::dnn::blobFromImage(inputBlob, 1.0, cv::Size(m_handposeInputSize, m_handposeInputSize), cv::Scalar(0, 0, 0), true, false);
        
        // Запуск оценки landmarks
        m_handposeEstimator.setInput(blob);
        cv::Mat output = m_handposeEstimator.forward();
        
        // Парсинг результатов (21 точка руки: [x, y, z] для каждой)
        // Формат: [batch, 63] где 63 = 21 * 3
        
        for (int i = 0; i < 21; ++i) {
            HandLandmark lm;
            
            float x = output.at<float>(0, i * 3) * palmBbox.width + palmBbox.x;
            float y = output.at<float>(0, i * 3 + 1) * palmBbox.height + palmBbox.y;
            float z = output.at<float>(0, i * 3 + 2);
            
            lm.position = glm::vec3(x, y, z);
            lm.screenPosition = glm::vec2(x, y);
            lm.confidence = 0.8f; // Уверенность не возвращается моделью
            lm.id = i;
            
            landmarks.push_back(lm);
        }
        
    } catch (const cv::Exception& e) {
        std::cerr << "[HandTracker] Exception in landmark estimation: " << e.what() << std::endl;
    }
    
    return landmarks;
}

Hand HandTracker::convertToHand(const std::vector<cv::Point2f>& landmarks, const cv::Rect& bbox, float confidence) {
    Hand hand;
    hand.boundingBox = bbox;
    hand.confidence = confidence;
    hand.isLeftHand = (bbox.x + bbox.width / 2) < bbox.width; // Простая эвристика
    
    // Вычисление центра
    float centerX = 0, centerY = 0;
    for (const auto& lm : landmarks) {
        centerX += lm.x;
        centerY += lm.y;
    }
    centerX /= landmarks.size();
    centerY /= landmarks.size();
    
    hand.center = glm::vec3(centerX, centerY, 0.0f);
    hand.screenCenter = glm::vec2(centerX, centerY);
    
    // Конвертация landmarks
    for (size_t i = 0; i < landmarks.size() && i < 21; ++i) {
        HandLandmark lm;
        lm.position = glm::vec3(landmarks[i].x, landmarks[i].y, 0.0f);
        lm.screenPosition = glm::vec2(landmarks[i].x, landmarks[i].y);
        lm.confidence = confidence;
        lm.id = static_cast<int>(i);
        hand.landmarks.push_back(lm);
    }
    
    return hand;
}

std::vector<Hand> HandTracker::detectHands(const cv::Mat& frame) {
    std::vector<Hand> hands;
    
    if (!m_initialized || frame.empty()) {
        return m_lastHands;
    }
    
    // Fallback на детекцию кожи если ONNX не загрузился
    if (m_useSkinDetection) {
        return detectHandsSkinDetection(frame);
    }
    
    try {
        // Детекция ладоней через DNN
        std::vector<cv::Rect> palms = detectPalms(frame);
        
        // Оценка landmarks для каждой ладони
        for (const auto& palm : palms) {
            std::vector<HandLandmark> landmarks = estimateHandLandmarks(frame, palm);
            
            if (!landmarks.empty()) {
                Hand hand;
                hand.boundingBox = palm;
                hand.confidence = 0.8f;
                hand.isLeftHand = (palm.x + palm.width / 2) < frame.cols / 2;
                hand.landmarks = landmarks;
                
                // Вычисление центра
                float centerX = 0, centerY = 0;
                for (const auto& lm : landmarks) {
                    centerX += lm.position.x;
                    centerY += lm.position.y;
                }
                centerX /= landmarks.size();
                centerY /= landmarks.size();
                
                hand.center = glm::vec3(centerX, centerY, 0.0f);
                hand.screenCenter = glm::vec2(centerX, centerY);
                
                hands.push_back(hand);
            }
        }
        
        m_lastHands = hands;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[HandTracker] Exception in hand detection: " << e.what() << std::endl;
    }
    
    return hands;
}

// Fallback: детекция кожи
cv::Mat HandTracker::detectSkin(const cv::Mat& frame) {
    cv::Mat hsv, skinMask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, m_skinLower, m_skinUpper, skinMask);
    
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::erode(skinMask, skinMask, kernel, cv::Point(-1, -1), 2);
    cv::dilate(skinMask, skinMask, kernel, cv::Point(-1, -1), 2);
    cv::GaussianBlur(skinMask, skinMask, cv::Size(3, 3), 0);
    
    return skinMask;
}

std::vector<Hand> HandTracker::detectHandsSkinDetection(const cv::Mat& frame) {
    std::vector<Hand> hands;
    
    cv::Mat skinMask = detectSkin(frame);
    
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(skinMask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        
        if (area >= m_minHandSize && area <= m_maxHandSize) {
            cv::Rect bbox = cv::boundingRect(contour);
            float aspectRatio = static_cast<float>(bbox.width) / bbox.height;
            
            if (aspectRatio > 0.3f && aspectRatio < 3.0f) {
                Hand hand;
                hand.boundingBox = bbox;
                hand.confidence = std::min(1.0f, static_cast<float>(area / 2000.0f));
                hand.center = glm::vec3(bbox.x + bbox.width / 2.0f, bbox.y + bbox.height / 2.0f, 0.0f);
                hand.screenCenter = glm::vec2(hand.center.x, hand.center.y);
                hand.isLeftHand = (bbox.x + bbox.width / 2) < frame.cols / 2;
                hand.landmarks = estimateLandmarksSimple(frame(bbox), bbox);
                hands.push_back(hand);
            }
        }
    }
    
    m_lastHands = hands;
    return hands;
}

std::vector<HandLandmark> HandTracker::estimateLandmarksSimple(const cv::Mat& handRegion, const cv::Rect& bbox) {
    std::vector<HandLandmark> landmarks;
    
    // Простая оценка 21 точки руки на основе bounding box
    HandLandmark palm;
    palm.position = glm::vec3(handRegion.cols / 2.0f, handRegion.rows / 2.0f, 0.0f);
    palm.screenPosition = glm::vec2(bbox.x + palm.position.x, bbox.y + palm.position.y);
    palm.confidence = 0.7f;
    palm.id = 0;
    landmarks.push_back(palm);
    
    HandLandmark wrist;
    wrist.position = glm::vec3(handRegion.cols / 2.0f, handRegion.rows * 0.9f, 0.0f);
    wrist.screenPosition = glm::vec2(bbox.x + wrist.position.x, bbox.y + wrist.position.y);
    wrist.confidence = 0.6f;
    wrist.id = 1;
    landmarks.push_back(wrist);
    
    for (int finger = 0; finger < 4; ++finger) {
        float fingerX = handRegion.cols * (0.2f + finger * 0.2f);
        for (int joint = 0; joint < 5; ++joint) {
            HandLandmark lm;
            lm.position = glm::vec3(fingerX, handRegion.rows * (0.8f - joint * 0.15f), 0.0f);
            lm.screenPosition = glm::vec2(bbox.x + lm.position.x, bbox.y + lm.position.y);
            lm.confidence = 0.5f - joint * 0.05f;
            lm.id = 2 + finger * 5 + joint;
            landmarks.push_back(lm);
        }
    }
    
    return landmarks;
}

std::vector<Hand> HandTracker::getLastHands() const {
    return m_lastHands;
}

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
