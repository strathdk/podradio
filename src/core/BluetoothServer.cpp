#include "core/BluetoothServer.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <chrono>

namespace podradio {
namespace core {

BluetoothServer::BluetoothServer(FeedManager& feedManager, Player& player, int port)
    : feedManager_(feedManager), player_(player), port_(port),
      serviceName_("PodRadio Control"), serviceDescription_("PodRadio Bluetooth Control Service"),
      serverSocket_(-1), sdpSession_(nullptr) {
}

BluetoothServer::~BluetoothServer() {
    stop();
}

bool BluetoothServer::start() {
    if (running_) {
        std::cerr << "Bluetooth server is already running" << std::endl;
        return false;
    }

    // Create RFCOMM socket
    serverSocket_ = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create Bluetooth socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set socket options for reuse
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return false;
    }

    // Bind socket to RFCOMM port
    struct sockaddr_rc addr = {0};
    addr.rc_family = AF_BLUETOOTH;
    bdaddr_t any_addr = *BDADDR_ANY;
    addr.rc_bdaddr = any_addr;
    addr.rc_channel = (uint8_t)port_;

    if (bind(serverSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind Bluetooth socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return false;
    }

    // Listen for connections
    if (listen(serverSocket_, 5) < 0) {
        std::cerr << "Failed to listen on Bluetooth socket: " << strerror(errno) << std::endl;
        close(serverSocket_);
        return false;
    }

    // Register service with SDP
    if (!registerService()) {
        std::cerr << "Failed to register Bluetooth service" << std::endl;
        close(serverSocket_);
        return false;
    }

    running_ = true;
    
    // Start server and cleanup threads
    serverThread_ = std::thread(&BluetoothServer::serverLoop, this);
    cleanupThread_ = std::thread(&BluetoothServer::cleanupDisconnectedClients, this);

    std::cout << "Bluetooth server started on channel " << port_ << std::endl;
    return true;
}

void BluetoothServer::stop() {
    if (!running_) return;

    running_ = false;

    // Close server socket
    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }

    // Disconnect all clients
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : connectedClients_) {
            client->connected = false;
            close(client->socket);
            if (client->clientThread.joinable()) {
                client->clientThread.join();
            }
        }
        connectedClients_.clear();
    }

    // Wait for threads to finish
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }

    // Unregister service
    unregisterService();

    std::cout << "Bluetooth server stopped" << std::endl;
}

std::vector<std::string> BluetoothServer::getConnectedClients() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::vector<std::string> clients;
    for (const auto& client : connectedClients_) {
        if (client->connected) {
            clients.push_back(client->address);
        }
    }
    return clients;
}

void BluetoothServer::serverLoop() {
    while (running_) {
        struct sockaddr_rc clientAddr = {0};
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
            }
            continue;
        }

        // Get client address
        char addr[1024] = {0};
        ba2str(&clientAddr.rc_bdaddr, addr);
        std::string clientAddress(addr);

        std::cout << "Client connected: " << clientAddress << std::endl;

        // Create client object
        auto client = std::make_shared<BluetoothClient>(clientSocket, clientAddress);
        
        // Add to connected clients
        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            connectedClients_.push_back(client);
        }

        // Start client handler thread
        client->clientThread = std::thread(&BluetoothServer::handleClient, this, client);

        // Notify connection
        if (onClientConnected_) {
            onClientConnected_(clientAddress);
        }
    }
}

