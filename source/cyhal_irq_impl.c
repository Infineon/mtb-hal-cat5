/***************************************************************************//**
* \file cyhal_irq_impl.c
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

#include "cyhal_irq_impl.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define _CYHAL_IRQ_NR       9   /* Total number of interrupts to cover in this driver */

#define _CYHAL_IRQ_TCPWM0   0
#define _CYHAL_IRQ_TCPWM1   1
#define _CYHAL_IRQ_SCB0     2
#define _CYHAL_IRQ_SCB1     3
#define _CYHAL_IRQ_SCB2     4
#define _CYHAL_IRQ_TDM0     5
#define _CYHAL_IRQ_TDM1     6

bool _cyhal_irq_status[_CYHAL_IRQ_NR] = {0};

bool _cyhal_irq_state(_cyhal_system_irq_t system_irq, bool enable, bool write)
{
    bool irq_status = false;
    
    switch (system_irq)
    {
        case tcpwm_0_interrupts_0_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_TCPWM0] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_TCPWM0];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_TCPWM0];
            break;
        case tcpwm_0_interrupts_256_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_TCPWM1] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_TCPWM1];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_TCPWM1];
            break;
        case scb_0_interrupt_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_SCB0] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_SCB0];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_SCB0];
            break;
        case scb_1_interrupt_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_SCB1] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_SCB1];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_SCB1];
            break;
        case scb_2_interrupt_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_SCB2] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_SCB2];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_SCB2];
            break;
        case tdm_0_interrupts_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_TDM0] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_TDM0];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_TDM0];
            break;
        case tdm_1_interrupts_IRQn:
            _cyhal_irq_status[_CYHAL_IRQ_TDM1] = write ? enable : _cyhal_irq_status[_CYHAL_IRQ_TDM1];
            irq_status = _cyhal_irq_status[_CYHAL_IRQ_TDM1];
            break;
        default:
            break;
    }

    return irq_status;    
}

#if defined(__cplusplus)
}
#endif
