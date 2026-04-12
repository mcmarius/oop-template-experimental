#include "VideoWindow.h"
#include <iostream>
#include <thread>
#include <vlc/vlc.h>

#include "IPCController.h"

// Include ObjectTracker for YOLO detection
#include "ObjectTracker.h"

namespace vlc {

namespace {
    VLC::Instance createInstance() {
        std::cout << "[VideoWindow] Constructor: Creating VLC Instance (libvlc_new)..." << std::endl;
        
        // libvlc_new on macOS might fail because it can't find plugins.
        // Try with no arguments first.
        std::cout << "[VideoWindow] Constructor: Trying libvlc_new(0, nullptr)..." << std::endl;
        libvlc_instance_t* rawInstance = libvlc_new(0, nullptr);
        if (rawInstance == nullptr) {
            std::cerr << "[VideoWindow] Constructor: libvlc_new returned nullptr!" << std::endl;
            throw std::runtime_error("Failed to create VLC instance - libvlc_new returned nullptr");
        }
        std::cout << "[VideoWindow] Constructor: libvlc_new succeeded, wrapping..." << std::endl;
        
        try {
            auto instance = VLC::Instance(rawInstance);
            std::cout << "[VideoWindow] Constructor: VLC Instance created successfully" << std::endl;
            return instance;
        } catch (const std::exception& e) {
            std::cerr << "[VideoWindow] Constructor: Exception: " << e.what() << std::endl;
            throw;
        }
    }
}

VideoWindow::VideoWindow()
    : m_instance(createInstance()),
      m_lastLoadedSource(),
      m_lastDetectionSize(),
      m_detectionDoneOnPause(false) {
    std::cout << "[VideoWindow] Constructor: Calling initializePlatform..." << std::endl;
    initializePlatform();
    std::cout << "[VideoWindow] Constructor: Done" << std::endl;
}

VideoWindow::~VideoWindow() {
    stop();
}

bool VideoWindow::initialize(const Config& config) {
    std::cout << "[VideoWindow] Initialize: Starting..." << std::endl;
    m_config = config;

    // Create the window
    std::cout << "[VideoWindow] Initialize: Creating SFML window..." << std::endl;
    m_window.create(sf::VideoMode(config.size), config.title, config.style);
    std::cout << "[VideoWindow] Initialize: SFML window created" << std::endl;

    // Disable vsync to avoid burning GPU, set FPS limit instead
    m_window.setVerticalSyncEnabled(false);
    m_window.setFramerateLimit(60);

    std::cout << "[VideoWindow] Initialize: Done" << std::endl;
    return true;
}

void VideoWindow::close() {
    stop();
    m_window.close();
}

bool VideoWindow::isOpen() const {
    return m_window.isOpen();
}

sf::RenderWindow& VideoWindow::getWindow() {
    return m_window;
}

const sf::RenderWindow& VideoWindow::getWindow() const {
    return m_window;
}

sf::WindowHandle VideoWindow::getNativeHandle() const {
    return m_window.getNativeHandle();
}

bool VideoWindow::pollEvents() {
    // IPC for window switching
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.overlayToVideoPath = m_config.overlayToVideoPath;
    if (ipc.initialize(config)) {
        // Read which window should be active
        std::string activeWindow = ipc.readActiveWindow();
        if (activeWindow == "video") {
            m_window.requestFocus();
            m_window.setVisible(true);
        }
    }
    if (!m_window.isOpen()) {
        return false;
    }

    bool shouldExit = false;

    // Check for IPC play/pause commands
    handlePlayPauseCommandFromIPC();

    // Run YOLO detection on current frame (throttled to ~5 FPS)
    runDetection();


    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
            std::cout << "Video window closed" << std::endl;
        }
        else if (event->is<sf::Event::Resized>()) {
            std::cout << "Video window resized: "
                      << m_window.getSize().x << "x"
                      << m_window.getSize().y << std::endl;
        }
        else if (event->is<sf::Event::KeyPressed>()) {
            const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();

            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                shouldExit = true;
            }
            else if (keyPressed->scancode == sf::Keyboard::Scancode::Grave) {
                // Alt+` pressed - write that video window is active
                std::cout << "Overlay: Alt+` pressed, writing active_window=overlay" << std::endl;
                ipc.writeActiveWindow("overlay");
            }
            else if (keyPressed->scancode == sf::Keyboard::Scancode::Space ||
                     keyPressed->scancode == sf::Keyboard::Scancode::P) {
                playPause();
                // NOTE: this is in reverse due to delay; we would have to add some sleep to get the real state
                // from m_mediaPlayer.isPlaying
                std::cout << "Video window: " << (m_mediaPlayer.isPlaying() ? "paused" : "playing") << std::endl;
            }
        }
    }

    return !shouldExit;
}

void VideoWindow::setPosition(sf::Vector2i position) {
    m_window.setPosition(position);
}

void VideoWindow::setSize(sf::Vector2u size) {
    m_window.setSize(size);
}

sf::Vector2u VideoWindow::getPosition() const {
    return sf::Vector2u(m_window.getPosition());
}

sf::Vector2u VideoWindow::getSize() const {
    return m_window.getSize();
}

void VideoWindow::playPause() {
    if (m_mediaPlayer.isPlaying()) {
        m_mediaPlayer.pause();
        // Reset detection flag when pausing - will run detection on next render
        m_detectionDoneOnPause = false;
    } else {
        // For live streams, restart to skip buffered frames during pause
        // For file sources, just play (frame-accurate resume)
        if (m_isStream) {
            std::cout << "Live stream detected - restarting to skip buffered frames" << std::endl;
            m_mediaPlayer.stop();
            m_mediaPlayer.play();
        } else {
            m_mediaPlayer.play();
        }
        // Reset detection flag when resuming - don't run detection while playing
        m_detectionDoneOnPause = false;
    }
}

bool VideoWindow::isPlaying() {
    return m_mediaPlayer.isPlaying();
}

bool VideoWindow::handlePlayPauseCommandFromIPC() {
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.overlayToVideoPath = m_config.overlayToVideoPath;
    
    if (ipc.initialize(config)) {
        if (ipc.readPlayPauseCommand()) {
            playPause();
            // NOTE: this is in reverse due to delay; we would have to add some sleep to get the real state
            // from m_mediaPlayer.isPlaying
            std::cout << "Video window (IPC): " << (m_mediaPlayer.isPlaying() ? "paused" : "playing") << std::endl;
            return true;
        }
    }
    return false;
}

} // namespace vlc
