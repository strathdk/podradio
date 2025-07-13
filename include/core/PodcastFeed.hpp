#pragma once

#include <string>
#include <vector>
#include <memory>
#include <pugixml.hpp>

namespace podradio {
namespace core {

struct Episode {
    std::string title;
    std::string description;
    std::string url;
    std::string pubDate;
    std::string duration;
    std::string guid;
};

class PodcastFeed {
public:
    PodcastFeed();
    ~PodcastFeed() = default;

    // Load and parse a podcast feed from a URL
    void loadFromUrl(const std::string& url);

    // Get the latest episode
    Episode getLatestEpisode() const;

    // Get all episodes
    const std::vector<Episode>& getEpisodes() const { return episodes_; }

    // Get feed metadata
    std::string getTitle() const { return title_; }
    std::string getDescription() const { return description_; }
    std::string getLink() const { return link_; }
    std::string getLanguage() const { return language_; }

private:
    void parseFeed(const std::string& xml);
    std::string extractAudioUrl(const pugi::xml_node& item) const;
    std::string cleanAndValidateUrl(const std::string& url) const;
    
    std::string title_;
    std::string description_;
    std::string link_;
    std::string language_;
    std::vector<Episode> episodes_;
    pugi::xml_document doc_;
};

} // namespace core
} // namespace podradio 