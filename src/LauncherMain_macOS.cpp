#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <mach-o/dyld.h>

#define PATH_SEPARATOR "/"

/// Platform-specific executable path detection
std::string getExecutablePath() {
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string result(path);
        size_t lastSlash = result.rfind('/');
        if (lastSlash != std::string::npos) {
            return result.substr(0, lastSlash + 1);
        }
    }
    return "./";
}

/// Platform-specific process management
void startProcesses(const std::string& execPath) {
    std::string videoPlayerExe = execPath + "video_player";
    std::string overlayExe = execPath + "overlay";

    std::cout << "Video player: " << videoPlayerExe << std::endl;
    std::cout << "Overlay: " << overlayExe << std::endl;

    // macOS: Use fork/exec (same as Linux)
    pid_t pidVideo, pidOverlay;

    // Start video player first
    std::cout << "Starting video player process..." << std::endl;
    pidVideo = fork();

    if (pidVideo == 0) {
        // Child process - video player
        execlp(videoPlayerExe.c_str(), "video_player", nullptr);
        // If we get here, exec failed
        std::cerr << "Failed to exec video player: " << strerror(errno) << std::endl;
        _exit(1);
    } else if (pidVideo < 0) {
        std::cerr << "Failed to fork video player: " << strerror(errno) << std::endl;
        std::exit(1);
    }
    std::cout << "Video player started with PID: " << pidVideo << std::endl;

    // Small delay to let video window initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Start overlay
    std::cout << "Starting overlay process..." << std::endl;
    pidOverlay = fork();

    if (pidOverlay == 0) {
        // Child process - overlay
        execlp(overlayExe.c_str(), "overlay", nullptr);
        // If we get here, exec failed
        std::cerr << "Failed to exec overlay: " << strerror(errno) << std::endl;
        _exit(1);
    } else if (pidOverlay < 0) {
        std::cerr << "Failed to fork overlay: " << strerror(errno) << std::endl;
        kill(pidVideo, SIGTERM);
        std::exit(1);
    }
    std::cout << "Overlay started with PID: " << pidOverlay << std::endl;

    // Wait for child processes
    std::cout << "Waiting for processes..." << std::endl;

    int status;
    pid_t exited = waitpid(-1, &status, 0);

    if (exited == pidVideo) {
        std::cout << "Video player exited (PID: " << pidVideo << "), terminating overlay..." << std::endl;
        kill(pidOverlay, SIGTERM);
    } else if (exited == pidOverlay) {
        std::cout << "Overlay exited (PID: " << pidOverlay << "), terminating video player..." << std::endl;
        kill(pidVideo, SIGTERM);
    }

    // Clean up both processes
    waitpid(pidVideo, &status, WNOHANG);
    waitpid(pidOverlay, &status, WNOHANG);
}

/// Platform-specific cleanup
void cleanupProcesses() {
    // Cleanup already handled in startProcesses
}