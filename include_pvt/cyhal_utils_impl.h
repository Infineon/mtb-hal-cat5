/***************************************************************************//**
* \file cyhal_utils_impl.h
*
* \brief
* Provides utility functions for working with the CAT5 HAL implementation.
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

/** \cond INTERNAL */
/**
* \addtogroup group_hal_impl
* \{
* Common utility macros & functions used by multiple HAL drivers.
*/

#pragma once

#include "cy_result.h"
#include "cyhal_hw_types.h"
#include "cyhal_triggers.h"
#include "cyhal_clock.h"
#include "cy_scb.h"

typedef int32_t  _cyhal_system_irq_t;
#define IRQn_Type _cyhal_system_irq_t


#if defined(__cplusplus)
extern "C" {
#endif

/**
* \addtogroup group_hal_impl_pin_package
* \{
*/

/** Converts the provided gpio pin to a resource instance object
 *
 * @param[in] pin  The pin to get a resource object for
 * @return The equivalent resource instance object for the provided pin.
 */
cyhal_resource_inst_t _cyhal_utils_get_gpio_resource(cyhal_gpio_t pin);

/** Attempts to reserve the specified pin and then initialize it to connect to the item defined by the provided mapping object.
 * @param[in] mapping    The pin/hardware block connection mapping information
 * @param[in] drive_mode The drive mode for the pin
 * @return CY_RSLT_SUCCESS if everything was ok, else an error.
 */
cy_rslt_t _cyhal_utils_reserve_and_connect(const cyhal_resource_pin_mapping_t *mapping, uint8_t drive_mode);

/** Disconnects any routing for the pin from the interconnect driver and then free's the pin from the hwmgr.
 *
 * @param[in] pin       The pin to disconnect and free
 */
void _cyhal_utils_disconnect_and_free(cyhal_gpio_t pin);

/** \} group_hal_impl_pin_package */


/** Gets the peripheral clock frequency that is feeding the clock tree for the specified
 * resource.
 *
 * @param[in] clocked_item  The resource to get the frequency for
 * @return The peripheral clock frequency for the provided resource type
 */
uint32_t _cyhal_utils_get_peripheral_clock_frequency(const cyhal_resource_inst_t *clocked_item);

/** Calculate a peripheral clock divider value that needs to be set to reach the frequency closest
 * to the one requested.
 * \note The caller may need to subtract one from the value returned in order to align with the
 * API/register requirements. This is necessary if the API/register expects a value of 0 to mean
 * divide by 1.
 *
 * @param[in] clocked_item  The resource to get the frequency for
 * @param[in] frequency     The desired frequency
 * @param[in] frac_bits     The number of fractional bits that the divider has
 * @return The calculated divider value
 */
static inline uint32_t _cyhal_utils_divider_value(const cyhal_resource_inst_t *clocked_item, uint32_t frequency, uint32_t frac_bits)
{
    return ((_cyhal_utils_get_peripheral_clock_frequency(clocked_item) * (1 << frac_bits)) + (frequency / 2)) / frequency;
}

/** Converts a hal pm mode to a pdl mode
 *
 * @param[in] mode          hal power management callback mode.
 * @return Equivalent pdl syspm mode.
 */
static inline cy_en_syspm_callback_mode_t _cyhal_utils_convert_haltopdl_pm_mode(cyhal_syspm_callback_mode_t mode)
{
    return (cy_en_syspm_callback_mode_t)mode;
}

/**
 * Allocates a clock that can drive the specified instance.
 *
 * @param[out]  clock               The clock object to initialize
 * @param[in]   clocked_item        The destination that the allocated clock must be able to drive
 * @param[in]   div                 The divider width that is required. This is ignored if the block is hard-wired to
 *                                  an HFCLK output
 * @param[in]   accept_larger       If no dividers of the specified width are available, can a wider divider be
 *                                  substituted.
 */
cy_rslt_t _cyhal_utils_allocate_clock(cyhal_clock_t *clock, const cyhal_resource_inst_t *clocked_item, cyhal_clock_block_t div, bool accept_larger);

/**
 * Attempts to set the clock to the specified frequency. This is similar to cyhal_clock_set_frequency,
 * but it will also make an attempt to set the frequency for HFCLK outputs, which are not supported by the public
 * API due to their limited range of supported dividers (1, 2, 4, 8)
 *
 * @param[in] clock                 The clock instance to set the frequency for.
 * @param[in] hz                    The frequency, in hertz, to set the clock to.
 * @param[in] tolerance             The allowed tolerance from the desired hz that is acceptable, use NULL if no
 *                                  tolerance check is required.
 */
static inline cy_rslt_t _cyhal_utils_set_clock_frequency(cyhal_clock_t* clock, uint32_t hz, const cyhal_clock_tolerance_t *tolerance)
{
    return cyhal_clock_set_frequency(clock, hz, tolerance);
}

cy_rslt_t _cyhal_utils_peri_pclk_set_divider(en_clk_dst_t clk_dest, const cyhal_clock_t *clock, uint32_t div);
cy_rslt_t _cyhal_utils_peri_pclk_set_freq(en_clk_dst_t clk_dest, const cyhal_clock_t *clock, uint32_t freq, uint32_t oversample);
uint32_t _cyhal_utils_peri_pclk_get_freq(en_clk_dst_t clk_dest, const cyhal_clock_t *clock);

static inline cy_rslt_t _cyhal_utils_peri_pclk_assign_divider(en_clk_dst_t clk_dest, const cyhal_clock_t *clock)
{
    // Clock dividers are dedicated. Can't assign a divider
    CY_UNUSED_PARAMETER(clk_dest);
    CY_UNUSED_PARAMETER(clock);
    return CY_RSLT_SUCCESS;
}
static inline cy_rslt_t _cyhal_utils_peri_pclk_enable_divider(en_clk_dst_t clk_dest, const cyhal_clock_t *clock)
{
    // There's no way to enable/disable the divider
    CY_UNUSED_PARAMETER(clk_dest);
    CY_UNUSED_PARAMETER(clock);
    return CY_RSLT_SUCCESS;
}
static inline cy_rslt_t _cyhal_utils_peri_pclk_disable_divider(en_clk_dst_t clk_dest, const cyhal_clock_t *clock)
{
    // There's no way to enable/disable the divider
    CY_UNUSED_PARAMETER(clk_dest);
    CY_UNUSED_PARAMETER(clock);
    return CY_RSLT_SUCCESS;
}


#if defined(__cplusplus)
}
#endif

/** \} group_hal_impl_utils */
/** \} group_hal_impl */
/** \endcond */
