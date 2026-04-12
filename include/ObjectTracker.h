#ifndef OBJECT_TRACKER_H
#define OBJECT_TRACKER_H

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

// For sf::RectangleShape in drawDetectionsWithText
#include <SFML/Graphics/RectangleShape.hpp>

namespace opencv {

/// Represents a detected object with bounding box and class info
struct DetectedObject {
    std::string className;    ///< Class name from COCO dataset
    float confidence;         ///< Confidence score (0-1)
    sf::FloatRect boundingBox;///< Bounding box (position, size)
    sf::IntRect intBoundingBox;///< Bounding box in pixel coordinates (position, size)
};

/// ObjectTracker - YOLO-based object detection using OpenCV DNN
/// Loads YOLO model and detects objects in images
class ObjectTracker {
public:
    /// Configuration for object tracker
    struct Config {
        std::string cfgPath = "assets/yolo/yolov4.cfg";      ///< YOLO config file path
        std::string weightsPath = "assets/yolo/yolov4.weights";  ///< YOLO weights file path
        std::string namesPath = "assets/yolo/coco.names";          ///< Class names file path
        std::string imagePath = "assets/test_image.png";           ///< Input image path
        float confidenceThreshold = 0.2f;                          ///< Detection confidence threshold
        float nmsThreshold = 0.2f;                                 ///< Non-maximum suppression threshold
    };

    ObjectTracker();
    ~ObjectTracker();

    /// Initialize the object tracker with given config
    /// @return true on success, false on failure
    bool initialize(const Config& config);

    /// Run object detection on the loaded image
    /// @return vector of detected objects
    std::vector<DetectedObject> detect();

    /// Draw detection results on the image
    /// @param inputImage Input image to draw on
    /// @param detections Vector of detected objects
    /// @return Image with detections drawn
    sf::Image drawDetections(const sf::Image& inputImage, const std::vector<DetectedObject>& detections);

    /// Draw detection results on the image using sf::Text for labels
    /// @param inputImage Input image to draw on
    /// @param detections Vector of detected objects
    /// @return Image with detections drawn
    sf::Image drawDetectionsWithText(const sf::Image& inputImage, const std::vector<DetectedObject>& detections);

    /// Get the loaded image
    const sf::Image& getImage() const;

    /// Set a new input image for detection
    /// @param image New image to detect objects in
    /// @return true on success, false on failure
    bool setImage(const sf::Image& image);

    /// Get the class names
    const std::vector<std::string>& getClassNames() const;

    /// Check if tracker is initialized
    bool isInitialized() const;

private:
    Config m_config;
    std::vector<std::string> m_classNames;
    cv::dnn::Net m_net;
    sf::Image m_inputImage;
    sf::Font m_font;
    bool m_initialized = false;

    /// Load class names from file
    bool loadClassNames(const std::string& path);

    /// Preprocess image for YOLO inference
    cv::Mat preprocessImage(const sf::Image& image);

    /// Postprocess YOLO output to get detections
    std::vector<DetectedObject> postprocess(const cv::Mat& output);

    /// Apply NMS to filter overlapping detections
    std::vector<DetectedObject> applyNMS(const std::vector<DetectedObject>& detections);

    /// Load font for text rendering
    bool loadFont();
};

} // namespace opencv

#endif // OBJECT_TRACKER_H
