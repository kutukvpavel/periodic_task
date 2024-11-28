/**
 * @file periodic_task.h
 * @author Kutukov Pavel (kutukovps@my.msu.ru)
 * @brief A library for embedded applications to execute periodic actions on a separate FreeRTOS thread.
 * @version 0.1
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
    void (*dbg_callback)(const char*);
};

#endif