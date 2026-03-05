#include "VideoWindow.h"
#include <iostream>

namespace vlc {

VideoWindow::VideoWindow()
    : m_instance(0, nullptr),
      m_lastLoadedSource() {
    initializePlatform();
}

VideoWindow::~VideoWindow() {
    stop();
}

bool VideoWindow::initialize(const Config& config) {
    m_config = config;

    // Create the window
    m_window.create(sf::VideoMode(config.size), config.title, config.style);

    // Disable vsync to avoid burning GPU, set FPS limit instead
    m_window.setVerticalSyncEnabled(false);
    m_window.setFramerateLimit(60);

    return true;
}

void VideoWindow::close() {
    stop();
    m_window.close();
}

bool VideoWindow::isOpen() const {
    return m_window.isOpen();
}

sf::RenderWindow& VideoWindow::getWindow() {
    return m_window;
}

const sf::RenderWindow& VideoWindow::getWindow() const {
    return m_window;
}

sf::WindowHandle VideoWindow::getNativeHandle() const {
    return m_window.getNativeHandle();
}

bool VideoWindow::pollEvents() {
    if (!m_window.isOpen()) {
        return false;
    }

    bool shouldExit = false;

    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
            std::cout << "Video window closed" << std::endl;
        }
        else if (event->is<sf::Event::Resized>()) {
            std::cout << "Video window resized: "
                      << m_window.getSize().x << "x"
                      << m_window.getSize().y << std::endl;
        }
        else if (event->is<sf::Event::KeyPressed>()) {
            const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                shouldExit = true;
            }
        }
    }

    return !shouldExit;
}

void VideoWindow::setPosition(sf::Vector2i position) {
    m_window.setPosition(position);
}

void VideoWindow::setSize(sf::Vector2u size) {
    m_window.setSize(size);
}

sf::Vector2u VideoWindow::getPosition() const {
    return sf::Vector2u(m_window.getPosition());
}

sf::Vector2u VideoWindow::getSize() const {
    return m_window.getSize();
}

} // namespace vlc