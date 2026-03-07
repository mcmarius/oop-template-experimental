#include "VideoWindow.h"
#include <X11/Xlib.h>

namespace vlc {

void VideoWindow::initializePlatform() {
    // Initialize X11 threads for Linux
    XInitThreads();
}

sf::WindowHandle VideoWindow::getValidNativeHandle() const {
    // On Linux, the window handle is immediately available after create()
    return m_window.getNativeHandle();
}

void VideoWindow::setupVLCRendering(sf::WindowHandle hndl) {
    m_mediaPlayer.setXwindow(hndl);
}

} // namespace vlc