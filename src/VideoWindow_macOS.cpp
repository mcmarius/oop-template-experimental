#include "VideoWindow.h"
#include <chrono>
#include <thread>

namespace vlc {

void VideoWindow::initializePlatform() {
    // macOS doesn't need special initialization
}

sf::WindowHandle VideoWindow::getValidNativeHandle() const {
    // On macOS, the window handle may not be immediately available after create()
    // Retry with a 200ms timeout
    sf::WindowHandle hndl = nullptr;
    int maxRetries = 20;
    int retryDelayMs = 10;

    for (int i = 0; i < maxRetries; ++i) {
        hndl = m_window.getNativeHandle();
        if (hndl != nullptr) {
            return hndl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
    }

    // Final attempt
    return m_window.getNativeHandle();
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    // On macOS, use setNsobject for VLC rendering
    m_mediaPlayer.setNsobject(hndl);
}

} // namespace vlc