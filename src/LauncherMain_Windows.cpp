#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <windows.h>
#include <cstring>
#include <climits>

#ifndef PATH_MAX
// MinGW defines it as 260, but in docs they say 255
#define PATH_MAX 250
#endif
#define PATH_SEPARATOR "\\"
#define EXECUTABLE_EXT ".exe"

/// Platform-specific executable path detection
std::string getExecutablePath() {
    char path[PATH_MAX];
    GetModuleFileNameA(nullptr, path, sizeof(path));
    std::string result(path);
    // Remove executable name
    size_t lastSlash = result.rfind(PATH_SEPARATOR);
    if (lastSlash != std::string::npos) {
        return result.substr(0, lastSlash + 1);
    }
    return "";
}

/// Platform-specific process management
void startProcesses(const std::string& execPath) {
    std::string videoPlayerExe = execPath + "video_player" EXECUTABLE_EXT;
    std::string overlayExe = execPath + "overlay" EXECUTABLE_EXT;

    std::cout << "Video player: " << videoPlayerExe << std::endl;
    std::cout << "Overlay: " << overlayExe << std::endl;

    // Windows: Use CreateProcess
    STARTUPINFOA si;
    PROCESS_INFORMATION piVideo, piOverlay;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWDEFAULT;

    // Start video player first
    std::cout << "Starting video player process..." << std::endl;
    if (!CreateProcessA(videoPlayerExe.c_str(), nullptr, nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &piVideo)) {
        std::cerr << "Failed to start video player. Error: " << GetLastError() << std::endl;
        std::exit(1);
    }
    std::cout << "Video player started with PID: " << piVideo.dwProcessId << std::endl;

    // Small delay to let video window initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start overlay
    std::cout << "Starting overlay process..." << std::endl;
    if (!CreateProcessA(overlayExe.c_str(), nullptr, nullptr, nullptr, FALSE,
                        CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &piOverlay)) {
        std::cerr << "Failed to start overlay. Error: " << GetLastError() << std::endl;
        TerminateProcess(piVideo.hProcess, 1);
        std::exit(1);
    }
    std::cout << "Overlay started with PID: " << piOverlay.dwProcessId << std::endl;

    // Wait for either process to exit
    std::cout << "Waiting for processes..." << std::endl;
    HANDLE processes[2] = {piVideo.hProcess, piOverlay.hProcess};
    DWORD result = WaitForMultipleObjects(2, processes, FALSE, INFINITE);

    if (result == WAIT_OBJECT_0) {
        std::cout << "Video player exited, closing overlay..." << std::endl;
        TerminateProcess(piOverlay.hProcess, 0);
    } else if (result == WAIT_OBJECT_0 + 1) {
        std::cout << "Overlay exited, closing video player..." << std::endl;
        TerminateProcess(piVideo.hProcess, 0);
    }

    CloseHandle(piVideo.hProcess);
    CloseHandle(piVideo.hThread);
    CloseHandle(piOverlay.hProcess);
    CloseHandle(piOverlay.hThread);
}

/// Platform-specific cleanup
void cleanupProcesses() {
    // Cleanup already handled in startProcesses
}
