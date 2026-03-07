#include "TransparentWindow.h"
#include <Cocoa/Cocoa.h>

// Category to make NSOpenGLView non-opaque
// This must be in global scope, not inside a namespace
@implementation NSOpenGLView (Opaque)
- (BOOL)isOpaque {
    return NO;
}
@end

namespace transparent {

bool TransparentWindow::setShape() {
    NSWindow* wnd = reinterpret_cast<NSWindow*>(m_window.getNativeHandle());

    // Make window non-opaque and transparent
    // Use the non-deprecated API constant
    GLint opaque = 0;
    [[[wnd contentView] openGLContext] setValues:&opaque forParameter:NSOpenGLContextParameterSurfaceOpacity];
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