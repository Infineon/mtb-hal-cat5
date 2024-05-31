/***************************************************************************//**
* \file cyhal_gpio.c
*
* Description:
* Provides a high level interface for interacting with the Infineon GPIO. This is
* a wrapper around the lower level API.
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

#include "cy_utils.h"
#include "cyhal_gpio.h"
#include "cyhal_hwmgr.h"
#include "cyhal_interconnect.h"
#include "cyhal_system.h"
#include "cyhal_utils.h"
#include "cyhal_syspm.h"

#if (CYHAL_DRIVER_AVAILABLE_GPIO)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*       Internal
*******************************************************************************/

#define _CYHAL_GPIO_UNASSIGNED                      (0xFF)

static bool _cyhal_gpio_arrays_initialized = false;

// Callback array for GPIO interrupts
CY_NOINIT static cyhal_gpio_callback_data_t* _cyhal_gpio_callbacks[BT_GPIO_LAST];

// Used to keep track of BTSS pin functionality to BT Pad assignment
CY_NOINIT static cyhal_gpio_t _cyhal_btss_pad_map[BTSS_GPIO_LAST + 1];

#define _CYHAL_GPIO_GET_PAD_MAP(btss_pin)           _cyhal_btss_pad_map[btss_pin]
#define _CYHAL_GPIO_SET_PAD_MAP(btss_pin, pin)      _cyhal_btss_pad_map[btss_pin] = pin

// Used to keep track of BT Pad to BTSS pin assignment
CY_NOINIT static BTSS_GPIO_t _cyhal_pad_btss_map[BT_GPIO_LAST];

#define _CYHAL_GPIO_GET_BTSS_MAP(pin)               _cyhal_pad_btss_map[pin]
#define _CYHAL_GPIO_SET_BTSS_MAP(pin, btss_pin)     _cyhal_pad_btss_map[pin] = btss_pin

bool _cyhal_gpio_switch_and_set(cyhal_gpio_t gpio)
{

    BTSS_PINMUX_FUNC_LIST_t func = FUNC_NONE;
    /* Set GPIO drive mode */
    bool pdl_status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)gpio, (BTSS_PAD_HW_CONFIG_t)CYHAL_GPIO_DRIVE_STRONG);

    if(pdl_status)
    {
        uint8_t arr_size = sizeof (cyhal_pin_map_sw_gpio)/ sizeof(cyhal_resource_pin_mapping_t);
        for (uint8_t pinIdx = 0; pinIdx < arr_size; pinIdx++)
        {
            /* There are 40+ pin pads and 24 pinmux selections for SW GPIO*/
            if (cyhal_pin_map_sw_gpio[pinIdx].pin == (cyhal_gpio_t)gpio)
            {
                func = cyhal_pin_map_sw_gpio[pinIdx].functionality;
                break;
            }
        }

        if (func != FUNC_NONE)
        {
            /* Assign the GPIO function */
            pdl_status = btss_pad_assignFunction((BTSS_PAD_LIST_t)gpio, func);
            if (pdl_status)
            {
                BTSS_GPIO_t btss_gpio = (BTSS_GPIO_t)_cyhal_gpio_convert_func_to_btss(func);
                /* Set GPIO Output direction to output and force it to logic high (locking the communication)*/
                btss_gpio_setDirection(btss_gpio, true);
                btss_gpio_write(btss_gpio, true);
                /* Restore direction before going to sleep to avoid GPIO stuck at wake-up.
                   This is not chnaging the output driver mode that still follow the 
                   btss_pad_setHwConfig() set at the beginning of this function.
                   It just prepare for the correct setup afterwards */
                btss_gpio_setDirection(btss_gpio, false);
            }
        }
    }
    return pdl_status;
}

