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
#if defined (CYW55900)
CY_NOINIT cyhal_gpio_callback_data_t* _cyhal_gpio_callbacks[WLSS_GPIO_LAST];
#else
CY_NOINIT cyhal_gpio_callback_data_t* _cyhal_gpio_callbacks[BT_GPIO_LAST];
#endif // defined (CYW55900)

// Used to keep track of BTSS/CTSS/WLSS pin functionality to BTSS/CTSS/WLSS Pad assignment
CY_NOINIT cyhal_gpio_t _cyhal_btss_pad_map[BT_GPIO_LAST + 1];
#if defined (CYW55900)
CY_NOINIT cyhal_gpio_t _cyhal_ctss_pad_map[(CTSS_GPIO_LAST - BT_GPIO_LAST)  + 1];
CY_NOINIT cyhal_gpio_t _cyhal_wlss_pad_map[(WLSS_GPIO_LAST - CTSS_GPIO_LAST) + 1];
#endif // defined (CYW55900)
// Used to keep track of BT/CT/WL Pad to BTSS/CTSS/WLSS pin assignment
CY_NOINIT BTSS_GPIO_t _cyhal_pad_btss_map[BT_GPIO_LAST];
#if defined (CYW55900)
CY_NOINIT CTSS_LHL_IO_t _cyhal_pad_ctss_map[(CTSS_GPIO_LAST - BT_GPIO_LAST)];
CY_NOINIT WLSS_IO_t _cyhal_pad_wlss_map[(WLSS_GPIO_LAST - CTSS_GPIO_LAST)];
#endif // defined (CYW55900)

bool _cyhal_gpio_switch_and_set(cyhal_gpio_t gpio)
{
    bool pdl_status = false;
    if (gpio < BT_GPIO_LAST)
    {
        BTSS_PINMUX_FUNC_LIST_t func = FUNC_NONE;
        /* Set GPIO drive mode */
        pdl_status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)gpio, (BTSS_PAD_HW_CONFIG_t)CYHAL_GPIO_DRIVE_STRONG);

        if(pdl_status)
        {
            uint8_t arr_size = sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS])/ sizeof(cyhal_resource_pin_mapping_t);
            for (uint8_t pinIdx = 0; pinIdx < arr_size; pinIdx++)
            {
                /* There are 40+ pin pads and 24 pinmux selections for SW GPIO*/
                if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].pin == (cyhal_gpio_t)gpio)
                {
                    func = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].functionality;
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
    }
#if defined (CYW55900)
    else if (gpio < CTSS_GPIO_LAST)
    {
        CTSS_PINMUX_FUNC_LIST_t func = (CTSS_PINMUX_FUNC_LIST_t)FUNC_NONE;
        uint16_t config = _CYHAL_GPIO_CTSS_OUTPUT_ENABLE();

        uint8_t arr_size = sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS])/ sizeof(cyhal_resource_pin_mapping_t);
        for (uint8_t pinIdx = 0; pinIdx < arr_size; pinIdx++)
        {
            if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].pin == (cyhal_gpio_t)gpio)
            {
                func = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].functionality;
                break;
            }
        }

        if (func != (CTSS_PINMUX_FUNC_LIST_t)FUNC_NONE)
        {
            /* Set GPIO drive mode */
            pdl_status = ctss_pad_configure((CTSS_PAD_LIST_t)gpio, (CTSS_PINMUX_FUNC_LIST_t)func, (uint16_t)config);
            if (pdl_status)
            {
                /* Set GPIO Output direction to output and force it to logic high (locking the communication)*/
                ctss_lhl_ioSetDirection(_CYHAL_GPIO_GET_CTSS_MAP(gpio), true);
                ctss_lhl_ioSet(_CYHAL_GPIO_GET_CTSS_MAP(gpio), true);
            }
        }
    }
    else if (gpio < WLSS_GPIO_LAST)
    {
        WLSS_PINMUX_FUNC_LIST_t func = (WLSS_PINMUX_FUNC_LIST_t)FUNC_NONE;
        uint16_t config = _CYHAL_GPIO_WLSS_OUTPUT_ENABLE();

        uint8_t arr_size = sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS])/ sizeof(cyhal_resource_pin_mapping_t);
        for (uint8_t pinIdx = 0; pinIdx < arr_size; pinIdx++)
        {
            if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].pin == (cyhal_gpio_t)gpio)
            {
                func = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].functionality;
                break;
            }
        }

        if (func != (WLSS_PINMUX_FUNC_LIST_t)FUNC_NONE)
        {
            /* Set GPIO drive mode */
            pdl_status = wlss_pad_configure((WLSS_PAD_LIST_t)gpio, (WLSS_PINMUX_FUNC_LIST_t)func, (uint16_t)config);
            if (pdl_status)
            {
                /* Set GPIO Output direction to output and force it to logic high (locking the communication)*/
                wlss_io_setDirection(_CYHAL_GPIO_GET_WLSS_MAP(gpio), true);
                wlss_io_set(_CYHAL_GPIO_GET_WLSS_MAP(gpio), true);
            }
        }
    }
