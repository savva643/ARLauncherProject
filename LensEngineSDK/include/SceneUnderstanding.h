#ifndef SCENE_UNDERSTANDING_H
#define SCENE_UNDERSTANDING_H

#include <opencv2/opencv.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

// Типы семантических классов
enum class SemanticClass {
    BACKGROUND = 0,
    PERSON = 1,
    VEHICLE = 2,
    BUILDING = 3,
    ROAD = 4,
    VEGETATION = 5,
    SKY = 6,
    OBJECT = 7,
    UNKNOWN = 255
};

// Результат семантической сегментации
struct SemanticSegment {
    cv::Rect bbox;
    SemanticClass classType;
    float confidence;
    cv::Mat mask;
    std::string label;
    glm::vec3 center;
};

class SceneUnderstanding {
public:
    SceneUnderstanding();
    ~SceneUnderstanding();
    
    // Инициализация
    bool initialize(const std::string& modelPath = "");
    
    // Семантическая сегментация кадра
    std::vector<SemanticSegment> segmentScene(const cv::Mat& frame);
    
    // Получение семантической карты (изображение с цветами классов)
    cv::Mat getSemanticMap(const cv::Mat& frame);
    
    // Анализ сцены (определение типа окружения)
    std::string analyzeSceneType(const cv::Mat& frame);
    
    // Обнаружение плоскостей с семантической информацией
    std::vector<SemanticSegment> detectSemanticPlanes(const cv::Mat& frame);
    
    // Настройки
    void setConfidenceThreshold(float threshold) { m_confidenceThreshold = threshold; }
    void setMinSegmentSize(int size) { m_minSegmentSize = size; }
    
private:
    // Цветовая сегментация (K-means)
    std::vector<SemanticSegment> colorBasedSegmentation(const cv::Mat& frame);
    
    // Сегментация на основе суперпикселей (SLIC)
    std::vector<SemanticSegment> superpixelSegmentation(const cv::Mat& frame);
    
    // Классификация сегмента по цвету и текстуре
    SemanticClass classifySegment(const cv::Mat& segment, const cv::Rect& bbox);
    
    // Получение цвета для класса
    cv::Scalar getClassColor(SemanticClass classType);
    
    // Получение имени класса
    std::string getClassName(SemanticClass classType);
    
    // Параметры
    float m_confidenceThreshold = 0.5f;
    int m_minSegmentSize = 100;
    int m_numSuperpixels = 50;
    
    // Кэширование
    cv::Mat m_lastFrame;
    std::vector<SemanticSegment> m_lastSegments;
    
    // Статистика цветов для классификации
    std::vector<cv::Scalar> m_classColors;
};

#endif // SCENE_UNDERSTANDING_H
