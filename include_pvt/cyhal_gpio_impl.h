/***************************************************************************//**
* \file cyhal_gpio_impl.h
*
* Description:
* Provides a high level interface for interacting with the Infineon GPIO. This is
* a wrapper around the lower level PDL API.
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

#pragma once

#include "cyhal_gpio.h"
#include "cyhal_utils.h"
#include "cy_utils.h"
#include "btss_pinmux.h"
#include "btss_gpio.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*       Functions
*******************************************************************************/

#define _CYHAL_GPIO_BTSS_CFG_SET(mask, cfg)  (mask | cfg)
#define _CYHAL_GPIO_BTSS_CFG_CLR(mask, cfg)  (((uint8_t)~mask) & cfg)

/* Pin configurations */

/* BTSS */
#define _CYHAL_GPIO_BTSS_INPUT_ENABLE(cfg)       _CYHAL_GPIO_BTSS_CFG_CLR(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_INPUT_DISABLE(cfg)      _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#if defined (CYW55500A0)
#define _CYHAL_GPIO_BTSS_OUTPUT_ENABLE(cfg)      _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_OUTPUT_DISABLE(cfg)     _CYHAL_GPIO_BTSS_CFG_CLR(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
#else
#define _CYHAL_GPIO_BTSS_OUTPUT_ENABLE(cfg)      _CYHAL_GPIO_BTSS_CFG_CLR(BTSS_PAD_CONFIG_OUTPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_OUTPUT_DISABLE(cfg)     _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_OUTPUT_DISABLE_MASK, cfg)
#endif
#define _CYHAL_GPIO_BTSS_HYSTERESIS_ON(cfg)      _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_HYSTERESIS_OFF(cfg)     _CYHAL_GPIO_BTSS_CFG_CLR(BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_PULL_UP_DOWN_NONE(cfg)  _CYHAL_GPIO_BTSS_CFG_CLR(BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK | BTSS_PAD_CONFIG_PULL_DOWN_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_PULL_UP(cfg)            _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_PULL_DOWN(cfg)          _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_PULL_DOWN_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_2MA(cfg)  _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_02MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_4MA(cfg)  _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_04MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_6MA(cfg)  _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_06MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_8MA(cfg)  _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_08MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_10MA(cfg) _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_10MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_12MA(cfg) _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_12MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_14MA(cfg) _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_14MA_MASK, cfg)
#define _CYHAL_GPIO_BTSS_ARM_DRIVE_SEL_16MA(cfg) _CYHAL_GPIO_BTSS_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK, cfg)

#if defined (CYW55900)
/* CTSS */
#define _CYHAL_GPIO_CTSS_INPUT_ENABLE()          (ctss_pad_configParamInit(0, CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_INPUT_DISABLE()         (ctss_pad_configParamInit(CTSS_PAD_INPUT_DISABLED, 0))
#define _CYHAL_GPIO_CTSS_OUTPUT_ENABLE()         (ctss_pad_configParamInit(CTSS_PAD_INPUT_DISABLED, CTSS_PAD_OUTPUT_ENABLED))
#define _CYHAL_GPIO_CTSS_OUTPUT_DISABLE()        (ctss_pad_configParamInit(0, CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_INOUT_ENABLE()          (ctss_pad_configParamInit(0, CTSS_PAD_OUTPUT_ENABLED))
#define _CYHAL_GPIO_CTSS_INOUT_DISABLE()         (ctss_pad_configParamInit(CTSS_PAD_INPUT_DISABLED, CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_PULL_UP_DOWN_NONE()     (ctss_pad_configParamInit(CTSS_PAD_INPUT_DISABLED, CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_PULL_UP_DOWN()          (ctss_pad_configParamInit((CTSS_PAD_INPUT_PUP | CTSS_PAD_INPUT_PDN), CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_PULL_UP()               (ctss_pad_configParamInit(CTSS_PAD_INPUT_PUP, CTSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_CTSS_PULL_DOWN()             (ctss_pad_configParamInit(CTSS_PAD_INPUT_PDN, CTSS_PAD_OUTPUT_DISABLED))

/* WLSS */
#define _CYHAL_GPIO_WLSS_INPUT_ENABLE()          (wlss_pad_configParamInit(WLSS_PAD_INPUT_ENABLED, WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_INPUT_DISABLE()         (wlss_pad_configParamInit(WLSS_PAD_INPUT_DISABLED, 0))
#define _CYHAL_GPIO_WLSS_OUTPUT_ENABLE()         (wlss_pad_configParamInit(WLSS_PAD_INPUT_DISABLED, WLSS_PAD_OUTPUT_ENABLED))
#define _CYHAL_GPIO_WLSS_OUTPUT_DISABLE()        (wlss_pad_configParamInit(0, WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_INOUT_ENABLE()          (wlss_pad_configParamInit(WLSS_PAD_INPUT_ENABLED, WLSS_PAD_OUTPUT_ENABLED))
#define _CYHAL_GPIO_WLSS_INOUT_DISABLE()         (wlss_pad_configParamInit(WLSS_PAD_INPUT_DISABLED, WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_PULL_UP_DOWN_NONE()     (wlss_pad_configParamInit(WLSS_PAD_INPUT_DISABLED, WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_PULL_UP_DOWN()          (wlss_pad_configParamInit((WLSS_PAD_INPUT_PUP | WLSS_PAD_INPUT_PDN), WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_PULL_UP()               (wlss_pad_configParamInit(WLSS_PAD_INPUT_PUP, WLSS_PAD_OUTPUT_DISABLED))
#define _CYHAL_GPIO_WLSS_PULL_DOWN()             (wlss_pad_configParamInit(WLSS_PAD_INPUT_PDN, WLSS_PAD_OUTPUT_DISABLED))
#endif // defined (CYW55900)

