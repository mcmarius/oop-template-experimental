#include "VideoWindow.h"
#include <X11/Xlib.h>

namespace vlc {

void VideoWindow::initializePlatform() {
    // Initialize X11 threads for Linux
    XInitThreads();
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    m_mediaPlayer.setXwindow(hndl);
}

} // namespace vlc