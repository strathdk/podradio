cmake_minimum_required(VERSION 3.10)
project(podradio)

# Set C++ standard to C++17 (required for cpr)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find VLC
if(APPLE)
    # On macOS, VLC.app contains the libraries
    set(VLC_APP_PATH "/Applications/VLC.app/Contents/MacOS")
    set(LIBVLC_INCLUDE_DIRS "/Applications/VLC.app/Contents/MacOS/include")
    set(LIBVLC_LIBRARIES "${VLC_APP_PATH}/lib/libvlc.dylib")
    
    if(NOT EXISTS ${LIBVLC_LIBRARIES})
        message(FATAL_ERROR "VLC libraries not found. Please install VLC.app first.")
    endif()
else()
    # On other platforms, use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBVLC REQUIRED libvlc)
endif()

# Add third-party libraries
include(FetchContent)

# CPR for HTTP client (replaces manual CURL)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

# TinyXML2 for better RSS parsing
FetchContent_Declare(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

# PugiXML for more robust XML parsing
FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)
FetchContent_MakeAvailable(pugixml)

# URL parsing library
FetchContent_Declare(
    ada-url
    GIT_REPOSITORY https://github.com/ada-url/ada.git
    GIT_TAG v2.7.4
)
FetchContent_MakeAvailable(ada-url)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests) 