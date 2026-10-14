#include "SceneUnderstanding.h"
#include <iostream>
#include <algorithm>
#include <cmath>

SceneUnderstanding::SceneUnderstanding() {
    // Инициализация цветов классов
    m_classColors = {
        cv::Scalar(0, 0, 0),        // BACKGROUND - черный
        cv::Scalar(0, 255, 0),      // PERSON - зеленый
        cv::Scalar(255, 0, 0),      // VEHICLE - красный
        cv::Scalar(128, 128, 128),  // BUILDING - серый
        cv::Scalar(64, 64, 64),     // ROAD - темно-серый
        cv::Scalar(0, 128, 0),      // VEGETATION - темно-зеленый
        cv::Scalar(135, 206, 235),  // SKY - голубой
        cv::Scalar(255, 255, 0)     // OBJECT - желтый
    };
}

SceneUnderstanding::~SceneUnderstanding() {
}

bool SceneUnderstanding::initialize(const std::string& modelPath) {
    // Для базовой реализации не требуются модели
    // В будущем можно добавить загрузку моделей для DL-based сегментации
    std::cout << "[SceneUnderstanding] Initialized with color-based segmentation" << std::endl;
    return true;
}

std::vector<SemanticSegment> SceneUnderstanding::segmentScene(const cv::Mat& frame) {
    if (frame.empty()) {
        return {};
    }
    
    // Используем цветовую сегментацию (K-means)
    return colorBasedSegmentation(frame);
}

std::vector<SemanticSegment> SceneUnderstanding::colorBasedSegmentation(const cv::Mat& frame) {
    std::vector<SemanticSegment> segments;
    
    // Преобразуем в LAB для лучшей сегментации
    cv::Mat labFrame;
    cv::cvtColor(frame, labFrame, cv::COLOR_BGR2Lab);
    
    // Подготовка данных для K-means
    cv::Mat data = labFrame.reshape(1, labFrame.rows * labFrame.cols);
    data.convertTo(data, CV_32F);
    
    // K-means кластеризация
    cv::Mat labels, centers;
    int numClusters = 8;
    cv::kmeans(data, numClusters, labels, 
               cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
               3, cv::KMEANS_PP_CENTERS, centers);
    
    // Создаем маски для каждого кластера
    for (int k = 0; k < numClusters; ++k) {
        cv::Mat mask = (labels == k);
        mask = mask.reshape(1, frame.rows);
        
        // Находим bounding box
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        if (!contours.empty()) {
            for (const auto& contour : contours) {
                cv::Rect bbox = cv::boundingRect(contour);
                
                if (bbox.area() >= m_minSegmentSize) {
                    // Создаем маску для этого сегмента
                    cv::Mat segmentMask = cv::Mat::zeros(frame.size(), CV_8UC1);
                    cv::drawContours(segmentMask, std::vector<std::vector<cv::Point>>{contour}, 0, 255, -1);
                    
                    // Получаем средний цвет сегмента
                    cv::Scalar meanColor = cv::mean(frame, segmentMask);
                    
                    // Классифицируем сегмент
                    SemanticSegment segment;
                    segment.bbox = bbox;
                    segment.classType = classifySegment(frame(bbox), bbox);
                    segment.confidence = 0.7f; // Базовая уверенность
                    segment.mask = segmentMask;
                    segment.label = getClassName(segment.classType);
                    segment.center = glm::vec3(
                        bbox.x + bbox.width / 2.0f,
                        bbox.y + bbox.height / 2.0f,
                        0.0f
                    );
                    
                    segments.push_back(segment);
                }
            }
        }
    }
    
    return segments;
}

std::vector<SemanticSegment> SceneUnderstanding::superpixelSegmentation(const cv::Mat& frame) {
    std::vector<SemanticSegment> segments;
    
    // SLIC суперпиксели
    cv::Mat labFrame;
    cv::cvtColor(frame, labFrame, cv::COLOR_BGR2Lab);
    
    // Используем cv::ximgproc::createSuperpixelSLIC если доступно
    // В противном случае используем упрощенный подход
    
    // Упрощенная реализация: сетка суперпикселей
    int superpixelSize = 50;
    int cols = frame.cols / superpixelSize;
    int rows = frame.rows / superpixelSize;
    
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            cv::Rect bbox(j * superpixelSize, i * superpixelSize, superpixelSize, superpixelSize);
            
            // Проверка границ
            if (bbox.x + bbox.width > frame.cols) bbox.width = frame.cols - bbox.x;
            if (bbox.y + bbox.height > frame.rows) bbox.height = frame.rows - bbox.y;
            
            if (bbox.area() >= m_minSegmentSize) {
                cv::Mat segment = frame(bbox);
                
                SemanticSegment seg;
                seg.bbox = bbox;
                seg.classType = classifySegment(segment, bbox);
                seg.confidence = 0.6f;
                seg.mask = cv::Mat::ones(bbox.size(), CV_8UC1) * 255;
                seg.label = getClassName(seg.classType);
                seg.center = glm::vec3(
                    bbox.x + bbox.width / 2.0f,
                    bbox.y + bbox.height / 2.0f,
                    0.0f
                );
                
                segments.push_back(seg);
            }
        }
    }
    
    return segments;
}

