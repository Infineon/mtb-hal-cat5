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
#define _CYHAL_GPIO_CFG_CLR(mask, cfg)  ((uint8_t)~mask) & cfg)

/* Pin configurations */
#define _CYHAL_GPIO_INPUT_ENABLE(cfg)       _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_INPUT_DISABLE(cfg)      _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_INPUT_DISABLE_MASK, cfg)
#define _CYHAL_GPIO_OUTPUT_ENABLE(cfg)      _CYHAL_GPIO_CFG_SET(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
#define _CYHAL_GPIO_OUTPUT_DISABLE(cfg)     _CYHAL_GPIO_CFG_CLR(BTSS_PAD_CONFIG_OUTPUT_ENABLE_MASK, cfg)
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


/*******************************************************************************
*       BWC Defines
*******************************************************************************/

// TODO: These are need for a successful build. 
// They are PSoC implementations that need to be modified for this device.

#define Cy_GPIO_PortToAddr(x)       NULL
#define Cy_GPIO_GetHSIOM(x, y)      0
#define Cy_GPIO_SetHSIOM(x, y, z)   NULL
#define Cy_GPIO_Set(x, y)           NULL
#define Cy_GPIO_Clr(x, y)           NULL

/** Gets a pin definition from the provided port and pin numbers */
#define CYHAL_GET_GPIO(port, pin)   ((((uint8_t)(port)) << 3U) + ((uint8_t)(pin)))
/** Macro that, given a gpio, will extract the pin number */
#define CYHAL_GET_PIN(pin)          ((uint8_t)(((uint8_t)pin) & 0x07U))
/** Macro that, given a gpio, will extract the port number */
#define CYHAL_GET_PORT(pin)         ((uint8_t)(((uint8_t)pin) >> 3U))
/**< Macro to get the port address from pin */
#define CYHAL_GET_PORTADDR(pin)    (Cy_GPIO_PortToAddr(CYHAL_GET_PORT(pin)))


#if defined(__cplusplus)
}
#endif /* __cplusplus */
