/***************************************************************************//**
* \file cyhal_syspm.c
*
* \brief
* Provides a high level interface for interacting with the Infineon power
* management and system clock configuration. This interface abstracts out the
* chip specific details. If any chip specific functionality is necessary, or
* performance is critical the low level functions can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2021-2022 Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

// TODO: This is a stub based on cat4 implementation.

#include <limits.h>
#include <math.h>
#include "cyhal_lptimer.h"
#include "cyhal_timer.h"
#include "cyhal_syspm.h"
#include "cyhal_system.h"
#include "cyhal_clock_impl.h"
#include "cy_utils.h"
#include "wiced_sleep.h"

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
*       Internal - Helper functions
*******************************************************************************/

/* Hz to KHz */
#define _CYHAL_HZ_TO_KHZ_CONVERSION_FACTOR (1000)

/* Set in timer driver */
cyhal_timer_t*    _cyhal_timer_copy;

static bool _cyhal_disable_systick_before_sleep_deepsleep = false;


#define _CYHAL_SYSPM_CB_ALL ((cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_SLEEP \
                                                        | CYHAL_SYSPM_CB_CPU_DEEPSLEEP \
                                                        | CYHAL_SYSPM_CB_SYSTEM_HIBERNATE \
                                                        | CYHAL_SYSPM_CB_SYSTEM_NORMAL \
                                                        | CYHAL_SYSPM_CB_SYSTEM_LOW))

static uint16_t _cyhal_deep_sleep_lock = 0;
static uint32_t _cyhal_syspm_supply_voltages[((size_t)CYHAL_VOLTAGE_SUPPLY_MAX) + 1] = { 0 };

/* The first entry in the callback chain is always reserved for the user set
 * cyhal_syspm_register_callback callback. This may be set to a sentinel value
 * indicating it is the end of the list. All subsequent slots are where
 * peripheral drivers are tracked. This makes it very easy to determine whether
 * the user registered a callback and to make sure we run that first. */
static cyhal_syspm_callback_data_t* _cyhal_syspm_callback_ptr = CYHAL_SYSPM_END_OF_LIST;
static cyhal_syspm_callback_data_t* _cyhal_syspm_peripheral_callback_ptr = CYHAL_SYSPM_END_OF_LIST;

/* General syspm callback */

static cyhal_syspm_callback_data_t* _cyhal_syspm_call_all_pm_callbacks(
    cyhal_syspm_callback_data_t* entry, bool* allow, cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode)
{
    while(entry != CYHAL_SYSPM_END_OF_LIST)
    {
        if (entry->callback != NULL &&
            (entry->states & state) == state &&
            (entry->ignore_modes & mode) != mode)
        {
            *allow = entry->callback(state, mode, entry->args) || mode != CYHAL_SYSPM_CHECK_READY;
            if (!(*allow))
            {
                // Do not increment pointer so that backtracking stop at the correct location
                break;
            }
        }
        entry = entry->next;
    }
    return entry;
}

static void _cyhal_syspm_backtrack_all_pm_callbacks(cyhal_syspm_callback_data_t* start, cyhal_syspm_callback_data_t* end, cyhal_syspm_callback_state_t state)
{
    while(start != end)
    {
        if (start->callback != NULL &&
            (start->states & state) == state &&
            (start->ignore_modes & CYHAL_SYSPM_CHECK_FAIL) != CYHAL_SYSPM_CHECK_FAIL)
        {
            start->callback(state, CYHAL_SYSPM_CHECK_FAIL, start->args);
        }
        start = start->next;
    }
}

