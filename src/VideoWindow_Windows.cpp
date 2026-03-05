#include "VideoWindow.h"
#include <X11/Xlib.h>

namespace vlc {

void VideoWindow::initializePlatform() {
    // Windows doesn't need special initialization
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    m_mediaPlayer.setHwnd(hndl);
}

} // namespace vlc