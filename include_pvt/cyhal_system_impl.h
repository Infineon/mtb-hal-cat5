/***************************************************************************//**
* \file cyhal_system_impl.h
*
* \brief
* Provides a PSoC™ Specific interface for interacting with the Infineon power
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

#pragma once

#include "cyhal_system.h"
#include "btss_system.h"

#define cyhal_system_delay_us(x)                Cy_SysLib_DelayUs(x)
#define cyhal_system_critical_section_enter()   Cy_SysLib_EnterCriticalSection()
#define cyhal_system_critical_section_exit(x)   Cy_SysLib_ExitCriticalSection(x)

/* The only way to reset is to do a force reset from SW. May be possible to know using non-volatile RAM */
#define cyhal_system_get_reset_reason()         CYHAL_SYSTEM_RESET_NONE
#define cyhal_system_clear_reset_reason()       /* NOP */
#define cyhal_system_reset_device()             btss_system_forceSystemReset()

static inline void __enable_irq(void) 
{
    /* Enable all peripheral interrupts */
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_GPIO);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_A_GPIO);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_B_GPIO);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_T2_TIMER1);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_T2_TIMER2);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_SCB0);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_SCB1);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_SCB2);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_ADCCOMP);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_MXTDM0);
    btss_system_intEnable(BTSS_SYSTEM_INTERRUPT_MXTDM1);
};

static inline void __disable_irq(void)
{
    /* Disable all peripheral interrupts */
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_GPIO);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_A_GPIO);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_B_GPIO);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_T2_TIMER1);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_T2_TIMER2);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_SCB0);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_SCB1);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_SCB2);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_ADCCOMP);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_MXTDM0);
    btss_system_intDisable(BTSS_SYSTEM_INTERRUPT_MXTDM1);
};
