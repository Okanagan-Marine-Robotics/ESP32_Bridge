#include "motor_control.h"
#include "ArduinoJson.h"
#include <MycilaWebSerial.h>

QueueHandle_t motorQueue;
extern WebSerial webSerial;

void motorControlTask(void *parameter)
{

    // Assuming ESC_PIN_NUMBERS is defined in configuration.h as an array of pin numbers
    ESCDriver *escDrivers[NUM_ESC];
    int esc_pins[NUM_ESC] = ESC_PINS; // Use the defined ESC pins from configuration.h

    // Initialize ESC drivers
    for (size_t i = 0; i < NUM_ESC; ++i)
    {
        escDrivers[i] = new ESCDriver(esc_pins[i], ESC_PWM_FREQUENCY, ESC_PWM_RESOLUTION, i, 0);
        escDrivers[i]->setThrottle(0.0f); // Initialize ESC with 0% throttle
    }

    JsonDocument *doc;
    for (;;)
    {
        if (xQueueReceive(motorQueue, &doc, portMAX_DELAY) == pdPASS)
        {

            if ((*doc)["esc"].is<JsonObject>())
            {
                JsonObject esc = (*doc)["esc"].as<JsonObject>();
                // Iterate only over present keys to avoid unnecessary lookups
                for (JsonPair kv : esc)
                {
                    int idx = atoi(kv.key().c_str());
                    if (idx >= 0 && idx < NUM_ESC)
                    {
                        float throttle = kv.value().as<float>();
                        escDrivers[idx]->setThrottle(throttle);
                        webSerial.println("ESC " + String(idx) + " set to " + String(throttle));
                    }
                }
            }
            delete doc;
        }
    }
}

QueueHandle_t *setupMotorControl()
{
    // Create a queue for motor control tasks
    motorQueue = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(JsonDocument *));
    if (motorQueue == NULL)
    {
        webSerial.println("Failed to create motor control queue");
        return nullptr;
    }

    // Create the motor control task
    xTaskCreatePinnedToCore(motorControlTask, "MotorControlTask", MOTOR_TASK_STACK_SIZE, NULL, MOTOR_TASK_PRIORITY, NULL, 1);
    if (motorQueue == NULL)
    {
        webSerial.println("Failed to create motor control task");
        return nullptr;
    }

    webSerial.println("Motor control task and queue created successfully");
    return &motorQueue;
}