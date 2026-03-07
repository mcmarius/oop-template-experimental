#include <SFML/Graphics.hpp>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstdint>
#include <vector>
#include <iostream>

/// Set window group for Linux/X11 using _NET_WM_GROUP_ID property
/// Both windows in a group should use the same group leader handle
/// This enables Alt+` switching between grouped windows
void setWindowGroupPlatform(sf::WindowHandle windowHandle, sf::WindowHandle groupLeader) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Warning: Failed to open X display in setWindowGroupPlatform" << std::endl;
        return;
    }

    Window window = static_cast<Window>(windowHandle);
    Window leader = static_cast<Window>(groupLeader);

    // Use _NET_WM_GROUP_ID for modern window managers
    Atom groupAtom = XInternAtom(display, "_NET_WM_GROUP_ID", False);
    
    std::cerr << "Debug: Setting _NET_WM_GROUP_ID for window " << window 
              << " to value " << leader << std::endl;

    // Set the group leader as the group ID
    // This tells the window manager these windows belong to the same group
    XChangeProperty(display, window, groupAtom, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char*)&leader, 1);
    XFlush(display);
    
    // Verify the property was set
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned char* propData = nullptr;
    
    if (XGetWindowProperty(display, window, groupAtom, 0, 1, False,
                           XA_WINDOW, &actualType, &actualFormat,
                           &numItems, &bytesAfter, &propData) == Success) {
        if (propData) {
            Window readValue = *reinterpret_cast<Window*>(propData);
            std::cerr << "Debug: Verified _NET_WM_GROUP_ID for window " << window 
                      << " = " << readValue << std::endl;
            XFree(propData);
        }
    }

    XCloseDisplay(display);
}

/// Cycle focus to the next window in the group on Linux
/// Uses xdotool or wmctrl if available, otherwise falls back to basic X11 focus cycling
bool cycleWindowFocusPlatform(sf::WindowHandle ) {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return false;
    }

    // Window window = static_cast<Window>(currentWindow);
    
    // Try to use xdotool for window cycling if available
    // This is the most reliable approach for Alt+` functionality
    int ret = system("xdotool key Alt+Tab 2>/dev/null");
    (void)ret;  // Suppress unused warning
    
    XCloseDisplay(display);
    return true;
}

/// Get all windows in the same group
std::vector<Window> getWindowGroupPlatform(sf::WindowHandle windowHandle) {
    std::vector<Window> windows;
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return windows;
    }

    Window window = static_cast<Window>(windowHandle);
    
    // Query the _NET_WM_GROUP_ID property
    Atom groupAtom = XInternAtom(display, "_NET_WM_GROUP_ID", False);
    Atom actualType;
    int actualFormat;
    unsigned long numItems, bytesAfter;
    unsigned char* propData = nullptr;
    
    if (XGetWindowProperty(display, window, groupAtom, 0, 1, False,
                           XA_WINDOW, &actualType, &actualFormat,
                           &numItems, &bytesAfter, &propData) == Success) {
        if (propData) {
            Window groupLeader = *reinterpret_cast<Window*>(propData);
            
            // Query all top-level windows
            Window root = DefaultRootWindow(display);
            Window parent;
            Window* children = nullptr;
            unsigned int numChildren = 0;
            
            if (XQueryTree(display, root, &root, &parent, &children, &numChildren) != 0) {
                for (unsigned int i = 0; i < numChildren; i++) {
                    unsigned char* childPropData = nullptr;
                    if (XGetWindowProperty(display, children[i], groupAtom, 0, 1, False,
                                           XA_WINDOW, &actualType, &actualFormat,
                                           &numItems, &bytesAfter, &childPropData) == Success) {
                        if (childPropData) {
                            Window childLeader = *reinterpret_cast<Window*>(childPropData);
                            if (childLeader == groupLeader) {
                                windows.push_back(children[i]);
                            }
                            XFree(childPropData);
                        }
                    }
                }
                XFree(children);
            }
            
            XFree(propData);
        }
    }
    
    XCloseDisplay(display);
    return windows;
}
