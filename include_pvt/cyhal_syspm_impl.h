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

#include "cyhal.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_hal_impl_syspm Syspm
 * \ingroup group_hal_impl
 * \{
 * \section group_hal_impl_syspm_process Low power entry/exit
 * On this device, low power mode (LPM) entry is only allowed when no threads are active
 * and when the low power callbacks succeed. The power modes are implemented as follows.
 * 
 * * Deep Sleep mode - The HAL implmentation maps this to PDS (Power Down Sleep) mode.
 *   Peripheral clocks are turned off and allows lower power to be consumed.
 * * Other sleep modes are not supported yet
 * 
 * 
 * \} group_hal_impl_syspm */

/**
* \cond INTERNAL
*/

/** Sleep mode */
#if defined(CYW55900)
#define _CYHAL_SYSPM_SLEEP_MODE         BTSS_SYSTEM_SLEEP_MODE_NO_TRANSPORT
#else
#if defined(CYHAL_SYSPM_WITH_TRANSPORT_MODE)
#define _CYHAL_SYSPM_SLEEP_MODE         BTSS_SYSTEM_SLEEP_MODE_WITH_TRANSPORT
#else
#define _CYHAL_SYSPM_SLEEP_MODE         BTSS_SYSTEM_SLEEP_MODE_NO_TRANSPORT
#endif
#endif

/** EPDS Sleep state */
#define CYHAL_SYSPM_EPDS_ENABLED        (0)
/*******************************************************************************
*       Data Structures
*******************************************************************************/
/* Connection type definition */
/** Represents an association between a pin and wake-up sources */
typedef struct
{
    BTSS_SYSTEM_SLEEP_PMU_WAKE_SRC_t    wakeup_src;     //!< Wake-up source associated to the pin
    cyhal_gpio_t                        pin;            //!< The GPIO pin
} _cyhal_wakeup_src_btss_pin_mapping_t;

/*******************************************************************************
*       Functions
*******************************************************************************/

void _cyhal_syspm_register_peripheral_callback(cyhal_syspm_callback_data_t *callback_data);
void _cyhal_syspm_unregister_peripheral_callback(cyhal_syspm_callback_data_t *callback_data);
cy_rslt_t _cyhal_syspm_set_gpio_wakeup_source(cyhal_gpio_t pin, cyhal_gpio_event_t event, bool enable);
cy_rslt_t _cyhal_syspm_set_lpcomp_wakeup_source(cy_en_adccomp_lpcomp_id_t comp_ch, bool enable);
/** \endcond */

#if defined(__cplusplus)
}
#endif
