#!/bin/bash

# PodRadio Build Script
# This script helps install dependencies and build PodRadio on Linux systems

set -e  # Exit on any error

echo "🎵 PodRadio Build Script"
echo "========================="

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v apt-get &> /dev/null; then
        DISTRO="debian"
    elif command -v dnf &> /dev/null; then
        DISTRO="fedora"
    elif command -v pacman &> /dev/null; then
        DISTRO="arch"
    else
        echo "❌ Unsupported Linux distribution"
        echo "Please install dependencies manually and run:"
        echo "  mkdir build && cd build && cmake .. && cmake --build ."
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    DISTRO="macos"
else
    echo "❌ Unsupported operating system: $OSTYPE"
    echo "Please install dependencies manually and run:"
    echo "  mkdir build && cd build && cmake .. && cmake --build ."
    exit 1
fi

echo "🖥️  Detected OS: $DISTRO"

# Function to install dependencies
install_dependencies() {
    echo "📦 Installing dependencies..."
    
    case $DISTRO in
        "debian")
            echo "Installing packages for Ubuntu/Debian/Raspberry Pi..."
            sudo apt update
            sudo apt install -y cmake g++ pkg-config libcurl4-openssl-dev vlc libvlc-dev
            
            # Ask about Bluetooth support
            read -p "Install Bluetooth support? (y/N): " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                sudo apt install -y libbluetooth-dev bluez-dev bluez bluez-tools uuid-dev
                echo "✅ Bluetooth support will be enabled"
            else
                echo "⚠️  Bluetooth support will be disabled"
            fi
            ;;
        "fedora")
            echo "Installing packages for Fedora/CentOS/RHEL..."
            sudo dnf install -y cmake gcc-c++ pkgconfig libcurl-devel vlc-devel
            
            # Ask about Bluetooth support
            read -p "Install Bluetooth support? (y/N): " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                sudo dnf install -y bluez-libs-devel bluez bluez-tools libuuid-devel
                echo "✅ Bluetooth support will be enabled"
            else
                echo "⚠️  Bluetooth support will be disabled"
            fi
            ;;
        "arch")
            echo "Installing packages for Arch Linux..."
            sudo pacman -Sy --noconfirm cmake gcc pkgconfig curl vlc
            
            # Ask about Bluetooth support
            read -p "Install Bluetooth support? (y/N): " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                sudo pacman -S --noconfirm bluez bluez-libs bluez-utils util-linux
                echo "✅ Bluetooth support will be enabled"
            else
                echo "⚠️  Bluetooth support will be disabled"
            fi
            ;;
        "macos")
            echo "Installing packages for macOS..."
            if ! command -v brew &> /dev/null; then
                echo "❌ Homebrew not found. Please install Homebrew first:"
                echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
                exit 1
            fi
            
            brew install cmake curl
            
            if ! brew list --cask vlc &> /dev/null; then
                echo "Installing VLC..."
                brew install --cask vlc
            else
                echo "✅ VLC already installed"
            fi
            
            echo "⚠️  Bluetooth support only available on Linux"
            ;;
    esac
}

# Function to build project
build_project() {
    echo "🔨 Building PodRadio..."
    
    # Create build directory
    if [ -d "build" ]; then
        echo "Cleaning existing build directory..."
        rm -rf build
    fi
    
    mkdir build
    cd build
    
    # Configure
    echo "⚙️  Configuring with CMake..."
    cmake ..
    
    # Build
    echo "🔧 Compiling..."
    cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    
    echo ""
    echo "✅ Build complete!"
    echo ""
    echo "🚀 To run PodRadio:"
    echo "   ./src/podradio"
    echo ""
    echo "📚 For help:"
    echo "   ./src/podradio --help"
    echo ""
    
    # Check if Bluetooth was enabled
    if grep -q "BlueZ found" CMakeFiles/CMakeOutput.log 2>/dev/null; then
        echo "📱 Bluetooth support enabled! To start with Bluetooth:"
        echo "   ./src/podradio --bluetooth"
        echo ""
    fi
    
    echo "📖 For more information, see README.md and BLUETOOTH_SETUP.md"
}

# Check if we should install dependencies
if [ "$1" = "--deps-only" ]; then
    install_dependencies
    echo "✅ Dependencies installed. Run './build.sh' to build."
    exit 0
elif [ "$1" = "--build-only" ]; then
    build_project
    exit 0
elif [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  --deps-only    Install dependencies only"
    echo "  --build-only   Build only (skip dependency installation)"
    echo "  --help, -h     Show this help"
    echo ""
    echo "With no options, installs dependencies and builds the project."
    exit 0
fi

# Default: install dependencies and build
echo "This script will install system dependencies and build PodRadio."
echo ""

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "❌ Please don't run this script as root"
    echo "The script will ask for sudo password when needed"
    exit 1
fi

read -p "Continue? (Y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Nn]$ ]]; then
    echo "❌ Build cancelled"
    exit 1
fi

# Install dependencies
install_dependencies

echo ""

# Build project
build_project 