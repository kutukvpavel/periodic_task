#include "periodic_task.h"

namespace periodic_task_internal
{
    void periodic_task_engine(bool* state, periodic_interop_cmd_t* cmd, void (*ctrl_func)(bool))
    {
        switch (cmd.type)
        {
        case periodic_cmd_types::switch_on:
            ctrl_func(true);
            *state = true;
            break;
        case periodic_cmd_types::continuous_on:
            ctrl_func(true);
            vTaskDelay(cmd->on);
            ctrl_func(*state);
            break;
        case periodic_cmd_types::continuous_off:
            ctrl_func(false);
            vTaskDelay(cmd->off);
            ctrl_func(*state);
            break;
        case periodic_cmd_types::pulsed:
            while (cmd->cycles--)
            {
                ctrl_func(true);
                vTaskDelay(cmd->on);
                ctrl_func(false);
                vTaskDelay(cmd->off);
            }
            ctrl_func(*state);
            break;
        case periodic_cmd_types::pulsed_perpetual:
            *state = !*state;
            ctrl_func(*state);
            vTaskDelay(*state ? cmd->on : cmd->off);
            break;
        default: //Switch off
            ctrl_func(false);
            *state = false;
            break;
        }
    }
} // namespace periodic_task_internal

void periodic_task::task(void *arg)
{
    static bool output_state = false;
    static periodic_interop_cmd_t last = {.type = periodic_cmd_types::switch_off};
    static periodic_interop_cmd_t cmd = {.type = periodic_cmd_types::switch_off};
    while (1)
    {
        periodic_interop_cmd_t c;
        if (xQueueReceive(led_queue_handle, &c,
                          cmd.type == periodic_cmd_types::pulsed_perpetual ? 0 : portMAX_DELAY) == pdTRUE)
        {
            last = cmd;
            cmd = c;
        }
        periodic_task_engine(&output_state, cmd, set_led_internal);
        switch (cmd.type)
        {
        case periodic_cmd_types::continuous_off: // Time-limited actions are finished when engine routine returns
        case periodic_cmd_types::continuous_on:
        case periodic_cmd_types::pulsed:
            cmd = last;
            break;
        default:
            break;
        }
    }
}

BaseType_t periodic_task::start(size_t stack_size, uint32_t priority)
{
    assert(!task_handle);
    return xTaskCreate(task, NULL, stack_size, NULL, priority, &task_handle);
}
BaseType_t periodic_task::enqueue(const periodic_interop_cmd_t* cmd)
{
    return xQueueSend(queue_handle, cmd, 0);
}