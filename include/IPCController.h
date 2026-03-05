#ifndef IPC_CONTROLLER_H
#define IPC_CONTROLLER_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <optional>

// Include nlohmann json header
#include "json.hpp"

namespace ipc {

/// Represents a discovered streaming source
struct StreamingSource {
    std::string name;   ///< Human-readable name of the device
    std::string mrl;    ///< Media Resource Locator (device path)
    std::string type;   ///< Source type (e.g., "camera", "dshow", "udev")
};

/// Compare two StreamingSource for equality (for loop protection)
inline bool operator==(const StreamingSource& a, const StreamingSource& b) {
    return a.name == b.name && a.mrl == b.mrl && a.type == b.type;
}

/// IPCController - Position and size synchronization between video and overlay processes
/// Uses JSON file-based approach for cross-platform compatibility
/// Video process writes window position/size and streaming sources to assets/video_to_overlay.json
/// Overlay process reads from that file and writes overlay position to assets/overlay_to_video.json
/// Video process reads overlay position from assets/overlay_to_video.json
class IPCController {
public:
    /// Configuration for IPC
    struct Config {
        std::string videoToOverlayPath = "assets/video_to_overlay.json";  ///< Video→Overlay data path
        std::string overlayToVideoPath = "assets/overlay_to_video.json";  ///< Overlay→Video data path
        int updateIntervalMs = 50;  ///< How often to update position (ms)
    };

    IPCController();
    ~IPCController();

    /// Initialize IPC with given config
    bool initialize(const Config& config);

    /// ========== Video → Overlay (video_to_overlay.json) ==========

    /// Write video window position and size to shared file
    /// Call this from video process
    bool writeVideoPosition(sf::Vector2i position, sf::Vector2u size) const;

    /// Read video window position and size from shared file
    /// Call this from overlay process
    /// Returns true if position was updated, false if file doesn't exist or is empty
    bool readVideoPosition(sf::Vector2i& position, sf::Vector2u& size) const;

    /// ========== Overlay → Video (overlay_to_video.json) ==========

    /// Write overlay window position and size to shared file
    /// Call this from overlay process
    bool writeOverlayPosition(sf::Vector2i position, sf::Vector2u size) const;

    /// Read overlay window position and size from shared file
    /// Call this from video process
    /// Returns true if position was updated, false if file doesn't exist or is empty
    bool readOverlayPosition(sf::Vector2i& position, sf::Vector2u& size) const;

    /// Write discovered streaming sources to shared file
    /// Call this from video process
    bool writeStreamingSources(const std::vector<StreamingSource>& sources) const;

    /// Read discovered streaming sources from shared file
    /// Call this from overlay process
    /// Returns sources if available, empty vector if none found
    std::vector<StreamingSource> readStreamingSources() const;

    /// ========== Video → Overlay (video_to_overlay.json) ==========

    /// Write no-sources-available flag to shared file
    /// Call this from video process
    bool writeNoSourcesAvailable(bool noSources) const;

    /// ========== Overlay → Video (overlay_to_video.json) ==========

    /// Write selected streaming source to shared file
    /// Call this from overlay process
    bool writeSelectedSource(const std::string& mrl) const;

    /// Read selected streaming source from shared file
    /// Call this from video process
    /// Returns selected MRL, empty string if none selected
    std::string readSelectedSource() const;

    /// Check if video→overlay file exists and has valid video data
    bool isVideoFileReady() const;

    /// Get the video→overlay file path
    const std::string& getVideoToOverlayPath() const;

    /// Get the overlay→video file path
    const std::string& getOverlayToVideoPath() const;

    /// Cleanup: clear this process's data from both files
    void cleanup() const;

private:
    Config m_config;
    bool m_initialized = false;

    /// Helper to load JSON from file (for internal use only)
    nlohmann::json loadJsonFile() const;

    /// Helper to save JSON to file (for internal use only)
    bool saveJsonFile(const nlohmann::json& j) const;

    /// Helper to load JSON from specified file path
    nlohmann::json loadJsonFile(const std::string& filePath) const;

    /// Helper to save JSON to specified file path
    bool saveJsonFile(const std::string& filePath, const nlohmann::json& j) const;
};

} // namespace ipc

#endif // IPC_CONTROLLER_H