// Callback array for GPIO interrupts
#if defined (CYW55900)
extern CY_NOINIT cyhal_gpio_callback_data_t* _cyhal_gpio_callbacks[WLSS_GPIO_LAST];
#else
extern CY_NOINIT cyhal_gpio_callback_data_t* _cyhal_gpio_callbacks[BT_GPIO_LAST];
#endif // defined (CYW55900)

// Used to keep track of BTSS pin functionality to BTSS/CTSS/WLSS Pad assignment
extern CY_NOINIT cyhal_gpio_t _cyhal_btss_pad_map[BT_GPIO_LAST + 1];
extern CY_NOINIT BTSS_GPIO_t _cyhal_pad_btss_map[BT_GPIO_LAST];

#define _CYHAL_GPIO_GET_BTSS_MAP(pin)               _cyhal_pad_btss_map[pin]
#define _CYHAL_GPIO_SET_BTSS_MAP(pin, btss_pin)     _cyhal_pad_btss_map[pin] = btss_pin
#define _CYHAL_GPIO_GET_BTSS_PAD_MAP(btss_pin)      _cyhal_btss_pad_map[btss_pin]
#define _CYHAL_GPIO_SET_BTSS_PAD_MAP(btss_pin, pin) _cyhal_btss_pad_map[btss_pin] = pin

#if defined (CYW55900)
// Used to keep track of BT/CT/WL Pad to BTSS/CTSS/WLSS pin assignment
extern CY_NOINIT cyhal_gpio_t _cyhal_ctss_pad_map[(CTSS_GPIO_LAST - BT_GPIO_LAST) + 1];
extern CY_NOINIT CTSS_LHL_IO_t _cyhal_pad_ctss_map[(CTSS_GPIO_LAST - BT_GPIO_LAST)];


#define _CYHAL_GPIO_GET_CTSS_MAP(pin)               _cyhal_pad_ctss_map[(pin - BT_GPIO_LAST)]
#define _CYHAL_GPIO_SET_CTSS_MAP(pin, ctss_pin)     _cyhal_pad_ctss_map[(pin - BT_GPIO_LAST)] = ctss_pin
#define _CYHAL_GPIO_GET_CTSS_PAD_MAP(ctss_pin)      _cyhal_ctss_pad_map[(ctss_pin - BT_GPIO_LAST)]
#define _CYHAL_GPIO_SET_CTSS_PAD_MAP(ctss_pin, pin) _cyhal_ctss_pad_map[(ctss_pin - BT_GPIO_LAST)] = pin


extern CY_NOINIT cyhal_gpio_t _cyhal_wlss_pad_map[(WLSS_GPIO_LAST - CTSS_GPIO_LAST) + 1];
extern CY_NOINIT WLSS_IO_t _cyhal_pad_wlss_map[(WLSS_GPIO_LAST - CTSS_GPIO_LAST)];

#define _CYHAL_GPIO_GET_WLSS_MAP(pin)               _cyhal_pad_wlss_map[(pin - CTSS_GPIO_LAST)]
#define _CYHAL_GPIO_SET_WLSS_MAP(pin, wlss_pin)     _cyhal_pad_wlss_map[(pin - CTSS_GPIO_LAST)] = wlss_pin
#define _CYHAL_GPIO_GET_WLSS_PAD_MAP(wlss_pin)      _cyhal_wlss_pad_map[(wlss_pin - CTSS_GPIO_LAST)]
#define _CYHAL_GPIO_SET_WLSS_PAD_MAP(wlss_pin, pin) _cyhal_wlss_pad_map[(wlss_pin - CTSS_GPIO_LAST)] = pin
#endif // defined (CYW55900)
/** Switch to GPIO output and set it to logic 1.
 *
 * @param[in] gpio           The gpio to switch and set
 * @return The result of the operation. If true is successful.
 */
bool _cyhal_gpio_switch_and_set(cyhal_gpio_t gpio);

/*******************************************************************************
*       BWC Defines
*******************************************************************************/
/** Macro that, given a gpio and function value, will setup the functionality */
#define Cy_GPIO_SetHSIOM(x, gpio, value)    (void)btss_pad_assignFunction((BTSS_PAD_LIST_t)gpio, value)
/** Macro that, given a gpio, will extract it's function */
#define Cy_GPIO_GetHSIOM(x, gpio)           btss_pad_getFunction((BTSS_PAD_LIST_t)gpio)
/** Macro that, given a gpio, will extract the pin number */
#define CYHAL_GET_PIN(gpio)                 (gpio)
/**< Macro to get the port address from pin */
#define CYHAL_GET_PORTADDR(gpio)            ((GPIO_PRT_Type*)gpio)

/**< Internal API to convert GPIO functionality into BTSS */
extern BTSS_GPIO_t _cyhal_gpio_convert_func_to_btss(BTSS_PINMUX_FUNC_LIST_t functionality);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
