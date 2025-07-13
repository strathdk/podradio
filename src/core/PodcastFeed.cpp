#include "core/PodcastFeed.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cpr/cpr.h>
#include <pugixml.hpp>
#include <ada.h>

namespace podradio {
namespace core {

PodcastFeed::PodcastFeed() {
    // No initialization needed
}

std::string PodcastFeed::cleanAndValidateUrl(const std::string& url) const {
    if (url.empty()) {
        return "";
    }

    // Trim whitespace
    std::string cleaned = url;
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);

    if (cleaned.empty()) {
        return "";
    }

    // Use ada-url for proper URL parsing and validation
    auto parsed_url = ada::parse<ada::url>(cleaned);
    if (!parsed_url) {
        return "";
    }

    // Validate scheme
    if (parsed_url->get_protocol() != "http:" && parsed_url->get_protocol() != "https:") {
        return "";
    }

    // Check for common audio file extensions or streaming patterns
    std::string path = std::string(parsed_url->get_pathname());
    std::string query = std::string(parsed_url->get_search());
    std::string full_url = parsed_url->get_href();
    
    std::vector<std::string> audioExts = {".mp3", ".m4a", ".wav", ".ogg", ".aac", ".mp4"};
    std::vector<std::string> streamingPatterns = {"podtrac.com", "megaphone.fm", "libsyn.com", "soundcloud.com"};
    
    bool isAudioUrl = false;
    
    // Check for audio file extensions
    for (const auto& ext : audioExts) {
        if (path.find(ext) != std::string::npos || query.find(ext) != std::string::npos) {
            isAudioUrl = true;
            break;
        }
    }
    
    // Check for known streaming patterns
    if (!isAudioUrl) {
        for (const auto& pattern : streamingPatterns) {
            if (full_url.find(pattern) != std::string::npos) {
                isAudioUrl = true;
                break;
            }
        }
    }

    return full_url;
}

void PodcastFeed::loadFromUrl(const std::string& url) {
    if (url.empty()) {
        throw std::runtime_error("Empty URL provided");
    }

    try {
        // Use cpr for HTTP request with proper headers
        auto response = cpr::Get(
            cpr::Url{url},
            cpr::Header{
                {"User-Agent", "Mozilla/5.0 (compatible; PodRadio/1.0)"},
                {"Accept", "application/rss+xml, application/xml, text/xml"},
                {"Accept-Encoding", "gzip, deflate"}
            },
            cpr::Timeout{30000}, // 30 seconds
            cpr::Redirect{50L},
            cpr::VerifySsl{true}
        );
        
        if (response.status_code != 200) {
            std::stringstream err;
            err << "Failed to fetch podcast feed: HTTP " << response.status_code;
            if (response.error.code != cpr::ErrorCode::OK) {
                err << " (" << response.error.message << ")";
            }
            throw std::runtime_error(err.str());
        }

        if (response.text.empty()) {
            throw std::runtime_error("Empty response received from feed URL");
        }

        // Validate content type
        auto content_type_it = response.header.find("content-type");
        if (content_type_it != response.header.end()) {
            std::string type = content_type_it->second;
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            if (type.find("xml") == std::string::npos && 
                type.find("rss") == std::string::npos && 
                type.find("atom") == std::string::npos) {
                std::cout << "Warning: Unexpected content type: " << type << std::endl;
            }
        }

        parseFeed(response.text);
        
    } catch (const std::exception& e) {
        std::cerr << "Error fetching feed: " << e.what() << std::endl;
        throw;
    }
}

std::string PodcastFeed::extractAudioUrl(const pugi::xml_node& item) const {
    std::string audioUrl;
    
    // First priority: Look for enclosure tag with audio type
    auto enclosure = item.child("enclosure");
    if (enclosure) {
        std::string url = enclosure.attribute("url").value();
        std::string type = enclosure.attribute("type").value();
        
        // Check if it's an audio type
        if (type.find("audio/") == 0 || type.find("application/octet-stream") == 0) {
            audioUrl = cleanAndValidateUrl(url);
            if (!audioUrl.empty()) {
                return audioUrl;
            }
        }
    }
    
    // Second priority: Look for media:content tag with audio URL
    auto media_content = item.child("media:content");
    if (media_content) {
        std::string url = media_content.attribute("url").value();
        std::string type = media_content.attribute("type").value();
        
        if (type.find("audio/") == 0 || type.find("application/octet-stream") == 0) {
            audioUrl = cleanAndValidateUrl(url);
            if (!audioUrl.empty()) {
                return audioUrl;
            }
        }
    }
    
    // Third priority: Look for link tag that might contain audio URL
    auto link = item.child("link");
    if (link && link.text()) {
        std::string linkUrl = link.text().get();
        if (linkUrl.find("http") == 0) {
            audioUrl = cleanAndValidateUrl(linkUrl);
            if (!audioUrl.empty()) {
                return audioUrl;
            }
        }
    }
    
    // Fourth priority: Look for audio URL in guid
    auto guid = item.child("guid");
    if (guid && guid.text()) {
        std::string guidStr = guid.text().get();
        if (guidStr.find("http") == 0) {
            audioUrl = cleanAndValidateUrl(guidStr);
            if (!audioUrl.empty()) {
                return audioUrl;
            }
        }
    }
    
    return "";
}

void PodcastFeed::parseFeed(const std::string& xml) {
    pugi::xml_parse_result result = doc_.load_string(xml.c_str());
    if (!result) {
        throw std::runtime_error("Failed to parse XML feed: " + std::string(result.description()));
    }

    // Clear existing data
    episodes_.clear();
    title_.clear();
    description_.clear();
    link_.clear();
    language_.clear();

    // Find the channel - try both RSS and Atom formats
    pugi::xml_node channel = doc_.child("rss").child("channel");
    if (!channel) {
        channel = doc_.child("feed"); // Try Atom format
    }
    
    if (!channel) {
        throw std::runtime_error("Invalid podcast feed format: no channel or feed element found");
    }

    // Parse channel metadata
    if (auto title = channel.child("title")) {
        title_ = title.text().get();
    }
    
    if (auto description = channel.child("description")) {
        description_ = description.text().get();
    }
    
    if (auto link = channel.child("link")) {
        link_ = link.text().get();
    }
    
    if (auto language = channel.child("language")) {
        language_ = language.text().get();
    }

    // Parse episodes (items)
    int episodes_with_audio = 0;

    for (auto item : channel.children("item")) {
        Episode episode;
        
        // Parse episode metadata
        if (auto title = item.child("title")) {
            episode.title = title.text().get();
        }
        
        if (auto description = item.child("description")) {
            episode.description = description.text().get();
        }
        
        if (auto pubDate = item.child("pubDate")) {
            episode.pubDate = pubDate.text().get();
        }
        
        if (auto duration = item.child("itunes:duration")) {
            episode.duration = duration.text().get();
        }
        
        if (auto guid = item.child("guid")) {
            episode.guid = guid.text().get();
        }
        
        // Extract audio URL
        std::string audioUrl = extractAudioUrl(item);
        
        if (!audioUrl.empty()) {
            episode.url = audioUrl;
            episodes_.push_back(episode);
            episodes_with_audio++;
        }
    }

    if (episodes_.empty()) {
        throw std::runtime_error("No episodes with valid audio URLs found in feed");
    }
}

Episode PodcastFeed::getLatestEpisode() const {
    if (episodes_.empty()) {
        throw std::runtime_error("No episodes available");
    }
    // The first episode in an RSS feed is typically the latest one
    return episodes_.front();
}

} // namespace core
} // namespace podradio 