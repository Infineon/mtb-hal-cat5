/***************************************************************************//**
* \file cyhal_syspm_impl.h
*
* \brief
* Provides a specific interface for interacting with the Infineon power
* management and system clock configuration. This interface abstracts out the
* chip specific details. If any chip specific functionality is necessary, or
* performance is critical the low level functions can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2018-2021 Cypress Semiconductor Corporation (an Infineon company) or
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

#if defined(__cplusplus)
extern "C" {
#endif

/**
* \cond INTERNAL
*/

void _cyhal_syspm_register_peripheral_callback(cyhal_syspm_callback_data_t *callback_data);
void _cyhal_syspm_unregister_peripheral_callback(cyhal_syspm_callback_data_t *callback_data);

cy_rslt_t cyhal_syspm_tickless_sleep_deepsleep(cyhal_lptimer_t *obj, uint32_t desired_ms, uint32_t *actual_ms, bool deep_sleep);
/*
#define cyhal_syspm_tickless_deepsleep(obj, desired_ms, actual_ms) cyhal_syspm_tickless_sleep_deepsleep(obj, desired_ms, actual_ms, true)

#define cyhal_syspm_tickless_sleep(obj, desired_ms, actual_ms) cyhal_syspm_tickless_sleep_deepsleep(obj, desired_ms, actual_ms, false)
*/
/** \endcond */

#if defined(__cplusplus)
}
#endif
