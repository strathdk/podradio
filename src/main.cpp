#include "core/Player.hpp"
#include "core/FeedManager.hpp"
#include "core/Subscription.hpp"
#ifdef ENABLE_BLUETOOTH
#include "core/BluetoothServer.hpp"
#endif
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <memory>

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
#ifdef ENABLE_BLUETOOTH
              << "Bluetooth:\n"
              << "  bluetooth start      - Start Bluetooth server\n"
              << "  bluetooth stop       - Stop Bluetooth server\n"
              << "  bluetooth status     - Show Bluetooth server status\n"
              << "  bluetooth clients    - List connected Bluetooth clients\n\n"
#endif
              << "General:\n"
              << "  help                 - Show this help\n"
              << "  quit                 - Exit program\n\n"
              << "Options:\n"
#ifdef ENABLE_BLUETOOTH
              << "  --bluetooth          - Start with Bluetooth server enabled\n"
              << "  --bt-port <port>     - Set Bluetooth RFCOMM port (default: 1)\n"
#endif
              << "\n"
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

#ifdef ENABLE_BLUETOOTH
void handleCommand(Player& player, FeedManager& feedManager, std::shared_ptr<BluetoothServer>& bluetoothServer, const std::string& command, const std::vector<std::string>& args = {})
#else
void handleCommand(Player& player, FeedManager& feedManager, const std::string& command, const std::vector<std::string>& args = {})
#endif {
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
#ifdef ENABLE_BLUETOOTH
        else if (command == "bluetooth") {
            if (args.empty()) {
                std::cout << "Usage: bluetooth <start|stop|status|clients>\n";
                return;
            }
            
            std::string btCommand = args[0];
            if (btCommand == "start") {
                if (!bluetoothServer) {
                    bluetoothServer = std::make_shared<BluetoothServer>(feedManager, player);
                    bluetoothServer->setOnClientConnected([](const std::string& address) {
                        std::cout << "Bluetooth client connected: " << address << std::endl;
                    });
                    bluetoothServer->setOnClientDisconnected([](const std::string& address) {
                        std::cout << "Bluetooth client disconnected: " << address << std::endl;
                    });
                    bluetoothServer->setOnCommandReceived([](const std::string& address, const std::string& command) {
                        std::cout << "Bluetooth command from " << address << ": " << command << std::endl;
                    });
                }
                
                if (bluetoothServer->start()) {
                    std::cout << "Bluetooth server started successfully\n";
                } else {
                    std::cout << "Failed to start Bluetooth server\n";
                }
            }
            else if (btCommand == "stop") {
                if (bluetoothServer) {
                    bluetoothServer->stop();
                    std::cout << "Bluetooth server stopped\n";
                } else {
                    std::cout << "Bluetooth server is not running\n";
                }
            }
            else if (btCommand == "status") {
                if (bluetoothServer && bluetoothServer->isRunning()) {
                    std::cout << "Bluetooth server is running\n";
                    std::cout << "Connected clients: " << bluetoothServer->getConnectedClientCount() << "\n";
                } else {
                    std::cout << "Bluetooth server is not running\n";
                }
            }
            else if (btCommand == "clients") {
                if (bluetoothServer && bluetoothServer->isRunning()) {
                    auto clients = bluetoothServer->getConnectedClients();
                    if (clients.empty()) {
                        std::cout << "No clients connected\n";
                    } else {
                        std::cout << "Connected clients:\n";
                        for (const auto& client : clients) {
                            std::cout << "  " << client << "\n";
                        }
                    }
                } else {
                    std::cout << "Bluetooth server is not running\n";
                }
            }
            else {
                std::cout << "Unknown bluetooth command. Use: start, stop, status, or clients\n";
            }
        }
#endif
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

// Global variables for signal handling
#ifdef ENABLE_BLUETOOTH
std::shared_ptr<BluetoothServer> g_bluetoothServer;
#endif
bool g_running = true;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down...\n";
    g_running = false;
#ifdef ENABLE_BLUETOOTH
    if (g_bluetoothServer) {
        g_bluetoothServer->stop();
    }
