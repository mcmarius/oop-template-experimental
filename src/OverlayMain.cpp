#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include "IPCController.h"
#include "TransparentWindow.h"

// Overlay executable - runs as separate process
// This window displays transparent overlay on top of video window
// Position is synchronized via IPC with video process
// Now includes streaming source picker UI


int main() {
    const std::string imageFile = "assets/image.png";
    const unsigned char alpha = 100;

    // Overlay process starting

    // Load the overlay image (keep a copy for shape re-application)
    sf::Image overlayImage;
    if (!overlayImage.loadFromFile(imageFile)) {
        std::cerr << "Failed to load overlay image: " << imageFile << std::endl;
        return -1;
    }

    // Keep original image for re-applying shape after resize
    sf::Image originalImage = overlayImage;

    // Store original image size for button scaling (this is the logical reference size)
    sf::Vector2u originalImageSize = overlayImage.getSize();

    // Create the overlay window using TransparentWindow class
    transparent::TransparentWindow overlay;
    transparent::TransparentWindow::Config config;
    config.size = originalImageSize;
    config.imageFile = imageFile;
    config.alpha = alpha;
    config.style = sf::Style::None;
    config.videoToOverlayPath = "assets/video_to_overlay.json";
    config.overlayToVideoPath = "assets/overlay_to_video.json";

    if (!overlay.initialize(config)) {
        std::cerr << "Failed to initialize transparent overlay window" << std::endl;
        return -1;
    }

    // Initialize IPC controller for position synchronization and source discovery
    ipc::IPCController ipc;
    ipc::IPCController::Config ipcConfig;
    ipcConfig.updateIntervalMs = 50;
    ipcConfig.videoToOverlayPath = "assets/video_to_overlay.json";
    ipcConfig.overlayToVideoPath = "assets/overlay_to_video.json";

    if (!ipc.initialize(ipcConfig)) {
        std::cerr << "Failed to initialize IPC controller" << std::endl;
        return -1;
    }

    // Wait for video window to start and write position
    int waitCount = 0;
    while (!ipc.isVideoFileReady() && waitCount < 200) {  // Wait up to 10 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        waitCount++;
    }

    if (!ipc.isVideoFileReady()) {
        std::cerr << "Timeout waiting for video window position file" << std::endl;
        std::cerr << "Make sure video process is running first" << std::endl;
        return -1;
    }

    // Wait for streaming sources to be available in IPC
    int sourceWaitCount = 0;
    std::vector<ipc::StreamingSource> sources;
    while (sourceWaitCount < 100) {  // Wait up to 5 seconds
        sources = ipc.readStreamingSources();
        if (!sources.empty()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        sourceWaitCount++;
    }
    
    if (sources.empty()) {
        // No streaming sources found - overlay will show 'Open File' option
    } else {
        // Sources discovered via IPC
    }

    // Main loop - sync with video window and handle drawing
    while (overlay.isOpen()) {
        bool shouldExit = false;

        // Read video window position and sync overlay
        sf::Vector2i videoPos;
        sf::Vector2u videoSize;

        if (ipc.readVideoPosition(videoPos, videoSize)) {
            // Sync overlay position and size to match video window
            overlay.setPosition(videoPos);
            overlay.setSize(videoSize);

            // Write overlay position to IPC file for symmetry
            ipc.writeOverlayPosition(videoPos, videoSize);

            // Scale UI elements based on window resize
            overlay.scaleUIElements(originalImageSize, videoSize);
        }

        // Sources are now refreshed only when user clicks the Sources button
        // (via refreshSources() in TransparentWindowEvents.cpp)

        // Check if no sources are available
        //bool noSources = false;
        //if (!sources.empty()) {
        //    noSources = false;
        //} else {
        //    // Also check the IPC flag
        //    noSources = true;
        //}

        // Process events through TransparentWindow
        if (!overlay.pollEvents()) {
            shouldExit = true;
        }

        // Handle special events
        while (const std::optional event = overlay.getWindow().pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                overlay.close();
                std::cout << "Overlay window closed" << std::endl;
            }
            else if (event->is<sf::Event::Resized>()) {
                std::cout << "Overlay resized: " << overlay.getSize().x << "x" << overlay.getSize().y << std::endl;
            }
            else if (event->is<sf::Event::KeyPressed>()) {
                const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    shouldExit = true;
                }
            }
            else if (event->is<sf::Event::MouseButtonPressed>()) {
                const auto& mouseEvent = *event->getIf<sf::Event::MouseButtonPressed>();
                if (mouseEvent.button == sf::Mouse::Button::Left) {
                    // Button click is handled by TransparentWindow::pollEvents()
                }
            }
        }

        if (shouldExit) {
            overlay.close();
            break;
        }

        // Draw the transparent overlay
        overlay.draw();

        // Small sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    ipc.cleanup();

    std::cout << "Overlay process terminated" << std::endl;
    return 0;
}
