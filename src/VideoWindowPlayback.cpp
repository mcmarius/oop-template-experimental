#include "VideoWindow.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "IPCController.h"

namespace vlc {

bool VideoWindow::start() {
    m_cameraSource = detectCameraSource();

    // Use MediaDiscoverer for real cameras, direct media for mock (file://)
    if (m_cameraSource.find("file://") == 0) {
        // Create media directly from video file (mock camera for CI)
        std::cout << "Using mock camera (video file) for CI testing" << std::endl;

        std::string path = m_cameraSource.substr(7);
        auto media = VLC::Media(m_instance, path, VLC::Media::FromPath);
        // Loop the video for continuous playback
        media.addOption(":input-repeat=65535");
        m_mediaPlayer = VLC::MediaPlayer(media);

        m_sourcesAvailable = true;
    } else {
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
    }

    // Set up VLC to render to our SFML window
    sf::WindowHandle hndl = m_window.getNativeHandle();
    setupVLCRendering(hndl);

    if (m_config.startPlaying) {
        m_mediaPlayer.play();
    }

    m_started = true;
    return true;
}

void VideoWindow::stop() {
    if (m_started) {
        m_mediaPlayer.stop();

        if (m_discoverer.has_value()) {
            m_discoverer->stop();
        }
        m_started = false;
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