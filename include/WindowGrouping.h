#ifndef WINDOW_GROUPING_H
#define WINDOW_GROUPING_H

#include <SFML/Graphics.hpp>
#include <vector>

/// Set window group for cross-platform Alt+` switching
/// Both windows in a group should use the same group leader handle
void setWindowGroup(sf::WindowHandle windowHandle, sf::WindowHandle groupLeader);

/// Cycle focus to the next window in the group
/// Returns true if successful
bool cycleWindowFocus(sf::WindowHandle currentWindow);

/// Get all windows in the same group
/// Returns vector of window handles
std::vector<sf::WindowHandle> getWindowGroup(sf::WindowHandle windowHandle);

#endif // WINDOW_GROUPING_H