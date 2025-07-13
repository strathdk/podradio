# PodRadio Bluetooth Setup Guide

This guide explains how to set up and use the Bluetooth functionality in PodRadio on Raspberry Pi.

## Prerequisites

### System Requirements
- Raspberry Pi 3B+ or newer (with built-in Bluetooth)
- Raspberry Pi OS (Bullseye or newer)
- Working audio output (HDMI, analog, or USB)

### Software Dependencies

#### For Debian/Ubuntu/Raspberry Pi OS:
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install VLC Media Player (REQUIRED)
sudo apt install -y vlc libvlc-dev

# Install Bluetooth development libraries (required for Bluetooth support)
sudo apt install -y libbluetooth-dev bluez-dev bluez bluez-tools

# Install other build dependencies
sudo apt install -y cmake g++ pkg-config libcurl4-openssl-dev uuid-dev
```

#### For Fedora/CentOS/RHEL:
```bash
# Install VLC Media Player (REQUIRED)
sudo dnf install -y vlc-devel

# Install Bluetooth development libraries
sudo dnf install -y bluez-libs-devel bluez bluez-tools

# Install other dependencies
sudo dnf install -y cmake gcc-c++ pkgconfig libcurl-devel libuuid-devel
```

#### For Arch Linux:
```bash
# Install VLC Media Player (REQUIRED)
sudo pacman -S vlc

# Install Bluetooth development libraries
sudo pacman -S bluez bluez-libs bluez-utils

# Install other dependencies
sudo pacman -S cmake gcc pkgconfig curl util-linux
```

#### Package Name Troubleshooting:
If you get "package not found" errors, try these alternative package names:
- `libbluetooth-dev` might be `bluez-libs-devel` or `bluez-devel`
- `bluez-dev` might be `bluez-libs-devel`
- `uuid-dev` might be `libuuid-devel` or `util-linux-devel`

## Build with Bluetooth Support

Bluetooth support is **optional** and automatically enabled when BlueZ libraries are detected on Linux systems:

### Option 1: Auto-detection (Recommended)
```bash
# Install BlueZ development libraries first
sudo apt install -y libbluetooth-dev bluez-dev

# Clone and build
git clone <repository-url>
cd podradio
mkdir build && cd build
cmake ..
cmake --build .
```

If BlueZ libraries are found, you'll see:
```
-- BlueZ found - Bluetooth support enabled
```

If BlueZ is not found, you'll see:
```
BlueZ not found - Bluetooth support disabled
To enable Bluetooth: sudo apt install libbluetooth-dev bluez-dev
```

### Option 2: Build without Bluetooth
You can build PodRadio without Bluetooth support on any platform:
```bash
# Build normally - works on macOS, Windows, and Linux without BlueZ
cmake ..
cmake --build .
```

### Checking Build Configuration
After building, check if Bluetooth support is available:
```bash
./src/podradio --help
```

If Bluetooth is enabled, you'll see Bluetooth-related options in the help.

## Bluetooth Service Configuration

### 1. Enable Bluetooth Service
```bash
# Enable and start Bluetooth service
sudo systemctl enable bluetooth
sudo systemctl start bluetooth

# Check status
sudo systemctl status bluetooth
```

### 2. Configure Bluetooth for SPP (Serial Port Profile)
Add the following to `/etc/bluetooth/main.conf`:
```ini
[General]
Class = 0x1F0100
DiscoverableTimeout = 0
PairableTimeout = 0
```

### 3. Make Pi Discoverable
```bash
# Make Bluetooth discoverable
sudo bluetoothctl

# In bluetoothctl:
discoverable on
pairable on
agent on
default-agent
```

## Running PodRadio with Bluetooth

### Start with Bluetooth Enabled
```bash
# Start PodRadio with Bluetooth server
./src/podradio --bluetooth

# Or specify a custom RFCOMM port
./src/podradio --bluetooth --bt-port 2
```

### Interactive Mode
```bash
# Start normally, then enable Bluetooth
./src/podradio

# In interactive mode:
bluetooth start
bluetooth status
bluetooth clients
```

## Bluetooth Protocol Reference

PodRadio uses a JSON-based protocol over Bluetooth Serial Port Profile (SPP).

### Connection
1. Pair your device with the Raspberry Pi
2. Connect to the "PodRadio Control" service
3. Send JSON commands terminated with `\n`

### Command Format
```json
{
  "action": "command_name",
  "parameter": "value"
}
```

### Available Commands

#### Add Podcast
```json
{
  "action": "add_podcast",
  "name": "Podcast Name",
  "url": "https://example.com/podcast.rss",
  "description": "Optional description"
}
```

Response:
```json
{
  "success": true,
  "data": {
    "message": "Podcast added successfully",
    "name": "Podcast Name",
    "url": "https://example.com/podcast.rss"
  }
}
```

#### Remove Podcast
```json
{
  "action": "remove_podcast",
  "identifier": "Podcast Name"
}
```

#### List Podcasts
```json
{
  "action": "list_podcasts"
}
```

Response:
```json
{
  "success": true,
  "data": {
    "podcasts": [
      {
        "index": 0,
        "name": "Podcast Name",
        "url": "https://example.com/podcast.rss",
        "description": "Description",
        "enabled": true,
        "is_current": true
      }
    ],
    "current_index": 0
  }
}
```

#### Play Podcast
```json
{
  "action": "play_podcast"
}
```

Or play from URL:
```json
{
  "action": "play_podcast",
  "url": "https://example.com/audio.mp3"
}
```

#### Player Control
```json
{
  "action": "player_control",
  "command": "pause"
}
```

Commands: `pause`, `stop`

#### Navigate Podcasts
```json
{
  "action": "navigate_podcasts",
  "direction": "next"
}
```

Directions: `next`, `previous`

#### Get Status
```json
{
  "action": "get_status"
}
```

Response:
```json
{
  "success": true,
  "data": {
    "player": {
      "playing": true
    },
    "current_podcast": {
      "name": "Podcast Name",
      "url": "https://example.com/podcast.rss",
      "description": "Description"
    },
    "subscription_count": 3,
    "current_index": 0,
    "connected_clients": 1
  }
}
```

### Error Responses
```json
{
  "success": false,
  "error": "Error message",
  "details": "Optional error details"
}
```

## Client Examples

### Python Client
```python
import bluetooth
import json

