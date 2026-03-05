#ifndef SYNCHRONIZED_WINDOWS_H
#define SYNCHRONIZED_WINDOWS_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <cstdint>

// Forward declarations
namespace vlc { class VideoWindow; }
namespace transparent { class TransparentWindow; }

namespace syncwindows {

/// Manages two windows that stay synchronized in position and size.
/// The overlay window follows the base (video) window.
class SynchronizedWindows {
public:
    /// Configuration for synchronized windows
    struct Config {
        // Video window config
        sf::Vector2u videoSize = {1280, 720};
        std::string videoTitle = "Video Stream";
        std::uint32_t videoStyle = sf::Style::Default;
        
        // Overlay (transparent) window config
        sf::Vector2u overlaySize = {1280, 720};
        std::string overlayImageFile = "assets/image.png";
        unsigned char overlayAlpha = 185;
        std::uint32_t overlayStyle = sf::Style::None;
        
        // Callback when windows are resized (for custom handling)
        std::function<void(sf::Vector2u)> onResize = nullptr;
    };

    SynchronizedWindows();
    ~SynchronizedWindows();

    /// Initialize both windows with the given config
    /// @return true on success
    bool initialize(const Config& config);

    /// Start the video stream
    /// @return true if video started successfully
    bool startVideo();

    /// Run the main loop
    /// @return exit code (0 for success)
    int run();

    /// Signal shutdown
    void shutdown();

    /// Check if windows are running
    bool isRunning() const;

    /// Get the video window
    vlc::VideoWindow& getVideoWindow();
    const vlc::VideoWindow& getVideoWindow() const;

    /// Get the transparent overlay window
    transparent::TransparentWindow& getOverlayWindow();
    const transparent::TransparentWindow& getOverlayWindow() const;

private:
    Config m_config;
    vlc::VideoWindow* m_videoWindow = nullptr;
    transparent::TransparentWindow* m_overlayWindow = nullptr;
    bool m_running = false;
    bool m_videoStarted = false;

    /// Sync overlay window position/size with video window
    void syncOverlayToVideo();

    /// Handle resize events from video window
    void handleVideoResize(sf::Vector2u newSize);
};

} // namespace syncwindows

#endif // SYNCHRONIZED_WINDOWS_H