SemanticClass SceneUnderstanding::classifySegment(const cv::Mat& segment, const cv::Rect& bbox) {
    if (segment.empty()) {
        return SemanticClass::UNKNOWN;
    }
    
    // Вычисляем средний цвет в RGB
    cv::Scalar meanColor = cv::mean(segment);
    float r = meanColor[2];
    float g = meanColor[1];
    float b = meanColor[0];
    
    // Простая эвристическая классификация по цвету
    float total = r + g + b;
    if (total == 0) return SemanticClass::BACKGROUND;
    
    float rRatio = r / total;
    float gRatio = g / total;
    float bRatio = b / total;
    
    // Классификация по цвету
    if (gRatio > 0.4 && bRatio < 0.3) {
        return SemanticClass::VEGETATION; // Зеленый
    } else if (bRatio > 0.4 && gRatio > 0.3) {
        return SemanticClass::SKY; // Голубой
    } else if (rRatio > 0.4 && gRatio > 0.3) {
        return SemanticClass::OBJECT; // Желто-оранжевый
    } else if (rRatio < 0.3 && gRatio < 0.3 && bRatio < 0.3) {
        return SemanticClass::ROAD; // Темный
    } else if (rRatio > 0.3 && gRatio > 0.3 && bRatio > 0.3) {
        return SemanticClass::BUILDING; // Светло-серый
    } else if (rRatio > 0.35 && gRatio < 0.35 && bRatio < 0.35) {
        return SemanticClass::VEHICLE; // Красный
    } else if (gRatio > 0.35 && rRatio < 0.35 && bRatio < 0.35) {
        return SemanticClass::PERSON; // Зеленый (кожа)
    }
    
    return SemanticClass::UNKNOWN;
}

cv::Mat SceneUnderstanding::getSemanticMap(const cv::Mat& frame) {
    cv::Mat semanticMap = cv::Mat::zeros(frame.size(), CV_8UC3);
    
    auto segments = segmentScene(frame);
    
    for (const auto& segment : segments) {
        cv::Scalar color = getClassColor(segment.classType);
        cv::Mat roi = semanticMap(segment.bbox);
        roi.setTo(color);
        
        // Добавляем текст метки
        if (segment.bbox.width > 50 && segment.bbox.height > 20) {
            cv::putText(semanticMap, segment.label, 
                       cv::Point(segment.bbox.x, segment.bbox.y - 5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255), 1);
        }
    }
    
    return semanticMap;
}

std::string SceneUnderstanding::analyzeSceneType(const cv::Mat& frame) {
    auto segments = segmentScene(frame);
    
    if (segments.empty()) {
        return "Unknown";
    }
    
    // Подсчет классов
    int classCounts[8] = {0};
    for (const auto& seg : segments) {
        classCounts[static_cast<int>(seg.classType)]++;
    }
    
    // Определение типа сцены по доминирующим классам
    int maxCount = 0;
    int dominantClass = 0;
    
    for (int i = 0; i < 8; ++i) {
        if (classCounts[i] > maxCount) {
            maxCount = classCounts[i];
            dominantClass = i;
        }
    }
    
    switch (dominantClass) {
        case static_cast<int>(SemanticClass::VEGETATION):
            return "Outdoor/Nature";
        case static_cast<int>(SemanticClass::BUILDING):
            return "Indoor/Urban";
        case static_cast<int>(SemanticClass::ROAD):
            return "Street/Road";
        case static_cast<int>(SemanticClass::SKY):
            return "Outdoor/Sky";
        case static_cast<int>(SemanticClass::PERSON):
            return "Indoor/Meeting";
        case static_cast<int>(SemanticClass::VEHICLE):
            return "Traffic/Parking";
        default:
            return "Mixed Environment";
    }
}

std::vector<SemanticSegment> SceneUnderstanding::detectSemanticPlanes(const cv::Mat& frame) {
    auto segments = segmentScene(frame);
    
    // Фильтруем только плоские поверхности (ROAD, BUILDING)
    std::vector<SemanticSegment> planes;
    for (const auto& seg : segments) {
        if (seg.classType == SemanticClass::ROAD || 
            seg.classType == SemanticClass::BUILDING) {
            planes.push_back(seg);
        }
    }
    
    return planes;
}

cv::Scalar SceneUnderstanding::getClassColor(SemanticClass classType) {
    int idx = static_cast<int>(classType);
    if (idx >= 0 && idx < static_cast<int>(m_classColors.size())) {
        return m_classColors[idx];
    }
    return cv::Scalar(128, 128, 128); // Серый для неизвестных
}

std::string SceneUnderstanding::getClassName(SemanticClass classType) {
    switch (classType) {
        case SemanticClass::BACKGROUND: return "Background";
        case SemanticClass::PERSON: return "Person";
        case SemanticClass::VEHICLE: return "Vehicle";
        case SemanticClass::BUILDING: return "Building";
        case SemanticClass::ROAD: return "Road";
        case SemanticClass::VEGETATION: return "Vegetation";
        case SemanticClass::SKY: return "Sky";
        case SemanticClass::OBJECT: return "Object";
        default: return "Unknown";
    }
}
