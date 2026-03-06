#include "VideoWindow.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <queue>
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
        
        // Determine if the source is a file path or a stream URL
        // Stream URLs have schemes like v4l2://, dshow://, avcapture://, rtsp://, etc.
        // File paths either have file:// or are plain paths
        bool isStreamUrl = false;
        
        // Check for common stream schemes
        if (path.find("://") != std::string::npos) {
            // It has a scheme - could be file://, v4l2://, dshow://, etc.
            // Check if it's NOT a file:// URL
            if (path.find("file://") != 0) {
                isStreamUrl = true;
            } else {
                // It's a file:// URL - strip the prefix for VLC::Media::FromPath
                path = path.substr(7);
            }
        }
        // If no :// found, treat as plain file path
        
        auto media = VLC::Media(m_instance, path, isStreamUrl ? VLC::Media::FromLocation : VLC::Media::FromPath);
        // Loop the video for continuous playback (only needed for file sources)
        if (!isStreamUrl) {
            media.addOption(":input-repeat=65535");
        }
        m_mediaPlayer = VLC::MediaPlayer(media);

        // Track if this is a live stream (for play/pause behavior)
        m_isStream = isStreamUrl;

        // Set up VLC to render to our SFML window
        sf::WindowHandle hndl = m_window.getNativeHandle();
        setupVLCRendering(hndl);

        m_mediaPlayer.play();
        m_started = true;
        m_lastLoadedSource = selectedSource;

        // Preserve discovered sources AND add the selected file
        // Read existing sources from IPC, append selected file if not already present
        // Limit to last 2 file sources maximum to prevent pile-up
        ipc::IPCController ipc;
        ipc::IPCController::Config config;
        config.videoToOverlayPath = m_config.videoToOverlayPath;
        config.overlayToVideoPath = m_config.overlayToVideoPath;

        if (ipc.initialize(config)) {
            std::vector<ipc::StreamingSource> sources = ipc.readStreamingSources();
            
            // Check if selected source is already in sources list
            bool found = false;
            for (const auto& src : sources) {
                if (src.mrl == selectedSource) {
                    found = true;
                    break;
                }
            }
            
            // If not found, add it as a new source
            if (!found) {
                ipc::StreamingSource newSource;
                newSource.mrl = selectedSource;
                // Extract filename for display name
                std::string path = selectedSource;
                if (path.find("file://") == 0) {
                    path = path.substr(7);
                }
                newSource.name = std::filesystem::path(path).filename().string();
                newSource.type = "file";

                // Filter out old file sources using a queue (keep last 2)
                std::vector<ipc::StreamingSource> nonFileSources;
                std::queue<ipc::StreamingSource> fileQueue;

                for (const auto& src : sources) {
                    if (src.type == "file") {
                        fileQueue.push(src);
                    } else {
                        nonFileSources.push_back(src);
                    }
                }

                // Add new file to queue
                fileQueue.push(newSource);

                // Keep only last 2 file sources by removing from front
                while (fileQueue.size() > 2) {
                    fileQueue.pop();
                }

                // Convert queue to vector
                std::vector<ipc::StreamingSource> fileSources;
                while (!fileQueue.empty()) {
                    fileSources.push_back(fileQueue.front());
                    fileQueue.pop();
                }

                // Combine: non-file sources first, then file sources
                sources = nonFileSources;
                sources.insert(sources.end(), fileSources.begin(), fileSources.end());

                std::cout << "Added file source to sources list: " << newSource.name << std::endl;
                std::cout << "Total sources: " << sources.size() << " (non-file: " << nonFileSources.size() << ", file: " << fileSources.size() << ")" << std::endl;
            }
            
            // Write updated sources back to IPC
            if (sources != m_lastWrittenSources) {
                if (writeSourcesToIPC(sources)) {
                    m_lastWrittenSources = sources;
                    std::cout << "Updated sources in IPC: " << sources.size() << " source(s)" << std::endl;
                }
            }
        } else {
            // Fallback: just discover and write sources (original behavior if IPC fails)
            std::vector<ipc::StreamingSource> sources = discoverSources();
            if (sources != m_lastWrittenSources) {
                if (writeSourcesToIPC(sources)) {
                    m_lastWrittenSources = sources;
                    std::cout << "Updated sources in IPC: " << sources.size() << " source(s)" << std::endl;
                }
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