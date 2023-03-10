/*******************************************************************************
* File Name: cyhal_clock_impl.h
*
* Description:
* Implementation details of Infineon Clock.
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

#include "cyhal_hw_resources.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * \addtogroup group_hal_impl_clock Clocks
 * \ingroup group_hal_impl
 * \{
 * Implementation specific interface for using the Clock driver. These items, while usable
 * within the HAL, are <b>not</b> necessarily portable between devices.
 *
 * \section section_clock_snippets_impl Code snippets
 * \note Error handling code has been intentionally left out of snippets to highlight API usage.
 *
 * \subsection subsection_clock_snippet_5_impl Snippet: System initialization
 * The system clocks are initialized in the cybsp_init() function.
 *
 * \note CAT5 SCB System Clock Maximum frequency is 192MHz, maximum divider
 * value is 128 and the maximum oversample value is 16. This limits the minimum
 * supported UART baud rate to be 115200bps and the minimum supported SPI clock
 * frequency to be 100KHz.
 */

/** PERI_SCB0 : Provides the clock for SCB0 */
extern const cyhal_clock_t CYHAL_CLOCK_PERI_SCB0;
/** PERI_SCB1 : Provides the clock for SCB1 */
extern const cyhal_clock_t CYHAL_CLOCK_PERI_SCB1;
/** PERI_SCB2 : Provides the clock for SCB2 */
extern const cyhal_clock_t CYHAL_CLOCK_PERI_SCB2;
/** PERI_TCPWM : Provides the clock for TCPWM */
extern const cyhal_clock_t CYHAL_CLOCK_PERI_TCPWM;
/** TDM : Provides the clock for TDM */
extern const cyhal_clock_t CYHAL_CLOCK_TDM;
/** ADCMIC : Provides the clock for ADCMIC */
extern const cyhal_clock_t CYHAL_CLOCK_ADCMIC;
/** CPU : Provides the clock for CPU */
extern const cyhal_clock_t CYHAL_CLOCK_CPU;

/** PERI_SCB0 : Provides the clock for SCB0 */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB0;
/** PERI_SCB1 : Provides the clock for SCB1 */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB1;
/** PERI_SCB2 : Provides the clock for SCB2 */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB2;
/** PERI_TCPWM : Provides the clock for TCPWM */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_TCPWM;
/** TDM : Provides the clock for TDM */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_TDM;
/** ADCMIC : Provides the clock for ADCMIC */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_ADCMIC;
/** CPU : Provides the clock for CPU */
extern const cyhal_resource_inst_t CYHAL_CLOCK_RSC_CPU;

/** \cond INTERNAL */
/** Performs a one time initialization of all system clocks. This should be done as part of initial
 * device startup.
 *
 * @return CY_RSLT_SUCCESS if the system clocks were properly initialized.
 */
cy_rslt_t _cyhal_clock_system_init();
/** \endcond */

/** \cond INTERNAL */
/** Gets if the clock divider value is valid.
 *
 * @param[in] resource  The clock resource whose divider value to be validated
 * @param[in] divider  The clock divider value
 * @return Whether the clock divider value is valid or not
 */
bool _cyhal_clock_is_divider_valid(const cyhal_resource_inst_t *resource,uint32_t divider);

/** \endcond */
/** \} group_hal_impl_clock */

#if defined(__cplusplus)
}
#endif