BTSS_GPIO_t _cyhal_gpio_convert_func_to_btss(BTSS_PINMUX_FUNC_LIST_t functionality)
{
    BTSS_GPIO_t btss_pin = BTSS_GPIO_0;

    if ((FUNC_A_GPIO_0 <= functionality) && (functionality <= FUNC_A_GPIO_7))
        btss_pin = (BTSS_GPIO_t)(functionality - FUNC_A_GPIO_0) + BTSS_A_GPIO_0;
    else if ((FUNC_B_GPIO_0 <= functionality) && (functionality <= FUNC_B_GPIO_7))
        btss_pin = (BTSS_GPIO_t)(functionality - FUNC_B_GPIO_0) + BTSS_B_GPIO_0;
    else if ((FUNC_GPIO_0 <= functionality) && (functionality <= FUNC_GPIO_7))
        btss_pin = (BTSS_GPIO_t)(functionality - FUNC_GPIO_0) + BTSS_GPIO_0;
    else
        CY_ASSERT(0); // This should never happen due to previous checks

    return btss_pin;
}

static cy_rslt_t _cyhal_gpio_hw(cyhal_gpio_t pin, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode, bool init_val)
{
    uint32_t config = 0UL;

    if (direction != CYHAL_GPIO_DIR_INPUT)
        config |= _CYHAL_GPIO_OUTPUT_ENABLE(config)
                | _CYHAL_GPIO_HYSTERESIS_ON(config)
                | _CYHAL_GPIO_ARM_DRIVE_SEL_16MA(config);

    if (direction == CYHAL_GPIO_DIR_OUTPUT)
        config |= _CYHAL_GPIO_INPUT_DISABLE(config);

    switch (drive_mode)
    {
        case CYHAL_GPIO_DRIVE_PULLUP:
            config |= _CYHAL_GPIO_PULL_UP(config);
            break;
        case CYHAL_GPIO_DRIVE_PULLDOWN:
            config |= _CYHAL_GPIO_PULL_DOWN(config);
            break;
        case CYHAL_GPIO_DRIVE_PULLUPDOWN:
            config |= _CYHAL_GPIO_PULL_UP(config) | _CYHAL_GPIO_PULL_DOWN(config);
            break;
        case CYHAL_GPIO_DRIVE_NONE:
        case CYHAL_GPIO_DRIVE_ANALOG:
        case CYHAL_GPIO_DRIVE_PULL_NONE:
        case CYHAL_GPIO_DRIVE_OPENDRAINDRIVESLOW:
        case CYHAL_GPIO_DRIVE_OPENDRAINDRIVESHIGH:
        case CYHAL_GPIO_DRIVE_STRONG:
        default:
            break;
    }

    bool dir = (direction == CYHAL_GPIO_DIR_INPUT) ? false : true;
    bool status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)pin, (BTSS_PAD_HW_CONFIG_t)config);

    if (status)
    {
        btss_gpio_setDirection(_CYHAL_GPIO_GET_BTSS_MAP(pin), dir);
        btss_gpio_write(_CYHAL_GPIO_GET_BTSS_MAP(pin), init_val);
    }

    return status ? CY_RSLT_SUCCESS : CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
}


/*******************************************************************************
*       Internal - Interrupt Service Routine
*******************************************************************************/

void _cyhal_gpio_irq_handler(uint8_t btss_pin)
{
    cyhal_gpio_t pin = _CYHAL_GPIO_GET_PAD_MAP(btss_pin);
    // Iterate through the interrupt sources and call registered callback
    cyhal_gpio_callback_data_t* cb_data = _cyhal_gpio_callbacks[pin];
    while (NULL != cb_data)
    {
        if (cb_data->pin == pin)
        {
            /* No way to determine event cause so return none */
            cb_data->callback(cb_data->callback_arg, CYHAL_GPIO_IRQ_NONE);
        }
        cb_data = cb_data->next;
    }
}