def connect_to_podradio():
    # Find PodRadio service
    services = bluetooth.find_service(name="PodRadio Control")
    if not services:
        print("PodRadio service not found")
        return None
    
    service = services[0]
    sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    sock.connect((service["host"], service["port"]))
    return sock

def send_command(sock, command):
    message = json.dumps(command) + "\n"
    sock.send(message.encode())
    response = sock.recv(1024).decode().strip()
    return json.loads(response)

# Usage
sock = connect_to_podradio()
if sock:
    # Add a podcast
    response = send_command(sock, {
        "action": "add_podcast",
        "name": "Test Podcast",
        "url": "https://example.com/podcast.rss"
    })
    print(response)
    
    # List podcasts
    response = send_command(sock, {"action": "list_podcasts"})
    print(response)
    
    sock.close()
```

### Android App (Java/Kotlin)
```java
// Connect to Bluetooth device
BluetoothAdapter adapter = BluetoothAdapter.getDefaultAdapter();
BluetoothDevice device = adapter.getRemoteDevice("PI_BLUETOOTH_ADDRESS");
BluetoothSocket socket = device.createRfcommSocketToServiceRecord(SPP_UUID);
socket.connect();

// Send command
String command = "{\"action\":\"list_podcasts\"}\n";
socket.getOutputStream().write(command.getBytes());

// Read response
byte[] buffer = new byte[1024];
int bytes = socket.getInputStream().read(buffer);
String response = new String(buffer, 0, bytes);
```

## Mobile App Development

### Service Discovery
The PodRadio service is registered with these characteristics:
- **Service Name**: "PodRadio Control"
- **Service Description**: "PodRadio Bluetooth Control Service"
- **Service Class**: Serial Port Profile (SPP)
- **Default Port**: 1 (configurable)

### Connection Flow
1. Scan for Bluetooth devices
2. Find paired Raspberry Pi
3. Discover "PodRadio Control" service
4. Connect to RFCOMM socket
5. Send JSON commands

### UI Recommendations
- **Main Screen**: Show connection status, current podcast, play controls
- **Podcast List**: Add/remove podcasts, navigate between them
- **Settings**: Bluetooth connection, server port
- **Now Playing**: Current episode info, playback controls

## Troubleshooting

### Common Issues

#### Bluetooth Service Not Starting
```bash
# Check Bluetooth status
sudo systemctl status bluetooth

# Restart Bluetooth service
sudo systemctl restart bluetooth

# Check for hardware issues
sudo hciconfig -a
```

#### Permission Denied
```bash
# Add user to bluetooth group
sudo usermod -a -G bluetooth $USER

# Or run with sudo (temporary)
sudo ./src/podradio --bluetooth
```

#### Cannot Connect to Service
```bash
# Check if service is running
sudo netstat -an | grep LISTEN

# Check Bluetooth visibility
sudo bluetoothctl
show
```

#### Audio Not Working
```bash
# Check audio output
aplay -l

# Configure audio for Pi
sudo raspi-config
# Advanced Options > Audio > Choose output

# Test VLC installation
vlc --version
```

### Debug Mode
Enable verbose logging:
```bash
# Run with debug output
./src/podradio --bluetooth 2>&1 | tee debug.log
```

### Firewall Issues
```bash
# Allow Bluetooth through firewall
sudo ufw allow bluetooth
```

## Performance Considerations

### Raspberry Pi Resources
- **Memory**: Bluetooth adds ~5MB RAM usage
- **CPU**: Minimal impact for JSON parsing
- **Network**: Uses local Bluetooth, no internet required for control

### Connection Limits
- **Max Clients**: 5 simultaneous connections
- **Timeout**: 30 seconds for idle connections
- **Cleanup**: Automatic disconnection handling

### Battery Life (for mobile clients)
- Use connection pooling
- Implement connection timeouts
- Send periodic keep-alive messages

## Security Notes

### Pairing Requirements
- Devices must be paired before connecting
- No authentication beyond Bluetooth pairing
- Local network only (Bluetooth range)

### Recommendations
- Change default Bluetooth PIN
- Use device whitelist if needed
- Monitor connected clients
- Implement command rate limiting

## Systemd Service (Optional)

Create `/etc/systemd/system/podradio.service`:
```ini
[Unit]
Description=PodRadio Bluetooth Service
After=bluetooth.service
Requires=bluetooth.service

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/podradio/build
ExecStart=/home/pi/podradio/build/src/podradio --bluetooth
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

Enable the service:
```bash
sudo systemctl daemon-reload
sudo systemctl enable podradio
sudo systemctl start podradio
```

## Future Enhancements

- **BLE Support**: Bluetooth Low Energy for mobile apps
- **Authentication**: User-based access control
- **Encryption**: Secure command transmission
- **Discovery**: Automatic service discovery
- **Streaming**: Audio streaming over Bluetooth
- **Notifications**: Push updates to clients 