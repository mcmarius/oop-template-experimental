#include "TransparentWindow.h"
#include <windows.h>

namespace transparent {

bool TransparentWindow::setShape() {
    HWND hWnd = reinterpret_cast<HWND>(m_window.getNativeHandle());
    const std::uint8_t* pixelData = m_image.getPixelsPtr();

    // Create a region with the size of the entire window
    HRGN hRegion = CreateRectRgn(0, 0, m_image.getSize().x, m_image.getSize().y);

    // Loop over pixels and remove transparent ones from region
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
                HRGN hRegionPixel = CreateRectRgn(rectLeft, y, x, y + 1);
                CombineRgn(hRegion, hRegion, hRegionPixel, RGN_XOR);
                DeleteObject(hRegionPixel);
                transparentPixelFound = false;
            }
        }

        if (transparentPixelFound) {
            HRGN hRegionPixel = CreateRectRgn(rectLeft, y, m_image.getSize().x, y + 1);
            CombineRgn(hRegion, hRegion, hRegionPixel, RGN_XOR);
            DeleteObject(hRegionPixel);
            transparentPixelFound = false;
        }
    }

    SetWindowRgn(hWnd, hRegion, true);
    DeleteObject(hRegion);
    return true;
}

bool TransparentWindow::setTransparency(unsigned char alpha) {
    HWND hWnd = reinterpret_cast<HWND>(m_window.getNativeHandle());
    SetWindowLong(hWnd, GWL_EXSTYLE,
        GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
    return true;
}

} // namespace transparent