#include "ObjectTracker.h"
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

namespace opencv {

ObjectTracker::ObjectTracker() = default;

ObjectTracker::~ObjectTracker() = default;

bool ObjectTracker::initialize(const Config& config) {
    m_config = config;

    std::cout << "[ObjectTracker] Loading class names from: " << config.namesPath << std::endl;
    if (!loadClassNames(config.namesPath)) {
        std::cerr << "[ObjectTracker] Failed to load class names" << std::endl;
        return false;
    }
    std::cout << "[ObjectTracker] Loaded " << m_classNames.size() << " class names" << std::endl;

    std::cout << "[ObjectTracker] Loading YOLO model..." << std::endl;
    try {
        m_net = cv::dnn::readNetFromDarknet(config.cfgPath, config.weightsPath);
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        std::cout << "[ObjectTracker] YOLO model loaded successfully" << std::endl;
    } catch (const cv::Exception& e) {
        std::cerr << "[ObjectTracker] Failed to load YOLO model: " << e.what() << std::endl;
        return false;
    }

    std::cout << "[ObjectTracker] Loading input image from: " << config.imagePath << std::endl;
    if (!m_inputImage.loadFromFile(config.imagePath)) {
        std::cerr << "[ObjectTracker] Failed to load image: " << config.imagePath << std::endl;
        return false;
    }
    std::cout << "[ObjectTracker] Image loaded: " << m_inputImage.getSize().x << "x" << m_inputImage.getSize().y << std::endl;

    // Load font for text rendering
    if (!loadFont()) {
        std::cerr << "[ObjectTracker] Warning: Failed to load font, using pixel-based labels" << std::endl;
    }

    m_initialized = true;
    return true;
}

bool ObjectTracker::loadFont() {
    std::cout << "[ObjectTracker] Loading font from assets/fonts/FiraSans-Regular.ttf" << std::endl;
    if (!m_font.openFromFile("assets/fonts/FiraSans-Regular.ttf")) {
        std::cerr << "[ObjectTracker] Failed to load font" << std::endl;
        return false;
    }
    std::cout << "[ObjectTracker] Font loaded successfully" << std::endl;
    return true;
}

bool ObjectTracker::loadClassNames(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    m_classNames.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            m_classNames.push_back(line);
        }
    }
    file.close();
    return !m_classNames.empty();
}

std::vector<DetectedObject> ObjectTracker::detect() {
    if (!m_initialized) {
        std::cerr << "[ObjectTracker] Not initialized" << std::endl;
        return {};
    }

    // Preprocess image
    cv::Mat blob = preprocessImage(m_inputImage);

    // Run inference
    std::cout << "[ObjectTracker] Running YOLO inference..." << std::endl;
    m_net.setInput(blob);
    std::vector<cv::Mat> outputBlobs;
    m_net.forward(outputBlobs, m_net.getUnconnectedOutLayersNames());

    // Merge all detections from all output layers
    std::vector<DetectedObject> allDetections;
    for (const auto& output : outputBlobs) {
        std::vector<DetectedObject> layerDetections = postprocess(output);
        allDetections.insert(allDetections.end(), layerDetections.begin(), layerDetections.end());
    }

    // Apply NMS across all detections
    std::vector<DetectedObject> finalDetections = applyNMS(allDetections);

    std::cout << "[ObjectTracker] Detected " << finalDetections.size() << " objects" << std::endl;
    return finalDetections;
}

cv::Mat ObjectTracker::preprocessImage(const sf::Image& image) {
    sf::Vector2u imgSize = image.getSize();

    // Convert SFML image to OpenCV Mat
    cv::Mat mat(imgSize.y, imgSize.x, CV_8UC4, const_cast<uchar*>(image.getPixelsPtr()));

    // Convert BGRA to BGR (drop alpha channel)
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);

    // Create blob from image for YOLO
    cv::Mat blob;
    cv::dnn::blobFromImage(bgr, blob, 1.0 / 255.0, cv::Size(416, 416), cv::Scalar(0, 0, 0), true, false);

    return blob;
}

