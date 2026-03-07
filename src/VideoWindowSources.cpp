#include "VideoWindow.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <queue>
#include "IPCController.h"

// Windows only
std::vector<std::string> GetWindowsWebcamNames();

namespace vlc {

std::string VideoWindow::getSelectedSourceFromIPC() const {
    ipc::IPCController ipcGet;
    ipc::IPCController::Config configGet;
    configGet.videoToOverlayPath = m_config.videoToOverlayPath;
    configGet.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipcGet.initialize(configGet)) {
        return "";
    }

    return ipcGet.readSelectedSource();
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
        source.name = std::filesystem::path(cameraSource).filename().string(); // strip file://  // "Sample Video File";
        source.mrl = cameraSource;
        source.type = "file";
        sources.push_back(source);
        return sources;
    }

#if defined(_WIN32) || defined(_WIN64)
        auto devices = GetWindowsWebcamNames();

        if (devices.empty()) {
            std::cout << "No cameras found!" << std::endl;
            return sources;
        }

        // List them for the user
        std::cout << "Available Devices:" << std::endl;
        for (size_t i = 0; i < devices.size(); ++i) {
            std::cout << i << ": " << devices[i] << std::endl;
            try {
                // Crucial: Format the option correctly for the dshow module
                // Syntax: :dshow-vdev="Device Name"
                std::string vdevOpt = ":dshow-vdev=" + devices[i];
                ipc::StreamingSource source;
                source.name = devices[i];
                source.mrl = "dshow://" + vdevOpt;
                source.type = cameraSource;
                sources.push_back(source);
            } catch(const std::exception& err) {
                std::cerr << "error adding source " << i << ": " << err.what() << "\n";
            }
        }

#else
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
#endif

    m_sourcesAvailable = !sources.empty();
    return sources;
}

bool VideoWindow::writeSourcesToIPC(const std::vector<ipc::StreamingSource>& sources) {
    ipc::IPCController ipcWrite;
    ipc::IPCController::Config configWrite;
    configWrite.videoToOverlayPath = m_config.videoToOverlayPath;
    configWrite.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipcWrite.initialize(configWrite)) {
        std::cerr << "Failed to initialize IPC for writing sources" << std::endl;
        return false;
    }

    if (!ipcWrite.writeStreamingSources(sources)) {
        std::cerr << "Failed to write sources to IPC" << std::endl;
        return false;
    }

    // Write no-sources-available flag
    ipcWrite.writeNoSourcesAvailable(!m_sourcesAvailable);

    return true;
}

bool VideoWindow::setSourceFromIPC() {
    ipc::IPCController ipcRead;
    ipc::IPCController::Config configRead;
    configRead.videoToOverlayPath = m_config.videoToOverlayPath;
    configRead.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipcRead.initialize(configRead)) {
        std::cerr << "Failed to initialize IPC for reading selected source" << std::endl;
        return false;
    }

    std::string selectedSource = ipcRead.readSelectedSource();

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
        std::string sourcePath = selectedSource;
        
        // Determine if the source is a file path or a stream URL
        // Stream URLs have schemes like v4l2://, dshow://, avcapture://, rtsp://, etc.
        // File paths either have file:// or are plain paths
        bool isStreamUrl = false;
        
        // Check for common stream schemes
        if (sourcePath.find("://") != std::string::npos) {
            // It has a scheme - could be file://, v4l2://, dshow://, etc.
            // Check if it's NOT a file:// URL
            if (sourcePath.find("file://") != 0) {
                isStreamUrl = true;
            } else {
                // It's a file:// URL - strip the prefix for VLC::Media::FromPath
                sourcePath = sourcePath.substr(7);
            }
        }
        // If no :// found, treat as plain file path
        
        auto media = VLC::Media(m_instance, sourcePath, isStreamUrl ? VLC::Media::FromLocation : VLC::Media::FromPath);
        // Loop the video for continuous playback (only needed for file sources)
        if (!isStreamUrl) {
            media.addOption(":input-repeat=65535");
        }
        m_mediaPlayer = VLC::MediaPlayer(media);

        // Track if this is a live stream (for play/pause behavior)
        m_isStream = isStreamUrl;

        // Set up VLC to render to our SFML window
        // On macOS, the window handle may not be immediately available, so use getValidNativeHandle()
        sf::WindowHandle hndl = getValidNativeHandle();
        if (hndl) {
            setupVLCRendering(hndl);
        } else {
            std::cerr << "Warning: Failed to get valid window handle for VLC rendering" << std::endl;
        }

        m_mediaPlayer.play();
        m_started = true;
        m_lastLoadedSource = selectedSource;

        // Preserve discovered sources AND add the selected file
        // Read existing sources from IPC, append selected file if not already present
        // Limit to last 2 file sources maximum to prevent pile-up
        ipc::IPCController ipcWrite;
        ipc::IPCController::Config configWrite;
        configWrite.videoToOverlayPath = m_config.videoToOverlayPath;
        configWrite.overlayToVideoPath = m_config.overlayToVideoPath;

        if (ipcWrite.initialize(configWrite)) {
            std::vector<ipc::StreamingSource> sources = ipcWrite.readStreamingSources();
            
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
                if (ipcWrite.writeStreamingSources(sources)) {
                    m_lastWrittenSources = sources;
                    std::cout << "Updated sources in IPC: " << sources.size() << " source(s)" << std::endl;
                }
            }
        } else {
            // Fallback: just discover and write sources (original behavior if IPC fails)
            std::vector<ipc::StreamingSource> sources = discoverSources();
            if (sources != m_lastWrittenSources) {
                if (ipcWrite.writeStreamingSources(sources)) {
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