#endif // defined (CYW55900)
    else
    {
        /* Not supported */
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

#if defined (CYW55900)
CTSS_LHL_IO_t _cyhal_gpio_convert_func_to_ctss(CTSS_PINMUX_FUNC_LIST_t functionality)
{
    CTSS_LHL_IO_t ctss_pin = CTSS_LHL_IO_0;

    if ((FUNC_LHL_IO_0 <= functionality) && (functionality <= FUNC_LHL_IO_1))
        ctss_pin = (CTSS_LHL_IO_t)(functionality - FUNC_LHL_IO_0);
    else if (FUNC_LHL_IO_10 == functionality)
        ctss_pin = CTSS_LHL_IO_10;
    else if ((FUNC_LHL_IO_2_ADC_MUX_SEL <= functionality) && (functionality <= FUNC_LHL_IO_9_ADC_MUX_SEL))
        ctss_pin = (CTSS_LHL_IO_t)(functionality - FUNC_LHL_IO_2_ADC_MUX_SEL) + 2;
    else
        CY_ASSERT(0); // This should never happen due to previous checks

    return ctss_pin;
}

WLSS_IO_t _cyhal_gpio_convert_func_to_wlss(WLSS_PINMUX_FUNC_LIST_t functionality)
{
    WLSS_IO_t wlss_pin = WLSS_IO_GPIO_0;

    if (FUNC_WL_GPIO_0_GPIO == functionality)
        wlss_pin = (WLSS_IO_t)(functionality - FUNC_WL_GPIO_0_GPIO) + 11;
    else if ((FUNC_WL_GPIO_2_GPIO <= functionality) && (functionality <= FUNC_WL_GPIO_4_GPIO))
        wlss_pin = (WLSS_IO_t)(functionality - FUNC_WL_GPIO_0_GPIO) + 12;
    else if ((FUNC_WL_GPIO_5_GPIO <= functionality) && (functionality <= FUNC_WL_GPIO_6_GPIO))
        wlss_pin = (WLSS_IO_t)(functionality - FUNC_WL_GPIO_0_GPIO) + 1;
    else if ((FUNC_SDIO_CMD_GPIO <= functionality) && (functionality <= FUNC_SDIO_DATA_3_GPIO))
    {
        switch(functionality)

        {
            case FUNC_SDIO_CMD_GPIO:
                wlss_pin = WLSS_IO_SDIO_CMD;
                break;
            case FUNC_SDIO_DATA_0_GPIO:
                wlss_pin = WLSS_IO_SDIO_DATA_0;
                break;
            case FUNC_SDIO_DATA_1_GPIO:
                wlss_pin = WLSS_IO_SDIO_DATA_1;
                break;
            case FUNC_SDIO_DATA_2_GPIO:
                wlss_pin = WLSS_IO_SDIO_DATA_2;
                break;
            case FUNC_SDIO_DATA_3_GPIO:
                wlss_pin = WLSS_IO_SDIO_DATA_3;
                break;
            default:
                CY_ASSERT(0); // This should never happen due to previous checks
                break;
        }
    }
    else if ((FUNC_RFSW_CTRL_6_GPIO <= functionality) && (functionality <= FUNC_RFSW_CTRL_7_GPIO))
    {
        switch(functionality)

        {
            case FUNC_RFSW_CTRL_6_GPIO:
                wlss_pin = WLSS_IO_RFSW_CTRL_6;
                break;
            case FUNC_RFSW_CTRL_7_GPIO:
                wlss_pin = WLSS_IO_RFSW_CTRL_7;
                break;
            default:
                CY_ASSERT(0); // This should never happen due to previous checks
                break;
        }
    }
    else
        CY_ASSERT(0); // This should never happen due to previous checks

    return wlss_pin;
}
#endif // defined (CYW55900)

static cy_rslt_t _cyhal_gpio_hw(cyhal_gpio_t pin, cyhal_pinmux_t functionality, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode, bool init_val)
{
    uint32_t config = 0UL;
    bool dir = (direction == CYHAL_GPIO_DIR_INPUT) ? false : true;
    bool status = false;

    if (pin < BT_GPIO_LAST)
    {
        if (direction != CYHAL_GPIO_DIR_INPUT)
            config |= _CYHAL_GPIO_BTSS_OUTPUT_ENABLE(config)
                    | _CYHAL_GPIO_BTSS_HYSTERESIS_ON(config)
                    | _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_16MA(config);

        if (direction == CYHAL_GPIO_DIR_OUTPUT)
            config |= _CYHAL_GPIO_BTSS_INPUT_DISABLE(config);

        switch (drive_mode)
        {
            case CYHAL_GPIO_DRIVE_PULLUP:
                config |= _CYHAL_GPIO_BTSS_PULL_UP(config);
                break;
            case CYHAL_GPIO_DRIVE_PULLDOWN:
                config |= _CYHAL_GPIO_BTSS_PULL_DOWN(config);
                break;
            case CYHAL_GPIO_DRIVE_PULLUPDOWN:
                config |= _CYHAL_GPIO_BTSS_PULL_UP(config) | _CYHAL_GPIO_BTSS_PULL_DOWN(config);
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

        status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)pin, (BTSS_PAD_HW_CONFIG_t)config);
        if (status && (functionality != FUNC_NONE))
            status = btss_pad_assignFunction((BTSS_PAD_LIST_t)pin, (BTSS_PINMUX_FUNC_LIST_t)functionality);

        if (status)
        {
            btss_gpio_setDirection(_CYHAL_GPIO_GET_BTSS_MAP(pin), dir);
            btss_gpio_write(_CYHAL_GPIO_GET_BTSS_MAP(pin), init_val);
        }
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        switch (direction)
        {
            case CYHAL_GPIO_DIR_INPUT:
                config |= _CYHAL_GPIO_CTSS_INPUT_ENABLE();
                break;
            case CYHAL_GPIO_DIR_OUTPUT:
                config |= _CYHAL_GPIO_CTSS_OUTPUT_ENABLE();
                break;
            default:
                break;
        }
        switch (drive_mode)
        {
            case CYHAL_GPIO_DRIVE_PULLUP:
                config |= _CYHAL_GPIO_CTSS_PULL_UP();
                break;
            case CYHAL_GPIO_DRIVE_PULLDOWN:
                config |= _CYHAL_GPIO_CTSS_PULL_DOWN();
                break;
            case CYHAL_GPIO_DRIVE_PULLUPDOWN:
                config |= _CYHAL_GPIO_CTSS_PULL_UP_DOWN();
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
        status = ctss_pad_configure((CTSS_PAD_LIST_t)pin, (CTSS_PINMUX_FUNC_LIST_t)functionality, (uint16_t)config);
        ctss_lhl_ioSetDirection(_CYHAL_GPIO_GET_CTSS_MAP(pin), dir);
        ctss_lhl_ioSet(_CYHAL_GPIO_GET_CTSS_MAP(pin), init_val);
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        switch (direction)
        {
            case CYHAL_GPIO_DIR_INPUT:
                config |= _CYHAL_GPIO_WLSS_INPUT_ENABLE();
                break;
            case CYHAL_GPIO_DIR_OUTPUT:
                config |= _CYHAL_GPIO_WLSS_OUTPUT_ENABLE();
                break;
            default:
                break;
        }
        switch (drive_mode)
        {
            case CYHAL_GPIO_DRIVE_PULLUP:
                config |= _CYHAL_GPIO_WLSS_PULL_UP();
                break;
            case CYHAL_GPIO_DRIVE_PULLDOWN:
                config |= _CYHAL_GPIO_WLSS_PULL_DOWN();
                break;
            case CYHAL_GPIO_DRIVE_PULLUPDOWN:
                config |= _CYHAL_GPIO_WLSS_PULL_UP_DOWN();
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
        status = wlss_pad_configure((WLSS_PAD_LIST_t)pin, (WLSS_PINMUX_FUNC_LIST_t)functionality, (uint16_t)config);
        wlss_io_setDirection(_CYHAL_GPIO_GET_WLSS_MAP(pin), dir);
        wlss_io_set(_CYHAL_GPIO_GET_WLSS_MAP(pin), init_val);
    }
#endif // defined (CYW55900)
    else
    {
        return CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
    }


    return status ? CY_RSLT_SUCCESS : CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
}


/*******************************************************************************
*       Internal - Interrupt Service Routine
*******************************************************************************/
void _cyhal_gpio_irq_handler(uint8_t pin)
{
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
*       Internal - Interrupt Service Routine (BTSS)
*******************************************************************************/

void _cyhal_gpio_btss_irq_handler(uint8_t btss_pin)
{
    cyhal_gpio_t pin = _CYHAL_GPIO_GET_BTSS_PAD_MAP(btss_pin);
    _cyhal_gpio_irq_handler((uint8_t)pin);
}

#if defined (CYW55900)
/*******************************************************************************
*       Internal - Interrupt Service Routine (CTSS)
*******************************************************************************/

void _cyhal_gpio_ctss_irq_handler(uint8_t ctss_pin)
{
    cyhal_gpio_t pin = _CYHAL_GPIO_GET_CTSS_PAD_MAP(ctss_pin);
    _cyhal_gpio_irq_handler((uint8_t)pin);
}

/*******************************************************************************
*       Internal - Interrupt Service Routine (WLSS)
*******************************************************************************/

void _cyhal_gpio_wlss_irq_handler(uint8_t wlss_pin)
{
    cyhal_gpio_t pin = _CYHAL_GPIO_GET_WLSS_PAD_MAP(wlss_pin);
    _cyhal_gpio_irq_handler((uint8_t)pin);
}
#endif // defined (CYW55900)


/*******************************************************************************
*       Internal - Init structures
*******************************************************************************/

static inline void _cyhal_gpio_arrays_init(void)
{
#if defined (CYW55900)
    for (uint8_t i = 0; i < WLSS_GPIO_LAST; i++)
#else
    for (uint8_t i = 0; i < BT_GPIO_LAST; i++)
#endif // defined (CYW55900)
    {
        _cyhal_gpio_callbacks[i] = NULL;
    }

    memset(_cyhal_pad_btss_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_pad_btss_map));
    memset(_cyhal_btss_pad_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_btss_pad_map));

#if defined (CYW55900)
    memset(_cyhal_pad_ctss_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_pad_ctss_map));
    memset(_cyhal_ctss_pad_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_ctss_pad_map));
    memset(_cyhal_pad_wlss_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_pad_wlss_map));
    memset(_cyhal_wlss_pad_map, _CYHAL_GPIO_UNASSIGNED, sizeof(_cyhal_wlss_pad_map));
