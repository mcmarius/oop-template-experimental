#include "TransparentWindow.h"
#include <iostream>
#include "IPCController.h"

namespace transparent {

void TransparentWindow::setStreamingSources(const std::vector<ipc::StreamingSource>& sources) {
    static int temp_last_sources_size = -1;
    static int temp_last_buttons_size = -1;

    if (static_cast<int>(sources.size()) != temp_last_sources_size) {
        std::cout << "setStreamingSources: received " << sources.size() << " sources" << std::endl;
        temp_last_sources_size = static_cast<int>(sources.size());
    }

    m_sources = sources;

    // Preserve selection if the selected source still exists in the new list
    if (m_selectedSourceIndex >= 0 && m_selectedSourceIndex < static_cast<int>(m_sources.size())) {
        // Selected source still exists, keep the index
    } else if (m_selectedSourceIndex >= 0) {
        // Selected source no longer exists, reset selection
        m_selectedSourceIndex = -1;
    }

    m_sourceButtons.clear();
    m_sourceButtonTexts.clear();

    // Create buttons for each source
    float buttonHeight = 30.0f;
    float spacing = 5.0f;
    float startY = m_sourcePickerBackground.getPosition().y + 25.0f;

    for (size_t i = 0; i < sources.size(); ++i) {
        // Create button for source
        auto sourceButton = std::make_unique<sf::RectangleShape>();
        sourceButton->setSize(sf::Vector2f(260, buttonHeight));
        sourceButton->setPosition(sf::Vector2f(
            m_sourcePickerBackground.getPosition().x + 20.0f,
            startY + i * (buttonHeight + spacing)
        ));

        // Highlight the selected source button
        if (static_cast<int>(i) == m_selectedSourceIndex) {
            sourceButton->setFillColor(sf::Color(50, 200, 100, 200));  // Green for selected
        } else {
            sourceButton->setFillColor(sf::Color(50, 150, 200, 200));  // Blue-ish for others
        }
        sourceButton->setOutlineThickness(1);
        sourceButton->setOutlineColor(sf::Color::White);

        // Create text for source
        auto sourceText = std::make_unique<sf::Text>(m_buttonFont, sources[i].name, 14);
        sourceText->setFillColor(sf::Color::White);
        sourceText->setPosition(sf::Vector2f(
            sourceButton->getPosition().x + 5.0f,
            sourceButton->getPosition().y + 5.0f
        ));

        m_sourceButtons.push_back(std::move(sourceButton));
        m_sourceButtonTexts.push_back(std::move(sourceText));
    }

    if (static_cast<int>(m_sourceButtons.size()) != temp_last_buttons_size) {
        std::cout << "setStreamingSources: created " << m_sourceButtons.size() << " buttons" << std::endl;
        temp_last_buttons_size = static_cast<int>(m_sourceButtons.size());
    }

    // Update Sources button text to show currently selected source
    updateSourcesButtonText();
}

std::string TransparentWindow::getSelectedSource() const {
    if (m_selectedSourceIndex >= 0 && m_selectedSourceIndex < static_cast<int>(m_sources.size())) {
        return m_sources[m_selectedSourceIndex].mrl;
    }
    return "";
}

std::string TransparentWindow::getSelectedSourceName() const {
    if (m_selectedSourceIndex >= 0 && m_selectedSourceIndex < static_cast<int>(m_sources.size())) {
        return m_sources[m_selectedSourceIndex].name;
    }
    return "";
}

bool TransparentWindow::writeSelectedSourceToIPC() const {
    ipc::IPCController ipc;
    ipc::IPCController::Config config;
    config.overlayToVideoPath = m_config.overlayToVideoPath;

    if (!ipc.initialize(config)) {
        std::cerr << "Failed to initialize IPC for writing selected source" << std::endl;
        return false;
    }

    std::string selectedSource = getSelectedSource();
    if (selectedSource.empty()) {
        std::cerr << "No source selected" << std::endl;
        return false;
    }

    if (!ipc.writeSelectedSource(selectedSource)) {
        std::cerr << "Failed to write selected source to IPC" << std::endl;
        return false;
    }

    return true;
}

bool TransparentWindow::areSourcesAvailable() const {
    return m_sourcesAvailable;
}

bool TransparentWindow::isSourcePickerOpen() const {
    return m_sourcePickerOpen;
}

void TransparentWindow::toggleSourcePicker() {
    m_sourcePickerOpen = !m_sourcePickerOpen;
}

void TransparentWindow::resetSelectedSource() {
    m_selectedSourceIndex = -1;
    updateSourcesButtonText();
}

void TransparentWindow::updateSourcesButtonText() {
    if (!m_sourcesToggleButtonText) {
        return;
    }

    std::string selectedName = getSelectedSourceName();
    if (!selectedName.empty()) {
        // Show the selected source name truncated if too long
        std::string text = selectedName;
        if (text.length() > 15) {
            text = text.substr(0, 12) + "...";
        }
        m_sourcesToggleButtonText->setString("Selected: " + text);
    } else {
        m_sourcesToggleButtonText->setString("Sources");
    }

    // Re-center text on Sources button
    sf::FloatRect textBounds = m_sourcesToggleButtonText->getLocalBounds();
    m_sourcesToggleButtonText->setPosition(sf::Vector2f(
        m_sourcesTogglebutton.getPosition().x + (m_sourcesTogglebutton.getSize().x - textBounds.size.x) / 2.0f,
        m_sourcesTogglebutton.getPosition().y + (m_sourcesTogglebutton.getSize().y - textBounds.size.y) / 2.0f
    ));
}

// Scale UI elements based on window size
void TransparentWindow::scaleUIElements(sf::Vector2u originalSize, sf::Vector2u newSize) {
    if (newSize.x == 0 || newSize.y == 0 || originalSize.x == 0 || originalSize.y == 0) {
        return;
    }

    float scaleX = static_cast<float>(newSize.x) / originalSize.x;
    float scaleY = static_cast<float>(newSize.y) / originalSize.y;

    // Scale Sources toggle button - position and size both scale
    m_sourcesTogglebutton.setSize(sf::Vector2f(100 * scaleX, 40 * scaleY));
    m_sourcesTogglebutton.setPosition(sf::Vector2f(100 * scaleX, 117 * scaleY));

    // Re-center text on Sources button
    if (m_sourcesToggleButtonText) {
        sf::FloatRect sourcesTextBounds = m_sourcesToggleButtonText->getLocalBounds();
        m_sourcesToggleButtonText->setPosition(sf::Vector2f(
            m_sourcesTogglebutton.getPosition().x + (m_sourcesTogglebutton.getSize().x - sourcesTextBounds.size.x) / 2.0f,
            m_sourcesTogglebutton.getPosition().y + (m_sourcesTogglebutton.getSize().y - sourcesTextBounds.size.y) / 2.0f
        ));
    }

    // Scale source picker background - position and size both scale
    m_sourcePickerBackground.setSize(sf::Vector2f(300 * scaleX, 400 * scaleY));
    m_sourcePickerBackground.setPosition(sf::Vector2f(50 * scaleX, 200 * scaleY));

    // Scale file picker button - position and size both scale
    m_filePickerButton.setSize(sf::Vector2f(200 * scaleX, 40 * scaleY));
    m_filePickerButton.setPosition(sf::Vector2f(50 * scaleX, 450 * scaleY));

    // Re-center text on file picker button
    if (m_filePickerButtonText) {
        sf::FloatRect fileTextBounds = m_filePickerButtonText->getLocalBounds();
        m_filePickerButtonText->setPosition(sf::Vector2f(
            m_filePickerButton.getPosition().x + (m_filePickerButton.getSize().x - fileTextBounds.size.x) / 2.0f,
            m_filePickerButton.getPosition().y + (m_filePickerButton.getSize().y - fileTextBounds.size.y) / 2.0f
        ));
    }

    // Re-scale source buttons and texts
    float buttonHeight = 30.0f * scaleY;
    float spacing = 5.0f * scaleY;
    float startY = m_sourcePickerBackground.getPosition().y + 25.0f * scaleY;

    for (size_t i = 0; i < m_sources.size(); ++i) {
        if (i < m_sourceButtons.size()) {
            m_sourceButtons[i]->setSize(sf::Vector2f(260 * scaleX, buttonHeight));
            m_sourceButtons[i]->setPosition(sf::Vector2f(
                m_sourcePickerBackground.getPosition().x + 20.0f * scaleX,
                startY + i * (buttonHeight + spacing)
            ));
        }

        if (i < m_sourceButtonTexts.size()) {
            m_sourceButtonTexts[i]->setPosition(sf::Vector2f(
                m_sourceButtons[i]->getPosition().x + 5.0f * scaleX,
                m_sourceButtons[i]->getPosition().y + 5.0f * scaleY
            ));
        }
    }
}

} // namespace transparent