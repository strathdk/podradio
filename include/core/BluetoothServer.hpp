#pragma once

#include "core/FeedManager.hpp"
#include "core/Player.hpp"
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

extern "C" {
#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <bluetooth/rfcomm.h>
}

namespace podradio {
namespace core {

struct BluetoothClient {
    int socket;
    std::string address;
    std::thread clientThread;
    std::atomic<bool> connected{true};
    
    BluetoothClient(int sock, const std::string& addr) 
        : socket(sock), address(addr) {}
};

class BluetoothServer {
public:
    BluetoothServer(FeedManager& feedManager, Player& player, int port = 1);
    ~BluetoothServer();
    
    // Server control
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void setServiceName(const std::string& name) { serviceName_ = name; }
    void setServiceDescription(const std::string& desc) { serviceDescription_ = desc; }
    
    // Client management
    int getConnectedClientCount() const { return connectedClients_.size(); }
    std::vector<std::string> getConnectedClients() const;
    
    // Callbacks for events
    void setOnClientConnected(std::function<void(const std::string&)> callback) {
        onClientConnected_ = callback;
    }
    void setOnClientDisconnected(std::function<void(const std::string&)> callback) {
        onClientDisconnected_ = callback;
    }
    void setOnCommandReceived(std::function<void(const std::string&, const std::string&)> callback) {
        onCommandReceived_ = callback;
    }

private:
    // Core references
    FeedManager& feedManager_;
    Player& player_;
    
    // Bluetooth configuration
    int port_;
    std::string serviceName_;
    std::string serviceDescription_;
    
    // Server state
    std::atomic<bool> running_{false};
    int serverSocket_;
    sdp_session_t* sdpSession_;
    
    // Client management
    std::vector<std::shared_ptr<BluetoothClient>> connectedClients_;
    std::mutex clientsMutex_;
    
    // Threading
    std::thread serverThread_;
    std::thread cleanupThread_;
    
    // Event callbacks
    std::function<void(const std::string&)> onClientConnected_;
    std::function<void(const std::string&)> onClientDisconnected_;
    std::function<void(const std::string&, const std::string&)> onCommandReceived_;
    
    // Private methods
    void serverLoop();
    void handleClient(std::shared_ptr<BluetoothClient> client);
    void cleanupDisconnectedClients();
    
    // SDP (Service Discovery Protocol) methods
    bool registerService();
    void unregisterService();
    
    // Protocol handlers
    nlohmann::json handleCommand(const std::string& command, const std::string& clientAddress);
    nlohmann::json handleAddPodcast(const nlohmann::json& request);
    nlohmann::json handleRemovePodcast(const nlohmann::json& request);
    nlohmann::json handleListPodcasts(const nlohmann::json& request);
    nlohmann::json handlePlayPodcast(const nlohmann::json& request);
    nlohmann::json handlePlayerControl(const nlohmann::json& request);
    nlohmann::json handleGetStatus(const nlohmann::json& request);
    nlohmann::json handleNavigatePodcasts(const nlohmann::json& request);
    
    // Utility methods
    void sendResponse(int clientSocket, const nlohmann::json& response);
    void broadcastMessage(const nlohmann::json& message);
    nlohmann::json createErrorResponse(const std::string& error, const std::string& details = "");
    nlohmann::json createSuccessResponse(const nlohmann::json& data = nlohmann::json::object());
    
    // Bluetooth utility methods
    std::string getBluetoothAddress(int socket);
    bool setSocketNonBlocking(int socket);
};

} // namespace core
} // namespace podradio 