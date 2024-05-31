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

#define _CYHAL_GPIO_CFG_SET(mask, cfg)  (mask | cfg)
#define _CYHAL_GPIO_CFG_CLR(mask, cfg)  (((uint8_t)~mask) & cfg)

/* Pin configurations */
#define _CYHAL_GPIO_INPUT_ENABLE(cfg)       _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_INPUT_DISABLE(cfg)      _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#if defined (CYW55500A0)
#define _CYHAL_GPIO_OUTPUT_ENABLE(cfg)      _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_OUTPUT_DISABLE(cfg)     _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
#else
#define _CYHAL_GPIO_OUTPUT_ENABLE(cfg)      _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_OUTPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_OUTPUT_DISABLE(cfg)     _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_OUTPUT_DISABLE_MASK, cfg)
#endif
#define _CYHAL_GPIO_HYSTERESIS_ON(cfg)      _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_HYSTERESIS_OFF(cfg)     _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_PULL_UP_DOWN_NONE(cfg)  _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK | BTSS_PAD_CONFIG_PULL_DOWN_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_PULL_UP(cfg)            _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_PULL_DOWN(cfg)          _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_PULL_DOWN_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_2MA(cfg)  _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_02MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_4MA(cfg)  _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_04MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_6MA(cfg)  _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_06MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_8MA(cfg)  _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_08MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_10MA(cfg) _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_10MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_12MA(cfg) _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_12MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_14MA(cfg) _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_14MA_MASK, cfg)
#define _CYHAL_GPIO_ARM_DRIVE_SEL_16MA(cfg) _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK, cfg)

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