static cy_rslt_t _cyhal_syspm_common_cb(cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode)
{
    if ((state == CYHAL_SYSPM_CB_CPU_DEEPSLEEP) && (mode == CYHAL_SYSPM_CHECK_READY) && (_cyhal_deep_sleep_lock != 0))
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }
    else
    {
        bool allow = true;
        cyhal_syspm_callback_data_t *first, *second;

        if (mode == CYHAL_SYSPM_CHECK_FAIL || mode == CYHAL_SYSPM_AFTER_TRANSITION)
        {
            first = _cyhal_syspm_peripheral_callback_ptr;
            second = _cyhal_syspm_callback_ptr;
        }
        else
        {
            second = _cyhal_syspm_peripheral_callback_ptr;
            first = _cyhal_syspm_callback_ptr;
        }

        cyhal_syspm_callback_data_t* first_current = _cyhal_syspm_call_all_pm_callbacks(first, &allow, state, mode);
        cyhal_syspm_callback_data_t* second_current = allow
            ? _cyhal_syspm_call_all_pm_callbacks(second, &allow, state, mode)
            : second;

        if (!allow && (CYHAL_SYSPM_CHECK_READY == mode))
        {
            _cyhal_syspm_backtrack_all_pm_callbacks(second, second_current, state);
            _cyhal_syspm_backtrack_all_pm_callbacks(first, first_current, state);
        }

        if (allow
            && (state == CYHAL_SYSPM_CB_CPU_DEEPSLEEP)
            && (_cyhal_timer_copy != NULL))
        {
            static bool timerOn = false;

            if (mode == CYHAL_SYSPM_BEFORE_TRANSITION)
            {
                if ((_cyhal_timer_copy->running) && _cyhal_disable_systick_before_sleep_deepsleep)
                {
                    timerOn = true;
                    allow = (cyhal_timer_stop(_cyhal_timer_copy) == CY_RSLT_SUCCESS);
                }
                else
                {
                    timerOn = false;
                }
            }
            else if (mode == CYHAL_SYSPM_AFTER_TRANSITION)
            {
                if (timerOn)
                {
                    allow = (cyhal_timer_start(_cyhal_timer_copy) == CY_RSLT_SUCCESS);
                    timerOn = false;
                }
            }
        }

        return allow ? CY_RSLT_SUCCESS : CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }
}

static void _cyhal_syspm_remove_callback_from_list(cyhal_syspm_callback_data_t **list, cyhal_syspm_callback_data_t *remove)
{
    uint32_t intr_status = cyhal_system_critical_section_enter();
    while(*list != CYHAL_SYSPM_END_OF_LIST)
    {
        if (*list == remove)
        {
            *list = remove->next;
            remove->next = NULL;
            break;
        }
        list = &((*list)->next);
    }
    cyhal_system_critical_section_exit(intr_status);
}

static bool _cyhal_syspm_is_registered(cyhal_syspm_callback_data_t *callback)
{
    // If callback->next is NULL it must not be registered since all registered
    // next ptrs in the list must point to the next callback or be equal to
    // CYHAL_SYSPM_END_OF_LIST
    return (callback->next != NULL);
}

void _cyhal_syspm_register_peripheral_callback(cyhal_syspm_callback_data_t *callback_data)
{
    CY_ASSERT(callback_data != NULL);
    uint32_t intr_status = cyhal_system_critical_section_enter();
    if(!_cyhal_syspm_is_registered(callback_data))
    {
        callback_data->next = _cyhal_syspm_peripheral_callback_ptr;
        _cyhal_syspm_peripheral_callback_ptr = callback_data;
    }
    cyhal_system_critical_section_exit(intr_status);
}

void _cyhal_syspm_unregister_peripheral_callback(cyhal_syspm_callback_data_t *callback_data)
{
    _cyhal_syspm_remove_callback_from_list(&_cyhal_syspm_peripheral_callback_ptr, callback_data);
}

cy_rslt_t _cyhal_syspm_tickless_sleep_deepsleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms, bool deep_sleep)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(desired_ms);
    CY_UNUSED_PARAMETER(actual_ms);
    CY_UNUSED_PARAMETER(deep_sleep);
    // Temporarily commented out function, needs to be reevaluated for CAT5 since lptimer is not supported
    // CY_ASSERT(obj != NULL);
    // uint32_t initial_ticks;
    // uint32_t sleep_ticks;
    // cyhal_lptimer_info_t timer_info;

    // *actual_ms = 0;
    // cy_rslt_t result = CY_RSLT_SUCCESS;

    // if(desired_ms > 0)
    // {
    //     cyhal_lptimer_get_info(obj, &timer_info);

    //     sleep_ticks = desired_ms * (timer_info.frequency_hz / _CYHAL_HZ_TO_KHZ_CONVERSION_FACTOR);
    //     initial_ticks = cyhal_lptimer_read(obj);

    //     result = cyhal_lptimer_set_delay(obj, sleep_ticks);
    //     if(result == CY_RSLT_SUCCESS)
    //     {
    //         /* Disabling and enabling the system timer is handled in _cyhal_syspm_common_cb in order
    //          * to prevent loosing kernel ticks when sleep/deep-sleep is rejected causing the time spent
    //          * in the callback handlers to check if the system can make the sleep/deep-sleep transition
    //          * to be not accounted for.
    //          */
    //         _cyhal_disable_systick_before_sleep_deepsleep = true;
    //         cyhal_lptimer_enable_event(obj, CYHAL_LPTIMER_COMPARE_MATCH, CYHAL_ISR_PRIORITY_DEFAULT, true);

    //         result = deep_sleep ? cyhal_syspm_deepsleep() : cyhal_syspm_sleep();
    //         if(result == CY_RSLT_SUCCESS)
    //         {
    //             uint32_t final_ticks = cyhal_lptimer_read(obj);
    //             uint32_t ticks = (final_ticks < initial_ticks)
    //                             ? (timer_info.max_counter_value - initial_ticks) + final_ticks
    //                             : final_ticks - initial_ticks;
    //             *actual_ms = ticks / (timer_info.frequency_hz / _CYHAL_HZ_TO_KHZ_CONVERSION_FACTOR);
    //         }

    //         cyhal_lptimer_enable_event(obj, CYHAL_LPTIMER_COMPARE_MATCH, CYHAL_ISR_PRIORITY_DEFAULT, false);
    //         _cyhal_disable_systick_before_sleep_deepsleep = false;
    //     }
    // }

    // return result;

    return CY_RSLT_SUCCESS;
}


