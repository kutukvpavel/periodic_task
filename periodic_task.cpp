/**
 * @file periodic_task.cpp
 * @author Kutukov Pavel (kutukovps@my.msu.ru)
 * @brief A library for embedded applications to execute periodic actions on a separate FreeRTOS thread.
 * @version 1
 * @date 2023-12-13
 * 
 * @copyright This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include "periodic_task.h"

/// @brief Constructor. Created a FreeRTOS interoperability queue.
/// @param ctrl_func Control function for the task to be executed periodically. Should be reentrant.
/// @param queue_depth Interoperability queue depth (usually 3-16)
periodic_task::periodic_task(void (*ctrl_func)(bool), size_t queue_depth) : control_func(ctrl_func)
{
    assert(ctrl_func);
    queue_handle = xQueueCreate(queue_depth, sizeof(periodic_interop_cmd_t));
    assert(queue_handle);
}
/// @brief Destructor. Deletes interoperability queue and task handle (if exists)
periodic_task::~periodic_task()
{
    if (task_handle) vTaskDelete(task_handle);
    vQueueDelete(queue_handle);
}

/// @brief Task state machine
/// @param arg Periodic task instance pointer
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

/// @brief Start periodic task. Creates a new FreeRTOS task. A periodic task instance can only be started once.
/// @param stack_size Internal state machine task stack size. Bear in mind that it calls task action (control) functions.
/// @param priority Internal state machine FreeRTOS task priority.
/// @return See xTaskCreate
BaseType_t periodic_task::start(size_t stack_size, uint32_t priority)
{
    assert(!task_handle);
    return xTaskCreate(task, NULL, stack_size, this, priority, &task_handle);
}
/// @brief Enqueue a command into the periodic task's interoperability queue.
/// @param cmd Command structure pointer. The structure will be copied into the queue buffer.
/// @return See xQueueSend
BaseType_t periodic_task::enqueue(const periodic_interop_cmd_t* cmd)
{
    return xQueueSend(queue_handle, cmd, 0);
}