std::vector<DetectedObject> ObjectTracker::postprocess(const cv::Mat& output) {
    std::vector<DetectedObject> detections;
    sf::Vector2u imgSize = m_inputImage.getSize();

    // YOLO output shape: (1, n, 85) where n is number of detections
    // Each detection: [x, y, w, h, objectness, class0_prob, class1_prob, ...]

    int numDetections = output.rows;

    for (int i = 0; i < numDetections; ++i) {
        // Get objectness score
        float objectness = output.at<float>(i, 4);
        if (objectness < m_config.confidenceThreshold) {
            continue;
        }

        // Find best class
        int classId = 0;
        float maxClassScore = 0.0f;
        for (int j = 5; j < output.cols; ++j) {
            float classScore = output.at<float>(i, j);
            if (classScore > maxClassScore) {
                maxClassScore = classScore;
                classId = j - 5;
            }
        }

        // Filter by confidence
        float confidence = objectness * maxClassScore;
        if (confidence < m_config.confidenceThreshold) {
            continue;
        }

        // Get bounding box coordinates (center x, y, width, height)
        float centerX = output.at<float>(i, 0) * imgSize.x;
        float centerY = output.at<float>(i, 1) * imgSize.y;
        float width = output.at<float>(i, 2) * imgSize.x;
        float height = output.at<float>(i, 3) * imgSize.y;

        // Convert to top-left coordinates and size
        float x = centerX - width / 2.0f;
        float y = centerY - height / 2.0f;

        DetectedObject obj;
        obj.className = m_classNames[classId];
        obj.confidence = confidence;
        obj.boundingBox = sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(width, height));
        obj.intBoundingBox = sf::IntRect(sf::Vector2i(static_cast<int>(x), static_cast<int>(y)),
                                          sf::Vector2i(static_cast<int>(width), static_cast<int>(height)));

        detections.push_back(obj);
    }

    return detections;
}

std::vector<DetectedObject> ObjectTracker::applyNMS(const std::vector<DetectedObject>& detections) {
    // Apply NMS using OpenCV Rect types
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    for (const auto& obj : detections) {
        boxes.push_back(cv::Rect(obj.intBoundingBox.position.x, obj.intBoundingBox.position.y,
                                  obj.intBoundingBox.size.x, obj.intBoundingBox.size.y));
        scores.push_back(obj.confidence);
    }

    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, scores, m_config.confidenceThreshold, m_config.nmsThreshold, indices);

    // Filter detections by NMS indices
    std::vector<DetectedObject> nmsDetections;
    for (int idx : indices) {
        nmsDetections.push_back(detections[idx]);
    }

    return nmsDetections;
}

