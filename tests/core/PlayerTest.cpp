#include "core/Player.hpp"
#include <gtest/gtest.h>

using namespace podradio::core;

TEST(PlayerTest, InitialState) {
    Player player;
    EXPECT_FALSE(player.isPlaying());
}

TEST(PlayerTest, PlayPauseStop) {
    Player player;
    
    // Test URL - using a short audio file for testing
    const std::string test_url = "https://www2.cs.uic.edu/~i101/SoundFiles/BabyElephantWalk60.wav";
    
    // Test play
    player.play(test_url);
    EXPECT_TRUE(player.isPlaying());
    
    // Test pause
    player.pause();
    EXPECT_FALSE(player.isPlaying());
    
    // Test stop
    player.play(test_url);
    player.stop();
    EXPECT_FALSE(player.isPlaying());
} 