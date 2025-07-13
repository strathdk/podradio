#include "core/Player.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

using namespace podradio::core;

void printHelp() {
    std::cout << "\nPodRadio Commands:\n"
              << "Usage: podradio [command] [url]\n\n"
              << "Commands:\n"
              << "  play <url>     - Play audio from URL or podcast feed\n"
              << "  pause          - Pause playback\n"
              << "  resume         - Resume playback\n"
              << "  stop           - Stop playback\n"
              << "  status         - Show playback status\n"
              << "  help           - Show this help\n"
              << "  quit          - Exit program\n\n"
              << "If no command line arguments are provided, the program will start in interactive mode.\n\n";
}

void handleCommand(Player& player, const std::string& command, const std::string& url = "") {
    try {
        if (command == "play") {
            if (!url.empty()) {
                // Check if URL is likely a podcast feed
                bool is_feed = url.find("feed") != std::string::npos || 
                             url.find("rss") != std::string::npos ||
                             url.find("xml") != std::string::npos ||
                             url.find("podcast") != std::string::npos ||
                             url.find("itunes") != std::string::npos;
                
                if (is_feed) {
                    std::cout << "Detected podcast feed URL, fetching latest episode...\n";
                    player.playPodcastFeed(url);
                } else {
                    // player.play(url);
                }
            } else {
                std::cout << "Usage: play <url>\n";
            }
        }
        else if (command == "pause") {
            player.pause();
            std::cout << "Playback paused\n";
        }
        else if (command == "resume") {
            if (!player.isPlaying()) {
                player.play("");  // Resume current media
                std::cout << "Playback resumed\n";
            }
        }
        else if (command == "stop") {
            player.stop();
            std::cout << "Playback stopped\n";
        }
        else if (command == "status") {
            std::cout << "Status: " << (player.isPlaying() ? "Playing" : "Stopped") << "\n";
        }
        else if (command == "help") {
            printHelp();
        }
        else {
            std::cout << "Unknown command. Type 'help' for available commands.\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        throw;
    }
}

int main(int argc, char* argv[]) {
    try {
        Player player;

        // Handle command line arguments if provided
        if (argc > 1) {
            std::string command = argv[1];
            std::string url = (argc > 2) ? argv[2] : "";
            
            try {
                handleCommand(player, command, url);
                
                // For play commands, keep the program running until interrupted
                if (command == "play") {
                    std::cout << "Playing... Press Ctrl+C to stop.\n";
                    while (player.isPlaying()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                return 1;
            }
            return 0;
        }

        // Interactive mode
        std::cout << "Welcome to PodRadio!\n";
        printHelp();

        std::string input;
        bool running = true;

        while (running) {
            std::cout << "\nEnter command: ";
            std::getline(std::cin, input);

            try {
                if (input.empty()) {
                    continue;
                }
                else if (input.substr(0, 4) == "play") {
                    if (input.length() > 5) {
                        handleCommand(player, "play", input.substr(5));
                    } else {
                        std::cout << "Usage: play <url>\n";
                    }
                }
                else if (input == "quit") {
                    running = false;
                }
                else {
                    handleCommand(player, input);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
} 