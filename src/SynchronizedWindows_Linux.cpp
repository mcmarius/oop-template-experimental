#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

namespace syncwindows {

void raiseWindowLinux(sf::WindowHandle windowHandle) {
    Display* display = XOpenDisplay(nullptr);
    if (display) {
        Window wnd = static_cast<Window>(windowHandle);
        XRaiseWindow(display, wnd);
        XFlush(display);
        XCloseDisplay(display);
    }
}

void raiseTwoWindowsLinux(sf::WindowHandle transparentHandle, sf::WindowHandle videoHandle) {
    Display* display = XOpenDisplay(nullptr);
    if (display) {
        Window transparentWnd = static_cast<Window>(transparentHandle);
        Window videoWnd = static_cast<Window>(videoHandle);

        // Raise transparent window above video window
        XRaiseWindow(display, transparentWnd);
        XFlush(display);

        // Also raise video window to maintain proper order, then transparent again
        XRaiseWindow(display, videoWnd);
        XRaiseWindow(display, transparentWnd);
        XFlush(display);
        XCloseDisplay(display);
    }
}

} // namespace syncwindows