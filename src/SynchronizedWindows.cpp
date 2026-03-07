#include "SynchronizedWindows.h"
#include "VideoWindow.h"
#include "TransparentWindow.h"
#include "SynchronizedWindowsPlatform.h"
#include <iostream>

namespace syncwindows {

SynchronizedWindows::SynchronizedWindows() = default;

SynchronizedWindows::~SynchronizedWindows() {
    shutdown();
}

bool SynchronizedWindows::initialize(const Config& config) {
    m_config = config;

    // Create transparent overlay window FIRST so it appears on top
    transparent::TransparentWindow::Config overlayConfig;
    overlayConfig.size = config.overlaySize;
    overlayConfig.imageFile = config.overlayImageFile;
    overlayConfig.alpha = config.overlayAlpha;
    overlayConfig.style = config.overlayStyle;

    m_overlayWindow = new transparent::TransparentWindow();
    if (!m_overlayWindow->initialize(overlayConfig)) {
        std::cerr << "Failed to initialize overlay window" << std::endl;
        delete m_overlayWindow;
        m_overlayWindow = nullptr;
        return false;
    }

    // Create video window
    vlc::VideoWindow::Config videoConfig;
    videoConfig.size = config.videoSize;
    videoConfig.title = config.videoTitle;
    videoConfig.style = config.videoStyle;
    videoConfig.startPlaying = false; // Start later manually

    m_videoWindow = new vlc::VideoWindow();
    if (!m_videoWindow->initialize(videoConfig)) {
        std::cerr << "Failed to initialize video window" << std::endl;
        delete m_videoWindow;
        m_videoWindow = nullptr;
        delete m_overlayWindow;
        m_overlayWindow = nullptr;
        return false;
    }

    // Sync initial position/size
    syncOverlayToVideo();

    // Window grouping for Alt+` switching
    // This allows the OS to treat video and overlay windows as a group
    // for Alt+` (or Cmd+` on macOS) window switching
    // Both windows must have the same _NET_WM_GROUP_ID value
    sf::WindowHandle groupLeader = m_overlayWindow->getNativeHandle();
    m_videoWindow->setWindowGroup(groupLeader);
    m_overlayWindow->setWindowGroup(groupLeader);

    // Raise the transparent window to the top so it appears above the video window
    // This is needed because the video window was created second
    raiseWindowLinux(m_overlayWindow->getNativeHandle());

    m_running = true;
    return true;
}

bool SynchronizedWindows::startVideo() {
    if (!m_videoWindow) {
        return false;
    }

    m_videoStarted = m_videoWindow->start();
    return m_videoStarted;
}

void SynchronizedWindows::syncOverlayToVideo() {
    if (!m_videoWindow || !m_overlayWindow) {
        return;
    }

    // Get video window position and size
    sf::Vector2u videoPos = m_videoWindow->getPosition();
    sf::Vector2u videoSize = m_videoWindow->getSize();

    // Set overlay to match
    m_overlayWindow->setPosition(sf::Vector2i(videoPos));
    m_overlayWindow->setSize(videoSize);
}

void SynchronizedWindows::handleVideoResize(sf::Vector2u newSize) {
    if (!m_overlayWindow) {
        return;
    }

    // Update overlay size
    m_overlayWindow->setSize(newSize);

    // Also resize the video window's internal rendering area if needed
    // (VLC handles its own rendering, but we ensure consistency)
    m_videoWindow->setSize(newSize);

    // Notify callback if set
    if (m_config.onResize) {
        m_config.onResize(newSize);
    }
}

int SynchronizedWindows::run() {
    if (!m_videoWindow || !m_overlayWindow) {
        std::cerr << "Windows not initialized" << std::endl;
        return 1;
    }

    // Start video playback
    if (!startVideo()) {
        std::cerr << "Failed to start video" << std::endl;
        return 1;
    }

    std::cout << "Synchronized windows running. Press ESC to exit." << std::endl;

    while (m_running) {
        // Process events for both windows
        bool videoAlive = m_videoWindow->pollEvents();
        bool overlayAlive = m_overlayWindow->pollEvents();

        if (!videoAlive || !overlayAlive) {
            std::cout << "Window closed, shutting down..." << std::endl;
            break;
        }

        // Sync overlay with video window
        syncOverlayToVideo();

        // Draw the transparent overlay each frame so it appears on top of the video
        // The overlay is cleared with transparent color so the video shows through
        m_overlayWindow->draw();

        // If button was clicked, raise overlay to ensure it stays on top
        if (m_overlayWindow->wasButtonClicked()) {
            raiseTwoWindowsLinux(m_overlayWindow->getNativeHandle(), m_videoWindow->getNativeHandle());
            m_overlayWindow->resetButtonClicked();
        }

        // Small delay to prevent busy-waiting
        sf::sleep(sf::milliseconds(10));
    }

    shutdown();
    return 0;
}

void SynchronizedWindows::shutdown() {
    m_running = false;

    if (m_videoWindow) {
        m_videoWindow->stop();
        m_videoWindow->close();
        delete m_videoWindow;
        m_videoWindow = nullptr;
    }

    if (m_overlayWindow) {
        m_overlayWindow->close();
        delete m_overlayWindow;
        m_overlayWindow = nullptr;
    }
}

bool SynchronizedWindows::isRunning() const {
    return m_running && m_videoWindow && m_overlayWindow;
}

vlc::VideoWindow& SynchronizedWindows::getVideoWindow() {
    if (!m_videoWindow) {
        throw std::runtime_error("Video window not initialized");
    }
    return *m_videoWindow;
}

const vlc::VideoWindow& SynchronizedWindows::getVideoWindow() const {
    if (!m_videoWindow) {
        throw std::runtime_error("Video window not initialized");
    }
    return *m_videoWindow;
}

transparent::TransparentWindow& SynchronizedWindows::getOverlayWindow() {
    if (!m_overlayWindow) {
        throw std::runtime_error("Overlay window not initialized");
    }
    return *m_overlayWindow;
}

const transparent::TransparentWindow& SynchronizedWindows::getOverlayWindow() const {
    if (!m_overlayWindow) {
        throw std::runtime_error("Overlay window not initialized");
    }
    return *m_overlayWindow;
}

} // namespace syncwindows