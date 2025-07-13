#include "core/Player.hpp"
#include "core/FeedManager.hpp"
#include "core/Subscription.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace podradio::core;

void printHelp() {
    std::cout << "\nPodRadio Commands:\n"
              << "Usage: podradio [command] [arguments]\n\n"
              << "Podcast Management:\n"
              << "  add <name> <url>     - Add a new podcast subscription\n"
              << "  remove <name>        - Remove a podcast subscription\n"
              << "  list                 - List all subscribed podcasts\n"
              << "  next                 - Select next podcast\n"
              << "  previous             - Select previous podcast\n"
              << "  current              - Show current podcast\n\n"
              << "Playback:\n"
              << "  play                 - Play current podcast's latest episode\n"
              << "  play <url>           - Play audio from URL\n"
              << "  pause                - Pause playback\n"
              << "  resume               - Resume playback\n"
              << "  stop                 - Stop playback\n"
              << "  status               - Show playback status\n\n"
              << "General:\n"
              << "  help                 - Show this help\n"
              << "  quit                 - Exit program\n\n"
              << "If no command line arguments are provided, the program will start in interactive mode.\n\n";
}

void printPodcastList(const FeedManager& feedManager) {
    auto subscriptions = feedManager.getSubscriptions();
    if (subscriptions.empty()) {
        std::cout << "No podcasts subscribed.\n";
        return;
    }
    
    std::cout << "\nSubscribed Podcasts:\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (size_t i = 0; i < subscriptions.size(); ++i) {
        const auto& sub = subscriptions[i];
        std::string marker = (i == feedManager.getCurrentIndex()) ? "* " : "  ";
        std::cout << marker << std::left << std::setw(20) << sub.name 
                  << " | " << sub.feedUrl << "\n";
    }
    std::cout << std::string(60, '-') << "\n";
    std::cout << "* = Currently selected\n";
}

void handleCommand(Player& player, FeedManager& feedManager, const std::string& command, const std::vector<std::string>& args = {}) {
    try {
        if (command == "add") {
            if (args.size() < 2) {
                std::cout << "Usage: add <name> <url> [description]\n";
                return;
            }
            std::string description = args.size() > 2 ? args[2] : "";
            feedManager.addPodcast(args[0], args[1], description);
        }
        else if (command == "remove") {
            if (args.empty()) {
                std::cout << "Usage: remove <name>\n";
                return;
            }
            feedManager.removePodcast(args[0]);
        }
        else if (command == "list") {
            printPodcastList(feedManager);
        }
        else if (command == "next") {
            auto podcast = feedManager.nextPodcast();
            if (podcast) {
                std::cout << "Selected next podcast: " << podcast->name << "\n";
            } else {
                std::cout << "No podcasts available\n";
            }
        }
        else if (command == "previous") {
            auto podcast = feedManager.previousPodcast();
            if (podcast) {
                std::cout << "Selected previous podcast: " << podcast->name << "\n";
            } else {
                std::cout << "No podcasts available\n";
            }
        }
        else if (command == "current") {
            auto podcast = feedManager.getCurrentPodcast();
            if (podcast) {
                std::cout << "Current podcast: " << podcast->name << "\n";
                std::cout << "  URL: " << podcast->feedUrl << "\n";
                if (!podcast->description.empty()) {
                    std::cout << "  Description: " << podcast->description << "\n";
                }
            } else {
                std::cout << "No podcast selected\n";
            }
        }
        else if (command == "play") {
            if (!args.empty()) {
                // Play from URL
                player.play(args[0]);
            } else {
                // Play current podcast's latest episode
                auto podcast = feedManager.getCurrentPodcast();
                if (!podcast) {
                    std::cout << "No podcast selected. Use 'next' or 'previous' to select one.\n";
                    return;
                }
                
                std::cout << "Loading latest episode from " << podcast->name << "...\n";
                auto episode = feedManager.getLatestEpisode(*podcast);
                if (episode) {
                    std::cout << "Playing: " << episode->title << "\n";
                    player.play(episode->url);
                } else {
                    std::cout << "Could not load episodes from " << podcast->name << "\n";
                }
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
            auto podcast = feedManager.getCurrentPodcast();
            if (podcast) {
                std::cout << "Current podcast: " << podcast->name << "\n";
            }
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

std::vector<std::string> parseArguments(const std::string& input) {
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string arg;
    
    while (ss >> arg) {
        args.push_back(arg);
    }
    
    return args;
}

int main(int argc, char* argv[]) {
    try {
        Player player;
        FeedManager feedManager;

        // Handle command line arguments if provided
        if (argc > 1) {
            std::string command = argv[1];
            std::vector<std::string> args;
            
            for (int i = 2; i < argc; ++i) {
                args.push_back(argv[i]);
            }
            
            try {
                handleCommand(player, feedManager, command, args);
                
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
                else if (input == "quit") {
                    running = false;
                }
                else {
                    auto parsedArgs = parseArguments(input);
                    if (!parsedArgs.empty()) {
                        std::string command = parsedArgs[0];
                        std::vector<std::string> args(parsedArgs.begin() + 1, parsedArgs.end());
                        handleCommand(player, feedManager, command, args);
                    }
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