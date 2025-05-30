#pragma once

/*********************
 * ESC CONFIGURATION *
 *********************/
#define NUM_ESC 8                                 // Number of ESCs
#define ESC_PINS {12, 13, 14, 15, 16, 17, 18, 19} // ESC control pins
#define ESC_PWM_FREQUENCY 400                     // PWM frequency in Hz
#define ESC_PWM_RESOLUTION 16                     // PWM resolution in bits
#define ESC_BIDIRECTIONAL true                    // Bidirectional throttle support
#define ESC_MAX 1900                              // Maximum forward throttle (μs)
#define ESC_MID 1500                              // Neutral throttle (μs)
#define ESC_MIN 1100                              // Maximum reverse throttle (μs)

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
#define USE_WEBSERIAL false // Enable/disable WebSerial debug

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

/**********************
 * RTOS CONFIGURATION *
 **********************/
#define MOTOR_QUEUE_SIZE 10        // Motor control queue size
#define MOTOR_TASK_STACK_SIZE 4096 // Stack size for motor control task
#define MOTOR_TASK_PRIORITY 9      // Priority for motor control task

// Signaling control tasks are used for obtaining and processing signaling data.
// this included pinging the esp32, getting memory usage, and other miscellaneous tasks.
#define SIGNALING_QUEUE_SIZE 5         // Signaling control queue size
#define SIGNALING_TASK_STACK_SIZE 2048 // Stack size for signaling control task
#define SIGNALING_TASK_PRIORITY 1      // Priority for signaling control task