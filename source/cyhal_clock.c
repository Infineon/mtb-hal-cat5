/*******************************************************************************
* File Name: cyhal_clock.c
*
* Description:
* Provides an implementation for high level interface for interacting with the
* Device Clocks.
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

#include <string.h>
#include "cyhal_clock.h"
#include "cyhal_hwmgr.h"
#include "cyhal_system.h"
#include "cyhal_utils.h"
#include "cy_utils.h"

#if defined(__cplusplus)
extern "C"
{
#endif

const cyhal_clock_tolerance_t CYHAL_TOLERANCE_0_P = {CYHAL_TOLERANCE_PERCENT, 0};
const cyhal_clock_tolerance_t CYHAL_TOLERANCE_1_P = {CYHAL_TOLERANCE_PERCENT, 1};
const cyhal_clock_tolerance_t CYHAL_TOLERANCE_5_P = {CYHAL_TOLERANCE_PERCENT, 5};


/*******************************************************************************
*       Clock resources
*******************************************************************************/

const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB0 = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_PERI_SCB0, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB1 = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_PERI_SCB1, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_SCB2 = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_PERI_SCB2, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_PERI_TCPWM = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_PERI_TCPWM, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_TDM = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_TDM, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_ADCMIC = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_ADCMIC, 0u };
const cyhal_resource_inst_t CYHAL_CLOCK_RSC_CPU = { CYHAL_RSC_CLOCK, (uint8_t)CYHAL_CLOCK_BLOCK_CPU, 0u };

#define _CYHAL_CLOCK_CREATE(x)	{ .block = (CYHAL_CLOCK_BLOCK_##x), .channel = 0u, .reserved = false }

const cyhal_clock_t CYHAL_CLOCK_PERI_SCB0   = _CYHAL_CLOCK_CREATE(PERI_SCB0);
const cyhal_clock_t CYHAL_CLOCK_PERI_SCB1   = _CYHAL_CLOCK_CREATE(PERI_SCB1);
const cyhal_clock_t CYHAL_CLOCK_PERI_SCB2   = _CYHAL_CLOCK_CREATE(PERI_SCB2);
const cyhal_clock_t CYHAL_CLOCK_PERI_TCPWM  = _CYHAL_CLOCK_CREATE(PERI_TCPWM);
const cyhal_clock_t CYHAL_CLOCK_TDM         = _CYHAL_CLOCK_CREATE(TDM);
const cyhal_clock_t CYHAL_CLOCK_ADCMIC      = _CYHAL_CLOCK_CREATE(ADCMIC);
const cyhal_clock_t CYHAL_CLOCK_CPU         = _CYHAL_CLOCK_CREATE(CPU);


/*******************************************************************************
*       HAL Implementation
*******************************************************************************/

cy_rslt_t cyhal_clock_allocate(cyhal_clock_t *clock, cyhal_clock_block_t block)
{
    CY_ASSERT(NULL != clock);

    cyhal_resource_inst_t clock_resource = { CYHAL_RSC_CLOCK, block, 0u };
    cy_rslt_t status = cyhal_hwmgr_reserve(&clock_resource);

    if (CY_RSLT_SUCCESS == status)
    {
        clock->block = block;
        clock->channel = 0u;
        clock->reserved = true;
    }

    return status;
}

