#pragma once
#include <Arduino.h>
#include "motor_control/esc_driver.h"

void motorControlTask(void *parameter);
QueueHandle_t *setupMotorControl();
