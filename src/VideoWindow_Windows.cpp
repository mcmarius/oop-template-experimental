#include "VideoWindow.h"

namespace vlc {

void VideoWindow::initializePlatform() {
    // Windows doesn't need special initialization
}

sf::WindowHandle VideoWindow::getValidNativeHandle() const {
    // On Windows, the window handle is immediately available after create()
    return m_window.getNativeHandle();
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    m_mediaPlayer.setHwnd(hndl);
}

} // namespace vlc
