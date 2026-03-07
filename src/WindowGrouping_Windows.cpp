#include <windows.h>
#include <vector>
#include <SFML/Window/WindowHandle.hpp>

/// Set window group for Windows using GWL_ID
/// Both windows in a group should use the same group leader handle
/// This enables Alt+` switching between grouped windows
void setWindowGroupPlatform(sf::WindowHandle windowHandle, sf::WindowHandle groupLeader) {
    HWND window = reinterpret_cast<HWND>(windowHandle);
    HWND leader = reinterpret_cast<HWND>(groupLeader);

    // Use GWL_ID to assign the window to a group
    // Windows with the same ID are grouped together for Alt+Tab/Alt+` switching
    // Use SetWindowLongPtr for 64-bit compatibility
    SetWindowLongPtr(window, GWL_ID, reinterpret_cast<LONG_PTR>(leader));
}

/// Cycle focus to the next window in the group on Windows
bool cycleWindowFocusPlatform(sf::WindowHandle /*currentWindow*/) {
    // Use Alt+Tab simulation for window cycling
    // This triggers the system's window switcher
    keybd_event(VK_MENU, 0, 0, 0);  // Press Alt
    keybd_event(VK_TAB, 0, 0, 0);   // Press Tab
    Sleep(50);                      // Short delay
    keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);  // Release Tab
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0); // Release Alt

    return true;
}

/// Get all windows in the same group
std::vector<HWND> getWindowGroupPlatform(sf::WindowHandle windowHandle) {
    std::vector<HWND> windows;
    HWND current = reinterpret_cast<HWND>(windowHandle);

    // Get the group ID (GWL_ID) of the current window
    [[maybe_unused]] LONG_PTR groupId = GetWindowLongPtr(current, GWL_ID);

    // Enumerate all top-level windows
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* windows = reinterpret_cast<std::vector<HWND>*>(lParam);
        LONG_PTR id = GetWindowLongPtr(hwnd, GWL_ID);
        if (windows->empty()) {
            // First window - just collect it
            windows->push_back(hwnd);
        } else if (id != 0) {
            // Subsequent windows - check if same group ID
            LONG_PTR groupID = GetWindowLongPtr((*windows)[0], GWL_ID);
            if (id == groupID) {
                windows->push_back(hwnd);
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));

    return windows;
}