#endif // defined (CYW55900)
}


/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_gpio_init(cyhal_gpio_t pin, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode, bool init_val)
{
    if (!_cyhal_gpio_arrays_initialized)
    {
        _cyhal_gpio_arrays_init();
        _cyhal_gpio_arrays_initialized = true;
    }

    cyhal_resource_inst_t pinRsc = _cyhal_utils_get_gpio_resource(pin);
    cy_rslt_t status = cyhal_hwmgr_reserve(&pinRsc);
    cyhal_resource_pin_mapping_t pinMap = {0};
    BTSS_GPIO_t btss_pin = _CYHAL_GPIO_UNASSIGNED;
#if defined (CYW55900)
    CTSS_LHL_IO_t ctss_pin = _CYHAL_GPIO_UNASSIGNED;
    WLSS_IO_t wlss_pin = _CYHAL_GPIO_UNASSIGNED;
#endif // defined (CYW55900)

    if (status == CY_RSLT_SUCCESS)
    {
        if (pin < BT_GPIO_LAST)
        {
            for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
            {
                // Reminder: There are 40+ pin pads and 24 pinmux selections for SW GPIO
                btss_pin = _cyhal_gpio_convert_func_to_btss(cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].functionality);
                if ((cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].pin == pin)
                    && (_CYHAL_GPIO_GET_BTSS_PAD_MAP(btss_pin) == _CYHAL_GPIO_UNASSIGNED))
                {
                    pinMap = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx];
                    _CYHAL_GPIO_SET_BTSS_MAP(pin, btss_pin);
                    _CYHAL_GPIO_SET_BTSS_PAD_MAP(btss_pin, pin);
                    break;
                }
            }
        }
