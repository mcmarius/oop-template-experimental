#include <X11/Xlib.h>
#undef Always
#undef None
#include <iostream>
// #include <array>
#include <chrono>
#include <thread>
#include <SFML/Graphics.hpp>

#include "include/Example.h"
// This also works if you do not want `include/`, but some editors might not like it
// #include "Example.h"
#include <vlcpp/vlc.hpp>

//////////////////////////////////////////////////////////////////////
/// This class is used to test that the memory leak checks work as expected even when using a GUI
class SomeClass {
public:
    explicit SomeClass(int) {}
};

SomeClass *getC() {
    return new SomeClass{2};
}
//////////////////////////////////////////////////////////////////////

std::string get_discovery_module() {
    // FIXME validate for windows and macos
#if defined(_WIN32) || defined(_WIN64)
    return "dshow";      // DirectShow for Windows
#elif defined(__APPLE__) || defined(__MACH__)
    return "avcapture";  // AVFoundation for macOS
#else
    return "udev";       // udev for Linux
#endif
}

int main() {
    XInitThreads();
    std::cout << "Hello, world!\n";
    Example e1;
    e1.g();
    // std::array<int, 100> v{};
    // int nr;
    // std::cout << "Introduceți nr: ";
    /////////////////////////////////////////////////////////////////////////
    /// Observație: dacă aveți nevoie să citiți date de intrare de la tastatură,
    /// dați exemple de date de intrare folosind fișierul tastatura.txt
    /// Trebuie să aveți în fișierul tastatura.txt suficiente date de intrare
    /// (în formatul impus de voi) astfel încât execuția programului să se încheie.
    /// De asemenea, trebuie să adăugați în acest fișier date de intrare
    /// pentru cât mai multe ramuri de execuție.
    /// Dorim să facem acest lucru pentru a automatiza testarea codului, fără să
    /// mai pierdem timp de fiecare dată să introducem de la zero aceleași date de intrare.
    ///
    /// Pe GitHub Actions (bife), fișierul tastatura.txt este folosit
    /// pentru a simula date introduse de la tastatură.
    /// Bifele verifică dacă programul are erori de compilare, erori de memorie și memory leaks.
    ///
    /// Dacă nu puneți în tastatura.txt suficiente date de intrare, îmi rezerv dreptul să vă
    /// testez codul cu ce date de intrare am chef și să nu pun notă dacă găsesc vreun bug.
    /// Impun această cerință ca să învățați să faceți un demo și să arătați părțile din
    /// program care merg (și să le evitați pe cele care nu merg).
    ///
    /////////////////////////////////////////////////////////////////////////
    // std::cin >> nr;
    /////////////////////////////////////////////////////////////////////////
    // for(int i = 0; i < nr; ++i) {
    //     std::cout << "v[" << i << "] = ";
    //     std::cin >> v[i];
    // }
    // std::cout << "\n\n";
    // std::cout << "Am citit de la tastatură " << nr << " elemente:\n";
    // for(int i = 0; i < nr; ++i) {
    //     std::cout << "- " << v[i] << "\n";
    // }
    ///////////////////////////////////////////////////////////////////////////
    /// Pentru date citite din fișier, NU folosiți tastatura.txt. Creați-vă voi
    /// alt fișier propriu cu ce alt nume doriți.
    /// Exemplu:
    /// std::ifstream fis("date.txt");
    /// for(int i = 0; i < nr2; ++i)
    ///     fis >> v2[i];
    ///
    ///////////////////////////////////////////////////////////////////////////

    SomeClass *c = getC();
    std::cout << c << "\n";
    delete c;  // comentarea acestui rând ar trebui să ducă la semnalarea unui mem leak

    // 1. Create a VLC instance
    // const char* vlc_args[] = { "--plugin-path=/usr/lib/x86_64-linux-gnu/vlc/plugins" };
    // const char* vlc_args[] = {"--no-xlib"};
    auto instance = VLC::Instance(0, nullptr);

    // 2. Create a MediaDiscoverer for video devices
    auto discoverer = VLC::MediaDiscoverer(instance, get_discovery_module());

    if (!discoverer.start()) {
        std::cerr << "Failed to start discovery." << std::endl;
        return 1;
    }

    // 3. Get the MediaList from the discoverer
    auto mediaList = discoverer.mediaList();

    // Note: Discovery is asynchronous. In a real app, you might need to wait
    // for events, but for a simple CLI tool, we'll poll the count.
    std::cout << "Searching for devices..." << std::endl;

    // Small delay to allow the OS to report devices
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 4. Iterate and print names/MRLs
    std::cout << "Found " << mediaList->count() << " devices:" << std::endl;
    for (int i = 0; i < mediaList->count(); ++i) {
        auto item = mediaList->itemAtIndex(i);
        std::cout << "[" << i << "] Name: " << item->meta(libvlc_meta_Title) << std::endl;
        std::cout << "    MRL:  " << item->mrl() << std::endl;
    }
    auto item = mediaList->itemAtIndex(0);
    auto mediaPlayer = VLC::MediaPlayer(*item);


    sf::RenderWindow window;
    ///////////////////////////////////////////////////////////////////////////
    /// NOTE: sync with env variable APP_WINDOW from .github/workflows/cmake.yml:31
    window.create(sf::VideoMode({1280, 720}), "My Window", sf::Style::Default);
    if (!window.setActive(false)) {
        std::cerr << "Failed to set active window." << std::endl;
        return 1;
    }
    ///////////////////////////////////////////////////////////////////////////
    std::cout << "Fereastra a fost creată\n";
    ///////////////////////////////////////////////////////////////////////////
    /// NOTE: mandatory use one of vsync or FPS limit (not both)            ///
    /// This is needed so we do not burn the GPU                            ///
    window.setVerticalSyncEnabled(false);                                    ///
    // window.setFramerateLimit(60);                                       ///
    // ///////////////////////////////////////////////////////////////////////////
    std::vector<uint8_t> videoPixels(1280 * 720 * 4);

    auto hndl = window.getNativeHandle();
    // Tell VLC to take over the SFML window's drawing area
    #if defined(_WIN32)
        mediaPlayer.setHwnd(window.getNativeHandle());
    #else
        mediaPlayer.setXwindow(hndl);
    #endif
    //
    mediaPlayer.play();
    discoverer.stop();

    while(window.isOpen()) {
        bool shouldExit = false;

        while(const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
                std::cout << "Fereastra a fost închisă\n";
            }
            else if (event->is<sf::Event::Resized>()) {
                std::cout << "New width: " << window.getSize().x << '\n'
                          << "New height: " << window.getSize().y << '\n';
            }
            else if (event->is<sf::Event::KeyPressed>()) {
                const auto* keyPressed = event->getIf<sf::Event::KeyPressed>();
                std::cout << "Received key " << (keyPressed->scancode == sf::Keyboard::Scancode::X ? "X" : "(other)") << "\n";
                if(keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    shouldExit = true;
                }
            }
        }
        if(shouldExit) {
            window.close();
            std::cout << "Fereastra a fost închisă (shouldExit == true)\n";
            break;
        }
        // using namespace std::chrono_literals;
        // std::this_thread::sleep_for(300ms);
    }

    std::cout << "Programul a terminat execuția\n";
    return 0;
}