/*******************************************************************************
*       Internal
*******************************************************************************/

/* 
    The ROM controls when to enter sleep. The application can let it
    know when to sleep only when asked through a callback.

    Shutdown sleep is mapped to deep-sleep. Regular sleep is sleep.
    When in shutdown mode, most of the hardware is turned off. Only
    retention registers and data in retention RAM are available.
*/

// Retention data (limited to 256 bytes)
uint8_t retention_data[256] __attribute__ ((section(".data_in_retention_ram")));

// May need to use wiced_rtos semaphore instead
volatile bool _cyhal_syspm_sleep_flag = false;
volatile bool _cyhal_syspm_deepsleep_flag = false;

uint32_t _cyhal_syspm_wiced_callback(wiced_sleep_poll_type_t type)
{
    if (type == WICED_SLEEP_POLL_TIME_TO_SLEEP)
    {
        // Return the time in microseconds allowed to sleep
        // If shutdown sleep, need to use WICED_SLEEP_MAX_TIME_TO_SLEEP
        return WICED_SLEEP_MAX_TIME_TO_SLEEP;
    }
    else if (type == WICED_SLEEP_POLL_SLEEP_PERMISSION)
    {
        if (_cyhal_syspm_sleep_flag)
        {
            // It's possible that this can be set in multiple threads.
            // In such a situation, opt for regular sleep.
            _cyhal_syspm_deepsleep_flag = false;
            _cyhal_syspm_sleep_flag = false;
            return WICED_SLEEP_ALLOWED_WITHOUT_SHUTDOWN;
        }

        if (_cyhal_syspm_deepsleep_flag)
        {
            _cyhal_syspm_deepsleep_flag = false;
            return WICED_SLEEP_ALLOWED_WITH_SHUTDOWN;
        }

        return WICED_SLEEP_NOT_ALLOWED;
    }
    else
    {
        // This should never happen.
        CY_ASSERT(0);
        return 0;
    }
};


/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_syspm_init(void)
{
    // Temporarily commented out since wiced_sleep_configure symbol is not exposed
    // wiced_sleep_config_t p_sleep_config = 
    // {
    //     .sleep_mode = WICED_SLEEP_MODE_NO_TRANSPORT,
    //     .host_wake_mode = WICED_SLEEP_WAKE_ACTIVE_LOW, /* Not used since sleep_mode is "no transport" */
    //     .device_wake_mode = WICED_SLEEP_WAKE_ACTIVE_LOW, /* Not used since sleep_mode is "no transport" */
    //     .device_wake_source = WICED_SLEEP_WAKE_SOURCE_MASK,
    //     .device_wake_gpio_num = 0, /* Not used since sleep_mode is "no transport" */
    //     .sleep_permit_handler = _cyhal_syspm_wiced_callback
    // };
    // 
    // wiced_result_t status = wiced_sleep_configure(&p_sleep_config);

    wiced_result_t status = WICED_SUCCESS;

    if (status == WICED_SUCCESS)
    {
        // TODO: Might need thread_ap_RegisterMPAFprePostSleepHandler();
        return CY_RSLT_SUCCESS;
    }
    else
    {
        return CYHAL_SYSPM_RSLT_INIT_ERROR;
    }
}