#if defined (CYW55900)
        else if (pin < CTSS_GPIO_LAST)
        {
            for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
            {
                ctss_pin = _cyhal_gpio_convert_func_to_ctss(cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].functionality);
                if ((cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].pin == pin)
                    && (_CYHAL_GPIO_GET_CTSS_PAD_MAP(ctss_pin) == _CYHAL_GPIO_UNASSIGNED)
                        )
                {
                    pinMap = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx];
                    _CYHAL_GPIO_SET_CTSS_MAP(pin, ctss_pin);
                    _CYHAL_GPIO_SET_CTSS_PAD_MAP(ctss_pin, pin);
                    break;
                }
            }
        }
        else if (pin < WLSS_GPIO_LAST)
        {
            for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
            {
                wlss_pin = _cyhal_gpio_convert_func_to_wlss(cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].functionality);
                if ((cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].pin == pin)
                    && (_CYHAL_GPIO_GET_WLSS_PAD_MAP(wlss_pin) == _CYHAL_GPIO_UNASSIGNED)
                        )
                {
                    pinMap = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx];
                    _CYHAL_GPIO_SET_WLSS_MAP(pin, wlss_pin);
                    _CYHAL_GPIO_SET_WLSS_PAD_MAP(wlss_pin, pin);
                    break;
                }
            }
        }
