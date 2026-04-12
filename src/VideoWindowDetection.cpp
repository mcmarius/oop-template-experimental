#include "VideoWindow.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "IPCController.h"

// OpenGL headers for glReadPixels
#ifdef SFML_SYSTEM_LINUX
    #include <GL/gl.h>
#elif defined(SFML_SYSTEM_WINDOWS)
    #include <GL/gl.h>
#elif defined(SFML_SYSTEM_MACOS)
    #include <OpenGL/gl.h>
#endif

namespace vlc {

bool VideoWindow::initializeDetection() {
    if (m_detectionInitialized) {
        return true;  // Already initialized
    }

    std::cout << "[VideoWindow] Initializing YOLO detection..." << std::endl;

    // Use default YOLO model files
    opencv::ObjectTracker::Config config;
    config.cfgPath = "assets/yolo/yolov4-tiny.cfg";
    config.weightsPath = "assets/yolo/yolov4-tiny.weights";
    config.namesPath = "assets/yolo/coco.names";
    config.confidenceThreshold = 0.5f;
    config.nmsThreshold = 0.4f;

    if (!m_objectTracker.initialize(config)) {
        std::cerr << "[VideoWindow] Failed to initialize YOLO detection" << std::endl;
        return false;
    }

    m_detectionInitialized = true;
    m_lastDetectionSize = {640, 480};  // Default detection size
    std::cout << "[VideoWindow] YOLO detection initialized successfully" << std::endl;
    return true;
}

void VideoWindow::runDetection() {
    if (!m_started || !m_detectionInitialized) {
        return;
    }

    // Only run detection when video is paused AND we haven't already detected on this pause
    // This prevents running detection on every render when nothing changes
    if (m_mediaPlayer.isPlaying()) {
        // std::cout << "[runDetection] Skipped - video playing" << std::endl;
        return;  // Video is playing - skip detection
    }
    if (m_detectionDoneOnPause) {
        // std::cout << "[runDetection] Skipped - already detected this pause" << std::endl;
        return;
    }

    // Detection running - flag is false and video is paused

    // Get current frame from VLC by reading OpenGL pixels from the window
    // In SFML 3, capture() method was removed, so we use glReadPixels
    sf::Vector2u windowSize = m_window.getSize();

    if (windowSize.x == 0 || windowSize.y == 0) {
        return;
    }

    // Activate the window's OpenGL context
    if (!m_window.setActive(true)) {
        std::cerr << "[VideoWindow] Failed to activate window context" << std::endl;
        return;
    }

    // Read pixels from the window
    std::vector<uint8_t> pixels(windowSize.x * windowSize.y * 4);  // RGBA format
    glReadPixels(0, 0, windowSize.x, windowSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    m_window.setActive(false);  // Deactivate

    // Create SFML Image from the pixels (SFML 3 constructor takes size and pixels)
    // Note: OpenGL origin is bottom-left, so we need to flip vertically
    sf::Image windowImage;
    {
        // Create a temporary flipped pixel buffer
        std::vector<uint8_t> flippedPixels(windowSize.x * windowSize.y * 4);
        for (unsigned int y = 0; y < windowSize.y; ++y) {
            for (unsigned int x = 0; x < windowSize.x; ++x) {
                unsigned int srcIdx = (y * windowSize.x + x) * 4;
                unsigned int dstIdx = ((windowSize.y - 1 - y) * windowSize.x + x) * 4;
                flippedPixels[dstIdx + 0] = pixels[srcIdx + 0];  // R
                flippedPixels[dstIdx + 1] = pixels[srcIdx + 1];  // G
                flippedPixels[dstIdx + 2] = pixels[srcIdx + 2];  // B
                flippedPixels[dstIdx + 3] = pixels[srcIdx + 3];  // A
            }
        }
        windowImage = sf::Image(windowSize, flippedPixels.data());
    }

    // Skip if image is empty
    if (windowImage.getSize().x == 0 || windowImage.getSize().y == 0) {
        return;
    }

    // Save original size for scaling detection results back
    sf::Vector2u originalSize = windowImage.getSize();

    // Update ObjectTracker's input image for detection
    if (!m_objectTracker.setImage(windowImage)) {
        std::cerr << "[VideoWindow] Failed to set image for detection" << std::endl;
        return;
    }

    // Run detection
    std::vector<opencv::DetectedObject> detections = m_objectTracker.detect();

    // Scale detection results from YOLO output size (416x416) to actual window size
    float scaleX = static_cast<float>(originalSize.x) / 416.0f;
    float scaleY = static_cast<float>(originalSize.y) / 416.0f;

    std::vector<ipc::DetectedObject> scaledDetections;
    for (const auto& det : detections) {
        ipc::DetectedObject scaledObj;
        scaledObj.className = det.className;
        scaledObj.confidence = det.confidence;

        // SFML 3: FloatRect uses position and size instead of left/top/width/height
        scaledObj.position = sf::Vector2f(
            det.boundingBox.position.x * scaleX,
            det.boundingBox.position.y * scaleY
        );
        scaledObj.size = sf::Vector2f(
            det.boundingBox.size.x * scaleX,
            det.boundingBox.size.y * scaleY
        );
        scaledDetections.push_back(scaledObj);
    }

    // Write detections to IPC
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.videoToOverlayPath = m_config.videoToOverlayPath;
    config.overlayToVideoPath = m_config.overlayToVideoPath;

    if (ipc.initialize(config)) {
        ipc.writeDetectedObjects(scaledDetections);
    }

    // Mark detection as done - won't run again until next play/pause cycle
    m_detectionDoneOnPause = true;
}

} // namespace vlc
