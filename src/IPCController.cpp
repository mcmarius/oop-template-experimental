#include "IPCController.h"
#include <fstream>
#include <iostream>
#include <json.hpp>

namespace ipc {

using json = nlohmann::json;

IPCController::IPCController() = default;

IPCController::~IPCController() = default;

bool IPCController::initialize(const Config& config) {
    m_config = config;
    m_initialized = true;
    return true;
}

// ==================== JSON File Helpers ====================

json IPCController::loadJsonFile(const std::string& filePath) const {
    json j;
    std::ifstream file(filePath);
    if (file.is_open()) {
        // Check if file is empty
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size > 0) {
            try {
                file >> j;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to parse JSON from " << filePath << ": " << e.what() << std::endl;
                j = json::object(); // Return empty object on parse error
            }
        } else {
            j = json::object(); // Return empty object for empty files
        }
        file.close();
    } else {
        j = json::object(); // Return empty object if file doesn't exist
    }
    return j;
}

bool IPCController::saveJsonFile(const std::string& filePath, const json& j) const {
    try {
        std::ofstream fileOut(filePath);
        if (!fileOut.is_open()) {
            return false;
        }
        fileOut << j.dump(4) << std::endl;
        fileOut.flush();
        return true;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// ==================== Video → Overlay Helpers (video_to_overlay.json) ====================

bool IPCController::writeVideoPosition(sf::Vector2i position, sf::Vector2u size) const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.videoToOverlayPath);

        // Update video-related fields
        j["video_position"]["x"] = position.x;
        j["video_position"]["y"] = position.y;
        j["video_size"]["width"] = size.x;
        j["video_size"]["height"] = size.y;

        return saveJsonFile(m_config.videoToOverlayPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool IPCController::readVideoPosition(sf::Vector2i& position, sf::Vector2u& size) const {
    if (!m_initialized) {
        return false;
    }

    try {
        std::ifstream file(m_config.videoToOverlayPath);
        if (!file.is_open()) {
            // File doesn't exist yet - video process hasn't started
            return false;
        }

        // Check if file is empty first
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            file.close();
            return false;
        }
        file.seekg(0, std::ios::beg);

        json j;
        file >> j;

        // Check if video data exists
        if (j.contains("video_position") && j.contains("video_size")) {
            position = sf::Vector2i(j["video_position"]["x"].get<int>(),
                                    j["video_position"]["y"].get<int>());
            size = sf::Vector2u(j["video_size"]["width"].get<int>(),
                                j["video_size"]["height"].get<int>());
            return true;
        }

        // Video data not present yet
        return false;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// ==================== Overlay → Video Helpers (overlay_to_video.json) ====================

bool IPCController::writeOverlayPosition(sf::Vector2i position, sf::Vector2u size) const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);

        // Update overlay-related fields
        j["overlay_position"]["x"] = position.x;
        j["overlay_position"]["y"] = position.y;
        j["overlay_size"]["width"] = size.x;
        j["overlay_size"]["height"] = size.y;

        return saveJsonFile(m_config.overlayToVideoPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool IPCController::readOverlayPosition(sf::Vector2i& position, sf::Vector2u& size) const {
    if (!m_initialized) {
        return false;
    }

    try {
        std::ifstream file(m_config.overlayToVideoPath);
        if (!file.is_open()) {
            // File doesn't exist yet - overlay process hasn't started
            return false;
        }

        // Check if file is empty first
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            file.close();
            return false;
        }
        file.seekg(0, std::ios::beg);

        json j;
        file >> j;

        // Check if overlay data exists
        if (j.contains("overlay_position") && j.contains("overlay_size")) {
            position = sf::Vector2i(j["overlay_position"]["x"].get<int>(),
                                    j["overlay_position"]["y"].get<int>());
            size = sf::Vector2u(j["overlay_size"]["width"].get<int>(),
                                j["overlay_size"]["height"].get<int>());
            return true;
        }

        // Overlay data not present yet
        return false;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// ==================== Streaming Source Helpers (video_to_overlay.json) ====================

bool IPCController::writeStreamingSources(const std::vector<StreamingSource>& sources) const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.videoToOverlayPath);

        // Clear existing sources
        j["streaming_sources"] = json::array();

        // Add each source
        for (const auto& source : sources) {
            json sourceObj;
            sourceObj["name"] = source.name;
            sourceObj["mrl"] = source.mrl;
            sourceObj["type"] = source.type;
            j["streaming_sources"].push_back(sourceObj);
        }

        return saveJsonFile(m_config.videoToOverlayPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

std::vector<StreamingSource> IPCController::readStreamingSources() const {
    if (!m_initialized) {
        return {};
    }

    try {
        json j = loadJsonFile(m_config.videoToOverlayPath);

        std::vector<StreamingSource> sources;

        // Check if streaming_sources exists and is an array
        if (j.contains("streaming_sources") && j["streaming_sources"].is_array()) {
            for (const auto& sourceObj : j["streaming_sources"]) {
                if (sourceObj.contains("name") && sourceObj.contains("mrl")) {
                    StreamingSource source;
                    source.name = sourceObj["name"].get<std::string>();
                    source.mrl = sourceObj["mrl"].get<std::string>();
                    if (sourceObj.contains("type")) {
                        source.type = sourceObj["type"].get<std::string>();
                    }
                    sources.push_back(source);
                }
            }
        }

        return sources;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return {};
    }
}

bool IPCController::writeNoSourcesAvailable(bool noSources) const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.videoToOverlayPath);
        j["no_sources_available"] = noSources;
        return saveJsonFile(m_config.videoToOverlayPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// ==================== Selected Source Helpers (overlay_to_video.json) ====================

bool IPCController::writeSelectedSource(const std::string& mrl) const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);
        j["selected_source"] = mrl;
        return saveJsonFile(m_config.overlayToVideoPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

std::string IPCController::readSelectedSource() const {
    if (!m_initialized) {
        return "";
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);

        if (j.contains("selected_source")) {
            return j["selected_source"].get<std::string>();
        }
        return "";
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return "";
    }
}

bool IPCController::writePlayPauseCommand() const {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);
        j["play_pause_command"] = true;
        return saveJsonFile(m_config.overlayToVideoPath, j);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

bool IPCController::readPlayPauseCommand() {
    if (!m_initialized) {
        return false;
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);

        if (j.contains("play_pause_command") && j["play_pause_command"].is_boolean()) {
            // Clear the command
            j.erase("play_pause_command");
            saveJsonFile(m_config.overlayToVideoPath, j);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

// ==================== Active Window Tracking (both JSON files) ====================

bool IPCController::writeActiveWindow(const std::string& windowName) const {
    if (!m_initialized) {
        return false;
    }

    try {
        // Write to overlay_to_video.json
        {
            json j = loadJsonFile(m_config.overlayToVideoPath);
            j["active_window"] = windowName;
            saveJsonFile(m_config.overlayToVideoPath, j);
        }
        // Write to video_to_overlay.json
        {
            json j = loadJsonFile(m_config.videoToOverlayPath);
            j["active_window"] = windowName;
            saveJsonFile(m_config.videoToOverlayPath, j);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

std::string IPCController::readActiveWindow() const {
    if (!m_initialized) {
        return "";
    }

    try {
        json j = loadJsonFile(m_config.overlayToVideoPath);

        if (j.contains("active_window") && j["active_window"].is_string()) {
            return j["active_window"].get<std::string>();
        }
        return "";
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return "";
    }
}

bool IPCController::isVideoFileReady() const {
    if (!m_initialized) {
        return false;
    }

    try {
        std::ifstream file(m_config.videoToOverlayPath);
        if (!file.is_open()) {
            return false;
        }

        // Check if file is empty first
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            file.close();
            return false;
        }
        file.seekg(0, std::ios::beg);

        json j;
        file >> j;

        // Check if video data exists
        return j.contains("video_position") && j.contains("video_size");
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

const std::string& IPCController::getVideoToOverlayPath() const {
    return m_config.videoToOverlayPath;
}

const std::string& IPCController::getOverlayToVideoPath() const {
    return m_config.overlayToVideoPath;
}

void IPCController::cleanup() const {
    if (m_initialized) {
        try {
            // Clean up video→overlay file - write sensible defaults
            {
                json j;
                j["video_position"]["x"] = 0;
                j["video_position"]["y"] = 0;
                j["video_size"]["width"] = 1280;
                j["video_size"]["height"] = 720;
                j["streaming_sources"] = json::array();
                j["no_sources_available"] = false;
                j["active_window"] = "";
                std::ofstream fileOut(m_config.videoToOverlayPath);
                if (fileOut.is_open()) {
                    fileOut << j.dump(4) << std::endl;
                }
            }

            // Clean up overlay→video file - write sensible defaults
            {
                json j;
                j["overlay_position"]["x"] = 0;
                j["overlay_position"]["y"] = 0;
                j["overlay_size"]["width"] = 1280;
                j["overlay_size"]["height"] = 720;
                j["selected_source"] = "";
                j["play_pause_command"] = false;
                j["active_window"] = "";
                std::ofstream fileOut(m_config.overlayToVideoPath);
                if (fileOut.is_open()) {
                    fileOut << j.dump(4) << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            // Silent cleanup - no errors on cleanup
        }
    }
}

} // namespace ipc