#endif // defined (CYW55900)
        else
        {
            return CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
        }
        status = (pinMap.pin == pin) ? cyhal_connect_pin(&pinMap, (uint16_t)drive_mode) : CYHAL_GPIO_RSLT_ERR_BAD_PARAM;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        status = (direction != CYHAL_GPIO_DIR_BIDIRECTIONAL) ?_cyhal_gpio_hw(pinMap.pin, pinMap.functionality, direction, drive_mode, init_val) : CYHAL_GPIO_RSLT_ERR_BAD_PARAM ;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        if (_CYHAL_GPIO_UNASSIGNED != btss_pin)
            btss_gpio_enableInterrupt(btss_pin, false);
#if defined (CYW55900)
        if (_CYHAL_GPIO_UNASSIGNED != ctss_pin)
            ctss_lhl_io_enableInterrupt(ctss_pin, false);
        if (_CYHAL_GPIO_UNASSIGNED != wlss_pin)
            wlss_io_enableInterrupt(wlss_pin, false);
#endif // defined (CYW55900)
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
        if (pin < BT_GPIO_LAST)
        {
            BTSS_GPIO_t btss_pin = _CYHAL_GPIO_GET_BTSS_MAP(pin);
            if (btss_pin != _CYHAL_GPIO_UNASSIGNED)
            {
                btss_gpio_enableInterrupt(btss_pin, false);
                _CYHAL_GPIO_SET_BTSS_PAD_MAP(btss_pin, _CYHAL_GPIO_UNASSIGNED);
            }
            _CYHAL_GPIO_SET_BTSS_MAP(pin, _CYHAL_GPIO_UNASSIGNED);
        }