cy_rslt_t cyhal_clock_get(cyhal_clock_t *clock, const cyhal_resource_inst_t *resource)
{
    CY_ASSERT(NULL != clock);
    CY_ASSERT(NULL != resource);
    CY_ASSERT(CYHAL_RSC_CLOCK == resource->type);

    clock->block = (cyhal_clock_block_t)resource->block_num;
    clock->channel = resource->channel_num;
    clock->reserved = false;

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_clock_reserve(cyhal_clock_t *clock, const cyhal_clock_t *clock_)
{
    CY_ASSERT(NULL != clock);
    CY_ASSERT(NULL != clock_);

    cyhal_resource_inst_t clock_resource = { CYHAL_RSC_CLOCK, clock_->block, clock_->channel };
    cy_rslt_t rslt = cyhal_hwmgr_reserve(&clock_resource);
    if (CY_RSLT_SUCCESS == rslt)
    {
        memcpy(clock, clock_, sizeof(cyhal_clock_t));
        clock->reserved = true;
    }
    return rslt;
}

cyhal_clock_feature_t cyhal_clock_get_features(const cyhal_clock_t *clock)
{
    CY_ASSERT(NULL != clock);

    cyhal_clock_feature_t features = CYHAL_CLOCK_FEATURE_NONE;

    switch (clock->block)
    {
        case CYHAL_CLOCK_BLOCK_PERI_TCPWM:
            features |= CYHAL_CLOCK_FEATURE_DIVIDER;
            break;
        case CYHAL_CLOCK_BLOCK_PERI_SCB0:
        case CYHAL_CLOCK_BLOCK_PERI_SCB1:
        case CYHAL_CLOCK_BLOCK_PERI_SCB2:
        case CYHAL_CLOCK_BLOCK_TDM:
        case CYHAL_CLOCK_BLOCK_CPU:
            features |= CYHAL_CLOCK_FEATURE_FREQUENCY;
        default:
            break;
    }

    return features;
}

bool cyhal_clock_is_enabled(const cyhal_clock_t *clock)
{
    CY_ASSERT(NULL != clock);
    CY_UNUSED_PARAMETER(clock);
    return true;
}

cy_rslt_t cyhal_clock_set_enabled(cyhal_clock_t *clock, bool enabled, bool wait_for_lock)
{
    CY_UNUSED_PARAMETER(enabled);
    CY_UNUSED_PARAMETER(wait_for_lock);
    CY_ASSERT(NULL != clock);
    cy_rslt_t status = CYHAL_CLOCK_RSLT_ERR_RESOURCE;

    if ((clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB0) ||
        (clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB1) ||
        (clock->block == CYHAL_CLOCK_BLOCK_PERI_SCB2) ||
        (clock->block == CYHAL_CLOCK_BLOCK_PERI_TCPWM) ||
        (clock->block == CYHAL_CLOCK_BLOCK_TDM))
    {
        // Cannot enable/disable clock. Done when setting frequency.
        status = CY_RSLT_SUCCESS;
    }

    return status;
}

bool _cyhal_clock_is_divider_valid(const cyhal_resource_inst_t *resource, uint32_t divider)
{
    CY_ASSERT(NULL != resource);

    bool valid = 0;
    if (resource->type == CYHAL_RSC_SCB)
    {
        valid = ( divider <= SCB_IP_SYS_CLK_MAX_DIVIDER ) ? 1: 0;
    }
    return valid;
}

// There's no way to query for the Audio PLL frequency. Keep track of it internally
static uint32_t clk_audio_pll = 0UL;

uint32_t cyhal_clock_get_frequency(const cyhal_clock_t *clock)
{
    CY_ASSERT(NULL != clock);
    uint32_t freq;
    BTSS_SYSTEM_CPU_CLK_FREQ_t cpu_freq;

    switch (clock->block)
    {
        case CYHAL_CLOCK_BLOCK_PERI_SCB0:
        case CYHAL_CLOCK_BLOCK_PERI_SCB1:
        case CYHAL_CLOCK_BLOCK_PERI_SCB2:
        case CYHAL_CLOCK_BLOCK_PERI_TCPWM:
            freq = _cyhal_utils_peri_pclk_get_freq(0, clock);
            break;
        case CYHAL_CLOCK_BLOCK_TDM:
            freq = clk_audio_pll;
            break;
        case CYHAL_CLOCK_BLOCK_CPU:
            cpu_freq = btss_system_clockGetCpuFreq();
            switch (cpu_freq)
            {
                case BTSS_SYSTEM_CPU_CLK_24MHZ:
                    freq = 24000000UL;
                    break;
                case BTSS_SYSTEM_CPU_CLK_32MHZ:
                    freq = 32000000UL;
                    break;
                case BTSS_SYSTEM_CPU_CLK_48MHZ:
                    freq = 48000000UL;
                    break;
                case BTSS_SYSTEM_CPU_CLK_96MHZ:
                    freq = 96000000UL;
                    break;
                case BTSS_SYSTEM_CPU_CLK_192MHZ:
                    freq = 192000000UL;
                    break;
                default:
                    freq = 0UL;
                    break;
            }
            break;
        default:
            freq = 0UL;
            break;
    }

    return freq;
}

cy_rslt_t cyhal_clock_set_frequency(cyhal_clock_t *clock, uint32_t hz, const cyhal_clock_tolerance_t *tolerance)
{
    CY_UNUSED_PARAMETER(tolerance);
    CY_ASSERT(NULL != clock);
    cy_rslt_t status = CY_RSLT_SUCCESS;
    bool rom_status;
    BTSS_SYSTEM_CPU_CLK_FREQ_t freq;
    
    switch (clock->block)
    {
        case CYHAL_CLOCK_BLOCK_PERI_SCB0:
        case CYHAL_CLOCK_BLOCK_PERI_SCB1:
        case CYHAL_CLOCK_BLOCK_PERI_SCB2:
            status = _cyhal_utils_peri_pclk_set_freq(0, clock, hz, 1);
            break;
        case CYHAL_CLOCK_BLOCK_TDM:
        //We will probably be able to introduce a switch case here as well to only set available frequency
        //the same way it is done for the CPU block. Allowed frequencies are believed to be 24000000,
        //36000000 and 48000000.
            if(clk_audio_pll == 0) 
            {
                //For now this is our only way to check that the PLL has been previously locked, once a FW API to query
                //the audio pll frequency is available (ticket BTSDK-8479) this function will be revisited. 
                rom_status = btss_system_clockRequestForAudioPll(BTSS_SYSTEM_AUDIO_PLL_CLK_REQ_LOCK_TO_SPEED, hz);
            }
            else
            {
                rom_status = btss_system_clockRequestForAudioPll(BTSS_SYSTEM_AUDIO_PLL_CLK_REQ_UNLOCK_TO_SPEED, hz);
                rom_status = rom_status ? btss_system_clockRequestForAudioPll(BTSS_SYSTEM_AUDIO_PLL_CLK_REQ_LOCK_TO_SPEED, hz) : rom_status;
            }
            status = rom_status ? CY_RSLT_SUCCESS : CYHAL_CLOCK_RSLT_ERR_FREQ;
            clk_audio_pll = (status == CY_RSLT_SUCCESS) ? hz : 0UL;
            break;
        case CYHAL_CLOCK_BLOCK_CPU:
            switch (hz)
            {
                case 24000000UL:
                    freq = BTSS_SYSTEM_CPU_CLK_24MHZ;
                    break;
                case 32000000UL:
                    freq = BTSS_SYSTEM_CPU_CLK_32MHZ;
                    break;
                case 48000000UL:
                    freq = BTSS_SYSTEM_CPU_CLK_48MHZ;
                    break;
                case 96000000UL:
                    freq = BTSS_SYSTEM_CPU_CLK_96MHZ;
                    break;
                case 192000000UL:
                    freq = BTSS_SYSTEM_CPU_CLK_192MHZ;
                    break;
                default:
                    status = CYHAL_CLOCK_RSLT_ERR_FREQ;
            }

            if (status == CY_RSLT_SUCCESS)
            {
                rom_status = btss_system_clockRequestForCpu(BTSS_SYSTEM_CPU_CLK_REQ_RELEASE_FROM, btss_system_clockGetCpuFreq());
                rom_status = rom_status ? btss_system_clockRequestForCpu(BTSS_SYSTEM_CPU_CLK_REQ_NEED_UPTO, freq) : rom_status;
                status = rom_status ? CY_RSLT_SUCCESS : CYHAL_CLOCK_RSLT_ERR_FREQ;
            }
            break;
        default:
            status = CYHAL_CLOCK_RSLT_ERR_RESOURCE;
            break;
    }

    return status;
}

cy_rslt_t cyhal_clock_set_divider(cyhal_clock_t *clock, uint32_t divider)
{
    CY_ASSERT(NULL != clock);
    cy_rslt_t status = CYHAL_CLOCK_RSLT_ERR_RESOURCE;
    
    if (clock->block == CYHAL_CLOCK_BLOCK_PERI_TCPWM)
    {
        // Need -1 to keep it consistent with rest of code base
        status = _cyhal_utils_peri_pclk_set_divider(0, clock, divider - 1);
    }

    return status;
}

cy_rslt_t cyhal_clock_get_sources(const cyhal_clock_t *clock, const cyhal_resource_inst_t **sources[], uint32_t *count)
{
    CY_ASSERT(NULL != clock);

    /* Clock source definitions */
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_PERI_SCB0[] = { &CYHAL_CLOCK_RSC_PERI_SCB0 };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_PERI_SCB1[] = { &CYHAL_CLOCK_RSC_PERI_SCB1 };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_PERI_SCB2[] = { &CYHAL_CLOCK_RSC_PERI_SCB2 };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_PERI_TCPWM[] = { &CYHAL_CLOCK_RSC_PERI_TCPWM };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_TDM[] = { &CYHAL_CLOCK_RSC_TDM };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_ADCMIC[] = { &CYHAL_CLOCK_RSC_ADCMIC };
    static const cyhal_resource_inst_t *_CYHAL_CLOCK_RSC_CPU[] = { &CYHAL_CLOCK_RSC_CPU };

    cy_rslt_t status = CY_RSLT_SUCCESS;
    *count = 1UL;

    switch (clock->block)
    {
        case CYHAL_CLOCK_BLOCK_PERI_SCB0:
            *sources = _CYHAL_CLOCK_RSC_PERI_SCB0;
            break;
        case CYHAL_CLOCK_BLOCK_PERI_SCB1:
            *sources = _CYHAL_CLOCK_RSC_PERI_SCB1;
            break;
        case CYHAL_CLOCK_BLOCK_PERI_SCB2:
            *sources = _CYHAL_CLOCK_RSC_PERI_SCB2;
            break;
        case CYHAL_CLOCK_BLOCK_PERI_TCPWM:
            *sources = _CYHAL_CLOCK_RSC_PERI_TCPWM;
            break;
        case CYHAL_CLOCK_BLOCK_TDM:
            *sources = _CYHAL_CLOCK_RSC_TDM;
            break;
        case CYHAL_CLOCK_BLOCK_ADCMIC:
            *sources = _CYHAL_CLOCK_RSC_ADCMIC;
            break;
        case CYHAL_CLOCK_BLOCK_CPU:
            *sources = _CYHAL_CLOCK_RSC_CPU;
            break;
        default:
            *count = 0UL;
            status = CYHAL_CLOCK_RSLT_ERR_SOURCE;
            break;
    }
    
    return status;
}

cy_rslt_t cyhal_clock_set_source(cyhal_clock_t *clock, const cyhal_clock_t *source)
{
    CY_UNUSED_PARAMETER(source);
    CY_ASSERT(NULL != clock);
    cy_rslt_t status = CYHAL_CLOCK_RSLT_ERR_NOT_SUPPORTED;
    return status;
}

void cyhal_clock_free(cyhal_clock_t *clock)
{
    CY_ASSERT(NULL != clock);
    cyhal_resource_inst_t rsc = { CYHAL_RSC_CLOCK, clock->block, clock->channel };
    cyhal_hwmgr_free(&rsc);
    clock->reserved = false;
}

#if defined(__cplusplus)
}
#endif
