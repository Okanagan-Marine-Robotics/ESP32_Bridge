#include "motor_control.h"
#include "ArduinoJson.h"

QueueHandle_t motorQueue;
double lastRequestTime = 0.0;

void motorControlTask(void *parameter)
{
    ESCDriver *escDrivers[NUM_ESC];
    int esc_pins[NUM_ESC] = ESC_PINS;

    for (size_t i = 0; i < NUM_ESC; ++i)
    {
        escDrivers[i] = new ESCDriver(esc_pins[i], ESC_PWM_FREQUENCY, ESC_PWM_RESOLUTION, i);
        escDrivers[i]->setThrottle(0.0f); // Initialize ESC with 0% throttle
    }

    JsonDocument *doc;

    for (;;)
    {
        if (xQueueReceive(motorQueue, &doc, pdMS_TO_TICKS(100)) == pdPASS)
        {
            lastRequestTime = millis() / 1000.0;
            for (JsonPair kv : (*doc).as<JsonObject>())
            {
                int idx = atoi(kv.key().c_str());
                if (idx >= 0 && idx < NUM_ESC)
                {
#if USE_DUTY_US
                    uint32_t dutyUs = kv.value().as<uint32_t>() | ESC_MID; // Default to the 0 value
                    escDrivers[idx]->setDutyUs(dutyUs);
#else
                    float throttle = kv.value().as<float>();

                    escDrivers[idx]
                        ->setThrottle(throttle);
#endif
                }
            }
            delete doc;
        }
        else
        {
            double now = millis() / 1000.0;
            if (now - lastRequestTime > SAFETY_TIMEOUT)
            {
                for (size_t i = 0; i < NUM_ESC; ++i)
                {
                    escDrivers[i]->setThrottle(0.0f);
                }
                lastRequestTime = now; // Prevent repeated resets
            }
        }
        taskYIELD(); // Yield only if there are higher priority tasks ready to run
    }
}

QueueHandle_t *setupMotorControl()
{
    motorQueue = xQueueCreate(MOTOR_QUEUE_SIZE, sizeof(JsonDocument *));
    if (motorQueue == NULL)
    {
        LOG_WEBSERIALLN("Failed to create motor control queue");
        return nullptr;
    }

    BaseType_t taskResult = xTaskCreatePinnedToCore(motorControlTask, "MotorControlTask", MOTOR_TASK_STACK_SIZE, NULL, MOTOR_TASK_PRIORITY, NULL, 1);
    if (taskResult != pdPASS)
    {
        LOG_WEBSERIALLN("Failed to create motor control task");
        return nullptr;
    }

    LOG_WEBSERIALLN("Motor control task and queue created successfully");
    return &motorQueue;
}
