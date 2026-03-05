#include "VideoWindow.h"

namespace vlc {

void VideoWindow::initializePlatform() {
    // macOS doesn't need special initialization
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    m_mediaPlayer.setXwindow(hndl);
}

} // namespace vlc