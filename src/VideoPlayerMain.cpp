#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <X11/Xlib.h>
#undef Always
#undef None
#include <SFML/Graphics.hpp>
#include "IPCController.h"
#include "VideoWindow.h"

// Video player executable - runs as separate process
// This window ONLY displays video - no overlay drawing
// Now includes streaming source discovery and IPC-based source selection

int main() {
    XInitThreads();

    // Create video window with IPC support
    vlc::VideoWindow videoWindow;
    vlc::VideoWindow::Config config;
    config.size = {1280, 720};
    config.title = "Video Stream";
    config.style = sf::Style::Default;
    config.startPlaying = true;
    config.videoToOverlayPath = "assets/video_to_overlay.json";
    config.overlayToVideoPath = "assets/overlay_to_video.json";

    if (!videoWindow.initialize(config)) {
        std::cerr << "Failed to initialize video window" << std::endl;
        return -1;
    }

    std::cout << "Video window created with size: " << config.size.x << "x" << config.size.y << std::endl;

    // Start video playback
    if (!videoWindow.start()) {
        std::cerr << "Failed to start video playback" << std::endl;
        return -1;
    }

    std::cout << "Video playback started. Window is ready for overlay." << std::endl;

    // Discover and write streaming sources to IPC
    std::vector<ipc::StreamingSource> sources = videoWindow.discoverSources();
    
    // Retry writing sources to IPC with a few attempts
    bool sourcesWritten = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (videoWindow.writeSourcesToIPC(sources)) {
            sourcesWritten = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!sourcesWritten) {
        std::cerr << "Warning: Failed to write sources to IPC after multiple attempts" << std::endl;
    }

    if (sources.empty()) {
        std::cout << "No streaming sources discovered." << std::endl;
        std::cout << "Overlay will show empty list with 'Open File' option." << std::endl;
    } else {
        std::cout << "Discovered " << sources.size() << " streaming source(s)." << std::endl;
        for (const auto& source : sources) {
            std::cout << "  - " << source.name << " (" << source.mrl << ")" << std::endl;
        }
    }

    // Get native handle and tell VLC to render to this window
    // VLC already configured in VideoWindow::start()

    // Initialize IPC controller for overlay synchronization
    ipc::IPCController ipc;
    ipc::IPCController::Config ipcConfig;
    ipcConfig.updateIntervalMs = 50;
    ipcConfig.videoToOverlayPath = "assets/video_to_overlay.json";
    ipcConfig.overlayToVideoPath = "assets/overlay_to_video.json";

    if (!ipc.initialize(ipcConfig)) {
        std::cerr << "Failed to initialize IPC controller" << std::endl;
        // Continue anyway - overlay will just not synchronize
    }

    std::cout << "Video→Overlay file: " << ipc.getVideoToOverlayPath() << std::endl;
    std::cout << "Overlay→Video file: " << ipc.getOverlayToVideoPath() << std::endl;

    // Main loop - just process events, VLC handles rendering
    while (videoWindow.isOpen()) {
        bool shouldExit = false;

        // Write current position to IPC file for overlay synchronization
        {
            sf::Vector2i pos = videoWindow.getWindow().getPosition();
            sf::Vector2u size = videoWindow.getWindow().getSize();
            ipc.writeVideoPosition(pos, size);
        }

        // Read selected source from IPC and apply if changed
        videoWindow.setSourceFromIPC();

        // Process events through the window directly
        while (const std::optional event = videoWindow.getWindow().pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                videoWindow.close();
                std::cout << "Video window closed" << std::endl;
            }
            else if (event->is<sf::Event::Resized>()) {
                std::cout << "Video window resized: " << videoWindow.getSize().x << "x" << videoWindow.getSize().y << std::endl;
            }
            else if (event->is<sf::Event::KeyPressed>()) {
                const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    shouldExit = true;
                }
            }
        }

        if (shouldExit) {
            videoWindow.close();
            std::cout << "Video player exiting (ESC pressed)" << std::endl;
            break;
        }

        // Small sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup IPC file
    ipc.cleanup();

    std::cout << "Video player terminated" << std::endl;
    return 0;
}