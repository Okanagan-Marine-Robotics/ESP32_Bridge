#pragma once

/*********************
 * ESC CONFIGURATION *
 *********************/
#define NUM_ESC 8 // Number of ESCs
#define ESC_PINS {12, 13, 14, 15, 16, 17, 18, 19}
#define ESC_PWM_FREQUENCY 50  // PWM frequency in Hz
#define ESC_PWM_RESOLUTION 16 // PWM resolution in bits

/**********************
 * RTOS CONFIGURATION *
 **********************/

// Motor control task configuration
#define MOTOR_QUEUE_SIZE 10        // Size of the motor control queue
#define MOTOR_TASK_STACK_SIZE 4096 // Stack size for the motor control task
#define MOTOR_TASK_PRIORITY 1      // Priority for the motor control task