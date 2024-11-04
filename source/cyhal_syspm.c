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


#include <limits.h>
#include <math.h>
#include "cyhal_lptimer.h"
#include "cyhal_timer.h"
#include "cyhal_syspm.h"
#include "cyhal_system.h"
#include "cyhal_gpio.h"
#include "cyhal_clock_impl.h"
#include "cy_utils.h"
#include "wiced_sleep.h"
#include "cyhal_syspm_impl.h"
// This is included to allow the user to control the idle task behavior via the configurator
// Power->MCU-> Idle Power Mode Config.
#include "cybsp.h"

#if defined(__cplusplus)
extern "C" {
#endif


/*******************************************************************************
*       Internal - Helper functions
*******************************************************************************/

#define _CYHAL_SYSPM_CB_ALL ((cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_SLEEP \
                                                        | CYHAL_SYSPM_CB_CPU_DEEPSLEEP \
                                                        | CYHAL_SYSPM_CB_SYSTEM_HIBERNATE \
                                                        | CYHAL_SYSPM_CB_SYSTEM_NORMAL \
                                                        | CYHAL_SYSPM_CB_SYSTEM_LOW))

static uint16_t _cyhal_deep_sleep_lock = 0;
static uint32_t _cyhal_syspm_supply_voltages[((size_t)CYHAL_VOLTAGE_SUPPLY_MAX) + 1] = { 0 };

/* Connections for: wakeup source GPIOs */
const _cyhal_wakeup_src_pin_mapping_t _cyhal_gpio_map_wakeup_src[36] = {
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_0, BT_GPIO_0},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_2, BT_GPIO_2},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_3, BT_GPIO_3},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_4, BT_GPIO_4},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_5, BT_GPIO_5},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_6, BT_GPIO_6},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_7, BT_GPIO_7},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_8, BT_GPIO_8},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_9, BT_GPIO_9},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_10, BT_GPIO_10},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_11, BT_GPIO_11},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_16, BT_GPIO_16},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_GPIO_17, BT_GPIO_17},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_HOST_WAKE, BT_HOST_WAKE},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_UART_CTS_N, BT_UART_CTS_N},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_UART_RTS_N, BT_UART_RTS_N},
    {BTSS_SYSTEM_PMU_WAKE_SRC_BT_UART_RXD, BT_UART_RXD},
    {BTSS_SYSTEM_PMU_WAKE_SRC_DMIC_CK, DMIC_CK},
    {BTSS_SYSTEM_PMU_WAKE_SRC_DMIC_DQ, DMIC_DQ},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM1_DI, TDM1_DI},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM1_DO, TDM1_DO},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM1_MCK, TDM1_MCK},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM1_SCK, TDM1_SCK},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM1_WS, TDM1_WS},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM2_CLK, TDM2_SCK},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM2_IN, TDM2_DI},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM2_MCK, TDM2_MCK},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM2_OUT, TDM2_DO},
    {BTSS_SYSTEM_PMU_WAKE_SRC_TDM2_SYNC, TDM2_WS},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_2, LHL_GPIO_2},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_3, LHL_GPIO_3},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_4, LHL_GPIO_4},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_5, LHL_GPIO_5},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_6, LHL_GPIO_6},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_8, LHL_GPIO_8},
    {BTSS_SYSTEM_PMU_WAKE_SRC_LHL_GPIO_9, LHL_GPIO_9}
};

/* The first entry in the callback chain is always reserved for the user set
 * cyhal_syspm_register_callback callback. This may be set to a sentinel value
 * indicating it is the end of the list. All subsequent slots are where
 * peripheral drivers are tracked. This makes it very easy to determine whether
 * the user registered a callback and to make sure we run that first. */
static cyhal_syspm_callback_data_t* _cyhal_syspm_callback_ptr = CYHAL_SYSPM_END_OF_LIST;
static cyhal_syspm_callback_data_t* _cyhal_syspm_peripheral_callback_ptr = CYHAL_SYSPM_END_OF_LIST;

static cyhal_syspm_callback_data_t* _cyhal_syspm_call_all_pm_callbacks(
    cyhal_syspm_callback_data_t* entry, bool* allow, cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode)
{
     while(entry != CYHAL_SYSPM_END_OF_LIST)
    {
        if ((entry->callback != NULL &&
             (entry->states & state) == state &&
             (entry->ignore_modes & mode) != mode))
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
        if (((start->callback) != NULL &&
             (start->states & state) == state &&
             (start->ignore_modes & CYHAL_SYSPM_CHECK_FAIL) != CYHAL_SYSPM_CHECK_FAIL))
        {
            start->callback(state, CYHAL_SYSPM_CHECK_FAIL, start->args);
        }
        start = start->next;
    }
}

