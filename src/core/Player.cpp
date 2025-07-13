#include "core/Player.hpp"
#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <cpr/cpr.h>
#include <ada.h>

namespace podradio {
namespace core {

// Helper function to clean and validate URLs using ada-url
static std::string cleanAndValidateUrl(const std::string& url) {
    std::cout << "DEBUG: Input URL: [" << url << "]" << std::endl;
    
    if (url.empty()) {
        std::cout << "DEBUG: URL is empty" << std::endl;
        return "";
    }

    // Trim whitespace
    std::string cleaned = url;
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);

    std::cout << "DEBUG: After trimming: [" << cleaned << "]" << std::endl;

    if (cleaned.empty()) {
        std::cout << "DEBUG: URL is empty after trimming" << std::endl;
        return "";
    }

    // Basic URL validation without ada-url for now
    if (cleaned.find("http://") != 0 && cleaned.find("https://") != 0) {
        std::cout << "DEBUG: Invalid protocol in URL" << std::endl;
        return "";
    }

    std::cout << "DEBUG: URL validation passed: [" << cleaned << "]" << std::endl;
    return cleaned;
}

// Helper function to resolve URL redirects and get final media URL
std::string Player::resolveMediaUrl(const std::string& url) {
    std::cout << "DEBUG resolveMediaUrl: Input URL: [" << url << "]" << std::endl;
    
    // Clean and validate the URL
    std::string cleaned_url = cleanAndValidateUrl(url);
    
    std::cout << "DEBUG resolveMediaUrl: Cleaned URL: [" << cleaned_url << "]" << std::endl;
    
    if (cleaned_url.empty()) {
        throw std::runtime_error("Invalid or empty URL provided");
    }

    std::cout << "Attempting to resolve URL: " << cleaned_url << std::endl;

    try {
        // Use cpr for HTTP redirect resolution with better configuration
        auto session = cpr::Session{};
        session.SetUrl(cpr::Url{cleaned_url});
        session.SetHeader(cpr::Header{
            {"Accept", "audio/mpeg, audio/mp4, audio/*, application/octet-stream"},
            {"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"},
            {"Accept-Encoding", "gzip, deflate"},
            {"Connection", "keep-alive"},
            {"Cache-Control", "no-cache"}
        });
        session.SetTimeout(cpr::Timeout{30000}); // 30 seconds
        session.SetRedirect(cpr::Redirect{50L, true, false, cpr::PostRedirectFlags::POST_ALL});
        session.SetVerifySsl(cpr::VerifySsl{false}); // For compatibility with some podcast services
        // SSL options removed as they're not compatible with this version of CPR

        // First try HEAD request to avoid downloading content
        std::cout << "Performing HEAD request to resolve redirects..." << std::endl;
        auto head_response = session.Head();
        
        if (head_response.status_code >= 200 && head_response.status_code < 300) {
            std::cout << "HEAD request successful: " << head_response.status_code << std::endl;
            
            // Check content type if available
            auto content_type_it = head_response.header.find("content-type");
            if (content_type_it != head_response.header.end()) {
                std::string content_type = content_type_it->second;
                std::cout << "Content-Type: " << content_type << std::endl;
                
                if (content_type.find("audio/") != std::string::npos ||
                    content_type.find("application/octet-stream") != std::string::npos) {
                    std::cout << "Confirmed audio content type" << std::endl;
                }
            }
            
            // Check content length if available
            auto content_length_it = head_response.header.find("content-length");
            if (content_length_it != head_response.header.end()) {
                std::cout << "Content-Length: " << content_length_it->second << " bytes" << std::endl;
            }
            
            std::cout << "Successfully resolved URL via HEAD request" << std::endl;
            return head_response.url.str();
        }
        
        // If HEAD fails due to SSL issues, try to simplify and retry
        if (head_response.error.code != cpr::ErrorCode::OK && 
            head_response.error.message.find("SSL") != std::string::npos) {
            std::cout << "SSL handshake failed, trying with simplified settings..." << std::endl;
            
            // Try with more relaxed SSL settings
            session.SetVerifySsl(cpr::VerifySsl{false});
            
            auto retry_response = session.Head();
            if (retry_response.status_code >= 200 && retry_response.status_code < 300) {
                std::cout << "HEAD request successful with relaxed SSL: " << retry_response.status_code << std::endl;
                return retry_response.url.str();
            }
        }
        
        // If HEAD fails, try GET request with limited range
        std::cout << "HEAD request failed (code: " << head_response.status_code 
                  << "), attempting GET request with range..." << std::endl;
        
        // Try a small range request first
        session.SetHeader(cpr::Header{
            {"Accept", "audio/mpeg, audio/mp4, audio/*, application/octet-stream"},
            {"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"},
            {"Range", "bytes=0-1023"}  // Just get first 1KB
        });
        
        auto get_response = session.Get();
        
        if (get_response.status_code >= 200 && get_response.status_code < 300) {
            std::cout << "GET request successful: " << get_response.status_code << std::endl;
            std::cout << "Successfully resolved URL via GET request" << std::endl;
            return get_response.url.str();
        }
        
        // If range request fails, try without range
        if (get_response.status_code == 416) { // Range not satisfiable
            std::cout << "Range request not supported, trying without range..." << std::endl;
            session.SetHeader(cpr::Header{
                {"Accept", "audio/mpeg, audio/mp4, audio/*, application/octet-stream"},
                {"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"}
            });
            
            auto simple_response = session.Get();
            if (simple_response.status_code >= 200 && simple_response.status_code < 300) {
                std::cout << "Simple GET request successful: " << simple_response.status_code << std::endl;
                return simple_response.url.str();
            }
        }
        
        // All requests failed, try a fallback approach for tracking URLs
        std::cout << "All standard requests failed, trying fallback approach..." << std::endl;
        
        // For complex tracking URLs, try to extract the final URL from the redirect chain
        // This is a workaround for services that have complex SSL requirements
        try {
            // Create a new session with minimal requirements
            auto fallback_session = cpr::Session{};
            fallback_session.SetUrl(cpr::Url{cleaned_url});
            fallback_session.SetHeader(cpr::Header{
                {"User-Agent", "curl/7.68.0"},
                {"Accept", "*/*"}
            });
            fallback_session.SetTimeout(cpr::Timeout{10000}); // Shorter timeout
            fallback_session.SetRedirect(cpr::Redirect{10L, true, false, cpr::PostRedirectFlags::POST_ALL});
            fallback_session.SetVerifySsl(cpr::VerifySsl{false});
            
            auto fallback_response = fallback_session.Get();
            if (fallback_response.status_code >= 200 && fallback_response.status_code < 400) {
                std::cout << "Fallback request successful: " << fallback_response.status_code << std::endl;
                return fallback_response.url.str();
            }
        } catch (const std::exception& e) {
            std::cout << "Fallback approach also failed: " << e.what() << std::endl;
        }
        
        // If all else fails, return the original URL and let VLC handle it
        std::cout << "All resolution attempts failed, returning original URL for VLC to handle directly" << std::endl;
        std::cout << "Debug info - ";
        if (head_response.status_code != 0) {
            std::cout << "HEAD: " << head_response.status_code << " ";
        }
        if (get_response.status_code != 0) {
            std::cout << "GET: " << get_response.status_code << " ";
        }
        if (get_response.error.code != cpr::ErrorCode::OK) {
            std::cout << "Error: " << get_response.error.message;
        }
        std::cout << std::endl;
        
        return cleaned_url;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during URL resolution: " << e.what() << std::endl;
        throw;
    }
}

Player::Player() : vlc_(nullptr), player_(nullptr), media_(nullptr), playing_(false) {
    // Set VLC plugin path for macOS
    #ifdef __APPLE__
    setenv("VLC_PLUGIN_PATH", "/Applications/VLC.app/Contents/MacOS/plugins", 1);
    #endif
    
    // Set up VLC command line arguments
    const char* args[] = {
        "--no-video",           // Disable video output
        "--verbose=2",          // Increase verbosity for debugging
        "--network-caching=3000" // Add network caching
    };
    
    // Create VLC instance with arguments
    vlc_ = libvlc_new(3, args);
    if (!vlc_) {
        throw std::runtime_error("Failed to initialize VLC");
    }

    // Create media player
    player_ = libvlc_media_player_new(vlc_);
    if (!player_) {
        libvlc_release(vlc_);
        throw std::runtime_error("Failed to create VLC media player");
    }

    // Set initial volume to 100%
    libvlc_audio_set_volume(player_, 100);
}

Player::~Player() {
    stop();
    if (player_) {
        libvlc_media_player_release(player_);
    }
    if (vlc_) {
        libvlc_release(vlc_);
    }
}

void Player::play(const std::string& url) {
    std::cout << "Attempting to play URL: " << url << std::endl;

    // If already playing, stop first
    if (playing_) {
        stop();
    }

    try {
        // Resolve the media URL
        std::string media_url;
        try {
            media_url = resolveMediaUrl(url);
            std::cout << "Creating media from resolved URL: " << media_url << std::endl;
        } catch (const std::exception& resolve_error) {
            std::cout << "URL resolution failed: " << resolve_error.what() << std::endl;
            std::cout << "Falling back to original URL: " << url << std::endl;
            media_url = url;
        }

        // Create media from resolved URL
        media_ = libvlc_media_new_location(vlc_, media_url.c_str());
        if (!media_) {
            throw std::runtime_error("Failed to create media from URL: " + media_url);
        }

        // Set media options for better streaming
        libvlc_media_add_option(media_, ":network-caching=5000");
        libvlc_media_add_option(media_, ":file-caching=2000");
        libvlc_media_add_option(media_, ":live-caching=2000");
        libvlc_media_add_option(media_, ":sout-mux-caching=2000");
        libvlc_media_add_option(media_, ":http-reconnect=true");
        libvlc_media_add_option(media_, ":http-user-agent=Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36");
        libvlc_media_add_option(media_, ":http-referrer=");
        libvlc_media_add_option(media_, ":network-timeout=30000");
        
        // Set the media to the player
        libvlc_media_player_set_media(player_, media_);
        
        // Release the media (player retains its own reference)
        libvlc_media_release(media_);
        media_ = nullptr;

        std::cout << "Starting playback..." << std::endl;
        
        // Start playback
        if (libvlc_media_player_play(player_) < 0) {
            throw std::runtime_error("Failed to start playback");
        }

        // Wait and check playback status
        int retry_count = 0;
        const int max_retries = 10;
        bool playback_started = false;

        while (retry_count < max_retries) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            libvlc_state_t state = libvlc_media_player_get_state(player_);
            std::cout << "Player state: " << getStateString(state) << std::endl;

            if (state == libvlc_Playing) {
                playback_started = true;
                break;
            } else if (state == libvlc_Error) {
                // Get error information
                libvlc_media_t* current_media = libvlc_media_player_get_media(player_);
                if (current_media) {
                    libvlc_media_stats_t stats;
                    if (libvlc_media_get_stats(current_media, &stats)) {
                        std::cerr << "Media statistics:" << std::endl;
                        std::cerr << "  Bytes read: " << stats.i_read_bytes << std::endl;
                        std::cerr << "  Input bitrate: " << stats.f_input_bitrate << std::endl;
                        std::cerr << "  Demux bytes read: " << stats.i_demux_read_bytes << std::endl;
                        std::cerr << "  Demux bitrate: " << stats.f_demux_bitrate << std::endl;
                    }
                    libvlc_media_release(current_media);
                }
                throw std::runtime_error("Playback failed: player reported error state");
            }
            
            retry_count++;
        }

        if (!playback_started) {
            std::stringstream err;
            err << "Failed to start playback after " << max_retries << " attempts. ";
            err << "Final state: " << getStateString(libvlc_media_player_get_state(player_));
            throw std::runtime_error(err.str());
        }

        playing_ = true;
        std::cout << "Playback started successfully" << std::endl;
        std::cout << "Current volume: " << libvlc_audio_get_volume(player_) << "%" << std::endl;
        std::cout << "Player state: " << getStateString(libvlc_media_player_get_state(player_)) << std::endl;
    } catch (const std::exception& e) {
        if (media_) {
            libvlc_media_release(media_);
            media_ = nullptr;
        }
        throw;
    }
}

