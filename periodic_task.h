#pragma once

#include <stddef.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

enum periodic_cmd_types : uint32_t
{
    switch_off,
    switch_on,
    continuous_off,
    continuous_on,
    pulsed,
    pulsed_perpetual
};
struct periodic_interop_cmd_t
{
    periodic_cmd_types type;
    TickType_t on;
    TickType_t off;
    uint32_t cycles;
};

#ifdef __cplusplus

class periodic_task
{
private:
    void (*control_func)(bool);
    TaskHandle_t task_handle = NULL;
    QueueHandle_t queue_handle = NULL;
    bool output_state = false;
    periodic_interop_cmd_t last = {.type = periodic_cmd_types::switch_off};
    periodic_interop_cmd_t cmd = {.type = periodic_cmd_types::switch_off};

    static void task(void* arg);
public:
    periodic_task(void (*ctrl_func)(bool), size_t queue_depth = 8);
    ~periodic_task();

    BaseType_t start(size_t stack_size = 3072, uint32_t priority = 2);
    BaseType_t enqueue(const periodic_interop_cmd_t* cmd);
};

periodic_task::periodic_task(void (*ctrl_func)(bool), size_t queue_depth) : control_func(ctrl_func)
{
    queue_handle = xQueueCreate(queue_depth, sizeof(periodic_interop_cmd_t));
    assert(queue_handle);
}

periodic_task::~periodic_task()
{
    if (task_handle) vTaskDelete(task_handle);
    vQueueDelete(queue_handle);
}

#endif