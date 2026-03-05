#include "TransparentWindow.h"
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/Xlib.h>

namespace transparent {

bool TransparentWindow::setShape() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return false;
    }

    // Check for XShape extension
    int event_base, error_base;
    if (!XShapeQueryExtension(display, &event_base, &error_base)) {
        XCloseDisplay(display);
        return false;
    }

    Window wnd = static_cast<Window>(m_window.getNativeHandle());
    const std::uint8_t* pixelData = m_image.getPixelsPtr();

    // Create a black and white pixmap
    Pixmap pixmap = XCreatePixmap(display, wnd, m_image.getSize().x, m_image.getSize().y, 1);
    GC gc = XCreateGC(display, pixmap, 0, nullptr);

    // Make entire pixmap white (visible)
    XSetForeground(display, gc, 1);
    XFillRectangle(display, pixmap, gc, 0, 0, m_image.getSize().x, m_image.getSize().y);

    // Set transparent pixels to black (clipped)
    XSetForeground(display, gc, 0);
    bool transparentPixelFound = false;
    unsigned int rectLeft = 0;
    for (unsigned int y = 0; y < m_image.getSize().y; y++) {
        for (unsigned int x = 0; x < m_image.getSize().x; x++) {
            const bool isTransparentPixel =
                (pixelData[y * m_image.getSize().x * 4 + x * 4 + 3] == 0);
            if (isTransparentPixel && !transparentPixelFound) {
                transparentPixelFound = true;
                rectLeft = x;
            } else if (!isTransparentPixel && transparentPixelFound) {
                XFillRectangle(display, pixmap, gc, rectLeft, y, x - rectLeft, 1);
                transparentPixelFound = false;
            }
        }

        if (transparentPixelFound) {
            XFillRectangle(display, pixmap, gc, rectLeft, y,
                m_image.getSize().x - rectLeft, 1);
            transparentPixelFound = false;
        }
    }

    // Apply shape mask
    XShapeCombineMask(display, wnd, ShapeBounding, 0, 0, pixmap, ShapeSet);

    // Cleanup
    XFreeGC(display, gc);
    XFreePixmap(display, pixmap);
    XFlush(display);
    XCloseDisplay(display);
    return true;
}

bool TransparentWindow::setTransparency(unsigned char alpha) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return false;
    }

    Window wnd = static_cast<Window>(m_window.getNativeHandle());
    unsigned long opacity = (0xffffffff / 0xff) * alpha;
    Atom property = XInternAtom(display, "_NET_WM_WINDOW_OPACITY", false);

    if (property != None) {
        XChangeProperty(display, wnd, property, XA_CARDINAL, 32,
            PropModeReplace, (unsigned char*)&opacity, 1);
        XFlush(display);
        XCloseDisplay(display);
        return true;
    } else {
        XCloseDisplay(display);
        return false;
    }
}

} // namespace transparent