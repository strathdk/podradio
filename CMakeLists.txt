cmake_minimum_required(VERSION 3.10)
project(podradio)

# Set C++ standard to C++17 (required for cpr)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find VLC Media Player
message(STATUS "Looking for VLC libraries...")

if(APPLE)
    # On macOS, VLC.app contains the libraries
    set(VLC_APP_PATH "/Applications/VLC.app/Contents/MacOS")
    set(LIBVLC_INCLUDE_DIRS "/Applications/VLC.app/Contents/MacOS/include")
    set(LIBVLC_LIBRARIES "${VLC_APP_PATH}/lib/libvlc.dylib")
    
    if(NOT EXISTS ${LIBVLC_LIBRARIES})
        message(FATAL_ERROR "
❌ VLC not found on macOS!

Please install VLC Media Player:
1. Download from: https://www.videolan.org/vlc/download-macosx.html
2. Install VLC.app to /Applications/
3. Run cmake again

Alternative installation via Homebrew:
  brew install --cask vlc
")
    endif()
    message(STATUS "✅ VLC found: ${LIBVLC_LIBRARIES}")
else()
    # On Linux/Windows, use pkg-config
    find_package(PkgConfig QUIET)
    if(NOT PkgConfig_FOUND)
        message(FATAL_ERROR "
❌ pkg-config not found!

Please install pkg-config:
  Ubuntu/Debian: sudo apt install pkg-config
  Fedora/RHEL:   sudo dnf install pkgconfig
  Arch Linux:    sudo pacman -S pkgconfig
")
    endif()
    
    pkg_check_modules(LIBVLC libvlc)
    if(NOT LIBVLC_FOUND)
        message(FATAL_ERROR "
❌ VLC development libraries not found!

Please install VLC development packages:

Ubuntu/Debian/Raspberry Pi OS:
  sudo apt update
  sudo apt install vlc libvlc-dev

Fedora/CentOS/RHEL:
  sudo dnf install vlc-devel

Arch Linux:
  sudo pacman -S vlc

Windows (using vcpkg):
  vcpkg install vlc

After installation, run cmake again.
")
    endif()
    message(STATUS "✅ VLC found: ${LIBVLC_LIBRARIES}")
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

# JSON library
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests) 