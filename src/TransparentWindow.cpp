#include "TransparentWindow.h"
#include <iostream>

namespace transparent {

TransparentWindow::TransparentWindow() = default;

TransparentWindow::~TransparentWindow() = default;

bool TransparentWindow::initialize(const Config& config) {
    m_config = config;

    // Load the image with transparency
    if (!m_image.loadFromFile(config.imageFile)) {
        std::cerr << "Failed to load image from " << config.imageFile << std::endl;
        return false;
    }

    // Create the window with the image size
    m_window.create(sf::VideoMode(m_image.getSize()), "Transparent Overlay", config.style);

    // Center the window initially
    const sf::Vector2u desktopSize = sf::VideoMode::getDesktopMode().size;
    m_window.setPosition(sf::Vector2i(
        (desktopSize.x - m_image.getSize().x) / 2,
        (desktopSize.y - m_image.getSize().y) / 2
    ));

    // Create texture from image and sprite from texture
    // This allows us to draw the image with transparency
    if (!m_texture.loadFromImage(m_image)) {
        std::cerr << "Failed to load texture from image" << std::endl;
        return false;
    }
    m_sprite = std::make_unique<sf::Sprite>(m_texture);

    // Initialize Sources toggle button
    m_sourcesTogglebutton.setSize(sf::Vector2f(100, 40));
    m_sourcesTogglebutton.setPosition(sf::Vector2f(100, 117));
    m_sourcesTogglebutton.setFillColor(sf::Color(255, 165, 0, 200));  // Orange
    m_sourcesTogglebutton.setOutlineThickness(2);
    m_sourcesTogglebutton.setOutlineColor(sf::Color::White);

    if (!m_buttonFont.openFromFile("assets/fonts/FiraSans-Regular.ttf")) {
        std::cerr << "Warning: Could not load font for Sources button" << std::endl;
    }
    m_sourcesToggleButtonText = std::make_unique<sf::Text>(m_buttonFont, "Sources", 16);
    m_sourcesToggleButtonText->setFillColor(sf::Color::White);

    // Center text on Sources button
    sf::FloatRect sourcesTextBounds = m_sourcesToggleButtonText->getLocalBounds();
    m_sourcesToggleButtonText->setPosition(sf::Vector2f(
        m_sourcesTogglebutton.getPosition().x + (m_sourcesTogglebutton.getSize().x - sourcesTextBounds.size.x) / 2.0f,
        m_sourcesTogglebutton.getPosition().y + (m_sourcesTogglebutton.getSize().y - sourcesTextBounds.size.y) / 2.0f
    ));

    // Initialize streaming source picker background
    m_sourcePickerBackground.setSize(sf::Vector2f(300, 400));
    m_sourcePickerBackground.setPosition(sf::Vector2f(50, 200));
    m_sourcePickerBackground.setFillColor(sf::Color(30, 30, 30, 220));  // Dark semi-transparent
    m_sourcePickerBackground.setOutlineThickness(2);
    m_sourcePickerBackground.setOutlineColor(sf::Color::White);
    m_sourcePickerOpen = false;

    // Initialize file picker button (outside the picker background)
    m_filePickerButton.setSize(sf::Vector2f(200, 40));
    m_filePickerButton.setPosition(sf::Vector2f(50, 450));
    m_filePickerButton.setFillColor(sf::Color(200, 50, 50, 200));  // Reddish
    m_filePickerButton.setOutlineThickness(2);
    m_filePickerButton.setOutlineColor(sf::Color::Yellow);
    std::cout << "File picker button initialized at: (" << m_filePickerButton.getPosition().x << ", " << m_filePickerButton.getPosition().y << ")" << std::endl;
    std::cout << "File picker button size: (" << m_filePickerButton.getSize().x << ", " << m_filePickerButton.getSize().y << ")" << std::endl;

    if (!m_buttonFont.openFromFile("assets/fonts/FiraSans-Regular.ttf")) {
        std::cerr << "Warning: Could not load font for file picker button" << std::endl;
    }
    m_filePickerButtonText = std::make_unique<sf::Text>(m_buttonFont, "Open File...", 18);
    m_filePickerButtonText->setFillColor(sf::Color::White);

    // Center text on file picker button
    sf::FloatRect fileTextBounds = m_filePickerButtonText->getLocalBounds();
    m_filePickerButtonText->setPosition(sf::Vector2f(
        m_filePickerButton.getPosition().x + (m_filePickerButton.getSize().x - fileTextBounds.size.x) / 2.0f,
        m_filePickerButton.getPosition().y + (m_filePickerButton.getSize().y - fileTextBounds.size.y) / 2.0f
    ));
    std::cout << "File picker text bounds: size=(" << fileTextBounds.size.x << ", " << fileTextBounds.size.y << ")" << std::endl;
    std::cout << "File picker text position set to: (" << m_filePickerButtonText->getPosition().x << ", " << m_filePickerButtonText->getPosition().y << ")" << std::endl;

    // Apply transparency and shape (must be done before setting m_initialized)
    if (!applyAlpha(m_config.alpha)) {
        std::cerr << "Warning: Failed to apply transparency (may not be supported)" << std::endl;
    }

    if (!applyShape()) {
        std::cerr << "Warning: Failed to apply custom shape (may not be supported)" << std::endl;
    }

    m_initialized = true;
    return true;
}

bool TransparentWindow::applyAlpha(unsigned char alpha) {
    return setTransparency(alpha);
}

bool TransparentWindow::applyShape() {
    if (!m_initialized) {
        return false;
    }

    return setShape();
}

void TransparentWindow::close() {
    m_window.close();
}

bool TransparentWindow::isOpen() const {
    return m_window.isOpen();
}

sf::RenderWindow& TransparentWindow::getWindow() {
    return m_window;
}

const sf::RenderWindow& TransparentWindow::getWindow() const {
    return m_window;
}

sf::WindowHandle TransparentWindow::getNativeHandle() const {
    return m_window.getNativeHandle();
}

void TransparentWindow::setPosition(sf::Vector2i position) {
    m_window.setPosition(position);
}

void TransparentWindow::setSize(sf::Vector2u size) {
    m_window.setSize(size);
}

sf::Vector2u TransparentWindow::getPosition() const {
    return sf::Vector2u(m_window.getPosition());
}

sf::Vector2u TransparentWindow::getSize() const {
    return m_window.getSize();
}

const sf::Image& TransparentWindow::getImage() const {
    return m_image;
}

const sf::Sprite& TransparentWindow::getSprite() const {
    return *m_sprite;
}

} // namespace transparent