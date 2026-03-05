#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>

/// Launcher - Starts both video player and overlay processes
/// This is the main entry point for the two-process solution

std::string getExecutablePath();

/// Platform-specific process management
void startProcesses(const std::string& execPath);

/// Platform-specific cleanup
void cleanupProcesses();

int main() {
    std::cout << "=== Transparent Windows Launcher ===" << std::endl;
    std::cout << "This launcher starts the video player and overlay as separate processes." << std::endl;
    std::cout << "Video window will stream content. Overlay window will appear transparent on top." << std::endl;
    std::cout << "Press ESC in either window to exit.\n" << std::endl;

    std::string execPath = getExecutablePath();
    std::cout << "Executable path: " << execPath << std::endl;

    startProcesses(execPath);
    cleanupProcesses();

    std::cout << "Launcher terminated" << std::endl;
    return 0;
}