static cy_rslt_t _cyhal_syspm_common_cb(cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode)
{
    if ((state == (state & (CYHAL_SYSPM_CB_CPU_DEEPSLEEP | CYHAL_SYSPM_CB_SYSTEM_HIBERNATE)))
            && (mode == CYHAL_SYSPM_CHECK_READY) && (_cyhal_deep_sleep_lock != 0))
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

/*******************************************************************************
*       Internal
*******************************************************************************/

cyhal_syspm_callback_state_t _cyhal_syspm_convert_pdltohal_pm_state(BTSS_SYSTEM_PMU_SLEEP_MODE_t sleep_state)
{
    cyhal_syspm_callback_state_t state = CYHAL_SYSPM_CB_CPU_DEEPSLEEP;
    switch (sleep_state)
    {
        case BTSS_SYSTEM_PMU_SLEEP_PDS:
            state = CYHAL_SYSPM_CB_CPU_DEEPSLEEP;
            break;
#if defined(CYHAL_SYSPM_EPDS_ENABLED) && (CYHAL_SYSPM_EPDS_ENABLED == 1)
        case BTSS_SYSTEM_PMU_SLEEP_EPDS:
            state = CYHAL_SYSPM_CB_SYSTEM_HIBERNATE;
            break;
#endif
        default:
            break;
    }

    return state;
}

cyhal_syspm_callback_mode_t _cyhal_syspm_convert_pdltohal_pm_mode(BTSS_SYSTEM_PMU_SLEEP_MODE_t sleep_state, bool *proceed)
{
    cyhal_syspm_callback_mode_t callback_mode = CYHAL_SYSPM_CHECK_READY;

    switch (sleep_state)
    {
        case BTSS_SYSTEM_PMU_SLEEP_PDS:
            callback_mode = CYHAL_SYSPM_CHECK_READY;
            *proceed = true;
            break;
#if defined(CYHAL_SYSPM_EPDS_ENABLED) && (CYHAL_SYSPM_EPDS_ENABLED == 1)
        case BTSS_SYSTEM_PMU_SLEEP_EPDS:
            callback_mode = CYHAL_SYSPM_CHECK_READY;
            *proceed = true;
            break;
#endif
        case BTSS_SYSTEM_PMU_SLEEP_NOT_ALLOWED:
            /* This state is mapped to the CHECK FAIL */
            callback_mode = CYHAL_SYSPM_CHECK_FAIL;
            *proceed = true;
            break;
        default:
            /* The states here are not handled since no sleep is actualy reached in these states */
            *proceed = false;
            break;
    }

    return callback_mode;
}

BTSS_SYSTEM_PMU_SLEEP_MODE_t _cyhal_syspm_pre_sleep_cback(BTSS_SYSTEM_PMU_SLEEP_MODE_t sleep_state , uint32_t sleep_time_in_lpo_cycles)
{
    CY_UNUSED_PARAMETER(sleep_time_in_lpo_cycles);
    bool proceed = true;

    /* Retrieve the Sleep Mode */
    cyhal_syspm_callback_mode_t callback_mode = _cyhal_syspm_convert_pdltohal_pm_mode(sleep_state, &proceed);

    /* Proceed only if the state is the expected one */
    if (proceed)
    {
        /* Retrieve the Sleep State */
        cyhal_syspm_callback_state_t callback_state = _cyhal_syspm_convert_pdltohal_pm_state(sleep_state);

        cy_rslt_t result = _cyhal_syspm_common_cb(callback_state, callback_mode);

        /* The expected mode to trigger BEFORE transition is CHECK READY */
        if((CY_RSLT_SUCCESS == result) && (CYHAL_SYSPM_CHECK_READY == callback_mode))
        {
            result = _cyhal_syspm_common_cb(callback_state, CYHAL_SYSPM_BEFORE_TRANSITION);
        }

        //Return the appropriate sleep state in case the sleep is allowed or sleep not allowed
        sleep_state = (result == CY_RSLT_SUCCESS) ? sleep_state:BTSS_SYSTEM_PMU_SLEEP_NOT_ALLOWED;
    }

    return sleep_state;
}

void _cyhal_syspm_post_sleep_cback (BTSS_SYSTEM_PMU_SLEEP_MODE_t sleep_state)
{
    //Invoke the exit sleep cback
    _cyhal_syspm_common_cb( _cyhal_syspm_convert_pdltohal_pm_state(sleep_state), CYHAL_SYSPM_AFTER_TRANSITION);

}

cy_rslt_t _cyhal_syspm_set_gpio_wakeup_source(cyhal_gpio_t pin, bool polarity, bool enable)
{
    bool pdl_status = false;
    for (uint8_t pin_idx = 0; pin_idx < (sizeof(_cyhal_gpio_map_wakeup_src)/sizeof(_cyhal_gpio_map_wakeup_src[0])); pin_idx++)
    {
        if(_cyhal_gpio_map_wakeup_src[pin_idx].pin == pin)
        {
            if (enable)
            {
                pdl_status = btss_system_sleepEnableWakeSource(_cyhal_gpio_map_wakeup_src[pin_idx].wakeup_src,
                            (BTSS_SYSTEM_SLEEP_ACTIVE_CONFIG_t) polarity);
            }
            else
            {
                pdl_status = btss_system_sleepDisableWakeSource(_cyhal_gpio_map_wakeup_src[pin_idx].wakeup_src);
            }
            //terminate the loop
            break;
        }
    }

    return (pdl_status) ? CY_RSLT_SUCCESS : CYHAL_SYSPM_RSLT_BAD_ARGUMENT;
}

cy_rslt_t _cyhal_syspm_set_lpcomp_wakeup_source(cy_en_adccomp_lpcomp_id_t comp_ch, bool enable)
{
    if (enable)
    {
        Cy_ADCCOMP_LPCOMP_EnableWakeConfig(comp_ch);
    }
    else
    {
        Cy_ADCCOMP_LPCOMP_DisableWakeConfig(comp_ch);
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_syspm_init(void)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;
    bool pdl_status = FALSE;

    BTSS_SYSTEM_SLEEP_PARAMS_t _syspm_params_default = {
        .sleep_config = _CYHAL_SYSPM_SLEEP_MODE,
        .device_wake_mode = (BTSS_SYSTEM_SLEEP_ACTIVE_CONFIG_t)TRUE,
        .host_wake_mode = (BTSS_SYSTEM_SLEEP_ACTIVE_CONFIG_t)TRUE,
    };

    if (CY_RSLT_SUCCESS == status)
    {
        /* Initialize callbacks */
        pdl_status = btss_system_sleepInit((BTSS_SYSTEM_PRE_SLEEP_CB_t)_cyhal_syspm_pre_sleep_cback,
                (BTSS_SYSTEM_POST_SLEEP_CB_t)_cyhal_syspm_post_sleep_cback);

        status = (pdl_status) ? CY_RSLT_SUCCESS : CYHAL_SYSPM_RSLT_CB_REGISTER_ERROR;
    }

    if (CY_RSLT_SUCCESS == status)
    {
        /* Set sleep mode */
#if defined(CY_CFG_PMU_SLEEP_MODE)
        pdl_status = btss_system_sleepAllowMode(CY_CFG_PMU_SLEEP_MODE);
#else
        /* Set sleep mode to PDS if not configured */
        pdl_status = btss_system_sleepAllowMode(BTSS_SYSTEM_PMU_SLEEP_PDS);
#endif // defined(CY_CFG_PMU_SLEEP_MODE)
        status = (pdl_status) ? CY_RSLT_SUCCESS : CYHAL_SYSPM_RSLT_INIT_ERROR;
    }

    if (CY_RSLT_SUCCESS == status)
    {
        /* Setup PMU with sleep mode and wake-up polarity */
        pdl_status = btss_system_sleepEnable(&_syspm_params_default);
        status = (pdl_status) ? CY_RSLT_SUCCESS : CYHAL_SYSPM_RSLT_INIT_ERROR;
    }

     return status;
}

cy_rslt_t cyhal_syspm_deepsleep(void)
{
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
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
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
}

void cyhal_syspm_lock_deepsleep(void)
{
    CY_ASSERT(_cyhal_deep_sleep_lock != USHRT_MAX);
    uint32_t intr_status = cyhal_system_critical_section_enter();
    if (_cyhal_deep_sleep_lock < USHRT_MAX)
    {
        _cyhal_deep_sleep_lock++;
        /* Set sleep mode as not Allowed  */
        (void)btss_system_sleepAllowMode(BTSS_SYSTEM_PMU_SLEEP_NOT_ALLOWED);
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
    if (_cyhal_deep_sleep_lock == 0)
    {
#if defined(CYHAL_SYSPM_EPDS_ENABLED) && (CYHAL_SYSPM_EPDS_ENABLED == 1)
        /* Set sleep mode as Allowed Hibernate */
        (void)btss_system_sleepAllowMode(BTSS_SYSTEM_PMU_SLEEP_EPDS);
#else
        /* Set sleep mode */
#if defined(CY_CFG_PMU_SLEEP_MODE)
        (void)btss_system_sleepAllowMode(CY_CFG_PMU_SLEEP_MODE);
#else
        /* Set sleep mode to PDS if not configured */
        (void)btss_system_sleepAllowMode(BTSS_SYSTEM_PMU_SLEEP_PDS);
#endif // defined(CY_CFG_PMU_SLEEP_MODE)
#endif
    }
    cyhal_system_critical_section_exit(intr_status);
}

cy_rslt_t cyhal_syspm_tickless_deepsleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(desired_ms);
    CY_UNUSED_PARAMETER(actual_ms);
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
}

cy_rslt_t cyhal_syspm_tickless_sleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(desired_ms);
    CY_UNUSED_PARAMETER(actual_ms);
    return CYHAL_SYSPM_RSLT_ERR_NOT_SUPPORTED;
}

cyhal_syspm_system_deep_sleep_mode_t cyhal_syspm_get_deepsleep_mode (void)
{
    return CYHAL_SYSPM_SYSTEM_DEEPSLEEP;
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
