#include "periodic_task.h"

periodic_task::periodic_task(void (*ctrl_func)(bool), size_t queue_depth) : control_func(ctrl_func)
{
    assert(ctrl_func);
    queue_handle = xQueueCreate(queue_depth, sizeof(periodic_interop_cmd_t));
    assert(queue_handle);
}

periodic_task::~periodic_task()
{
    if (task_handle) vTaskDelete(task_handle);
    vQueueDelete(queue_handle);
}

void periodic_task::task(void *arg)
{
    assert(arg);
    auto instance = reinterpret_cast<periodic_task*>(arg);
    while (1)
    {
        periodic_interop_cmd_t c;
        if (xQueueReceive(instance->queue_handle, &c, instance->cmd.type == periodic_cmd_types::pulsed_perpetual ? 0 : portMAX_DELAY) == pdTRUE)
        {
            switch (c.type)
            {
            case periodic_cmd_types::switch_off:
            case periodic_cmd_types::switch_on:
            case periodic_cmd_types::pulsed_perpetual:
                if (instance->cmd.type == c.type) continue;
            default:
                break;
            }
            if (instance->dbg_callback) instance->dbg_callback("Changing state");
            instance->last = instance->cmd;
            instance->cmd = c;
        }
        switch (instance->cmd.type)
        {
        case periodic_cmd_types::switch_on:
            instance->control_func(true);
            instance->output_state = true;
            break;
        case periodic_cmd_types::continuous_on:
            instance->control_func(true);
            vTaskDelay(instance->cmd.on);
            instance->control_func(instance->output_state);
            break;
        case periodic_cmd_types::continuous_off:
            instance->control_func(false);
            vTaskDelay(instance->cmd.off);
            instance->control_func(instance->output_state);
            break;
        case periodic_cmd_types::pulsed:
            while (instance->cmd.cycles--)
            {
                instance->control_func(true);
                vTaskDelay(instance->cmd.on);
                instance->control_func(false);
                vTaskDelay(instance->cmd.off);
            }
            instance->control_func(instance->output_state);
            break;
        case periodic_cmd_types::pulsed_perpetual:
            instance->output_state = !instance->output_state;
            instance->control_func(instance->output_state);
            vTaskDelay(instance->output_state ? instance->cmd.on : instance->cmd.off);
            break;
        default: //Switch off
            instance->control_func(false);
            instance->output_state = false;
            break;
        }
        switch (instance->cmd.type)
        {
        case periodic_cmd_types::continuous_off: // Time-limited actions are finished when engine routine returns
        case periodic_cmd_types::continuous_on:
        case periodic_cmd_types::pulsed:
            instance->cmd = instance->last;
            break;
        default:
            break;
        }
    }
}

BaseType_t periodic_task::start(size_t stack_size, uint32_t priority)
{
    assert(!task_handle);
    return xTaskCreate(task, NULL, stack_size, this, priority, &task_handle);
}
BaseType_t periodic_task::enqueue(const periodic_interop_cmd_t* cmd)
{
    return xQueueSend(queue_handle, cmd, 0);
}