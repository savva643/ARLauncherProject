#pragma once

#ifdef LENSENGINE_USE_OPENCV

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace LensEngine {

// Структура для обнаруженного объекта
struct DetectedObject {
    std::string label;           // Класс объекта (например, "person", "chair")
    float confidence;            // Уверенность детекции (0-1)
    cv::Rect bbox;              // Bounding box на кадре
    glm::vec3 center;            // Центр объекта в 3D (если доступно)
    glm::vec2 screenCenter;     // Центр объекта на экране
    int classId;                // ID класса
};

class ObjectDetector {
public:
    ObjectDetector();
    ~ObjectDetector();

    // Инициализация с путём к YOLO модели
    // modelPath: путь к .onnx модели YOLO
    // configPath: путь к config файлу (если нужен)
    // classesPath: путь к файлу с названиями классов
    bool initialize(const std::string& modelPath, const std::string& configPath = "", const std::string& classesPath = "");

    // Детекция объектов на кадре
    std::vector<DetectedObject> detectObjects(const cv::Mat& frame);

    // Получить последний результат
    std::vector<DetectedObject> getLastObjects() const;

    // Установить порог уверенности
    void setConfidenceThreshold(float threshold);

    // Установить NMS порог (Non-Maximum Suppression)
    void setNMSThreshold(float threshold);

private:
    // Предобработка кадра для YOLO
    cv::Mat preprocessFrame(const cv::Mat& frame, int targetSize);

    // Парсинг результатов YOLO
    std::vector<DetectedObject> parseYOLOOutput(const cv::Mat& output, const cv::Size& frameSize);

    // Загрузка названий классов
    bool loadClassNames(const std::string& classesPath);

    // YOLO сеть
    cv::dnn::Net m_net;

    // Названия классов
    std::vector<std::string> m_classNames;

    // Размер входа для YOLO
    int m_inputSize;

    // Пороги
    float m_confidenceThreshold;
    float m_nmsThreshold;

    // Флаг инициализации
    bool m_initialized;

    // Последний результат
    std::vector<DetectedObject> m_lastObjects;
};

} // namespace LensEngine

#endif // LENSENGINE_USE_OPENCV
