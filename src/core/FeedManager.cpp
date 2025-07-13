#include "core/FeedManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace podradio {
namespace core {

FeedManager::FeedManager(const std::string& storageFile) 
    : currentIndex_(0), storageFile_(storageFile) {
    load();
}

FeedManager::~FeedManager() {
    save();
}

bool FeedManager::addPodcast(const std::string& name, const std::string& feedUrl, const std::string& description) {
    if (name.empty() || feedUrl.empty()) {
        return false;
    }
    
    // Check if podcast already exists
    for (const auto& sub : subscriptions_) {
        if (sub.feedUrl == feedUrl || sub.name == name) {
            std::cout << "Podcast with this name or URL already exists\n";
            return false;
        }
    }
    
    // Create new subscription
    Subscription newSub(name, feedUrl, description);
    subscriptions_.push_back(newSub);
    
    // If this is the first subscription, set it as current
    if (subscriptions_.size() == 1) {
        currentIndex_ = 0;
    }
    
    save();
    std::cout << "Added podcast: " << name << "\n";
    return true;
}

bool FeedManager::removePodcast(const std::string& identifier) {
    int index = findSubscriptionIndex(identifier);
    if (index == -1) {
        std::cout << "Podcast not found: " << identifier << "\n";
        return false;
    }
    
    std::string name = subscriptions_[index].name;
    subscriptions_.erase(subscriptions_.begin() + index);
    
    // Adjust current index if necessary
    if (currentIndex_ >= static_cast<int>(subscriptions_.size())) {
        currentIndex_ = subscriptions_.empty() ? 0 : subscriptions_.size() - 1;
    } else if (currentIndex_ > index) {
        currentIndex_--;
    }
    
    ensureValidIndex();
    save();
    std::cout << "Removed podcast: " << name << "\n";
    return true;
}

std::vector<Subscription> FeedManager::getSubscriptions() const {
    return subscriptions_;
}

std::optional<Subscription> FeedManager::getCurrentPodcast() const {
    if (subscriptions_.empty() || currentIndex_ < 0 || currentIndex_ >= static_cast<int>(subscriptions_.size())) {
        return std::nullopt;
    }
    return subscriptions_[currentIndex_];
}

std::optional<Subscription> FeedManager::nextPodcast() {
    if (subscriptions_.empty()) {
        return std::nullopt;
    }
    
    currentIndex_ = (currentIndex_ + 1) % subscriptions_.size();
    save();
    return subscriptions_[currentIndex_];
}

std::optional<Subscription> FeedManager::previousPodcast() {
    if (subscriptions_.empty()) {
        return std::nullopt;
    }
    
    currentIndex_ = (currentIndex_ - 1 + subscriptions_.size()) % subscriptions_.size();
    save();
    return subscriptions_[currentIndex_];
}

bool FeedManager::selectPodcast(const std::string& identifier) {
    int index = findSubscriptionIndex(identifier);
    if (index == -1) {
        return false;
    }
    
    currentIndex_ = index;
    save();
    return true;
}

std::optional<Episode> FeedManager::getLatestEpisode(const Subscription& subscription) {
    try {
        PodcastFeed feed;
        feed.loadFromUrl(subscription.feedUrl);
        
        if (feed.getEpisodes().empty()) {
            return std::nullopt;
        }
        
        return feed.getLatestEpisode();
    } catch (const std::exception& e) {
        std::cerr << "Error loading episodes for " << subscription.name << ": " << e.what() << "\n";
        return std::nullopt;
    }
}

void FeedManager::save() {
    try {
        nlohmann::json j;
        j["currentIndex"] = currentIndex_;
        j["subscriptions"] = nlohmann::json::array();
        
        for (const auto& sub : subscriptions_) {
            j["subscriptions"].push_back(sub.toJson());
        }
        
        std::ofstream file(storageFile_);
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        } else {
            std::cerr << "Error: Could not open file for writing: " << storageFile_ << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving subscriptions: " << e.what() << "\n";
    }
}

void FeedManager::load() {
    try {
        std::ifstream file(storageFile_);
        if (!file.is_open()) {
            // File doesn't exist yet, start with empty subscriptions
            subscriptions_.clear();
            currentIndex_ = 0;
            return;
        }
        
        nlohmann::json j;
        file >> j;
        file.close();
        
        subscriptions_.clear();
        
        if (j.contains("currentIndex")) {
            currentIndex_ = j["currentIndex"].get<int>();
        }
        
        if (j.contains("subscriptions") && j["subscriptions"].is_array()) {
            for (const auto& subJson : j["subscriptions"]) {
                try {
                    subscriptions_.push_back(Subscription::fromJson(subJson));
                } catch (const std::exception& e) {
                    std::cerr << "Error loading subscription: " << e.what() << "\n";
                }
            }
        }
        
        ensureValidIndex();
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading subscriptions: " << e.what() << "\n";
        subscriptions_.clear();
        currentIndex_ = 0;
    }
}

int FeedManager::findSubscriptionIndex(const std::string& identifier) const {
    for (size_t i = 0; i < subscriptions_.size(); ++i) {
        if (subscriptions_[i].name == identifier || 
            subscriptions_[i].feedUrl == identifier ||
            subscriptions_[i].id == identifier) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FeedManager::ensureValidIndex() {
    if (subscriptions_.empty()) {
        currentIndex_ = 0;
    } else if (currentIndex_ < 0) {
        currentIndex_ = 0;
    } else if (currentIndex_ >= static_cast<int>(subscriptions_.size())) {
        currentIndex_ = subscriptions_.size() - 1;
    }
}

} // namespace core
} // namespace podradio 