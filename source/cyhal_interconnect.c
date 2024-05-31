/***************************************************************************//**
* \file cyhal_interconnect.c
*
* \brief
* Provides a high level interface for interacting with the internal digital
* routing on the chip. This is a wrapper around the lower level PDL API.
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

#include "cyhal_interconnect.h"
#include "cyhal_gpio_impl.h"

#include "btss_pinmux.h"

#if defined(__cplusplus)
extern "C"
{
#endif

cy_rslt_t cyhal_connect_pin(const cyhal_resource_pin_mapping_t *pin_connection, uint8_t drive_mode)
{
    cyhal_gpio_t pin = pin_connection->pin;
    cyhal_pinmux_t functionality = pin_connection->functionality;
    bool status = true;

    // Skip configuration if the connection is direct
    if ((BTSS_PAD_LIST_t)pin < PAD_MAX_NUM)
    {
        // Skip drive mode setup and evaluation for GPIOs as it is done in the GPIO driver.
        // Init value appears to always be high for peripherals. Revisit if it is not.
        if (pin_connection->block_num != CYHAL_RSC_GPIO)
            status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)pin, (BTSS_PAD_HW_CONFIG_t)drive_mode);
        
        // FUNC_NONE is currently not supported. Remove the check when it is.
        if (status && (functionality != FUNC_NONE))
            status = btss_pad_assignFunction((BTSS_PAD_LIST_t)pin, (BTSS_PINMUX_FUNC_LIST_t)functionality);
    }

    return (status) ? CY_RSLT_SUCCESS : CYHAL_INTERCONNECT_RSLT_INVALID_CONNECTION;
}

cy_rslt_t cyhal_disconnect_pin(cyhal_gpio_t pin)
{
    bool status = btss_pad_setHwConfig((BTSS_PAD_LIST_t)pin, (BTSS_PAD_HW_CONFIG_t)0u);
    return (status) ? CY_RSLT_SUCCESS : CYHAL_INTERCONNECT_RSLT_CANNOT_DISCONNECT;
}

#if defined(__cplusplus)
}
#endif