void Player::playPodcastFeed(const std::string& feedUrl) {
    std::cout << "Loading podcast feed: " << feedUrl << std::endl;
    
    try {
        // Load and parse the feed
        podcast_feed_.loadFromUrl(feedUrl);
        
        // Get the latest episode
        current_episode_ = podcast_feed_.getLatestEpisode();
        
        std::cout << "Playing latest episode: " << current_episode_.title << std::endl;
        std::cout << "Description: " << current_episode_.description << std::endl;
        std::cout << "Publication date: " << current_episode_.pubDate << std::endl;
        std::cout << "DEBUG: Episode URL: [" << current_episode_.url << "]" << std::endl;
        
        // Play the episode's audio URL
        play(current_episode_.url);
    } catch (const std::exception& e) {
        std::cerr << "Failed to play podcast feed: " << e.what() << std::endl;
        throw;
    }
}

void Player::pause() {
    if (!playing_) return;
    
    if (player_) {
        libvlc_media_player_pause(player_);
        playing_ = false;
    }
}

void Player::stop() {
    if (player_) {
        libvlc_media_player_stop(player_);
        playing_ = false;
    }
}

bool Player::isPlaying() const {
    return playing_ && player_ && libvlc_media_player_is_playing(player_);
}

// Helper function to convert VLC state to string
std::string Player::getStateString(libvlc_state_t state) {
    switch (state) {
        case libvlc_NothingSpecial: return "Nothing Special";
        case libvlc_Opening: return "Opening";
        case libvlc_Buffering: return "Buffering";
        case libvlc_Playing: return "Playing";
        case libvlc_Paused: return "Paused";
        case libvlc_Stopped: return "Stopped";
        case libvlc_Ended: return "Ended";
        case libvlc_Error: return "Error";
        default: return "Unknown";
    }
}

} // namespace core
} // namespace podradio 