void BluetoothServer::handleClient(std::shared_ptr<BluetoothClient> client) {
    std::string buffer;
    char readBuffer[1024];
    
    while (client->connected && running_) {
        int bytesRead = read(client->socket, readBuffer, sizeof(readBuffer) - 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0) {
                std::cerr << "Error reading from client " << client->address << ": " << strerror(errno) << std::endl;
            }
            break;
        }
        
        readBuffer[bytesRead] = '\0';
        buffer += readBuffer;
        
        // Process complete JSON messages (terminated by newline)
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string message = buffer.substr(0, pos);
            buffer = buffer.substr(pos + 1);
            
            if (!message.empty()) {
                try {
                    nlohmann::json response = handleCommand(message, client->address);
                    sendResponse(client->socket, response);
                    
                    if (onCommandReceived_) {
                        onCommandReceived_(client->address, message);
                    }
                } catch (const std::exception& e) {
                    nlohmann::json errorResponse = createErrorResponse(
                        "Invalid command", e.what());
                    sendResponse(client->socket, errorResponse);
                }
            }
        }
    }
    
    // Client disconnected
    client->connected = false;
    close(client->socket);
    
    if (onClientDisconnected_) {
        onClientDisconnected_(client->address);
    }
    
    std::cout << "Client disconnected: " << client->address << std::endl;
}

