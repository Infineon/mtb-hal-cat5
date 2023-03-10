/***************************************************************************//**
* \file cyhal_utils_impl.c
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

#include <stdlib.h>
#include <stdarg.h>
#include "cyhal_interconnect.h"
#include "cyhal_utils.h"
#include "cyhal_utils_impl.h"
#include "cyhal_hwmgr.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    uint32_t div; /**< Divider */
    uint32_t freq; /**< Frequency */
} _cyhal_utils_clock_cfg_t;

// Used to store dividers and frequency. Index should match cyhal_clock_block_t
static _cyhal_utils_clock_cfg_t _cyhal_clock_data[7] = {0};

uint32_t _cyhal_utils_get_peripheral_clock_frequency(const cyhal_resource_inst_t *clocked_item)
{
    if (clocked_item->type == CYHAL_RSC_SCB)
        return SCB_IP_SYS_CLK_MAX_FREQUENCY;
    else if (clocked_item->type == CYHAL_RSC_TCPWM)
        return TCPWM_IP_SYS_CLK_MAX_FREQUENCY;
    else
        return 0UL;
}

cy_rslt_t _cyhal_utils_allocate_clock(cyhal_clock_t *clock, const cyhal_resource_inst_t *clocked_item, cyhal_clock_block_t div, bool accept_larger)
{
    CY_UNUSED_PARAMETER(div);
    CY_UNUSED_PARAMETER(accept_larger);
    CY_ASSERT(NULL != clock);

    cy_rslt_t status = CY_RSLT_SUCCESS;

    // Clocks are dedicated (SCB2 is off by one)
    if (clocked_item->type == CYHAL_RSC_SCB)
    {
        //SCB2 block number is 2 and the clock block for SCB2 is CYHAL_CLOCK_BLOCK_PERI_SCB2(3). SCB0 and SCB1 block
        //number and the clock block number is the same
        clock->block = (cyhal_clock_block_t)(clocked_item->block_num + ((clocked_item->block_num == 2u) ? 1u : 0u));
        clock->reserved = true;
    }
    else if (clocked_item->type == CYHAL_RSC_TCPWM)
    {
        clock->block = CYHAL_CLOCK_BLOCK_PERI_TCPWM;
        clock->reserved = true;
    }
    else if (clocked_item->type == CYHAL_RSC_TDM)
    {
        clock->block = CYHAL_CLOCK_BLOCK_TDM;
        clock->reserved = true;
    }
    else
        status = 1;

    return status;
}

cy_rslt_t _cyhal_utils_peri_pclk_set_divider(en_clk_dst_t clk_dest, const cyhal_clock_t *clock, uint32_t div)
{
    CY_UNUSED_PARAMETER(clk_dest);
    CY_ASSERT(NULL != clock);
    
    cy_rslt_t status = CY_RSLT_SUCCESS;
    uint32_t actual_div = div + 1; // Need +1 to undo the -1 from upper layer (aside: no chance of overflow)

    if (clock->block == CYHAL_CLOCK_BLOCK_PERI_TCPWM)
    {
        // Only select dividers in TCPWM_TPORT_CLK_DIV_SEL_t are supported
        TCPWM_TPORT_CLK_DIV_SEL_t div_enum = 0;
        switch (actual_div)
        {
            case 1UL:
            case 2UL:
            case 4UL:
            case 6UL:
            case 8UL:
            case 10UL:
            case 12UL:
                div_enum = (TCPWM_TPORT_CLK_DIV_SEL_t)((div/2) + 1u);
                break;
            case 16UL:
                div_enum = TCPWM_TPORT_CLK_DIV_16;
                break;
            case 32UL:
                div_enum = TCPWM_TPORT_CLK_DIV_32;
                break;
            default:
                status = 1;
                break;
        }

        if (status == CY_RSLT_SUCCESS)
        {
            _cyhal_clock_data[clock->block].div = actual_div;
            cyhal_resource_inst_t clocked_item = {CYHAL_RSC_TCPWM, 0, 0}; // Only need type
            _cyhal_clock_data[clock->block].freq = _cyhal_utils_get_peripheral_clock_frequency(&clocked_item) / actual_div;
            Cy_TCPWM_EnableClock(div_enum, false);
        }
    }

    return status;
}

cy_rslt_t _cyhal_utils_peri_pclk_set_freq(en_clk_dst_t clk_dest, const cyhal_clock_t *clock, uint32_t freq, uint32_t oversample)
{
    CY_UNUSED_PARAMETER(clk_dest);
    CY_ASSERT(NULL != clock);

    cy_rslt_t status = CY_RSLT_SUCCESS;
    uint32_t freq_actual = 0;

    if (clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB0)
        freq_actual = Cy_SCB_EnableClock(SCB0, (freq * oversample), false);
    else if (clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB1)
        freq_actual = Cy_SCB_EnableClock(SCB1, (freq * oversample), false);
    else if (clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB2)
        freq_actual = Cy_SCB_EnableClock(SCB2, (freq * oversample), false);
    else
        status = 1;

    if (status == CY_RSLT_SUCCESS)
    {
        _cyhal_clock_data[clock->block].freq = freq_actual;
        cyhal_resource_inst_t clocked_item = {CYHAL_RSC_SCB, 0, 0};  // Only need type
        _cyhal_clock_data[clock->block].div = _cyhal_utils_get_peripheral_clock_frequency(&clocked_item) / freq_actual;
    }
    
    return status;
}

uint32_t _cyhal_utils_peri_pclk_get_freq(en_clk_dst_t clk_dest, const cyhal_clock_t *clock)
{
    CY_UNUSED_PARAMETER(clk_dest);
    return _cyhal_clock_data[clock->block].freq;
}

cyhal_resource_inst_t _cyhal_utils_get_gpio_resource(cyhal_gpio_t pin)
{
    cyhal_resource_inst_t rsc = {CYHAL_RSC_GPIO, 0, 0};

    if (pin < DIRECT_BASE)
    {
        rsc.block_num = 0;
        rsc.channel_num = pin - BT_GPIO_BASE;
    }
    else
    {
        rsc.block_num = 1;
        rsc.channel_num = pin - DIRECT_BASE;
    }

    return rsc;
}

cy_rslt_t _cyhal_utils_reserve_and_connect(const cyhal_resource_pin_mapping_t *mapping, uint8_t drive_mode)
{
    CY_ASSERT(NULL != mapping);

    cyhal_resource_inst_t pinRsc = _cyhal_utils_get_gpio_resource(mapping->pin);
    cy_rslt_t status = cyhal_hwmgr_reserve(&pinRsc);
    if (CY_RSLT_SUCCESS == status)
    {
        status = cyhal_connect_pin(mapping, drive_mode);
        if (CY_RSLT_SUCCESS != status)
        {
            cyhal_hwmgr_free(&pinRsc);
        }
    }
    return status;
}

void _cyhal_utils_disconnect_and_free(cyhal_gpio_t pin)
{
    cyhal_resource_inst_t rsc = _cyhal_utils_get_gpio_resource(pin);
    cyhal_hwmgr_free(&rsc);
}

#if defined(__cplusplus)
}
#endif
