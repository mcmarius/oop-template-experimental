#include <Cocoa/Cocoa.h>
#include <vector>
#include <SFML/Window/WindowHandle.hpp>

/// Set window group for macOS using NSWindow collection behavior
/// Both windows in a group should use the same group leader handle
/// This enables Cmd+` switching between grouped windows
/// Note: macOS groups windows by application by default
/// For custom grouping, windows need to be in the same process
void setWindowGroupPlatform(sf::WindowHandle windowHandle, sf::WindowHandle /*groupLeader*/) {
    NSWindow* window = reinterpret_cast<NSWindow*>(windowHandle);

    // Set collection behavior to participate in window cycling
    // NSWindowCollectionBehaviorParticipatesInCycle enables Cmd+` switching
    [window setCollectionBehavior:NSWindowCollectionBehaviorParticipatesInCycle];

    // Note: macOS automatically groups windows from the same application
    // The group leader concept is handled internally by AppKit
    // For cross-window switching, all windows should be in the same process
    // and have the collection behavior set appropriately
}

/// Cycle focus to the next window in the group on macOS
/// Not implemented yet - placeholder for future implementation
bool cycleWindowFocusPlatform(sf::WindowHandle /*currentWindow*/) {
    // TODO: Implement window cycling on macOS
    // For now, return true to indicate success without action
    return true;
}

/// Get all windows in the same application (macOS groups by app)
std::vector<NSWindow*> getWindowGroupPlatform(sf::WindowHandle /*windowHandle*/) {
    std::vector<NSWindow*> windows;

    // Get all windows from the current application
    NSArray* windowsArray = [[NSApplication sharedApplication] orderedWindows];

    for (NSWindow* window in windowsArray) {
        [window retain];
        windows.push_back(window);
    }

    return windows;
}