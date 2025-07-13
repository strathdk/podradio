#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

namespace podradio {
namespace core {

struct Subscription {
    std::string id;
    std::string name;
    std::string feedUrl;
    std::string description;
    std::chrono::system_clock::time_point lastUpdated;
    bool enabled;

    // Constructor
    Subscription() : enabled(true) {}
    
    Subscription(const std::string& name, const std::string& feedUrl, const std::string& description = "")
        : name(name), feedUrl(feedUrl), description(description), enabled(true) {
        
        // Generate a simple ID based on feed URL
        id = generateId(feedUrl);
        lastUpdated = std::chrono::system_clock::now();
    }

    // Generate a simple ID from the feed URL
    static std::string generateId(const std::string& feedUrl) {
        std::hash<std::string> hasher;
        return std::to_string(hasher(feedUrl));
    }

    // JSON serialization
    nlohmann::json toJson() const {
        return nlohmann::json{
            {"id", id},
            {"name", name},
            {"feedUrl", feedUrl},
            {"description", description},
            {"lastUpdated", std::chrono::duration_cast<std::chrono::seconds>(lastUpdated.time_since_epoch()).count()},
            {"enabled", enabled}
        };
    }

    // JSON deserialization
    static Subscription fromJson(const nlohmann::json& j) {
        Subscription sub;
        sub.id = j.at("id").get<std::string>();
        sub.name = j.at("name").get<std::string>();
        sub.feedUrl = j.at("feedUrl").get<std::string>();
        sub.description = j.at("description").get<std::string>();
        sub.enabled = j.at("enabled").get<bool>();
        
        // Convert timestamp back to time_point
        auto timestamp = j.at("lastUpdated").get<int64_t>();
        sub.lastUpdated = std::chrono::system_clock::from_time_t(timestamp);
        
        return sub;
    }

    // Comparison operator
    bool operator==(const Subscription& other) const {
        return id == other.id;
    }
};

} // namespace core
} // namespace podradio 