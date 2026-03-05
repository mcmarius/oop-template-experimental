#ifndef TRANSPARENT_WINDOW_H
#define TRANSPARENT_WINDOW_H

#include <SFML/Graphics.hpp>
#include <string>
#include <cstdint>
#include <memory>
#include <vector>

// Include StreamingSource definition
#include "IPCController.h"

namespace transparent {

/// Manages a transparent overlay window with custom shape and alpha.
/// Works on Windows, Linux, and macOS.
class TransparentWindow {
public:
    /// Configuration for transparent window
    struct Config {
        sf::Vector2u size = {640, 480};      ///< Window size
        std::string imageFile = "assets/image.png";  ///< Path to image with transparency
        unsigned char alpha = 185;           ///< Window opacity (0-255)
        std::uint32_t style = sf::Style::None;  ///< Window style (std::uint32_t bitmask)
        std::string videoToOverlayPath = "assets/video_to_overlay.json";  ///< Video→Overlay data path
        std::string overlayToVideoPath = "assets/overlay_to_video.json";  ///< Overlay→Video data path
    };

    TransparentWindow();
    ~TransparentWindow();

    /// Initialize the transparent window with given config
    /// @return true on success, false on failure
    bool initialize(const Config& config);

    /// Apply transparency/alpha to the window
    /// @return true on success
    bool applyAlpha(unsigned char alpha);

    /// Apply custom shape based on image alpha channel
    /// @return true on success
    bool applyShape();

    /// Close the window
    void close();

    /// Check if window is open
    bool isOpen() const;

    /// Get the SFML window (for positioning/sizing)
    sf::RenderWindow& getWindow();
    const sf::RenderWindow& getWindow() const;

    /// Get native window handle (for OS-specific operations)
    sf::WindowHandle getNativeHandle() const;

    /// Process events
    /// @return true if window should remain open
    bool pollEvents();

    /// Draw the transparent overlay (call this each frame)
    void draw();

    /// Set position of the window
    void setPosition(sf::Vector2i position);

    /// Set size of the window
    void setSize(sf::Vector2u size);

    /// Get current window position
    sf::Vector2u getPosition() const;

    /// Get current window size
    sf::Vector2u getSize() const;

    /// Get the loaded image (for shape calculation reference)
    const sf::Image& getImage() const;

    /// Get the sprite for drawing
    const sf::Sprite& getSprite() const;

    /// Toggle the source picker visibility
    void toggleSourcePicker();

    /// Set streaming sources to display in picker
    void setStreamingSources(const std::vector<ipc::StreamingSource>& sources);

    /// Get the selected source from the picker (returns MRL)
    std::string getSelectedSource() const;

    /// Get the selected source name for display
    std::string getSelectedSourceName() const;

    /// Write selected source to IPC
    bool writeSelectedSourceToIPC() const;

    /// Check if no sources are available
    bool areSourcesAvailable() const;

    /// Check if source picker is open
    bool isSourcePickerOpen() const;

    /// Reset selected source
    void resetSelectedSource();

    /// Scale UI elements based on window size
    void scaleUIElements(sf::Vector2u originalSize, sf::Vector2u newSize);

    /// Update Sources button text to show selected source
    void updateSourcesButtonText();

private:
    Config m_config;
    sf::RenderWindow m_window;
    sf::Image m_image;
    sf::Texture m_texture;
    std::unique_ptr<sf::Sprite> m_sprite;

    sf::Font m_buttonFont;

    // Streaming source picker state
    std::vector<ipc::StreamingSource> m_sources;
    sf::RectangleShape m_sourcePickerBackground;
    std::vector<std::unique_ptr<sf::RectangleShape>> m_sourceButtons;
    std::vector<std::unique_ptr<sf::Text>> m_sourceButtonTexts;
    bool m_sourcePickerOpen = false;
    int m_selectedSourceIndex = -1;
    bool m_sourcesAvailable = true;

    // Toggle Sources button
    sf::RectangleShape m_sourcesTogglebutton;
    std::unique_ptr<sf::Text> m_sourcesToggleButtonText;

    // File picker button
    sf::RectangleShape m_filePickerButton;
    std::unique_ptr<sf::Text> m_filePickerButtonText;

    bool m_initialized = false;

    /// Platform-specific implementations (defined in platform-specific .cpp files)
    bool setShape();
    bool setTransparency(unsigned char alpha);
};

} // namespace transparent

#endif // TRANSPARENT_WINDOW_H