#endif
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        Player player;
        FeedManager feedManager;
#ifdef ENABLE_BLUETOOTH
        std::shared_ptr<BluetoothServer> bluetoothServer;
#endif
        
        // Command line argument parsing
        bool enableBluetooth = false;
#ifdef ENABLE_BLUETOOTH
        int bluetoothPort = 1;
#endif
        std::vector<std::string> commands;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
#ifdef ENABLE_BLUETOOTH
            if (arg == "--bluetooth") {
                enableBluetooth = true;
            } else if (arg == "--bt-port" && i + 1 < argc) {
                bluetoothPort = std::stoi(argv[++i]);
            } else
#endif
            if (arg == "--help") {
                printHelp();
                return 0;
            } else {
                commands.push_back(arg);
            }
        }
        
        // Start Bluetooth server if requested
#ifdef ENABLE_BLUETOOTH
        if (enableBluetooth) {
            bluetoothServer = std::make_shared<BluetoothServer>(feedManager, player, bluetoothPort);
            g_bluetoothServer = bluetoothServer;
            
            // Set up event handlers
            bluetoothServer->setOnClientConnected([](const std::string& address) {
                std::cout << "Bluetooth client connected: " << address << std::endl;
            });
            bluetoothServer->setOnClientDisconnected([](const std::string& address) {
                std::cout << "Bluetooth client disconnected: " << address << std::endl;
            });
            bluetoothServer->setOnCommandReceived([](const std::string& address, const std::string& command) {
                std::cout << "Bluetooth command from " << address << ": " << command << std::endl;
            });
            
            if (bluetoothServer->start()) {
                std::cout << "Bluetooth server started on port " << bluetoothPort << std::endl;
            } else {
                std::cerr << "Failed to start Bluetooth server" << std::endl;
                return 1;
            }
        }
#else
        if (enableBluetooth) {
            std::cerr << "Bluetooth support not available in this build" << std::endl;
            std::cerr << "Install BlueZ development libraries and rebuild:" << std::endl;
            std::cerr << "  sudo apt install libbluetooth-dev bluez-dev" << std::endl;
            return 1;
        }
#endif

        // Handle command line arguments if provided
        if (!commands.empty()) {
            std::string command = commands[0];
            std::vector<std::string> args(commands.begin() + 1, commands.end());
            
            try {
#ifdef ENABLE_BLUETOOTH
                handleCommand(player, feedManager, bluetoothServer, command, args);
#else
                handleCommand(player, feedManager, command, args);
#endif
                
                // For play commands, keep the program running until interrupted
                if (command == "play") {
                    std::cout << "Playing... Press Ctrl+C to stop.\n";
                    while (player.isPlaying() && g_running) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                return 1;
            }
            
            // If Bluetooth is enabled, keep running
            if (enableBluetooth) {
                std::cout << "Bluetooth server running. Press Ctrl+C to stop.\n";
                while (g_running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            
            return 0;
        }

        // Interactive mode
        std::cout << "Welcome to PodRadio!\n";
#ifdef ENABLE_BLUETOOTH
        if (enableBluetooth) {
            std::cout << "Bluetooth server is running on port " << bluetoothPort << "\n";
        }
#endif
        printHelp();

        std::string input;

        while (g_running) {
            std::cout << "\nEnter command: ";
            std::getline(std::cin, input);

            try {
                if (input.empty()) {
                    continue;
                }
                else if (input == "quit") {
                    g_running = false;
                }
                else {
                    auto parsedArgs = parseArguments(input);
                    if (!parsedArgs.empty()) {
                        std::string command = parsedArgs[0];
                        std::vector<std::string> args(parsedArgs.begin() + 1, parsedArgs.end());
#ifdef ENABLE_BLUETOOTH
                        handleCommand(player, feedManager, bluetoothServer, command, args);
#else
                        handleCommand(player, feedManager, command, args);
#endif
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }

        // Clean shutdown
#ifdef ENABLE_BLUETOOTH
        if (bluetoothServer) {
            std::cout << "Stopping Bluetooth server...\n";
            bluetoothServer->stop();
        }
#endif

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
} 