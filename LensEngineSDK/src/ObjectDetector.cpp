#include "ObjectDetector.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef LENSENGINE_USE_OPENCV

namespace LensEngine {

ObjectDetector::ObjectDetector()
    : m_inputSize(640)
    , m_confidenceThreshold(0.5f)
    , m_nmsThreshold(0.4f)
    , m_initialized(false)
{
}

ObjectDetector::~ObjectDetector() {
}

bool ObjectDetector::initialize(const std::string& modelPath, const std::string& configPath, const std::string& classesPath) {
    try {
        // Попытка загрузки YOLO модели
        if (!configPath.empty()) {
            m_net = cv::dnn::readNetFromDarknet(configPath, modelPath);
        } else {
            m_net = cv::dnn::readNetFromONNX(modelPath);
        }
        
        if (m_net.empty()) {
            std::cerr << "[ObjectDetector] Failed to load model from: " << modelPath << std::endl;
            return false;
        }
        
        // Установка бэкенда (CPU)
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        
        // Загрузка названий классов
        if (!classesPath.empty()) {
            loadClassNames(classesPath);
        }
        
        m_initialized = true;
        std::cout << "[ObjectDetector] Initialized with model: " << modelPath << std::endl;
        std::cout << "[ObjectDetector] Classes loaded: " << m_classNames.size() << std::endl;
        
        return true;
    } catch (const cv::Exception& e) {
        std::cerr << "[ObjectDetector] OpenCV exception: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat ObjectDetector::preprocessFrame(const cv::Mat& frame, int targetSize) {
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(targetSize, targetSize));
    
    // Нормализация (0-1)
    cv::Mat blob = cv::dnn::blobFromImage(resized, 1.0/255.0, cv::Size(targetSize, targetSize), cv::Scalar(0, 0, 0), true, false);
    
    return blob;
}

std::vector<DetectedObject> ObjectDetector::detectObjects(const cv::Mat& frame) {
    std::vector<DetectedObject> objects;
    
    if (!m_initialized || frame.empty()) {
        return m_lastObjects;
    }
    
    try {
        // Предобработка
        cv::Mat blob = preprocessFrame(frame, m_inputSize);
        
        // Запуск детекции
        m_net.setInput(blob);
        std::vector<cv::Mat> outputs;
        m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());
        
        // Парсинг результатов
        objects = parseYOLOOutput(outputs[0], frame.size());
        
        m_lastObjects = objects;
        std::cout << "[ObjectDetector] Detected " << objects.size() << " objects" << std::endl;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[ObjectDetector] Exception in detection: " << e.what() << std::endl;
    }
    
    return objects;
}

std::vector<DetectedObject> ObjectDetector::parseYOLOOutput(const cv::Mat& output, const cv::Size& frameSize) {
    std::vector<DetectedObject> objects;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;
    std::vector<int> classIds;
    
    // YOLOv8 output format: [1, 84, 8400] для yolov8n
    // 1 = batch, 84 = 4 (bbox) + 80 (classes), 8400 = num_detections
    // Переставляем оси: [8400, 84]
    cv::Mat reshaped = output.reshape(1, output.size[1] * output.size[2]);
    reshaped = reshaped.reshape(1, output.size[2]); // [8400, 84]
    
    int numDetections = reshaped.size[0];
    int numClasses = reshaped.size[1] - 4;
    
    for (int i = 0; i < numDetections; ++i) {
        // Получаем класс scores (индексы 4..end)
        cv::Mat scores = reshaped.row(i).colRange(4, reshaped.size[1]);
        
        // Находим класс с максимальной уверенностью
        cv::Point classIdPoint;
        double maxScore;
        cv::minMaxLoc(scores, 0, &maxScore, 0, &classIdPoint);
        
        if (maxScore > m_confidenceThreshold) {
            // Получаем bbox (индексы 0..3): x, y, w, h (center format)
            float cx = reshaped.at<float>(i, 0) * frameSize.width;
            float cy = reshaped.at<float>(i, 1) * frameSize.height;
            float w = reshaped.at<float>(i, 2) * frameSize.width;
            float h = reshaped.at<float>(i, 3) * frameSize.height;
            
            // Конвертируем из center format в corner format
            int x1 = static_cast<int>(cx - w / 2);
            int y1 = static_cast<int>(cy - h / 2);
            int x2 = static_cast<int>(cx + w / 2);
            int y2 = static_cast<int>(cy + h / 2);
            
            // Проверка границ
            x1 = std::max(0, x1);
            y1 = std::max(0, y1);
            x2 = std::min(frameSize.width, x2);
            y2 = std::min(frameSize.height, y2);
            
            if (x2 > x1 && y2 > y1) {
                boxes.push_back(cv::Rect(x1, y1, x2 - x1, y2 - y1));
                confidences.push_back(static_cast<float>(maxScore));
                classIds.push_back(classIdPoint.x);
            }
        }
    }
    
    // Non-Maximum Suppression
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, m_confidenceThreshold, m_nmsThreshold, indices);
    
    // Создание объектов
    for (size_t i = 0; i < indices.size(); ++i) {
        int idx = indices[i];
        DetectedObject obj;
        
        obj.bbox = boxes[idx];
        obj.confidence = confidences[idx];
        obj.classId = classIds[idx];
        
        if (classIds[idx] < static_cast<int>(m_classNames.size())) {
            obj.label = m_classNames[classIds[idx]];
        } else {
            obj.label = "class_" + std::to_string(classIds[idx]);
        }
        
        obj.screenCenter = glm::vec2(
            obj.bbox.x + obj.bbox.width / 2.0f,
            obj.bbox.y + obj.bbox.height / 2.0f
        );
        
        obj.center = glm::vec3(obj.screenCenter.x, obj.screenCenter.y, 0.0f);
        
        objects.push_back(obj);
    }
    
    return objects;
}

bool ObjectDetector::loadClassNames(const std::string& classesPath) {
    std::ifstream file(classesPath);
    if (!file.is_open()) {
        std::cerr << "[ObjectDetector] Failed to open classes file: " << classesPath << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            m_classNames.push_back(line);
        }
    }
    
    file.close();
    return true;
}

std::vector<DetectedObject> ObjectDetector::getLastObjects() const {
    return m_lastObjects;
}

void ObjectDetector::setConfidenceThreshold(float threshold) {
    m_confidenceThreshold = threshold;
}

void ObjectDetector::setNMSThreshold(float threshold) {
    m_nmsThreshold = threshold;
}

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
