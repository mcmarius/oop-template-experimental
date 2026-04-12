#include "TransparentWindow.h"
#include <iostream>

namespace transparent {

void TransparentWindow::drawDetections() {
    // Draw detection bounding boxes on top of everything
    if (!m_detectionsVisible || m_detections.empty()) {
        return;
    }

    sf::RectangleShape boxShape;
    boxShape.setFillColor(sf::Color::Transparent);
    boxShape.setOutlineColor(sf::Color::Red);
    boxShape.setOutlineThickness(2);

    for (const auto& obj : m_detections) {
        // Draw bounding box
        boxShape.setSize(obj.size);
        boxShape.setPosition(obj.position);
        m_window.draw(boxShape);

        // Draw label with class name and confidence
        std::string label = obj.className + " " + std::to_string(static_cast<int>(obj.confidence * 100)) + "%";
        sf::Text text(m_buttonFont, sf::String::fromUtf8(label.begin(), label.end()), 16u);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1);
        text.setPosition(sf::Vector2f(obj.position.x, obj.position.y - 25));

        m_window.draw(text);
    }
}

void TransparentWindow::setDetectedObjects(const std::vector<ipc::DetectedObject>& detections) {
    m_detections = detections;
}

void TransparentWindow::draw() {
    if (!m_window.isOpen() || !m_initialized) {
        return;
    }

    // Clear with transparent color
    m_window.clear(sf::Color::Transparent);

    // Draw the sprite if we have one
    if (m_sprite) {
        m_window.draw(*m_sprite);
    }

    // Draw Sources toggle button (always visible)
    m_window.draw(m_sourcesTogglebutton);
    if (m_sourcesToggleButtonText) {
        m_window.draw(*m_sourcesToggleButtonText);
    }

    // Draw selected source name indicator (when picker is closed)
    std::string selectedNameTop = getSelectedSourceName();
    if (!selectedNameTop.empty() && !m_sourcePickerOpen) {
        sf::Text selectedText(m_buttonFont, "Selected: " + selectedNameTop, 12);
        selectedText.setFillColor(sf::Color(50, 200, 100, 255));  // Green
        selectedText.setPosition(sf::Vector2f(
            m_sourcesTogglebutton.getPosition().x + 110.0f,
            m_sourcesTogglebutton.getPosition().y + 5.0f
        ));
        m_window.draw(selectedText);
    }

    // Draw source picker UI if open
    if (m_sourcePickerOpen) {
        static int temp_last_button_count = -1;
        if (static_cast<int>(m_sourceButtons.size()) != temp_last_button_count) {
            std::cout << "draw: source picker open, buttons.size()=" << m_sourceButtons.size() << std::endl;
            temp_last_button_count = static_cast<int>(m_sourceButtons.size());
        }
        // Draw source picker background
        m_window.draw(m_sourcePickerBackground);

        // Draw source buttons
        for (const auto& button : m_sourceButtons) {
            m_window.draw(*button);
        }

        // Draw source button texts
        for (const auto& text : m_sourceButtonTexts) {
            m_window.draw(*text);
        }

        // Draw file picker button
        m_window.draw(m_filePickerButton);
        if (m_filePickerButtonText) {
            m_window.draw(*m_filePickerButtonText);
        }

        // Draw selected source name indicator
        std::string selectedNameBottom = getSelectedSourceName();
        if (!selectedNameBottom.empty()) {
            sf::Text selectedText(m_buttonFont, "Selected: " + selectedNameBottom, 14);
            selectedText.setFillColor(sf::Color(50, 200, 100, 255));  // Green
            selectedText.setPosition(sf::Vector2f(
                m_sourcePickerBackground.getPosition().x + 20.0f,
                m_sourcePickerBackground.getPosition().y - 30.0f
            ));
            m_window.draw(selectedText);
        }
    }

    // Draw detection bounding boxes from IPC
    drawDetections();

    m_window.display();
}

} // namespace transparent