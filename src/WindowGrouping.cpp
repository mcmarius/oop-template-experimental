#include "WindowGrouping.h"
#include <SFML/Graphics.hpp>

// Platform-specific implementations are in WindowGrouping_<Platform>.cpp
// These files define setWindowGroupPlatform, cycleWindowFocusPlatform, and getWindowGroupPlatform

// Forward declarations of platform-specific functions
void setWindowGroupPlatform(sf::WindowHandle windowHandle, sf::WindowHandle groupLeader);
bool cycleWindowFocusPlatform(sf::WindowHandle currentWindow);
std::vector<sf::WindowHandle> getWindowGroupPlatform(sf::WindowHandle windowHandle);

void setWindowGroup(sf::WindowHandle windowHandle, sf::WindowHandle groupLeader) {
    setWindowGroupPlatform(windowHandle, groupLeader);
}

bool cycleWindowFocus(sf::WindowHandle currentWindow) {
    return cycleWindowFocusPlatform(currentWindow);
}

std::vector<sf::WindowHandle> getWindowGroup(sf::WindowHandle windowHandle) {
    return getWindowGroupPlatform(windowHandle);
}