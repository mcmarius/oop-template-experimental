#include "VideoWindow.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include "IPCController.h"

namespace vlc {

std::string VideoWindow::getSelectedSourceFromIPC() const {
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.videoToOverlayPath = m_config.videoToOverlayPath;
    config.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipc.initialize(config)) {
        return "";
    }

    return ipc.readSelectedSource();
}

std::string VideoWindow::detectCameraSource() {
    if (std::getenv("CI")) {
        // Always use mock video file (CI mode) to ensure consistent behavior
        // Real camera discovery is complex and varies by system; for demo purposes,
        // we use a sample video file which provides the same streaming functionality.
        const char* ci_test_video = std::getenv("CI_TEST_VIDEO_PATH");
        std::string video_path = ci_test_video ? ci_test_video : "assets/sample_video.webm";
        return "file://" + video_path;
    }

    // Normal device discovery for local development
#if defined(_WIN32) || defined(_WIN64)
    return "dshow";      // DirectShow for Windows
#elif defined(__APPLE__) || defined(__MACH__)
    return "avcapture";  // AVFoundation for macOS
#else
    return "udev";       // udev for Linux
#endif
}

std::vector<ipc::StreamingSource> VideoWindow::discoverSources() {
    std::vector<ipc::StreamingSource> sources;

    // First check if a source has been explicitly selected from IPC
    std::string selectedSource = getSelectedSourceFromIPC();
    if (!selectedSource.empty()) {
        // Return the selected source
        ipc::StreamingSource source;
        // Extract just the filename for display
        source.name = std::filesystem::path(selectedSource).filename().string();
        source.mrl = selectedSource;
        source.type = "file";
        sources.push_back(source);
        return sources;
    }

    // Always check for CI mode first
    const char* ci_test_video = std::getenv("CI_TEST_VIDEO_PATH");
    if (ci_test_video) {
        // CI mode - return a single source for the test video file
        std::string video_path = std::string("file://") + ci_test_video;
        ipc::StreamingSource source;
        source.name = "Test Video File";
        source.mrl = video_path;
        source.type = "file";
        sources.push_back(source);
        return sources;
    }

    std::string cameraSource = detectCameraSource();

    // Use MediaDiscoverer for real cameras, direct media for mock (file://)
    if (cameraSource.find("file://") == 0) {
        // Mock mode - return a single source for the video file
        ipc::StreamingSource source;
        source.name = std::filesystem::path(cameraSource).filename(); // strip file://  // "Sample Video File";
        source.mrl = cameraSource;
        source.type = "file";
        sources.push_back(source);
        return sources;
    }

    // Create a MediaDiscoverer for video devices
    VLC::MediaDiscoverer discoverer(m_instance, cameraSource.c_str());

    if (!discoverer.start()) {
        std::cerr << "Failed to start discovery." << std::endl;
        return sources;
    }

    auto mediaList = discoverer.mediaList();

    std::cout << "Searching for devices..." << std::endl;

    // Small delay to allow the OS to report devices
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Iterate and collect device info
    int count = mediaList->count();
    std::cout << "Found " << count << " devices:" << std::endl;

    for (int i = 0; i < count; ++i) {
        auto item = mediaList->itemAtIndex(i);
        ipc::StreamingSource source;
        source.name = item->meta(libvlc_meta_Title);
        source.mrl = item->mrl();
        source.type = cameraSource;
        sources.push_back(source);

        std::cout << "[" << i << "] Name: " << source.name << std::endl;
        std::cout << "    MRL:  " << source.mrl << std::endl;
    }

    discoverer.stop();

    m_sourcesAvailable = !sources.empty();
    return sources;
}

bool VideoWindow::writeSourcesToIPC(const std::vector<ipc::StreamingSource>& sources) {
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.videoToOverlayPath = m_config.videoToOverlayPath;
    config.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipc.initialize(config)) {
        std::cerr << "Failed to initialize IPC for writing sources" << std::endl;
        return false;
    }

    if (!ipc.writeStreamingSources(sources)) {
        std::cerr << "Failed to write sources to IPC" << std::endl;
        return false;
    }

    // Write no-sources-available flag
    ipc.writeNoSourcesAvailable(!m_sourcesAvailable);

    return true;
}

bool VideoWindow::setSourceFromIPC() {
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.videoToOverlayPath = m_config.videoToOverlayPath;
    config.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipc.initialize(config)) {
        std::cerr << "Failed to initialize IPC for reading selected source" << std::endl;
        return false;
    }

    std::string selectedSource = ipc.readSelectedSource();

    if (selectedSource.empty()) {
        // No source selected yet, use default
        return true;
    }

    // Only reload if the source has changed
    if (selectedSource == m_lastLoadedSource) {
        return true;  // Already loaded, nothing to do
    }

    std::cout << "Setting source from IPC: " << selectedSource << std::endl;

    // Stop current playback
    if (m_started) {
        m_mediaPlayer.stop();
    }

    // Create new media from selected source
    try {
        std::string path = selectedSource;
        // Strip file:// prefix if present (VLC::Media::FromPath expects a path, not a URL)
        if (path.find("file://") == 0) {
            path = path.substr(7);
        }
        auto media = VLC::Media(m_instance, path, VLC::Media::FromPath);
        // Loop the video for continuous playback
        media.addOption(":input-repeat=65535");
        m_mediaPlayer = VLC::MediaPlayer(media);

        // Set up VLC to render to our SFML window
        sf::WindowHandle hndl = m_window.getNativeHandle();
        setupVLCRendering(hndl);

        m_mediaPlayer.play();
        m_started = true;
        m_lastLoadedSource = selectedSource;

        // Discover and write sources to IPC to update overlay's source picker
        // Loop protection: only write if sources have actually changed
        std::vector<ipc::StreamingSource> sources = discoverSources();
        if (sources != m_lastWrittenSources) {
            if (writeSourcesToIPC(sources)) {
                m_lastWrittenSources = sources;
                std::cout << "Updated sources in IPC: " << sources.size() << " source(s)" << std::endl;
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set source: " << e.what() << std::endl;
        return false;
    }
}

bool VideoWindow::areSourcesAvailable() const {
    return m_sourcesAvailable;
}

} // namespace vlc