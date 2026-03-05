#include "TransparentWindow.h"
#include <Cocoa/Cocoa.h>

namespace transparent {

// Category to make NSOpenGLView non-opaque
@implementation NSOpenGLView (Opaque)
- (BOOL)isOpaque {
    return NO;
}
@end

bool TransparentWindow::setShape() {
    NSWindow* wnd = reinterpret_cast<NSWindow*>(m_window.getNativeHandle());

    // Make window non-opaque and transparent
    GLint opaque = 0;
    [[[wnd contentView] openGLContext] setValues:&opaque forParameter:NSOpenGLCPSurfaceOpacity];
    [wnd setBackgroundColor:[NSColor clearColor]];
    [wnd setOpaque:NO];

    return true;
}

bool TransparentWindow::setTransparency(unsigned char alpha) {
    NSWindow* wnd = reinterpret_cast<NSWindow*>(m_window.getNativeHandle());

    CGFloat opacity = alpha / 255.0f;
    [wnd setAlphaValue:opacity];
    [wnd setOpaque:NO];

    return true;
}

} // namespace transparent