/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_gpio_init(cyhal_gpio_t pin, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode, bool init_val)
{
    if (!_cyhal_gpio_arrays_initialized)
    {
        for (uint8_t i = 0; i < BT_GPIO_LAST; i++)
        {
            _cyhal_gpio_callbacks[i] = NULL;
            _cyhal_pad_btss_map[i] = _CYHAL_GPIO_UNASSIGNED;
        }
        for (uint8_t i = 0; i < BTSS_GPIO_LAST + 1; i++)
        {
            _cyhal_btss_pad_map[i] = _CYHAL_GPIO_UNASSIGNED;
        }
        _cyhal_gpio_arrays_initialized = true;
    }

    cyhal_resource_inst_t pinRsc = _cyhal_utils_get_gpio_resource(pin);
    cy_rslt_t status = cyhal_hwmgr_reserve(&pinRsc);
    cyhal_resource_pin_mapping_t pinMap = {0};
    BTSS_GPIO_t btss_pin = _CYHAL_GPIO_UNASSIGNED;

    if (status == CY_RSLT_SUCCESS)
    {
        for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio)/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
        {
            // Reminder: There are 40+ pin pads and 24 pinmux selections for SW GPIO
            btss_pin = _cyhal_gpio_convert_func_to_btss(cyhal_pin_map_sw_gpio[pinIdx].functionality);
            if ((cyhal_pin_map_sw_gpio[pinIdx].pin == pin)
                && (_CYHAL_GPIO_GET_PAD_MAP(btss_pin) == _CYHAL_GPIO_UNASSIGNED))
            {
                pinMap = cyhal_pin_map_sw_gpio[pinIdx];
                _CYHAL_GPIO_SET_BTSS_MAP(pin, btss_pin);
                _CYHAL_GPIO_SET_PAD_MAP(btss_pin, pin);
                break;
            }
        }
        status = (pinMap.pin == pin) ? cyhal_connect_pin(&pinMap, drive_mode) : CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        status = (direction != CYHAL_GPIO_DIR_BIDIRECTIONAL) ?_cyhal_gpio_hw(pin, direction, drive_mode, init_val) : CYHAL_GPIO_RSLT_ERR_BAD_PARAM ;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        btss_gpio_enableInterrupt(btss_pin, false);
    }

    if (status != CY_RSLT_SUCCESS)
    {
        cyhal_gpio_free(pin);
    }

    return status;
}

void cyhal_gpio_free(cyhal_gpio_t pin)
{
    CY_ASSERT(_cyhal_gpio_arrays_initialized); /* Should not be freeing if we never initialized anything */

    if (pin != CYHAL_NC_PIN_VALUE)
    {
        cyhal_gpio_register_callback(pin, NULL);
        (void)cyhal_disconnect_pin(pin);
        BTSS_GPIO_t btss_pin = _CYHAL_GPIO_GET_BTSS_MAP(pin);
        if (btss_pin != _CYHAL_GPIO_UNASSIGNED)
        {
            btss_gpio_enableInterrupt(btss_pin, false);
            _CYHAL_GPIO_SET_PAD_MAP(btss_pin, _CYHAL_GPIO_UNASSIGNED);
        }    
        _CYHAL_GPIO_SET_BTSS_MAP(pin, _CYHAL_GPIO_UNASSIGNED);
        cyhal_resource_inst_t pinRsc = _cyhal_utils_get_gpio_resource(pin);
        cyhal_hwmgr_free(&pinRsc);
    }
}

cy_rslt_t cyhal_gpio_configure(cyhal_gpio_t pin, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode)
{
    // output buffer is returned if gpio direction is output
    uint32_t init_val = btss_gpio_read(_CYHAL_GPIO_GET_BTSS_MAP(pin));
    _cyhal_gpio_hw(pin, direction, drive_mode, init_val);

    return CY_RSLT_SUCCESS;
}

void cyhal_gpio_write(cyhal_gpio_t pin, bool value)
{
    btss_gpio_write(_CYHAL_GPIO_GET_BTSS_MAP(pin), value);
}

bool cyhal_gpio_read(cyhal_gpio_t pin)
{
    return btss_gpio_read(_CYHAL_GPIO_GET_BTSS_MAP(pin));
}

