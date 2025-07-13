#!/usr/bin/env python3
"""
PodRadio Bluetooth Client Example

This script demonstrates how to connect to and control PodRadio via Bluetooth.
Make sure to install the required dependencies:
    pip install pybluez

Usage:
    python bluetooth_client.py
"""

import bluetooth
import json
import sys
import time

class PodRadioClient:
    def __init__(self):
        self.socket = None
        self.connected = False
    
    def connect(self, device_address=None):
        """Connect to PodRadio Bluetooth service"""
        try:
            if device_address:
                # Connect to specific device
                services = bluetooth.find_service(address=device_address, name="PodRadio Control")
            else:
                # Find any PodRadio service
                services = bluetooth.find_service(name="PodRadio Control")
            
            if not services:
                print("❌ PodRadio service not found")
                print("Make sure PodRadio is running with Bluetooth enabled")
                return False
            
            service = services[0]
            print(f"📡 Found PodRadio service on {service['host']}:{service['port']}")
            
            # Create Bluetooth socket
            self.socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.socket.connect((service["host"], service["port"]))
            
            self.connected = True
            print("✅ Connected to PodRadio!")
            return True
            
        except bluetooth.BluetoothError as e:
            print(f"❌ Bluetooth error: {e}")
            return False
        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False
    
    def send_command(self, command):
        """Send a JSON command to PodRadio"""
        if not self.connected:
            print("❌ Not connected to PodRadio")
            return None
        
        try:
            message = json.dumps(command) + "\n"
            self.socket.send(message.encode())
            
            # Read response
            response = self.socket.recv(4096).decode().strip()
            return json.loads(response)
            
        except Exception as e:
            print(f"❌ Command error: {e}")
            return None
    
    def disconnect(self):
        """Disconnect from PodRadio"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("👋 Disconnected from PodRadio")
    
    def get_status(self):
        """Get current PodRadio status"""
        response = self.send_command({"action": "get_status"})
        if response and response.get("success"):
            return response["data"]
        return None
    
    def list_podcasts(self):
        """List all podcasts"""
        response = self.send_command({"action": "list_podcasts"})
        if response and response.get("success"):
            return response["data"]["podcasts"]
        return []
    
    def add_podcast(self, name, url, description=""):
        """Add a new podcast"""
        command = {
            "action": "add_podcast",
            "name": name,
            "url": url,
            "description": description
        }
        return self.send_command(command)
    
    def remove_podcast(self, identifier):
        """Remove a podcast by name or URL"""
        command = {
            "action": "remove_podcast",
            "identifier": identifier
        }
        return self.send_command(command)
    
    def play_podcast(self, url=None):
        """Play current podcast or from URL"""
        command = {"action": "play_podcast"}
        if url:
            command["url"] = url
        return self.send_command(command)
    
    def pause_playback(self):
        """Pause playback"""
        return self.send_command({
            "action": "player_control",
            "command": "pause"
        })
    
    def stop_playback(self):
        """Stop playback"""
        return self.send_command({
            "action": "player_control",
            "command": "stop"
        })
    
    def navigate_podcasts(self, direction):
        """Navigate to next or previous podcast"""
        return self.send_command({
            "action": "navigate_podcasts",
            "direction": direction
        })

def print_status(client):
    """Print current status"""
    print("\n" + "="*50)
    print("📊 PODRADIO STATUS")
    print("="*50)
    
    status = client.get_status()
    if status:
        # Player status
        player_status = "🎵 Playing" if status["player"]["playing"] else "⏸️ Stopped"
        print(f"Player: {player_status}")
        
        # Current podcast
        if status["current_podcast"]:
            current = status["current_podcast"]
            print(f"Current: {current['name']}")
            print(f"URL: {current['url']}")
            if current['description']:
                print(f"Description: {current['description']}")
        else:
            print("Current: No podcast selected")
        
        # Stats
        print(f"Total Podcasts: {status['subscription_count']}")
        print(f"Current Index: {status['current_index']}")
        print(f"Connected Clients: {status['connected_clients']}")
    else:
        print("❌ Could not get status")

def print_podcasts(client):
    """Print podcast list"""
    print("\n" + "="*50)
    print("📻 PODCAST LIST")
    print("="*50)
    
    podcasts = client.list_podcasts()
    if podcasts:
        for podcast in podcasts:
            marker = "👉" if podcast["is_current"] else "  "
            status = "✅" if podcast["enabled"] else "❌"
            print(f"{marker} {status} {podcast['name']}")
            print(f"     URL: {podcast['url']}")
            if podcast['description']:
                print(f"     Description: {podcast['description']}")
            print()
    else:
        print("No podcasts found")

def interactive_menu(client):
    """Interactive menu system"""
    while True:
        print("\n" + "="*50)
        print("🎵 PODRADIO BLUETOOTH CLIENT")
        print("="*50)
        print("1. Show Status")
        print("2. List Podcasts")
        print("3. Add Podcast")
        print("4. Remove Podcast")
        print("5. Play Current Podcast")
        print("6. Play from URL")
        print("7. Pause Playback")
        print("8. Stop Playback")
        print("9. Next Podcast")
        print("10. Previous Podcast")
        print("0. Quit")
        
        choice = input("\nEnter your choice (0-10): ").strip()
        
        if choice == "0":
            break
        elif choice == "1":
            print_status(client)
        elif choice == "2":
            print_podcasts(client)
        elif choice == "3":
            name = input("Podcast name: ").strip()
            url = input("RSS URL: ").strip()
            description = input("Description (optional): ").strip()
            
            if name and url:
                response = client.add_podcast(name, url, description)
                if response and response.get("success"):
                    print(f"✅ Added podcast: {name}")
                else:
                    print(f"❌ Failed to add podcast: {response.get('error', 'Unknown error')}")
            else:
                print("❌ Name and URL are required")
        
        elif choice == "4":
            podcasts = client.list_podcasts()
            if podcasts:
                print("\nAvailable podcasts:")
                for i, podcast in enumerate(podcasts):
                    print(f"{i+1}. {podcast['name']}")
                
                try:
                    index = int(input("Select podcast to remove (number): ")) - 1
                    if 0 <= index < len(podcasts):
                        response = client.remove_podcast(podcasts[index]['name'])
                        if response and response.get("success"):
                            print(f"✅ Removed podcast: {podcasts[index]['name']}")
                        else:
                            print(f"❌ Failed to remove podcast: {response.get('error', 'Unknown error')}")
                    else:
                        print("❌ Invalid selection")
                except ValueError:
                    print("❌ Invalid number")
            else:
                print("❌ No podcasts available")
        
        elif choice == "5":
            response = client.play_podcast()
            if response and response.get("success"):
                data = response["data"]
                print(f"✅ Playing: {data.get('episode', 'Current podcast')}")
            else:
                print(f"❌ Playback failed: {response.get('error', 'Unknown error')}")
        
        elif choice == "6":
            url = input("Enter audio URL: ").strip()
            if url:
                response = client.play_podcast(url)
                if response and response.get("success"):
                    print(f"✅ Playing from URL: {url}")
                else:
                    print(f"❌ Playback failed: {response.get('error', 'Unknown error')}")
            else:
                print("❌ URL is required")
        
        elif choice == "7":
            response = client.pause_playback()
            if response and response.get("success"):
                print("✅ Playback paused")
            else:
                print(f"❌ Pause failed: {response.get('error', 'Unknown error')}")
        
        elif choice == "8":
            response = client.stop_playback()
            if response and response.get("success"):
                print("✅ Playback stopped")
            else:
                print(f"❌ Stop failed: {response.get('error', 'Unknown error')}")
        
        elif choice == "9":
            response = client.navigate_podcasts("next")
            if response and response.get("success"):
                data = response["data"]
                print(f"✅ Selected next podcast: {data['podcast']['name']}")
            else:
                print(f"❌ Navigation failed: {response.get('error', 'Unknown error')}")
        
        elif choice == "10":
            response = client.navigate_podcasts("previous")
            if response and response.get("success"):
                data = response["data"]
                print(f"✅ Selected previous podcast: {data['podcast']['name']}")
            else:
                print(f"❌ Navigation failed: {response.get('error', 'Unknown error')}")
        
        else:
            print("❌ Invalid choice")
        
        input("\nPress Enter to continue...")

def main():
    """Main function"""
    print("🎵 PodRadio Bluetooth Client")
    print("="*40)
    
    client = PodRadioClient()
    
    try:
        # Connect to PodRadio
        if len(sys.argv) > 1:
            # Use provided device address
            device_address = sys.argv[1]
            print(f"Connecting to device: {device_address}")
            if not client.connect(device_address):
                return
        else:
            # Auto-discover
            print("Searching for PodRadio service...")
            if not client.connect():
                return
        
        # Show initial status
        print_status(client)
        
        # Start interactive menu
        interactive_menu(client)
        
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    main() 