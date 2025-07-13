# Find libcurl
find_package(CURL REQUIRED)

# Add library
add_library(podradio_core
    core/Player.cpp
    core/PodcastFeed.cpp
)

target_include_directories(podradio_core
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/include
        ${LIBVLC_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)

target_link_libraries(podradio_core
    PUBLIC
        ${LIBVLC_LIBRARIES}
        CURL::libcurl
        cpr::cpr
        tinyxml2::tinyxml2
        pugixml
        ada::ada
)

# Add executable
add_executable(podradio main.cpp)
target_link_libraries(podradio PRIVATE podradio_core) 