# ESP32 WiFi & BLE Provisioning Skeleton

> This project is ideal as a starting point for any IoT device that needs to be configured easily by a user without hardcoding credentials into the firmware.

## ✨ Features

* **WiFi Provisioning via BLE**: Use any standard BLE scanner app (like nRF Connect, LightBlue) to set the WiFi SSID and password.
* **Persistent Storage**: Credentials are saved to the ESP32's internal flash memory using the EEPROM library, so they are retained after power cycles and reboots.
* **Concurrent Operations with FreeRTOS**: Leverages the dual-core architecture of the ESP32 to handle BLE communications and WiFi management in separate, non-blocking tasks.
* **Automatic WiFi Management**: The device continuously monitors the WiFi connection and attempts to reconnect if it drops.
* **Status Indicator**: The onboard LED provides visual feedback on the device's status (WiFi connected, BLE connected, or disconnected).
* **Clean and Modular Code**: Refactored for clarity, efficiency, and easy extension.

## ⚙️ How It Works

1.  **On first boot**, the ESP32 has no WiFi credentials. It starts a BLE server and begins advertising itself. The onboard LED will blink slowly.
2.  **Using a BLE scanner app** on your phone or computer, connect to the ESP32 device (default name: `ESP32 Provisioning`).
3.  **Discover the service** and its two characteristics: one for the SSID and one for the password.
4.  **Write the SSID** to the corresponding characteristic.
5.  **Write the password** to its characteristic.
6.  Upon receiving the password, the ESP32 saves both credentials to its flash memory and **automatically reboots**.
7.  **After rebooting**, the device will load the saved credentials and attempt to connect to the specified WiFi network. The onboard LED will turn solid once connected.

## 🚀 Getting Started

### Prerequisites

* An ESP32 development board.
* [Arduino IDE](https://www.arduino.cc/en/software) or [PlatformIO](https://platformio.org/) installed and configured for ESP32 development.
* A smartphone or computer with a BLE scanner application (e.g., [nRF Connect for Mobile](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Connect-for-mobile)).

### Dependencies

This project relies on standard libraries included with the ESP32 board support package for the Arduino IDE. No external installation should be required.

```cpp
#include <WiFi.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
```

### Installation & Flashing

1.  **Clone the repository**:
    ```sh
    git clone https://github.com/tberndt5/ESP32-RTOS_Skeleton_BLE-WiFi-EEPROM.git
    ```
2.  **Open the `.ino` file** in the Arduino IDE or the project folder in PlatformIO.
3.  **Select your ESP32 board** and the correct COM port from the Tools menu.
4.  **Upload** the code to your device.
5.  **Open the Serial Monitor** at a baud rate of `115200` to view status messages and debug output.

## 📡 BLE Service Details

| Property             | Value                                        |
| -------------------- | -------------------------------------------- |
| **Device Name** | `ESP32 Provisioning`                         |
| **Service UUID** | `4fafc201-1fb5-459e-8fcc-c5c9c331914b`       |
| **SSID Char. UUID** | `beb5483e-36e1-4688-b7f5-ea07361b26a8`       |
| ↳ *Properties* | `READ`, `WRITE`                              |
| **Pass. Char. UUID** | `beb5483e-36e1-4688-b7f5-ea07361b26a9`       |
| ↳ *Properties* | `WRITE`                                      |

> **Note**: The Password characteristic is write-only for a minor layer of security (it cannot be read back over BLE after being written).

## 📜 License

This project is licensed under the MIT License. See the `LICENSE` file for details.
