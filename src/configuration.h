#pragma once
#include <MycilaWebSerial.h>

extern WebSerial webSerial; // Forward declaration for WebSerial

/************************
 * SERIAL CONFIGURATION *
 ************************/

#define ESP32_SERIAL Serial
#define ESP32_BAUDRATE 115200
#define MAX_SERIAL_BUFFER_SIZE 1024 // Maximum size of the serial buffer
#define CRC8_POLY 0x07
#define CRC8_INIT_VALUE 0x00

/*********************
 * ESC CONFIGURATION *
 *********************/
#define NUM_ESC 8                                 // Number of ESCs
#define ESC_PINS {13, 12, 14, 15, 16, 17, 18, 19} // ESC control pins
#define ESC_PWM_FREQUENCY 400                     // PWM frequency in Hz
#define ESC_PWM_RESOLUTION 16                     // PWM resolution in bits
#define ESC_BIDIRECTIONAL true                    // Bidirectional throttle support
#define ESC_MAX 1900                              // Maximum forward throttle (μs)
#define ESC_MID 1500                              // Neutral throttle (μs)
#define ESC_MIN 1100                              // Maximum reverse throttle (μs)
#define USE_DUTY_US false                         // Use duty cycle in microseconds for ESC control
#define SAFETY_TIMEOUT 5.0                        // Safety timeout in seconds for ESCs to reset to neutral if no commands are received
#define INVERTING_OPTOCOUPLER true                // we can invert the signal if we have an optocoupler that inverts the output

/**********************
 * WIFI CONFIGURATION *
 **********************/
#define WIFI_ENABLED false // Enable/disable WiFi

#if WIFI_ENABLED
#define WIFI_SSID "ESP Bridge"   // WiFi SSID
#define WIFI_PASSWORD "ogopogo1" // WiFi Password
#endif

/***************************
 * WEBSERIAL CONFIGURATION *
 ***************************/
#define USE_WEBSERIAL false     // Enable/disable WebSerial debug
#define LOG_LEVEL ESP_LOG_INFO // Default log level for WebSerial

#if !WIFI_ENABLED && USE_WEBSERIAL
#error "WebSerial requires WiFi to be enabled. Please set WIFI_ENABLED to true."
#endif

// WebSerial logging macros
#if USE_WEBSERIAL
#define LOG_WEBSERIALLN(msg) webSerial.println(msg)
#define LOG_WEBSERIAL(msg) webSerial.print(msg)
#else
#define LOG_WEBSERIALLN(msg) ((void)0)
#define LOG_WEBSERIAL(msg) ((void)0)
#endif

/*********************
 * LED CONFIGURATION *
 *********************/
#define NUM_LEDS 6         // Number of LEDs 6 on ESP32 Mobo
#define LED_PIN 2          // GPIO pin for LED control
#define LED_BRIGHTNESS 255 // Default (0-255) Brightness level
#define LED_SPEED 500      // Default speed of LED patterns in milliseconds

/**********************
 * RTOS CONFIGURATION *
 **********************/
#define MOTOR_QUEUE_SIZE 10        // Motor control queue size
#define MOTOR_TASK_STACK_SIZE 4096 // Stack size for motor control task
#define MOTOR_TASK_PRIORITY 1      // Priority for motor control task

// Signaling control tasks are used for obtaining and processing signaling data.
// this included pinging the esp32, getting memory usage, and other miscellaneous tasks.
#define SIGNALING_QUEUE_SIZE 10            // Signaling control queue size
#define SIGNALING_TASK_STACK_SIZE 4096 * 2 // Stack size for signaling control task
#define SIGNALING_TASK_PRIORITY 1          // Priority for signaling control task

#define SERIAL_TASK_STACK_SIZE 4096 * 2 // Stack size for serial task
#define SERIAL_TASK_PRIORITY 1          // Priority for serial task

/**********************
 * DEVICE BUS CONFIGURATION *
 **********************/
#define I2C_SPEED 100000 // I2C speed in Hz

/*************************
 * GENERAL CONFIGURATION *
 *************************/
#define STARTUP_DELAY 0 // Startup delay in seconds to allow devices to connect to the WiFi network for logging