void BluetoothServer::cleanupDisconnectedClients() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = connectedClients_.begin();
        while (it != connectedClients_.end()) {
            if (!(*it)->connected) {
                if ((*it)->clientThread.joinable()) {
                    (*it)->clientThread.join();
                }
                it = connectedClients_.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool BluetoothServer::registerService() {
    // Create SDP session
    bdaddr_t any_addr = *BDADDR_ANY;
    bdaddr_t local_addr = *BDADDR_LOCAL;
    sdpSession_ = sdp_connect(&any_addr, &local_addr, SDP_RETRY_IF_BUSY);
    if (!sdpSession_) {
        std::cerr << "Failed to create SDP session" << std::endl;
        return false;
    }

    // Create service record
    sdp_record_t* record = sdp_record_alloc();
    if (!record) {
        sdp_close(sdpSession_);
        return false;
    }

    // Set service class
    uuid_t svc_uuid;
    sdp_uuid16_create(&svc_uuid, SERIAL_PORT_SVCLASS_ID);
    
    sdp_list_t* svc_class = sdp_list_append(NULL, &svc_uuid);
    sdp_set_service_classes(record, svc_class);

    // Set protocol descriptor
    uuid_t l2cap_uuid, rfcomm_uuid;
    sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
    sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);

    sdp_list_t* l2cap_list = sdp_list_append(NULL, &l2cap_uuid);
    sdp_list_t* rfcomm_list = sdp_list_append(NULL, &rfcomm_uuid);
    
    sdp_data_t* channel = sdp_data_alloc(SDP_UINT8, &port_);
    rfcomm_list = sdp_list_append(rfcomm_list, channel);
    
    sdp_list_t* proto_list = sdp_list_append(NULL, l2cap_list);
    proto_list = sdp_list_append(proto_list, rfcomm_list);
    
    sdp_list_t* access_proto_list = sdp_list_append(NULL, proto_list);
    sdp_set_access_protos(record, access_proto_list);

    // Set service name and description
    sdp_set_info_attr(record, serviceName_.c_str(), "PodRadio", serviceDescription_.c_str());

    // Register service
    if (sdp_record_register(sdpSession_, record, 0) < 0) {
        std::cerr << "Failed to register service record" << std::endl;
        sdp_record_free(record);
        sdp_close(sdpSession_);
        return false;
    }

    // Cleanup
    sdp_data_free(channel);
    sdp_list_free(l2cap_list, NULL);
    sdp_list_free(rfcomm_list, NULL);
    sdp_list_free(proto_list, NULL);
    sdp_list_free(access_proto_list, NULL);
    sdp_list_free(svc_class, NULL);
    sdp_record_free(record);

    return true;
}

void BluetoothServer::unregisterService() {
    if (sdpSession_) {
        sdp_close(sdpSession_);
        sdpSession_ = nullptr;
    }
}

nlohmann::json BluetoothServer::handleCommand(const std::string& command, const std::string& clientAddress) {
    try {
        nlohmann::json request = nlohmann::json::parse(command);
        
        if (!request.contains("action")) {
            return createErrorResponse("Missing 'action' field");
        }
        
        std::string action = request["action"];
        
        if (action == "add_podcast") {
            return handleAddPodcast(request);
        } else if (action == "remove_podcast") {
            return handleRemovePodcast(request);
        } else if (action == "list_podcasts") {
            return handleListPodcasts(request);
        } else if (action == "play_podcast") {
            return handlePlayPodcast(request);
        } else if (action == "player_control") {
            return handlePlayerControl(request);
        } else if (action == "get_status") {
            return handleGetStatus(request);
        } else if (action == "navigate_podcasts") {
            return handleNavigatePodcasts(request);
        } else {
            return createErrorResponse("Unknown action: " + action);
        }
        
    } catch (const nlohmann::json::exception& e) {
        return createErrorResponse("JSON parsing error", e.what());
    }
}

nlohmann::json BluetoothServer::handleAddPodcast(const nlohmann::json& request) {
    if (!request.contains("name") || !request.contains("url")) {
        return createErrorResponse("Missing 'name' or 'url' field");
    }
    
    std::string name = request["name"];
    std::string url = request["url"];
    std::string description = request.value("description", "");
    
    bool success = feedManager_.addPodcast(name, url, description);
    
    if (success) {
        nlohmann::json data;
        data["message"] = "Podcast added successfully";
        data["name"] = name;
        data["url"] = url;
        return createSuccessResponse(data);
    } else {
        return createErrorResponse("Failed to add podcast");
    }
}

nlohmann::json BluetoothServer::handleRemovePodcast(const nlohmann::json& request) {
    if (!request.contains("identifier")) {
        return createErrorResponse("Missing 'identifier' field");
    }
    
    std::string identifier = request["identifier"];
    bool success = feedManager_.removePodcast(identifier);
    
    if (success) {
        nlohmann::json data;
        data["message"] = "Podcast removed successfully";
        data["identifier"] = identifier;
        return createSuccessResponse(data);
    } else {
        return createErrorResponse("Failed to remove podcast: not found");
    }
}

nlohmann::json BluetoothServer::handleListPodcasts(const nlohmann::json& request) {
    auto subscriptions = feedManager_.getSubscriptions();
    auto currentPodcast = feedManager_.getCurrentPodcast();
    
    nlohmann::json data;
    data["podcasts"] = nlohmann::json::array();
    data["current_index"] = feedManager_.getCurrentIndex();
    
    for (size_t i = 0; i < subscriptions.size(); ++i) {
        const auto& sub = subscriptions[i];
        nlohmann::json podcast;
        podcast["index"] = i;
        podcast["name"] = sub.name;
        podcast["url"] = sub.feedUrl;
        podcast["description"] = sub.description;
        podcast["enabled"] = sub.enabled;
        podcast["is_current"] = (currentPodcast && currentPodcast->id == sub.id);
        
        data["podcasts"].push_back(podcast);
    }
    
    return createSuccessResponse(data);
}

nlohmann::json BluetoothServer::handlePlayPodcast(const nlohmann::json& request) {
    try {
        if (request.contains("url")) {
            // Play from direct URL
            std::string url = request["url"];
            player_.play(url);
            
            nlohmann::json data;
            data["message"] = "Playing from URL";
            data["url"] = url;
            return createSuccessResponse(data);
        } else {
            // Play current podcast's latest episode
            auto podcast = feedManager_.getCurrentPodcast();
            if (!podcast) {
                return createErrorResponse("No podcast selected");
            }
            
            auto episode = feedManager_.getLatestEpisode(*podcast);
            if (!episode) {
                return createErrorResponse("Could not load episodes");
            }
            
            player_.play(episode->url);
            
            nlohmann::json data;
            data["message"] = "Playing podcast episode";
            data["podcast"] = podcast->name;
            data["episode"] = episode->title;
            data["url"] = episode->url;
            return createSuccessResponse(data);
        }
    } catch (const std::exception& e) {
        return createErrorResponse("Playback failed", e.what());
    }
}

nlohmann::json BluetoothServer::handlePlayerControl(const nlohmann::json& request) {
    if (!request.contains("command")) {
        return createErrorResponse("Missing 'command' field");
    }
    
    std::string command = request["command"];
    
    try {
        if (command == "pause") {
            player_.pause();
            nlohmann::json data;
            data["message"] = "Playback paused";
            return createSuccessResponse(data);
        } else if (command == "stop") {
            player_.stop();
            nlohmann::json data;
            data["message"] = "Playback stopped";
            return createSuccessResponse(data);
        } else {
            return createErrorResponse("Unknown player command: " + command);
        }
    } catch (const std::exception& e) {
        return createErrorResponse("Player control failed", e.what());
    }
}

nlohmann::json BluetoothServer::handleGetStatus(const nlohmann::json& request) {
    nlohmann::json data;
    
    // Player status
    data["player"]["playing"] = player_.isPlaying();
    
    // Current podcast
    auto currentPodcast = feedManager_.getCurrentPodcast();
    if (currentPodcast) {
        data["current_podcast"]["name"] = currentPodcast->name;
        data["current_podcast"]["url"] = currentPodcast->feedUrl;
        data["current_podcast"]["description"] = currentPodcast->description;
    } else {
        data["current_podcast"] = nullptr;
    }
    
    // Subscription count
    data["subscription_count"] = feedManager_.getSubscriptionCount();
    data["current_index"] = feedManager_.getCurrentIndex();
    
    // Connected clients
    data["connected_clients"] = getConnectedClients().size();
    
    return createSuccessResponse(data);
}

nlohmann::json BluetoothServer::handleNavigatePodcasts(const nlohmann::json& request) {
    if (!request.contains("direction")) {
        return createErrorResponse("Missing 'direction' field");
    }
    
    std::string direction = request["direction"];
    
    try {
        std::optional<Subscription> podcast;
        
        if (direction == "next") {
            podcast = feedManager_.nextPodcast();
        } else if (direction == "previous") {
            podcast = feedManager_.previousPodcast();
        } else {
            return createErrorResponse("Invalid direction: " + direction);
        }
        
        if (podcast) {
            nlohmann::json data;
            data["message"] = "Selected " + direction + " podcast";
            data["podcast"]["name"] = podcast->name;
            data["podcast"]["url"] = podcast->feedUrl;
            data["podcast"]["description"] = podcast->description;
            data["index"] = feedManager_.getCurrentIndex();
            return createSuccessResponse(data);
        } else {
            return createErrorResponse("No podcasts available");
        }
    } catch (const std::exception& e) {
        return createErrorResponse("Navigation failed", e.what());
    }
}

void BluetoothServer::sendResponse(int clientSocket, const nlohmann::json& response) {
    std::string responseStr = response.dump() + "\n";
    
    ssize_t bytesSent = write(clientSocket, responseStr.c_str(), responseStr.length());
    if (bytesSent == -1) {
        std::cerr << "Failed to send response: " << strerror(errno) << std::endl;
    }
}

void BluetoothServer::broadcastMessage(const nlohmann::json& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    for (const auto& client : connectedClients_) {
        if (client->connected) {
            sendResponse(client->socket, message);
        }
    }
}

nlohmann::json BluetoothServer::createErrorResponse(const std::string& error, const std::string& details) {
    nlohmann::json response;
    response["success"] = false;
    response["error"] = error;
    if (!details.empty()) {
        response["details"] = details;
    }
    return response;
}

nlohmann::json BluetoothServer::createSuccessResponse(const nlohmann::json& data) {
    nlohmann::json response;
    response["success"] = true;
    response["data"] = data;
    return response;
}

std::string BluetoothServer::getBluetoothAddress(int socket) {
    struct sockaddr_rc addr;
    socklen_t len = sizeof(addr);
    
    if (getpeername(socket, (struct sockaddr*)&addr, &len) == 0) {
        char str[1024];
        ba2str(&addr.rc_bdaddr, str);
        return std::string(str);
    }
    
    return "unknown";
}

bool BluetoothServer::setSocketNonBlocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;
    
    return fcntl(socket, F_SETFL, flags | O_NONBLOCK) != -1;
}

} // namespace core
} // namespace podradio 