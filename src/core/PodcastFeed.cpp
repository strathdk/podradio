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
        std::cerr << "Warning: Invalid URL format: " << cleaned << std::endl;
        return "";
    }

    // Validate scheme
    if (parsed_url->get_protocol() != "http:" && parsed_url->get_protocol() != "https:") {
        std::cerr << "Warning: Invalid URL scheme: " << cleaned << std::endl;
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

    if (!isAudioUrl) {
        std::cout << "Warning: URL may not point to an audio file: " << cleaned << std::endl;
    }

    return full_url;
}

void PodcastFeed::loadFromUrl(const std::string& url) {
    if (url.empty()) {
        throw std::runtime_error("Empty URL provided");
    }

    std::cout << "Fetching podcast feed from: " << url << std::endl;
    
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

        std::cout << "Successfully fetched podcast feed (" << response.text.length() << " bytes)" << std::endl;
        parseFeed(response.text);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during feed fetch: " << e.what() << std::endl;
        throw;
    }
}

std::string PodcastFeed::extractAudioUrl(const pugi::xml_node& item) const {
    std::string audioUrl;
    
    // 1. Try standard enclosure tag (most common for podcasts)
    auto enclosure = item.child("enclosure");
    if (enclosure) {
        const char* type = enclosure.attribute("type").value();
        const char* url = enclosure.attribute("url").value();
        
        if (url && strlen(url) > 0) {
            if (type && (strstr(type, "audio/") || strstr(type, "application/octet-stream"))) {
                audioUrl = cleanAndValidateUrl(url);
                if (!audioUrl.empty()) {
                    std::cout << "Found audio URL in enclosure: " << audioUrl << std::endl;
                    return audioUrl;
                }
            }
        }
    }
    
    // 2. Try media:content tag (Media RSS extension)
    auto media_content = item.child("media:content");
    if (media_content) {
        const char* type = media_content.attribute("type").value();
        const char* url = media_content.attribute("url").value();
        
        if (url && strlen(url) > 0 && type && strstr(type, "audio/")) {
            audioUrl = cleanAndValidateUrl(url);
            if (!audioUrl.empty()) {
                std::cout << "Found audio URL in media:content: " << audioUrl << std::endl;
                return audioUrl;
            }
        }
    }
    
    // 3. Try link tag if it points to audio
    auto link = item.child("link");
    if (link && link.text()) {
        std::string linkUrl = cleanAndValidateUrl(link.text().get());
        if (!linkUrl.empty()) {
            // Check if it's likely an audio file
            std::vector<std::string> audioExts = {".mp3", ".m4a", ".wav", ".ogg", ".aac"};
            for (const auto& ext : audioExts) {
                if (linkUrl.find(ext) != std::string::npos) {
                    std::cout << "Found audio URL in link: " << linkUrl << std::endl;
                    return linkUrl;
                }
            }
        }
    }
    
    // 4. Try guid if it's a permalink and looks like a URL
    auto guid = item.child("guid");
    if (guid && guid.text()) {
        std::string guidStr = guid.text().get();
        if (guidStr.find("http") == 0) {
            audioUrl = cleanAndValidateUrl(guidStr);
            if (!audioUrl.empty()) {
                std::cout << "Found audio URL in guid: " << audioUrl << std::endl;
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

    std::cout << "Parsing podcast feed: " << title_ << std::endl;

    // Parse episodes (items)
    int episode_count = 0;
    int episodes_with_audio = 0;

    for (auto item : channel.children("item")) {
        episode_count++;
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
        
        std::cout << "Processing episode: " << episode.title << std::endl;
        
        // Extract audio URL
        std::string audioUrl = extractAudioUrl(item);
        
        if (!audioUrl.empty()) {
            episode.url = audioUrl;
            episodes_.push_back(episode);
            episodes_with_audio++;
        } else {
            std::cout << "Warning: No valid audio URL found for episode: " << episode.title << std::endl;
        }
    }

    std::cout << "Found " << episodes_with_audio << " episodes with audio URLs out of " << episode_count << " total episodes" << std::endl;
    
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