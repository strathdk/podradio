#pragma once

#include "core/Subscription.hpp"
#include "core/PodcastFeed.hpp"
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace podradio {
namespace core {

class FeedManager {
public:
    FeedManager(const std::string& storageFile = "podcasts.json");
    ~FeedManager();

    // Subscription management
    bool addPodcast(const std::string& name, const std::string& feedUrl, const std::string& description = "");
    bool removePodcast(const std::string& identifier); // Can be name or feed URL
    std::vector<Subscription> getSubscriptions() const;
    
    // Navigation
    std::optional<Subscription> getCurrentPodcast() const;
    std::optional<Subscription> nextPodcast();
    std::optional<Subscription> previousPodcast();
    bool selectPodcast(const std::string& identifier); // Can be name or feed URL
    
    // Playback helper
    std::optional<Episode> getLatestEpisode(const Subscription& subscription);
    
    // Persistence
    void save();
    void load();
    
    // Status
    int getCurrentIndex() const { return currentIndex_; }
    int getSubscriptionCount() const { return subscriptions_.size(); }
    
private:
    std::vector<Subscription> subscriptions_;
    int currentIndex_;
    std::string storageFile_;
    
    // Helper methods
    int findSubscriptionIndex(const std::string& identifier) const;
    void ensureValidIndex();
};

} // namespace core
} // namespace podradio 