#if defined (CYW55900)
        else if (pin < CTSS_GPIO_LAST)
        {
            CTSS_LHL_IO_t ctss_pin = _CYHAL_GPIO_GET_CTSS_MAP(pin);
            if (ctss_pin != _CYHAL_GPIO_UNASSIGNED)
            {
                ctss_lhl_io_enableInterrupt(ctss_pin, false);
                ctss_lhl_io_enable_lhlInterrupt(false);
                _CYHAL_GPIO_SET_CTSS_PAD_MAP(ctss_pin, _CYHAL_GPIO_UNASSIGNED);
            }
            _CYHAL_GPIO_SET_CTSS_MAP(pin, _CYHAL_GPIO_UNASSIGNED);
        }
        else if (pin < WLSS_GPIO_LAST)
        {
            WLSS_IO_t wlss_pin = _CYHAL_GPIO_GET_WLSS_MAP(pin);
            if (wlss_pin != _CYHAL_GPIO_UNASSIGNED)
            {
                wlss_io_enableInterrupt(wlss_pin, false);
                wlss_io_enableGCIInterrupt(false);
                _CYHAL_GPIO_SET_WLSS_PAD_MAP(wlss_pin, _CYHAL_GPIO_UNASSIGNED);
            }
            _CYHAL_GPIO_SET_WLSS_MAP(pin, _CYHAL_GPIO_UNASSIGNED);
        }
#endif // defined (CYW55900)
        else
        {
            /* Not supported */
        }
        cyhal_resource_inst_t pinRsc = _cyhal_utils_get_gpio_resource(pin);
        cyhal_hwmgr_free(&pinRsc);
    }
}


cy_rslt_t cyhal_gpio_configure(cyhal_gpio_t pin, cyhal_gpio_direction_t direction, cyhal_gpio_drive_mode_t drive_mode)
{
    // output buffer is returned if gpio direction is output
    // Default GPIO functionality will be used
    bool init_val = false;
    cyhal_pinmux_t functionality = FUNC_NONE;
    if (pin < BT_GPIO_LAST)
    {
        init_val = (bool)btss_gpio_read(_CYHAL_GPIO_GET_BTSS_MAP(pin));
        for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
        {
            if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].pin == pin)
            {
                functionality = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_BTSS][pinIdx].functionality;
                break;
            }
        }
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        init_val = ctss_lhl_ioGet(_CYHAL_GPIO_GET_CTSS_MAP(pin));
        for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
        {
            if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].pin == pin)
            {
                functionality = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_CTSS][pinIdx].functionality;
                break;
            }
        }
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        init_val = wlss_io_get(_CYHAL_GPIO_GET_WLSS_MAP(pin));
        for (uint8_t pinIdx = 0; pinIdx < sizeof (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS])/ sizeof(cyhal_resource_pin_mapping_t); pinIdx++)
        {
            if (cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].pin == pin)
            {
                functionality = cyhal_pin_map_sw_gpio[CYHAL_GPIO_CLUSTER_WLSS][pinIdx].functionality;
                break;
            }
        }
    }
#endif // defined (CYW55900)
    else
    {
        /* Not Supported */
    }
    _cyhal_gpio_hw(pin, functionality, direction, drive_mode, init_val);

    return CY_RSLT_SUCCESS;
}

