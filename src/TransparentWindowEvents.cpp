#include "TransparentWindow.h"
#include <iostream>
#include "IPCController.h"
#include <portable-file-dialogs.h>

namespace transparent {

bool TransparentWindow::pollEvents() {
    // IPC for window switching
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.overlayToVideoPath = m_config.overlayToVideoPath;
    if (ipc.initialize(config)) {
        // Read which window should be active
        std::string activeWindow = ipc.readActiveWindow();
        if (activeWindow == "overlay") {
            m_window.requestFocus();
            m_window.setVisible(true);
        }
    }
    if (!m_window.isOpen()) {
        return false;
    }

    bool shouldExit = false;


    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>() ||
            (event->is<sf::Event::KeyPressed>() &&
             event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape)) {
            m_window.close();
            shouldExit = true;
        }
        else if (event->is<sf::Event::KeyPressed>()) {
            const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();

            // Check for Alt+` (grave/backtick) to switch between windows
            if (keyPressed->code == sf::Keyboard::Key::Grave) {
                std::cout << "Overlay: Alt+` pressed, writing active_window=video" << std::endl;
                // Write that overlay window is active
                ipc.writeActiveWindow("video");
                m_window.setVisible(false);
                //m_window.setVisible(true);
            }
            // Send play/pause command via IPC to video process
            else if (keyPressed->code == sf::Keyboard::Key::Space ||
                     keyPressed->code == sf::Keyboard::Key::P) {
                if (ipc.initialize(config)) {
                    // Write play/pause command to IPC
                    ipc.writePlayPauseCommand();
                    std::cout << "Overlay: Play/pause command sent to video process" << std::endl;
                }
            }
        }

        // Handle mouse click on Sources toggle button or file picker button
        if (event->is<sf::Event::MouseButtonPressed>()) {
            const auto& mouseEvent = *event->getIf<sf::Event::MouseButtonPressed>();
            if (mouseEvent.button == sf::Mouse::Button::Left) {
                // Get mouse position relative to this window to handle DPI scaling correctly
                sf::Vector2i mousePosInt = sf::Mouse::getPosition(m_window);
                // Convert from screen coordinates to world coordinates (accounts for view transforms)
                sf::Vector2f mousePos = m_window.mapPixelToCoords(mousePosInt);

                // Check if click is on Sources toggle button (refreshes sources and toggles picker visibility)
                sf::FloatRect sourcesToggleBounds(m_sourcesTogglebutton.getPosition(), m_sourcesTogglebutton.getSize());
                if (sourcesToggleBounds.contains(mousePos)) {
                    refreshSources();
                    m_sourcePickerOpen = !m_sourcePickerOpen;
                }

                // Check if click is on a source button (selects that source)
                if (m_sourcePickerOpen && !m_sourceButtons.empty()) {
                    for (size_t i = 0; i < m_sourceButtons.size(); ++i) {
                        sf::FloatRect sourceBounds(m_sourceButtons[i]->getPosition(), m_sourceButtons[i]->getSize());
                        if (sourceBounds.contains(mousePos)) {
                            m_selectedSourceIndex = static_cast<int>(i);
                            std::cout << "Selected source: " << m_sources[i].name << std::endl;
                            // Write selected source to IPC
                            writeSelectedSourceToIPC();
                            // Update Sources button text to show selected source
                            updateSourcesButtonText();
                            // Close picker to force natural refresh next time it's opened
                            m_sourcePickerOpen = false;
                            break;
                        }
                    }
                }

                // Check if click is on file picker button (outside picker background)
                sf::FloatRect filePickerBounds(m_filePickerButton.getPosition(), m_filePickerButton.getSize());
                if (filePickerBounds.contains(mousePos)) {
                    // Use portable file dialog to select a file
                    // Use multiple filter entries - one per extension for better compatibility
                    std::vector<std::string> filters = {
                        "Video Files", "*.webm *.mp4 *.avi *.mkv",
                        "WebM files", "*.webm",
                        "MP4 files", "*.mp4",
                        "AVI files", "*.avi",
                        "MKV files", "*.mkv",
                        "All Files", "*"
                    };
                    pfd::open_file chooser("Select a video file",
                                           "",
                                           filters);
                    std::vector<std::string> result = chooser.result();
                    std::cout << "Dialog returned with " << result.size() << " files" << std::endl;
                    if (!result.empty()) {
                        std::string filepath = result[0];
                        std::cout << "Selected file: " << filepath << std::endl;
                        // Write file:// URL to IPC (overlay→video path)
                        if (ipc.initialize(config)) {
                            std::string url = "file://" + filepath;
                            ipc.writeSelectedSource(url);
                            std::cout << "Selected source written to IPC: " << url << std::endl;
                        }
                        // Close picker to force natural refresh next time it's opened
                        m_sourcePickerOpen = false;
                    } else {
                        std::cout << "No file selected" << std::endl;
                    }
                }
            }
        }
    }

    return !shouldExit;
}

} // namespace transparent
