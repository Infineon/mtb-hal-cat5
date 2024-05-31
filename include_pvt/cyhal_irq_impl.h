/***************************************************************************//**
* \file cyhal_irq_impl.h
*
* \brief
* Provides internal utility functions for working with interrupts on CAT5.
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
#include "cyhal_utils.h"
#include "cy_result.h"

/** \cond INTERNAL */

typedef int32_t  _cyhal_system_irq_t;

bool _cyhal_irq_state(_cyhal_system_irq_t system_irq, bool enable, bool write);

static inline cy_rslt_t _cyhal_irq_register(_cyhal_system_irq_t system_irq, uint8_t intr_priority, cy_israddress irq_handler)
{
    CY_UNUSED_PARAMETER(system_irq);
    CY_UNUSED_PARAMETER(intr_priority);
    CY_UNUSED_PARAMETER(irq_handler);
    return CY_RSLT_SUCCESS;
}

static inline void _cyhal_irq_set_priority(_cyhal_system_irq_t system_irq, uint8_t intr_priority)
{
    CY_UNUSED_PARAMETER(system_irq);
    CY_UNUSED_PARAMETER(intr_priority);
}

static inline uint8_t _cyhal_irq_get_priority(_cyhal_system_irq_t system_irq)
{
    CY_UNUSED_PARAMETER(system_irq);
    return 0u;
}

static inline void _cyhal_irq_clear_pending(_cyhal_system_irq_t system_irq)
{
    CY_UNUSED_PARAMETER(system_irq);
}

static inline void _cyhal_irq_enable(_cyhal_system_irq_t system_irq)
{
    (void)_cyhal_irq_state(system_irq, true, true);
}

static inline void _cyhal_irq_disable(_cyhal_system_irq_t system_irq)
{
    (void)_cyhal_irq_state(system_irq, false, true);
}

static inline void _cyhal_irq_free(_cyhal_system_irq_t system_irq)
{  
    // TCPWM counter interrupts are ganged. Better not to track them as disabled.
    if ((system_irq != tcpwm_0_interrupts_0_IRQn) && (system_irq != tcpwm_0_interrupts_256_IRQn))
        _cyhal_irq_disable(system_irq);
}

static inline bool _cyhal_irq_is_enabled(_cyhal_system_irq_t system_irq)
{
    return _cyhal_irq_state(system_irq, false, false);
}

static inline _cyhal_system_irq_t _cyhal_irq_get_active(void)
{
    return 0;
}

/** \endcond */
