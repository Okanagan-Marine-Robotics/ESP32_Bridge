# ESP32 Bridge

## Overview

ESP32 Bridge implements a publish-subscribe messaging architecture over a serial port connection. Devices can subscribe to specific topics and receive messages published to those topics. The system includes error checking mechanisms to ensure reliable data transmission and integrity across the serial interface.

Leveraging the flexibility of PlatformIO to combine multiple frameworks for robust development. The following technologies are used:

- **FreeRTOS**: An open-source real-time operating system kernel for embedded devices. FreeRTOS enables multitasking by allowing multiple tasks to run concurrently on the ESP32, improving responsiveness and reliability for time-critical applications.

- **ESP-IDF (Espressif IoT Development Framework)**: The official development framework for the ESP32 chip. ESP-IDF provides low-level access to ESP32 hardware features, advanced networking, and peripheral control, enabling fine-grained management of system resources. We use this for performance-critical tasks and hardware-specific operations.

- **Arduino Framework**: A high-level, user-friendly framework that simplifies embedded programming. By integrating Arduino with ESP-IDF under PlatformIO, developers can use familiar Arduino libraries and APIs while still accessing the advanced features of ESP-IDF.

- **PlatformIO**: An open-source ecosystem for IoT development. PlatformIO allows seamless integration of multiple frameworks (such as Arduino and ESP-IDF) in a single project, simplifying dependency management, building, and deployment.

### Why Combine These Frameworks?

Combining FreeRTOS, ESP-IDF, and Arduino under PlatformIO allows the ESP32 Bridge project to:

- Use Arduino’s simplicity for rapid prototyping and access to a wide range of libraries useful for driver creation.
- Harness ESP-IDF’s advanced features for performance-critical or hardware-specific tasks such as high speed serial.
- Employ FreeRTOS for real-time task scheduling and multitasking.
- Benefit from PlatformIO’s unified build system and project management.

This hybrid approach provides both ease of use and powerful control, making the ESP32 bridge project flexible and scalable.

## Configuration

All settings can be configured using the `configuration.h` file.
Please note that you can also adjust low level settings such as buffers sizes and stack sizes which could break the system if not set correctly.

### Potentially Useful Settings

Below are some configuration options you may want to adjust in `configuration.h`:

- **ESP32_BAUDRATE**: The baud rate for the serial communication. Default is set to 115200, but you can change it based on your hardware capabilities and requirements.

- **NUM_ESC**: Defines the number of Electronic Speed Controllers (ESCs) connected to the ESP32. For example, `#define NUM_ESC 8` sets up the system to control 8 ESCs.

- **ESC_PINS**: Specifies the GPIO pins used to control each ESC. Define this as an array or list of pin numbers, e.g., `#define ESC_PINS {12, 13, 14, 15, 16, 17, 18, 19}` to assign each ESC to a specific pin.

- WIFI_ENABLED: Set to true to create a WiFi network

  - WIFI_SSID: The name (SSID) of the WiFi network to create
  - WIFI_PASSWORD: The password for the WiFi network.

- USE_WEBSERIAL: Set to true to enable WebSerial debugging, which allows you to have access to publish data specifically for debugging purposes via a web interface.
  - LOG_LEVEL: Sets the default log level for WebSerial output (e.g., ESP_LOG_INFO).

Note:

- WebSerial requires WiFi to be enabled. If USE_WEBSERIAL is true but WIFI_ENABLED is false, compilation will fail with an error.

Adjust these settings based on your hardware and application needs for optimal performance.

## Serial Communication

The ESP32 Bridge uses a combination of ArduinoJson -> MessagePack -> CRC8 Check -> COBS (Consistent Overhead Byte Stuffing) to ensure efficient and reliable serial communication. This approach minimizes overhead while maintaining data integrity, making it suitable for high-speed serial connections.
This happens in the opposite direction as well, where the data is received and then decoded using COBS, CRC8 Check, and MessagePack to reconstruct the original JSON message.

## Python Interface

To interact with the ESP32 Bridge from a host computer, you can use the [SeaPortPy](https://github.com/Okanagan-Marine-Robotics/SeaPortPy/tree/main) Python library. SeaPortPy provides a convenient API for sending and receiving messages over the serial connection, handling encoding and decoding automatically.

### Installation

Install SeaPortPy using pip:

```sh
pip install git+https://github.com/Okanagan-Marine-Robotics/SeaPortPy.git
```

Refer to the [SeaPortPy documentation](https://github.com/Okanagan-Marine-Robotics/SeaPortPy/tree/main) for usage and API details.

## Brief Explination of PlatformIO Setup for Beginners

PlatformIO is a modern development environment for embedded systems. To get started with this project:

1. **Install PlatformIO**:

   - Use the [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode) for Visual Studio Code, or install the [PlatformIO Core CLI](https://platformio.org/install/cli).

2. **Clone the Repository**:

   ```sh
   git clone https://github.com/yourusername/ESP32_Bridge.git
   cd ESP32_Bridge
   ```

3. **Open the Project**:

   - If using VS Code, open the project folder.
   - PlatformIO will automatically detect the project configuration.

4. **Select the Board**:

   - Edit `platformio.ini` to match your ESP32 board if needed.

5. **Build and Upload**:
   - Use the PlatformIO toolbar or run:
     ```sh
     pio run --target upload
     ```
   - This will compile and upload the firmware to your ESP32.

For more details, see the [PlatformIO documentation](https://docs.platformio.org/).
