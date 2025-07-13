# Find libcurl
find_package(CURL REQUIRED)

# Find Bluetooth libraries (optional)
set(ENABLE_BLUETOOTH OFF)
if(UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(BLUEZ bluez)
        if(BLUEZ_FOUND)
            set(ENABLE_BLUETOOTH ON)
            message(STATUS "BlueZ found - Bluetooth support enabled")
        else()
            message(WARNING "BlueZ not found - Bluetooth support disabled")
            message(STATUS "To enable Bluetooth: sudo apt install libbluetooth-dev bluez-dev")
        endif()
    else()
        message(WARNING "pkg-config not found - Bluetooth support disabled")
    endif()
else()
    message(STATUS "Bluetooth support only available on Linux")
endif()

# Add library sources
set(CORE_SOURCES
    core/Player.cpp
    core/PodcastFeed.cpp
    core/FeedManager.cpp
)

if(ENABLE_BLUETOOTH)
    list(APPEND CORE_SOURCES core/BluetoothServer.cpp)
    add_compile_definitions(ENABLE_BLUETOOTH)
endif()

add_library(podradio_core ${CORE_SOURCES})

target_include_directories(podradio_core
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/include
        ${LIBVLC_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)

if(ENABLE_BLUETOOTH)
    target_include_directories(podradio_core PUBLIC ${BLUEZ_INCLUDE_DIRS})
endif()

target_link_libraries(podradio_core
    PUBLIC
        ${LIBVLC_LIBRARIES}
        CURL::libcurl
        cpr::cpr
        tinyxml2::tinyxml2
        pugixml
        ada::ada
        nlohmann_json::nlohmann_json
)

if(ENABLE_BLUETOOTH)
    target_link_libraries(podradio_core PUBLIC ${BLUEZ_LIBRARIES} bluetooth uuid)
endif()

# Add executable
add_executable(podradio main.cpp)
target_link_libraries(podradio PRIVATE podradio_core) 