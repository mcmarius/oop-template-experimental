#include "VideoWindow.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "IPCController.h"

// Windows only
std::vector<std::string> GetWindowsWebcamNames();

namespace vlc {

bool VideoWindow::start() {
    std::cout << "[VideoWindow::start] Starting..." << std::endl;
    m_cameraSource = detectCameraSource();
    std::cout << "[VideoWindow::start] Camera source: " << m_cameraSource << std::endl;

    // Use MediaDiscoverer for real cameras, direct media for mock (file://)
    if (m_cameraSource.find("file://") == 0) {
        // Create media directly from video file (mock camera for CI)
        std::cout << "[VideoWindow::start] Using mock camera (video file) for CI testing" << std::endl;

        std::string path = m_cameraSource.substr(7);
        std::cout << "[VideoWindow::start] Creating Media from path: " << path << std::endl;
        try {
            auto media = VLC::Media(m_instance, path, VLC::Media::FromPath);
            std::cout << "[VideoWindow::start] Media created successfully" << std::endl;
            // Loop the video for continuous playback
            media.addOption(":input-repeat=65535");
            std::cout << "[VideoWindow::start] Creating MediaPlayer from Media..." << std::endl;
            m_mediaPlayer = VLC::MediaPlayer(media);
            std::cout << "[VideoWindow::start] MediaPlayer created successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[VideoWindow::start] Exception: " << e.what() << std::endl;
            throw;
        }

        m_isStream = false;
        m_sourcesAvailable = true;
    } else {
        // Live stream (camera device) - will skip frames on resume
        m_isStream = true;
        #if defined(_WIN32) || defined(_WIN64)

        auto devices = GetWindowsWebcamNames();

        if (devices.empty()) {
            std::cout << "No cameras found!" << std::endl;
            return false;
        }

        // List them for the user
        std::cout << "Available Devices:" << std::endl;
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << i << ": " << devices[i] << std::endl;
        }
        int count = devices.size();

        // Let's pick the first one found
        std::string selectedDevice = devices[0];
        std::cout << "\nOpening: " << selectedDevice << std::endl;
        auto media = VLC::Media(m_instance, "dshow://", VLC::Media::FromLocation);

        try {
            // Crucial: Format the option correctly for the dshow module
            // Syntax: :dshow-vdev="Device Name"
            std::string vdevOpt = ":dshow-vdev=" + selectedDevice;
            media.addOption(vdevOpt);

        }  catch(const std::exception& err) {
            std::cerr << "error selecting first source: " << err.what() << "\n";
        }

        // Safety check: ensure we have at least one device
        m_sourcesAvailable = (count > 0);
        if (count == 0) {
            std::cerr << "No devices found. Make sure a camera is connected or set CI=1 for mock mode." << std::endl;
            return false;
        }
        m_mediaPlayer = VLC::MediaPlayer(media);
        #else
        // Create a MediaDiscoverer for video devices
        m_discoverer = VLC::MediaDiscoverer(m_instance, m_cameraSource.c_str());

        if (!m_discoverer->start()) {
            std::cerr << "Failed to start discovery." << std::endl;
            return false;
        }

        auto mediaList = m_discoverer->mediaList();

        std::cout << "Searching for devices..." << std::endl;

        // Small delay to allow the OS to report devices
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Check for devices
        int count = mediaList->count();
        std::cout << "Found " << count << " devices:" << std::endl;
        for (int i = 0; i < count; ++i) {
            auto item = mediaList->itemAtIndex(i);
            std::cout << "[" << i << "] Name: " << item->meta(libvlc_meta_Title) << std::endl;
            std::cout << "    MRL:  " << item->mrl() << std::endl;
        }

        // Safety check: ensure we have at least one device
        m_sourcesAvailable = (count > 0);
        if (count == 0) {
            std::cerr << "No devices found. Make sure a camera is connected or set CI=1 for mock mode." << std::endl;
            return false;
        }

        auto item = mediaList->itemAtIndex(0);
        m_mediaPlayer = VLC::MediaPlayer(*item);
        #endif
    }

    // Set up VLC to render to our SFML window
    // On macOS, the window handle may not be immediately available, so use getValidNativeHandle()
    sf::WindowHandle hndl = getValidNativeHandle();
    if (hndl) {
        setupVLCRendering(hndl);
    } else {
        std::cerr << "Warning: Failed to get valid window handle for VLC rendering" << std::endl;
    }

    if (m_config.startPlaying) {
        m_mediaPlayer.play();
    }

    m_started = true;

    // Initialize YOLO detection after VLC is set up
    // This ensures the window handle is available for texture capture
    initializeDetection();

    return true;
}

void VideoWindow::stop() {
    if (m_started) {
        m_mediaPlayer.stop();

        if (m_discoverer.has_value()) {
            m_discoverer->stop();
        }
        m_started = false;
        m_isStream = false;  // Reset stream flag on stop
    }

    // Clear sources from IPC on stop
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.videoToOverlayPath = m_config.videoToOverlayPath;
    config.overlayToVideoPath = m_config.overlayToVideoPath;
    if (ipc.initialize(config)) {
        ipc.cleanup();
    }
}

} // namespace vlc
