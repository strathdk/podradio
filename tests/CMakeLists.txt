# Download and configure GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add test executable
add_executable(player_test
    core/PlayerTest.cpp
)

target_link_libraries(player_test
    PRIVATE
        podradio_core
        GTest::gtest_main
)

# Register test
include(GoogleTest)
gtest_discover_tests(player_test) 