cy_rslt_t cyhal_syspm_hibernate(cyhal_syspm_hibernate_source_t wakeup_source)
{
    CY_UNUSED_PARAMETER(wakeup_source);
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
}

cy_rslt_t cyhal_syspm_set_system_state(cyhal_syspm_system_state_t state)
{
    CY_UNUSED_PARAMETER(state);
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
}

cyhal_syspm_system_state_t cyhal_syspm_get_system_state(void)
{
    return CYHAL_SYSPM_SYSTEM_NORMAL;
}

void cyhal_syspm_register_callback(cyhal_syspm_callback_data_t *callback_data)
{
    CY_ASSERT(callback_data != NULL);
    uint32_t intr_status = cyhal_system_critical_section_enter();
    if(!_cyhal_syspm_is_registered(callback_data))
    {
        callback_data->next = _cyhal_syspm_callback_ptr;
        _cyhal_syspm_callback_ptr = callback_data;
    }
    cyhal_system_critical_section_exit(intr_status);
}

void cyhal_syspm_unregister_callback(cyhal_syspm_callback_data_t *callback_data)
{
    _cyhal_syspm_remove_callback_from_list(&_cyhal_syspm_callback_ptr, callback_data);
}

cy_rslt_t cyhal_syspm_sleep(void)
{
    cy_rslt_t result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_SLEEP, CYHAL_SYSPM_CHECK_READY);
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_SLEEP, CYHAL_SYSPM_BEFORE_TRANSITION);
        if (result == CY_RSLT_SUCCESS)
        {
            // Spin until either an interrupt or sleep routine has been executed.
            _cyhal_syspm_sleep_flag = true;
            while (_cyhal_syspm_sleep_flag){};
            result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_SLEEP, CYHAL_SYSPM_AFTER_TRANSITION);
        }
    }

    return result;
}

cy_rslt_t cyhal_syspm_deepsleep(void)
{
    cy_rslt_t result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_DEEPSLEEP, CYHAL_SYSPM_CHECK_READY);
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_DEEPSLEEP, CYHAL_SYSPM_BEFORE_TRANSITION);
        if (result == CY_RSLT_SUCCESS)
        {
            // Spin until either an interrupt or deepsleep routine has been executed.
            _cyhal_syspm_deepsleep_flag = true;
            while (_cyhal_syspm_deepsleep_flag){};
            result = _cyhal_syspm_common_cb(CYHAL_SYSPM_CB_CPU_DEEPSLEEP, CYHAL_SYSPM_AFTER_TRANSITION);
        }
    }

    return result;
}

void cyhal_syspm_lock_deepsleep(void)
{
    CY_ASSERT(_cyhal_deep_sleep_lock != USHRT_MAX);
    uint32_t intr_status = cyhal_system_critical_section_enter();
    if (_cyhal_deep_sleep_lock < USHRT_MAX)
    {
        _cyhal_deep_sleep_lock++;
    }
    cyhal_system_critical_section_exit(intr_status);
}

void cyhal_syspm_unlock_deepsleep(void)
{
    CY_ASSERT(_cyhal_deep_sleep_lock != 0U);
    uint32_t intr_status = cyhal_system_critical_section_enter();
    if (_cyhal_deep_sleep_lock > 0U)
    {
        _cyhal_deep_sleep_lock--;
    }
    cyhal_system_critical_section_exit(intr_status);
}

cy_rslt_t cyhal_syspm_tickless_deepsleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms)
{
    return _cyhal_syspm_tickless_sleep_deepsleep(obj, desired_ms, actual_ms, true);
}

cy_rslt_t cyhal_syspm_tickless_sleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms)
{
    return _cyhal_syspm_tickless_sleep_deepsleep(obj, desired_ms, actual_ms, false);
}

void cyhal_syspm_set_supply_voltage(cyhal_syspm_voltage_supply_t supply, uint32_t mvolts)
{
    CY_ASSERT((size_t)supply <= CYHAL_VOLTAGE_SUPPLY_MAX);
    _cyhal_syspm_supply_voltages[(size_t)supply] = mvolts;
}

uint32_t cyhal_syspm_get_supply_voltage(cyhal_syspm_voltage_supply_t supply)
{
    CY_ASSERT((size_t)supply <= CYHAL_VOLTAGE_SUPPLY_MAX);
    return _cyhal_syspm_supply_voltages[(size_t)supply];
}

#if defined(__cplusplus)
}
#endif
