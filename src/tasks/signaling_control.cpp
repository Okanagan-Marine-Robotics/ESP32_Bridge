#include "signaling_control.h"
#include "configuration.h"
#include "ArduinoJson.h"
#include "serial_coms/serial_io.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"

extern SerialIO serialio; // Serial communication handler

// string hash function for switch case statements
constexpr unsigned long long hash_str(const char *str, unsigned long long h = 0)
{
    return (*str == 0) ? h : hash_str(str + 1, h * 31 + *str);
}

QueueHandle_t signalQueue;

void getHighWater(JsonDocument &doc)
{
    UBaseType_t numTasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *taskStatusArray = (TaskStatus_t *)pvPortMalloc(numTasks * sizeof(TaskStatus_t));
    if (!taskStatusArray)
    {
        doc["error"] = "Memory allocation failed";
        return;
    }

    numTasks = uxTaskGetSystemState(taskStatusArray, numTasks, NULL);

    JsonArray tasks = doc["tasks"].to<JsonArray>();
    for (UBaseType_t i = 0; i < numTasks; i++)
    {
        JsonObject t = tasks.add<JsonObject>();
        t["name"] = taskStatusArray[i].pcTaskName;
        t["stack_high_water_mark"] = taskStatusArray[i].usStackHighWaterMark;
    }

    vPortFree(taskStatusArray);
}

void signalingTask(void *parameter)
{
    JsonDocument *doc;

    for (;;)
    {
        if (xQueueReceive(signalQueue, &doc, pdMS_TO_TICKS(10)) == pdPASS && doc != nullptr)
        {
            // get the command type from the document
            String commandType = (*doc)["cmd"] | "unknown";
            unsigned long long commandHash = hash_str(commandType.c_str());
            switch (commandHash)
            {
            case hash_str("ping"):
            {
                JsonDocument response;

                // LOG_WEBSERIALLN("Received ping command");
                // Respond with a pong message
                response["msg"] = "pong";
                response["status"] = 200;
                response["timestamp"] = millis();

                if (response
                        .overflowed())
                {

                    LOG_WEBSERIALLN("Response document overflowed");
                    response.clear();
                    response["status"] = 500;
                    response["error"] = "Response document overflowed";
                }

                serialio.publish(254, response);
                break;
            }

            case hash_str("get_water_level"):
            {
                JsonDocument response;

                LOG_WEBSERIALLN("Received get_water_level command");
                // Respond with the current water level
                getHighWater(response); // Assume this function exists
                response["status"] = 200;
                response["timestamp"] = millis();
                serialio.publish(254, response);
                break;
            }

            default:
                LOG_WEBSERIALLN("Unknown command: " + commandType);
                break;
            }
            delete doc; // Clean up the received document
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // Yield CPU to other tasks
    }
}

QueueHandle_t *setupSignalingControl()
{
    signalQueue = xQueueCreate(SIGNALING_QUEUE_SIZE, sizeof(JsonDocument *));
    if (signalQueue == NULL)
    {
        LOG_WEBSERIALLN("Failed to create signaling queue");
        return nullptr;
    }

    BaseType_t taskResult = xTaskCreatePinnedToCore(signalingTask, "SignalingTask", SIGNALING_TASK_STACK_SIZE, NULL, SIGNALING_TASK_PRIORITY, NULL, 1);
    if (taskResult != pdPASS)
    {
        LOG_WEBSERIALLN("Failed to create signaling task");
        return nullptr;
    }

    LOG_WEBSERIALLN("Motor signaling task and queue created successfully");
    return &signalQueue;
}
