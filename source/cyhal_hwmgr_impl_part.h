/***************************************************************************//**
* \file cyhal_hwmgr_impl_part.h
*
* \brief
* Provides device specific information to the hardware manager. This file must
* only ever be included by cyhal_hwmgr.c.
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

/*******************************************************************************
*       Defines
*******************************************************************************/

#include "cyhal_hwmgr_impl.h"
#include "cyhal_interconnect.h"
#include "cyhal_utils.h"

#define CY_BLOCK_COUNT_ADCMIC       (1)
#define CY_BLOCK_COUNT_CLOCK        (7)
#define CY_BLOCK_COUNT_DMA          (1)
#define CY_CHANNEL_COUNT_DMA        (8)
#define CY_BLOCK_COUNT_GPIO         (2)
#define CY_CHANNEL_COUNT_GPIO       (49 + 4) // 49 allocatable, 4 dedicated SDIO?
#define CY_BLOCK_COUNT_PDMPCM       (1)
#define CY_BLOCK_COUNT_RTC          (1)
#define CY_BLOCK_COUNT_SCB          (3)
#define CY_BLOCK_COUNT_TCPWM        (1)
#define CY_CHANNEL_COUNT_TCPWM      (9) // Grp0 Ch2, Grp1 Ch7
#define CY_BLOCK_COUNT_TDM          (1)
#define CY_CHANNEL_COUNT_TDM        (2)
#define CY_BLOCK_COUNT_T2TIMER      (2)


/*
    All resources have an offset and a size, offsets are stored in an array
    Subsequent resource offset equals the preceding offset + size
    Offsets are bit indexes in the arrays that track used, configured etc.

    Channel based resources have an extra array for block offsets

    Note these are NOT offsets into the device's MMIO address space;
    they are bit offsets into arrays that are internal to the HW mgr.
*/


#define CY_OFFSET_ADCMIC   0
#define CY_SIZE_ADCMIC     CY_BLOCK_COUNT_ADCMIC
#define CY_OFFSET_CLOCK    (CY_OFFSET_ADCMIC + CY_SIZE_ADCMIC)
#define CY_SIZE_CLOCK      CY_BLOCK_COUNT_CLOCK
#define CY_OFFSET_DMA      (CY_OFFSET_CLOCK + CY_SIZE_CLOCK)
#define CY_SIZE_DMA        CY_CHANNEL_COUNT_DMA
#define CY_OFFSET_GPIO     (CY_OFFSET_DMA + CY_SIZE_DMA)
#define CY_SIZE_GPIO       CY_CHANNEL_COUNT_GPIO
#define CY_OFFSET_PDMPCM   (CY_OFFSET_GPIO + CY_SIZE_GPIO)
#define CY_SIZE_PDMPCM     CY_BLOCK_COUNT_PDMPCM
#define CY_OFFSET_RTC      (CY_OFFSET_PDMPCM + CY_SIZE_PDMPCM)
#define CY_SIZE_RTC        CY_BLOCK_COUNT_RTC
#define CY_OFFSET_SCB      (CY_OFFSET_RTC + CY_SIZE_RTC)
#define CY_SIZE_SCB        CY_BLOCK_COUNT_SCB
#define CY_OFFSET_TCPWM    (CY_OFFSET_SCB + CY_SIZE_SCB)
#define CY_SIZE_TCPWM      CY_CHANNEL_COUNT_TCPWM
#define CY_OFFSET_TDM      (CY_OFFSET_TCPWM + CY_SIZE_TCPWM)
#define CY_SIZE_TDM        CY_CHANNEL_COUNT_TDM
#define CY_OFFSET_T2TIMER  (CY_OFFSET_TDM + CY_SIZE_TDM)
#define CY_SIZE_T2TIMER    CY_BLOCK_COUNT_T2TIMER

#define CY_TOTAL_ALLOCATABLE_ITEMS     (CY_OFFSET_T2TIMER + CY_SIZE_T2TIMER)

/*******************************************************************************
*       Variables
*******************************************************************************/

typedef uint8_t _cyhal_hwmgr_offset_t;

static const _cyhal_hwmgr_offset_t cyhal_block_offsets_dma[2] =
{
    0, 8 // TBD
};

static const _cyhal_hwmgr_offset_t cyhal_block_offsets_gpio[3] =
{
    0, 49, 53 // TBD
};

static const _cyhal_hwmgr_offset_t cyhal_block_offsets_tcpwm[2] =
{
    2, 9,
};

static const _cyhal_hwmgr_offset_t cyhal_block_offsets_tdm[2] =
{
    0,
    2
};

static uint8_t cyhal_used[(CY_TOTAL_ALLOCATABLE_ITEMS + 7) / 8] = {0};

// Note: the ordering here needs to be parallel to that of cyhal_resource_t
static const uint8_t cyhal_resource_offsets[] =
{
    CY_OFFSET_ADCMIC,
    CY_OFFSET_CLOCK,
    CY_OFFSET_DMA,
    CY_OFFSET_GPIO,
    CY_OFFSET_PDMPCM,
    CY_OFFSET_RTC,
    CY_OFFSET_SCB,
    CY_OFFSET_TCPWM,
    CY_OFFSET_TDM,
    CY_OFFSET_T2TIMER
};

#define _CYHAL_RESOURCES (sizeof(cyhal_resource_offsets)/sizeof(cyhal_resource_offsets[0]))

static const uint32_t cyhal_has_channels =
    (1 << CYHAL_RSC_DMA)   |
    (1 << CYHAL_RSC_GPIO)  |
    (1 << CYHAL_RSC_TCPWM) |
    (1 << CYHAL_RSC_TDM);

/*******************************************************************************
*       Utility helper functions
*******************************************************************************/

static inline uint16_t _cyhal_uses_channels(cyhal_resource_t type)
{
    return (cyhal_has_channels & (1 << type)) > 0;
}

static inline uint16_t _cyhal_get_resource_offset(cyhal_resource_t type)
{
    return cyhal_resource_offsets[type];
}

static inline const _cyhal_hwmgr_offset_t* _cyhal_get_block_offsets(cyhal_resource_t type)
{
    switch (type)
    {
        case CYHAL_RSC_DMA:
            return cyhal_block_offsets_dma;
        case CYHAL_RSC_GPIO:
            return cyhal_block_offsets_gpio;
        case CYHAL_RSC_TCPWM:
            return cyhal_block_offsets_tcpwm;
        case CYHAL_RSC_TDM:
            return cyhal_block_offsets_tdm;
        default:
            CY_ASSERT(false);
            return NULL;
    }
}

// Gets the number of block offset entries, only valid for blocks which have channels.
static inline uint8_t _cyhal_get_block_offset_length(cyhal_resource_t type)
{
    switch (type)
    {
        case CYHAL_RSC_DMA:
            return sizeof(cyhal_block_offsets_dma)/sizeof(cyhal_block_offsets_dma[0]);
        case CYHAL_RSC_GPIO:
            return sizeof(cyhal_block_offsets_gpio)/sizeof(cyhal_block_offsets_gpio[0]);
        case CYHAL_RSC_TCPWM:
            return sizeof(cyhal_block_offsets_tcpwm)/sizeof(cyhal_block_offsets_tcpwm[0]);
        case CYHAL_RSC_TDM:
            return sizeof(cyhal_block_offsets_tdm)/sizeof(cyhal_block_offsets_tdm[0]);
        default:
            CY_ASSERT(false);
            return 0;
    }
}