sf::Image ObjectTracker::drawDetections(const sf::Image& inputImage, const std::vector<DetectedObject>& detections) {
    sf::Vector2u imgSize = inputImage.getSize();
    sf::Image outputImage = inputImage;

    // Get pixel data
    std::uint8_t* pixels = const_cast<std::uint8_t*>(outputImage.getPixelsPtr());

    for (const auto& obj : detections) {
        // Draw bounding box
        int x = obj.intBoundingBox.position.x;
        int y = obj.intBoundingBox.position.y;
        int width = obj.intBoundingBox.size.x;
        int height = obj.intBoundingBox.size.y;

        // Draw rectangle border (red, 2px thick)
        sf::Color borderColor(255, 0, 0, 255);

        // Top and bottom borders
        for (int i = x; i < x + width; ++i) {
            if (i >= 0 && i < static_cast<int>(imgSize.x) && y >= 0 && y < static_cast<int>(imgSize.y)) {
                pixels[y * imgSize.x * 4 + i * 4 + 0] = borderColor.r;
                pixels[y * imgSize.x * 4 + i * 4 + 1] = borderColor.g;
                pixels[y * imgSize.x * 4 + i * 4 + 2] = borderColor.b;
                pixels[y * imgSize.x * 4 + i * 4 + 3] = borderColor.a;
            }
            int bottomY = y + height - 1;
            if (i >= 0 && i < static_cast<int>(imgSize.x) && bottomY >= 0 && bottomY < static_cast<int>(imgSize.y)) {
                pixels[bottomY * imgSize.x * 4 + i * 4 + 0] = borderColor.r;
                pixels[bottomY * imgSize.x * 4 + i * 4 + 1] = borderColor.g;
                pixels[bottomY * imgSize.x * 4 + i * 4 + 2] = borderColor.b;
                pixels[bottomY * imgSize.x * 4 + i * 4 + 3] = borderColor.a;
            }
        }

        // Left and right borders
        for (int j = y; j < y + height; ++j) {
            if (x >= 0 && x < static_cast<int>(imgSize.x) && j >= 0 && j < static_cast<int>(imgSize.y)) {
                pixels[j * imgSize.x * 4 + x * 4 + 0] = borderColor.r;
                pixels[j * imgSize.x * 4 + x * 4 + 1] = borderColor.g;
                pixels[j * imgSize.x * 4 + x * 4 + 2] = borderColor.b;
                pixels[j * imgSize.x * 4 + x * 4 + 3] = borderColor.a;
            }
            int rightX = x + width - 1;
            if (rightX >= 0 && rightX < static_cast<int>(imgSize.x) && j >= 0 && j < static_cast<int>(imgSize.y)) {
                pixels[j * imgSize.x * 4 + rightX * 4 + 0] = borderColor.r;
                pixels[j * imgSize.x * 4 + rightX * 4 + 1] = borderColor.g;
                pixels[j * imgSize.x * 4 + rightX * 4 + 2] = borderColor.b;
                pixels[j * imgSize.x * 4 + rightX * 4 + 3] = borderColor.a;
            }
        }
    }

    return outputImage;
}

sf::Image ObjectTracker::drawDetectionsWithText(const sf::Image& inputImage, const std::vector<DetectedObject>& detections) {
    sf::Vector2u imgSize = inputImage.getSize();

    // Create texture from input image
    sf::Texture texture;
    if (!texture.loadFromImage(inputImage)) {
        // Fallback to pixel-based drawing if texture loading fails
        return drawDetections(inputImage, detections);
    }

    sf::Sprite sprite(texture);

    // Create render texture for drawing with sf::Text
    sf::RenderTexture renderTexture(imgSize);

    // Draw sprite first
    renderTexture.clear();
    renderTexture.draw(sprite);

    // Draw bounding boxes and labels with sf::Text
    sf::RectangleShape boxShape;
    boxShape.setFillColor(sf::Color::Transparent);
    boxShape.setOutlineColor(sf::Color::Red);
    boxShape.setOutlineThickness(2);

    for (const auto& obj : detections) {
        // Draw bounding box
        boxShape.setSize(sf::Vector2f(obj.intBoundingBox.size));
        boxShape.setPosition(sf::Vector2f(obj.intBoundingBox.position));
        renderTexture.draw(boxShape);

        // Draw label with sf::Text
        std::string label = obj.className + " " + std::to_string(static_cast<int>(obj.confidence * 100)) + "%";
        sf::Text text(m_font, sf::String::fromUtf8(label.begin(), label.end()), 16u);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1);

        // Position label above or below the bounding box
        sf::Vector2f labelPos(obj.intBoundingBox.position.x, obj.intBoundingBox.position.y - 25);
        if (labelPos.y < 0) labelPos.y = obj.intBoundingBox.position.y + obj.intBoundingBox.size.y + 5;
        text.setPosition(labelPos);

        renderTexture.draw(text);
    }

    renderTexture.display();

    // Get the rendered image
    sf::Image result = renderTexture.getTexture().copyToImage();

    return result;
}

const sf::Image& ObjectTracker::getImage() const {
    return m_inputImage;
}

bool ObjectTracker::setImage(const sf::Image& image) {
    if (image.getSize().x == 0 || image.getSize().y == 0) {
        return false;
    }
    m_inputImage = image;
    return true;
}

const std::vector<std::string>& ObjectTracker::getClassNames() const {
    return m_classNames;
}

bool ObjectTracker::isInitialized() const {
    return m_initialized;
}

} // namespace opencv
