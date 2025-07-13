# Find libcurl
find_package(CURL REQUIRED)

# Find Bluetooth libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLUEZ REQUIRED bluez)

# Add library
add_library(podradio_core
    core/Player.cpp
    core/PodcastFeed.cpp
    core/FeedManager.cpp
    core/BluetoothServer.cpp
)

target_include_directories(podradio_core
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/include
        ${LIBVLC_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${BLUEZ_INCLUDE_DIRS}
)

target_link_libraries(podradio_core
    PUBLIC
        ${LIBVLC_LIBRARIES}
        CURL::libcurl
        cpr::cpr
        tinyxml2::tinyxml2
        pugixml
        ada::ada
        nlohmann_json::nlohmann_json
        ${BLUEZ_LIBRARIES}
        bluetooth
        uuid
)

# Add executable
add_executable(podradio main.cpp)
target_link_libraries(podradio PRIVATE podradio_core) 