#pragma once
#include <Arduino.h>

void signalingTask(void *parameter);
QueueHandle_t *setupSignalingControl();