void cyhal_gpio_write(cyhal_gpio_t pin, bool value)
{
    if (pin < BT_GPIO_LAST)
    {
        btss_gpio_write(_CYHAL_GPIO_GET_BTSS_MAP(pin), value);
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        ctss_lhl_ioSet(_CYHAL_GPIO_GET_CTSS_MAP(pin), value);
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        wlss_io_set(_CYHAL_GPIO_GET_WLSS_MAP(pin), value);
    }
#endif // defined (CYW55900)
    else
    {
        /* Not Supported */
    }
}

bool cyhal_gpio_read(cyhal_gpio_t pin)
{
    bool status = false;
    if (pin < BT_GPIO_LAST)
    {
        status = btss_gpio_read(_CYHAL_GPIO_GET_BTSS_MAP(pin));
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        status = ctss_lhl_ioGet(_CYHAL_GPIO_GET_CTSS_MAP(pin));
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        status = wlss_io_get(_CYHAL_GPIO_GET_WLSS_MAP(pin));
    }
#endif // defined (CYW55900)
    else
    {
        /* Not Supported */
    }
    return status;
}

void cyhal_gpio_toggle(cyhal_gpio_t pin)
{
    if (pin < BT_GPIO_LAST)
    {
        btss_gpio_toggle(_CYHAL_GPIO_GET_BTSS_MAP(pin));
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        ctss_lhl_io_toggle(_CYHAL_GPIO_GET_CTSS_MAP(pin));
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        wlss_io_toggle(_CYHAL_GPIO_GET_WLSS_MAP(pin));
    }
#endif // defined (CYW55900)
    else
    {
        /* Not Supported */
    }
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
    if (pin < BT_GPIO_LAST)
    {
        btss_gpio_registerInterruptCallback(_CYHAL_GPIO_GET_BTSS_MAP(pin), _cyhal_gpio_btss_irq_handler);
    }
#if defined (CYW55900)
    else if (pin < CTSS_GPIO_LAST)
    {
        ctss_lhl_io_registerInterruptCallback(_CYHAL_GPIO_GET_CTSS_MAP(pin), _cyhal_gpio_ctss_irq_handler);
    }
    else if (pin < WLSS_GPIO_LAST)
    {
        wlss_io_registerInterruptCallback(_CYHAL_GPIO_GET_WLSS_MAP(pin), _cyhal_gpio_wlss_irq_handler);
    }
#endif // defined (CYW55900)
    else
    {
        /* Not supported */
    }

}

void cyhal_gpio_enable_event(cyhal_gpio_t pin, cyhal_gpio_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);

    #define _CYHAL_GPIO_INVALID_TRIGGER     (0xFF)
    BTSS_GPIO_INT_TRIGGER_TYPE_t btss_trigger = (BTSS_GPIO_INT_TRIGGER_TYPE_t)_CYHAL_GPIO_INVALID_TRIGGER;
#if defined (CYW55900)
    CTSS_LHL_IO_INT_TRIGGER_TYPE_t ctss_trigger = (CTSS_LHL_IO_INT_TRIGGER_TYPE_t)_CYHAL_GPIO_INVALID_TRIGGER;
    WLSS_IO_INT_TRIGGER_TYPE_t wlss_trigger = (WLSS_IO_INT_TRIGGER_TYPE_t)_CYHAL_GPIO_INVALID_TRIGGER;
