/*******************************************************************************
* File Name: cyhal_analog_common.h
*
* Description:
* Provides common functionality for the analog drivers.
*
********************************************************************************
* \copyright
* Copyright 2022-2023 Cypress Semiconductor Corporation (an Infineon company) or
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

#include <stdbool.h>
#include <stdint.h>
#include "cyhal_gpio.h"
#include "cyhal_hw_resources.h"

#if (CYHAL_DRIVER_AVAILABLE_ADC || CYHAL_DRIVER_AVAILABLE_COMP)
#include "cy_adccomp.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/** \cond INTERNAL */

extern CyADCCOMP_Type *const _cyhal_adccomp_base[CY_IP_MXS40ADCMIC_INSTANCES];

#if (CYHAL_DRIVER_AVAILABLE_ADC)
void _cyhal_adcmic_calibrate();
void _cyhal_adcmic_get_result();
#endif

#if (CYHAL_DRIVER_AVAILABLE_COMP)
void _cyhal_comp_process_event(uint32_t intr);
#endif

cy_rslt_t _cyhal_adccomp_register_cb(void);
void _cyhal_adccomp_cb(void);

/** \endcond */

#if defined(__cplusplus)
}
#endif

#endif /* (CYHAL_DRIVER_AVAILABLE_ADC || CYHAL_DRIVER_AVAILABLE_COMP) */
