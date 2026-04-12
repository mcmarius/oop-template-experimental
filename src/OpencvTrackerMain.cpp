#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include "ObjectTracker.h"

/// OpencvTrackerMain - Main entry point for OpenCV YOLO object tracker
/// Loads an image, runs YOLO detection, and displays results in an SFML window

int main() {
    std::cout << "=== OpenCV YOLO Object Tracker ===" << std::endl;
    std::cout << "This example demonstrates YOLO-based object detection on a static image." << std::endl;
    std::cout << "Press ESC to exit.\n" << std::endl;

    // Create and initialize object tracker
    opencv::ObjectTracker tracker;
    opencv::ObjectTracker::Config config;

    config.imagePath = "assets/test_image.png";
    config.confidenceThreshold = 0.5f;
    config.nmsThreshold = 0.4f;

    if (!tracker.initialize(config)) {
        std::cerr << "Failed to initialize object tracker" << std::endl;
        return -1;
    }

    // Run detection
    std::cout << "\n=== Running Object Detection ===" << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<opencv::DetectedObject> detections = tracker.detect();
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Detection completed in " << duration.count() << " ms" << std::endl;
    std::cout << "Objects detected: " << detections.size() << std::endl;

    for (const auto& obj : detections) {
        std::cout << "  - " << obj.className << ": " 
                  << static_cast<int>(obj.confidence * 100) << "%"
                  << " at (" << obj.intBoundingBox.position.x << ", " << obj.intBoundingBox.position.y << ")"
                  << std::endl;
    }

    // Draw detections on image with sf::Text labels
    std::cout << "\n=== Drawing Detections ===" << std::endl;
    sf::Image resultImage = tracker.drawDetectionsWithText(tracker.getImage(), detections);

    // Create display window
    sf::Vector2u imgSize = resultImage.getSize();
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{imgSize.x, imgSize.y}), "YOLO Object Detection", sf::Style::Close);
    window.setFramerateLimit(60);

    // Create texture from result image
    sf::Texture resultTexture;
    if (!resultTexture.loadFromImage(resultImage)) {
        std::cerr << "Failed to create texture from result image" << std::endl;
        return -1;
    }

    sf::Sprite resultSprite(resultTexture);

    // Main loop
    while (window.isOpen()) {
        // Process events
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>() || 
                (event->is<sf::Event::KeyPressed>() && 
                 event->getIf<sf::Event::KeyPressed>()->scancode == sf::Keyboard::Scancode::Escape)) {
                window.close();
            }
        }

        // Draw
        window.clear();
        window.draw(resultSprite);
        window.display();

        // Small sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Object tracker terminated" << std::endl;
    return 0;
}
