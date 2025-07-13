#ifndef PODRADIO_CORE_PLAYER_HPP
#define PODRADIO_CORE_PLAYER_HPP

#include "core/PodcastFeed.hpp"
#include <vlc/vlc.h>
#include <string>

namespace podradio {
namespace core {

class Player {
public:
    Player();
    ~Player();

    void play(const std::string& url);
    void playPodcastFeed(const std::string& feedUrl);
    void pause();
    void stop();
    bool isPlaying() const;

private:
    std::string resolveMediaUrl(const std::string& url);
    std::string getStateString(libvlc_state_t state);

    libvlc_instance_t* vlc_;
    libvlc_media_player_t* player_;
    libvlc_media_t* media_;
    bool playing_;
    PodcastFeed podcast_feed_;
    Episode current_episode_;
};

} // namespace core
} // namespace podradio

#endif // PODRADIO_CORE_PLAYER_HPP 