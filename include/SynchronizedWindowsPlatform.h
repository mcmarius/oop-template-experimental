#ifndef SYNCHRONIZED_WINDOWS_PLATFORM_H
#define SYNCHRONIZED_WINDOWS_PLATFORM_H

#include <SFML/Graphics.hpp>

namespace syncwindows {

// Platform-specific window raising functions
void raiseWindowLinux(sf::WindowHandle windowHandle);
void raiseTwoWindowsLinux(sf::WindowHandle transparentHandle, sf::WindowHandle videoHandle);

void raiseWindowmacOS(sf::WindowHandle windowHandle);
void raiseTwoWindowsmacOS(sf::WindowHandle transparentHandle, sf::WindowHandle videoHandle);

void raiseWindowWindows(sf::WindowHandle windowHandle);
void raiseTwoWindowsWindows(sf::WindowHandle transparentHandle, sf::WindowHandle videoHandle);

} // namespace syncwindows

#endif // SYNCHRONIZED_WINDOWS_PLATFORM_H