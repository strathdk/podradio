# PodRadio

A modern C++ podcast player and radio streaming application with RSS feed support.

## Features

- 🎵 **Podcast Playback**: Stream and play podcast episodes
- 📡 **RSS Feed Support**: Subscribe to and manage podcast RSS feeds
- 🔍 **Episode Management**: Browse and organize podcast episodes
- 🎛️ **Audio Controls**: Play, pause, seek, and volume control
- 📱 **Bluetooth Control**: Remote control via Bluetooth (Raspberry Pi)
- 🧪 **Well Tested**: Comprehensive unit test coverage

## Prerequisites

- **CMake** (>= 3.10)
- **C++17** compatible compiler (GCC, Clang, or MSVC)
- **VLC Media Player** with development libraries
- **Internet connection** for downloading dependencies

### Installing VLC

#### Ubuntu/Debian/Raspberry Pi OS:
```bash
sudo apt update
sudo apt install vlc libvlc-dev
```

#### macOS:
```bash
# Option 1: Download from website
# Visit: https://www.videolan.org/vlc/download-macosx.html

# Option 2: Using Homebrew
brew install --cask vlc
```

#### Fedora/CentOS/RHEL:
```bash
sudo dnf install vlc-devel
```

#### Arch Linux:
```bash
sudo pacman -S vlc
```

#### Windows:
```bash
# Using vcpkg
vcpkg install vlc

# Or download from: https://www.videolan.org/vlc/download-windows.html
```

### Supported Platforms

- macOS (tested)
- Linux (with Bluetooth support on Raspberry Pi)
- Windows

## Dependencies

This project uses [CPM](https://github.com/cpm-cmake/CPM.cmake) for dependency management. All dependencies are automatically downloaded and configured during the build process:

- **fmt** - Modern formatting library
- **cpr** - HTTP client library for RSS feed fetching
- **pugixml** - XML parsing for RSS feeds
- **tinyxml2** - Alternative XML parsing
- **cxxopts** - Command line argument parsing
- **GoogleTest** - Unit testing framework

## Building the Project

### Quick Start (Recommended)

Use the automated build script:
```bash
# Clone the repository
git clone <repository-url>
cd podradio

# Run the build script (installs dependencies and builds)
./build.sh
```

The script will:
- Detect your operating system
- Install required dependencies (VLC, build tools)
- Ask if you want Bluetooth support (Linux only)
- Build the project automatically

### Manual Build

```bash
# Install VLC first (see prerequisites above)

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
cmake --build .

# Run the application
./src/podradio
```

### Development Build

```bash
# Create build directory with debug symbols
mkdir build && cd build

# Configure for development
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
cmake --build .

# Run tests
ctest --verbose
```

### Release Build

```bash
# Create build directory
mkdir build && cd build

# Configure for release
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .
```

## Usage

### Basic Usage

```bash
# Start the application
./src/podradio

# With command line options
./src/podradio --help

# Start with Bluetooth server (Raspberry Pi)
./src/podradio --bluetooth

# Start with Bluetooth on custom port
./src/podradio --bluetooth --bt-port 2
```

### Managing Podcast Feeds

```cpp
#include "core/PodcastFeed.hpp"

// Create a feed manager
PodcastFeed feed("https://example.com/podcast.rss");

// Fetch latest episodes
feed.updateFeed();

// Get episodes
auto episodes = feed.getEpisodes();
```

### Audio Playback

```cpp
#include "core/Player.hpp"

// Create player instance
Player player;

// Load and play audio
player.load("audio_file.mp3");
player.play();

// Control playback
player.pause();
player.setVolume(0.8);
```

### Bluetooth Control (Raspberry Pi)

Control PodRadio remotely via Bluetooth:

```bash
# Start with Bluetooth enabled
./src/podradio --bluetooth

# Or enable in interactive mode
bluetooth start
bluetooth status
bluetooth clients
```

Use the Python client example:
```bash
cd examples
python bluetooth_client.py
```

See [BLUETOOTH_SETUP.md](BLUETOOTH_SETUP.md) for detailed setup instructions.

## Testing

Run the test suite:

```bash
# From build directory
ctest --verbose

# Or run specific test executable
./tests/player_test
```

### Test Coverage

- **Player Tests**: Audio playback functionality
- **Feed Tests**: RSS feed parsing and management
- **Integration Tests**: End-to-end workflow testing

## Project Structure

```
podradio/
├── CMakeLists.txt              # Main CMake configuration
├── README.md                   # This file
├── .gitignore                  # Git ignore rules
├── Makefile                    # Alternative build system
├── include/                    # Header files
│   └── core/
│       ├── Player.hpp          # Audio player interface
│       └── PodcastFeed.hpp     # RSS feed manager
├── src/                        # Source files
│   ├── CMakeLists.txt         # Source CMake config
│   ├── main.cpp               # Application entry point
│   └── core/
│       ├── Player.cpp         # Audio player implementation
│       └── PodcastFeed.cpp    # RSS feed implementation
├── tests/                      # Unit tests
│   ├── CMakeLists.txt         # Test CMake config
│   └── core/
│       └── PlayerTest.cpp     # Player unit tests
└── build/                      # Build output (ignored by git)
```

## Development

### Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass (`ctest`)
6. Commit your changes (`git commit -m 'Add amazing feature'`)
7. Push to the branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request

### Code Style

- Follow C++17 best practices
- Use RAII for resource management
- Prefer standard library containers
- Include unit tests for new features

### Building Documentation

```bash
# Generate documentation (if Doxygen is installed)
doxygen Doxyfile
```

## Troubleshooting

### Build Issues

**VLC Not Found Error**
```bash
# Ubuntu/Debian/Raspberry Pi:
sudo apt install vlc libvlc-dev

# macOS:
brew install --cask vlc

# Fedora/RHEL:
sudo dnf install vlc-devel

# Then rebuild:
rm -rf build/
mkdir build && cd build
cmake ..
```

**Bluetooth Libraries Not Found (Optional)**
```bash
# Ubuntu/Debian/Raspberry Pi:
sudo apt install libbluetooth-dev bluez bluez-tools

# Fedora/RHEL:
sudo dnf install bluez-libs-devel

# Rebuild to enable Bluetooth support
```

**CMake Configuration Fails**
```bash
# Clear CMake cache
rm -rf build/
mkdir build && cd build
cmake ..
```

**Missing Dependencies**
- Ensure you have a stable internet connection for dependency download
- Check that CMake version is >= 3.10

**Compiler Errors**
- Verify C++17 support: `gcc --version` or `clang --version`
- Try a different compiler if available

### Runtime Issues

**Audio Playback Problems**
- Check audio file format compatibility
- Verify system audio drivers

**Network Issues**
- Ensure internet connectivity for RSS feed fetching
- Check firewall settings

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Thanks to the open source community for the excellent libraries
- Built with modern C++ best practices
- Inspired by minimalist audio player designs 