#endif // defined (CYW55900)

    switch (event)
    {
        case CYHAL_GPIO_IRQ_RISE:
            btss_trigger = BTSS_GPIO_TRIGGER_EDGE_RISING;
#if defined (CYW55900)
            ctss_trigger = CTSS_LHL_IO_TRIGGER_EDGE_RISING;
            wlss_trigger = WLSS_IO_INT_TRIGGER_EDGE_RISING;
#endif // defined (CYW55900)
            break;
        case CYHAL_GPIO_IRQ_FALL:
            btss_trigger = BTSS_GPIO_TRIGGER_EDGE_FALLING;
#if defined (CYW55900)
            ctss_trigger = CTSS_LHL_IO_TRIGGER_EDGE_FALLING;
            wlss_trigger = WLSS_IO_INT_TRIGGER_EDGE_FALLING;
#endif // defined (CYW55900)
            break;
        case CYHAL_GPIO_IRQ_BOTH:
            btss_trigger = BTSS_GPIO_TRIGGER_EDGES_BOTH;
#if defined (CYW55900)
            ctss_trigger = CTSS_LHL_IO_TRIGGER_EDGES_BOTH;
            wlss_trigger = WLSS_IO_INT_TRIGGER_EDGE_BOTH;
#endif // defined (CYW55900)
            break;
#if defined (CYW55900)
        case CYHAL_GPIO_IRQ_HIGH:
            ctss_trigger = CTSS_LHL_IO_TRIGGER_LEVEL_HIGH;
            break;
        case CYHAL_GPIO_IRQ_LOW:
            ctss_trigger = CTSS_LHL_IO_TRIGGER_LEVEL_LOW;
            break;
#endif // defined (CYW55900)
        case CYHAL_GPIO_IRQ_NONE:
        default:
            break;
    }

    if ((btss_trigger != _CYHAL_GPIO_INVALID_TRIGGER)
#if defined (CYW55900)
     || (ctss_trigger != _CYHAL_GPIO_INVALID_TRIGGER)
     || (wlss_trigger != _CYHAL_GPIO_INVALID_TRIGGER)
#endif // defined (CYW55900)
       )
    {
        if (pin < BT_GPIO_LAST)
        {
            btss_gpio_configInterrupt(_CYHAL_GPIO_GET_BTSS_MAP(pin), btss_trigger);
            btss_gpio_enableInterrupt(_CYHAL_GPIO_GET_BTSS_MAP(pin), enable);
        }
#if defined (CYW55900)
        else if (pin < CTSS_GPIO_LAST)
        {
            ctss_lhl_io_configInterrupt(_CYHAL_GPIO_GET_CTSS_MAP(pin), ctss_trigger);
            ctss_lhl_io_enableInterrupt(_CYHAL_GPIO_GET_CTSS_MAP(pin), enable);
            /* Enable/Disable global interrupt for LHL IOs  */
            ctss_lhl_io_enable_lhlInterrupt(enable);
        }
        else if (pin < WLSS_GPIO_LAST)
        {
            wlss_io_configInterrupt(_CYHAL_GPIO_GET_WLSS_MAP(pin), wlss_trigger);
            wlss_io_enableInterrupt(_CYHAL_GPIO_GET_WLSS_MAP(pin), enable);
            /* Enable/Disable GCI interrupt to core */
            wlss_io_enableGCIInterrupt(enable);
        }
#endif // defined (CYW55900)
        else
        {
            /* Not supported */
        }
#if (CYHAL_DRIVER_AVAILABLE_SYSPM == 1)
        (void)_cyhal_syspm_set_gpio_wakeup_source(pin, event, enable);
#endif
    }
    else
    {
        // Turn off the interrupt if invalid type
        if (pin < BT_GPIO_LAST)
        {
            btss_gpio_enableInterrupt(_CYHAL_GPIO_GET_BTSS_MAP(pin), false);
        }
#if defined (CYW55900)
        else if (pin < CTSS_GPIO_LAST)
        {
            ctss_lhl_io_enableInterrupt(_CYHAL_GPIO_GET_CTSS_MAP(pin), false);
            /* Disable global interrupt for LHL IOs  */
            ctss_lhl_io_enable_lhlInterrupt(false);
        }
        else if (pin < WLSS_GPIO_LAST)
        {
            wlss_io_enableInterrupt(_CYHAL_GPIO_GET_WLSS_MAP(pin), false);
            /* Disable GCI interrupt to core */
            wlss_io_enableGCIInterrupt(false);
        }
#endif // defined (CYW55900)
        else
        {
            /* Not supported */
        }
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