void cyhal_gpio_toggle(cyhal_gpio_t pin)
{
    btss_gpio_toggle(_CYHAL_GPIO_GET_BTSS_MAP(pin));
}

void cyhal_gpio_register_callback(cyhal_gpio_t pin, cyhal_gpio_callback_data_t* callback_data)
{
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();

    // Remove if already registered;
    cyhal_gpio_callback_data_t** ptr = &(_cyhal_gpio_callbacks[pin]);
    while (NULL != *ptr)
    {
        if ((*ptr)->pin == pin)
        {
            *ptr = (*ptr)->next;
            break;
        }
        ptr = &((*ptr)->next);
    }
    // Add if requested
    if (NULL != callback_data)
    {
        CY_ASSERT(NULL != callback_data->callback);
        callback_data->pin = pin;
        callback_data->next = _cyhal_gpio_callbacks[pin];
        _cyhal_gpio_callbacks[pin] = callback_data;
    }

    cyhal_system_critical_section_exit(savedIntrStatus);
    btss_gpio_registerInterruptCallback(_CYHAL_GPIO_GET_BTSS_MAP(pin), _cyhal_gpio_irq_handler);
}

void cyhal_gpio_enable_event(cyhal_gpio_t pin, cyhal_gpio_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);

    #define _CYHAL_GPIO_INVALID_TRIGGER     (0xFF)
    BTSS_GPIO_INT_TRIGGER_TYPE_t trigger;
    BTSS_GPIO_t btss_pin = _CYHAL_GPIO_GET_BTSS_MAP(pin);

    switch (event)
    {
        case CYHAL_GPIO_IRQ_RISE:
            trigger = BTSS_GPIO_TRIGGER_EDGE_RISING;
            break;
        case CYHAL_GPIO_IRQ_FALL:
            trigger = BTSS_GPIO_TRIGGER_EDGE_FALLING;
            break;
        case CYHAL_GPIO_IRQ_BOTH:
            trigger = BTSS_GPIO_TRIGGER_EDGES_BOTH;
            break;
        case CYHAL_GPIO_IRQ_NONE:
        default:
            trigger = _CYHAL_GPIO_INVALID_TRIGGER;
            break;
    }

    if (trigger != _CYHAL_GPIO_INVALID_TRIGGER)
    {
        btss_gpio_configInterrupt(btss_pin, trigger);
        btss_gpio_enableInterrupt(btss_pin, enable);
#if (CYHAL_DRIVER_AVAILABLE_SYSPM == 1)
        (void)_cyhal_syspm_set_wakeup_source(pin, ((event == CYHAL_GPIO_IRQ_RISE)? true : false), enable);
#endif
    }
    else
    {
        // Turn off the interrupt if invalid type
        btss_gpio_enableInterrupt(btss_pin, false);
    }
}

cy_rslt_t cyhal_gpio_connect_digital(cyhal_gpio_t pin, cyhal_source_t source)
{
    CY_UNUSED_PARAMETER(pin);
    CY_UNUSED_PARAMETER(source);
    return CYHAL_INTERCONNECT_RSLT_INVALID_CONNECTION;
}

cy_rslt_t cyhal_gpio_enable_output(cyhal_gpio_t pin, cyhal_signal_type_t type, cyhal_source_t *source)
{
    CY_UNUSED_PARAMETER(pin);
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(type);
    return CYHAL_INTERCONNECT_RSLT_INVALID_CONNECTION;
}

cy_rslt_t cyhal_gpio_disconnect_digital(cyhal_gpio_t pin, cyhal_source_t source)
{
    CY_UNUSED_PARAMETER(pin);
    CY_UNUSED_PARAMETER(source);
    return CYHAL_INTERCONNECT_RSLT_INVALID_CONNECTION;
}

cy_rslt_t cyhal_gpio_disable_output(cyhal_gpio_t pin)
{
    CY_UNUSED_PARAMETER(pin);
    return CYHAL_INTERCONNECT_RSLT_INVALID_CONNECTION;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CYHAL_DRIVER_AVAILABLE_GPIO */
