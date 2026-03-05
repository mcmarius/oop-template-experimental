#ifndef VIDEO_WINDOW_H
#define VIDEO_WINDOW_H

#include <SFML/Graphics.hpp>
#include <vlcpp/vlc.hpp>
#include <string>
#include <optional>
#include <cstdint>
#include <vector>

// Include IPCController for StreamingSource definition
#include "IPCController.h"

namespace vlc {

/// Manages a video streaming window using VLC media player.
/// Displays camera feed or video file in an SFML window.
class VideoWindow {
public:
    /// Configuration for video window
    struct Config {
        sf::Vector2u size = {1280, 720};      ///< Window size
        std::string title = "Video Stream";   ///< Window title
        std::uint32_t style = sf::Style::Default; ///< Window style (std::uint32_t bitmask)
        bool startPlaying = true;             ///< Start playback immediately
        std::string videoToOverlayPath = "assets/video_to_overlay.json";  ///< Video→Overlay data path
        std::string overlayToVideoPath = "assets/overlay_to_video.json";  ///< Overlay→Video data path
    };

    VideoWindow();
    ~VideoWindow();

    /// Initialize the video window with given config
    /// @return true on success, false on failure
    bool initialize(const Config& config);

    /// Start camera/video discovery and playback
    /// @return true if media source was found and started
    bool start();

    /// Stop the media playback and discovery
    void stop();

    /// Close the window
    void close();

    /// Check if window is open
    bool isOpen() const;

    /// Get the SFML window (for synchronization with overlay)
    sf::RenderWindow& getWindow();
    const sf::RenderWindow& getWindow() const;

    /// Get native window handle (for VLC integration)
    sf::WindowHandle getNativeHandle() const;

    /// Process events - call this in main loop
    /// @return true if window should remain open
    bool pollEvents();

    /// Set position of the window
    void setPosition(sf::Vector2i position);

    /// Set size of the window
    void setSize(sf::Vector2u size);

    /// Get current window position
    sf::Vector2u getPosition() const;

    /// Get current window size
    sf::Vector2u getSize() const;

    /// Discover and return available streaming sources
    /// @return vector of discovered sources
    std::vector<ipc::StreamingSource> discoverSources();

    /// Set the selected source from IPC and restart playback
    /// @param sourceMRL The MRL of the source to play, or empty to use default
    /// @return true if source was set and playback started
    bool setSourceFromIPC();

    /// Write discovered sources to IPC (with loop protection)
    /// @param sources Vector of sources to write
    /// @return true on success
    bool writeSourcesToIPC(const std::vector<ipc::StreamingSource>& sources);

    /// Check if no sources are available
    /// @return true if no sources were discovered
    bool areSourcesAvailable() const;

    /// Get the selected source from IPC (for source discovery)
    /// @return selected source MRL, or empty string if none selected
    std::string getSelectedSourceFromIPC() const;

private:
    Config m_config;
    sf::RenderWindow m_window;
    VLC::Instance m_instance;
    std::optional<VLC::MediaDiscoverer> m_discoverer;
    VLC::MediaPlayer m_mediaPlayer;
    std::string m_cameraSource;
    bool m_started = false;
    bool m_sourcesAvailable = true;
    std::string m_lastLoadedSource;  ///< Track the last source loaded from IPC to avoid reloading
    std::vector<ipc::StreamingSource> m_lastWrittenSources;  ///< Track last written sources to avoid IPC writes

    /// Detect camera source based on OS
    std::string detectCameraSource();

    /// Platform-specific VLC rendering setup
    void setupVLCRendering(sf::WindowHandle hndl);

    /// Platform-specific initialization
    void initializePlatform();
};

} // namespace vlc

#endif // VIDEO_WINDOW_H