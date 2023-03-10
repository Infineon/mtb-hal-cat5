/***************************************************************************//**
* \file cyhal_system.c
*
* Description:
* Provides a high level interface for interacting with the System driver.
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

#include "cyhal_system.h"
#include "cy_utils.h"
#include "cyhal_pin_package.h"

#if defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE)
#include "cyabs_rtos.h"
#endif


/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_system_delay_ms(uint32_t milliseconds)
{
#if defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE)
    return cy_rtos_delay_milliseconds(milliseconds);
#else
    for (uint16_t msec = 0; msec < milliseconds; msec++)
    {
        cyhal_system_delay_us(1000);
    }
    return CY_RSLT_SUCCESS;
#endif
}

cy_rslt_t cyhal_system_set_isr(int32_t irq_num, int32_t irq_src, uint8_t priority, cyhal_irq_handler handler)
{
    /* Interrupt connections are dedicated */
    CY_UNUSED_PARAMETER(irq_num);
    CY_UNUSED_PARAMETER(irq_src);
    CY_UNUSED_PARAMETER(priority);
    CY_UNUSED_PARAMETER(handler);
    return CYHAL_SYSTEM_RSLT_ERR_NOT_SUPPORTED;
}

#if defined(__cplusplus)
}
#endif
