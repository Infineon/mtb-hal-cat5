/***************************************************************************//**
* \file cyhal_dma_impl.h
*
* \brief
* Implementation details of Infineon DMA.
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

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 * \addtogroup group_hal_impl_dma DMA (Direct Memory Access)
 * \ingroup group_hal_impl
 * \{
 * DMA allows transferring data without CPU intervention.
 * It can be used to transfer data from one memory location to another.
 *
 * \section section_dma_channels Channel Usage
 * The DMA driver supports queuing of requests from different sources to the same DMA channel.
 * Therfore it's possible to use any DMA channel but the application might observe performance
 * degradation (delays) if too many requests are queued for the same channel.
 * The details of channel usage by the BT/BLE drivers in embedded mode are as follows.
 * 
 * When BT/BLE is in use, it could use channels 0 and 3.
 * 0 - BT Baseband
 * 3 - BLE AoA/AoD (direction finding)
 * UART (HCI) transport thread uses channels 1 and 2.
 * In embedded mode/CP, UART is in App control.
 * 1 - HCI UART TX (device to host)
 * 2 - HCI UART RX (host to device)
 * MXTDM LE/Classic Audio would use 4,5 for TDM0 and 6,7 for TDM1.
 * 4 - TDM0
 * 5 - TDM0
 * 6 - TDM1
 * 7 - TDM1
 * 
 * The following can be expected by the application when BT/BLE is used.
 * - TDM0 and TDM1 audio use is expected to be high traffic.
 * - BT baseband and BLE direction finding are also expected to be high traffic.
 * - HCI UART is not expected to be used in regular operation.
 * 
 * Therefore the only available channels to the application are 1 and 2.
 * - M2M and M2P transactions are mapped to channel 1.
 * - P2M transactions are mapped to 2.
 * 
 * Note: If BT/BLE is not used, or if you'd like to customize your channel use,
 * you can manually modify the channel number in cyhal_dma_t after the initialization
 * and configuration of the DMA object.
 * 
 * \section section_dma_limitations Limitations
 * The DMA on this device only supports the CYHAL_DMA_TRANSFER_FULL transfer.
 * It does not support,
 * - CYHAL_DMA_TRANSFER_BURST
 * - CYHAL_DMA_TRANSFER_BURST_DISABLE
 * - CYHAL_DMA_TRANSFER_FULL_DISABLE
 * 
 */

/** Default DMA channel priority */
#define CYHAL_DMA_PRIORITY_DEFAULT     0u
/** High DMA channel priority */
#define CYHAL_DMA_PRIORITY_HIGH        0u
/** Medium DMA channel priority */
#define CYHAL_DMA_PRIORITY_MEDIUM      0u
/** Low DMA channel priority */
#define CYHAL_DMA_PRIORITY_LOW         0u

/** Channel dedicated to memory-UART transactions */
#define _CYHAL_DMA_CH_PERIPH_TO_MEM    2u
/** Channel dedicated to UART-memory transactions */
#define _CYHAL_DMA_CH_MEM_TO_PERIPH    1u
/** Channel dedicated to memory-memory transactions */
#define _CYHAL_DMA_CH_MEM_TO_MEM       _CYHAL_DMA_CH_MEM_TO_PERIPH

/** Channel dedicated to memory-TDM0 transactions */
#define _CYHAL_DMA_CH_MEM_TO_TDM0      4u
/** Channel dedicated to TDM0-memory transactions */
#define _CYHAL_DMA_CH_TDM0_TO_MEM      5u
/** Channel dedicated to memory-TDM1 transactions */
#define _CYHAL_DMA_CH_MEM_TO_TDM1      6u
/** Channel dedicated to TDM1-memory transactions */
#define _CYHAL_DMA_CH_TDM1_TO_MEM      7u

/** Unallocated channel */
#define _CYHAL_DMA_CH_NOT_ALLOCATED    255u

/** Control line m2m-connection */
#define _CYHAL_DMAC_CONTROL_LINE_M2M   0u
/** Control line non-connection */
#define _CYHAL_DMAC_CONTROL_LINE_NONE  255u

/** TDM struct size */
#define _CYHAL_DMA_TDM_STRUCT_SIZE     0x200

/** Start of SRAM address */
#define _CYHAL_DMA_ADDR_SRAM_START     sram_adr_base
/** End of SRAM address */
#define _CYHAL_DMA_ADDR_SRAM_END       cc312_adr_base
/** Start of SCB0 address */
#define _CYHAL_DMA_ADDR_SCB0_START     scb0_adr_base
/** End of SCB0 address */
#define _CYHAL_DMA_ADDR_SCB0_END       scb1_adr_base
/** Start of SCB1 address */
#define _CYHAL_DMA_ADDR_SCB1_START     scb1_adr_base
/** End of SCB1 address */
#define _CYHAL_DMA_ADDR_SCB1_END       driver_cfg_adr_base
/** Start of SCB2 address */
#define _CYHAL_DMA_ADDR_SCB2_START     scb2_adr_base
/** End of SCB2 address */
#define _CYHAL_DMA_ADDR_SCB2_END       mxtdm_adr_base
/** Start of TDM0 address */
#define _CYHAL_DMA_ADDR_TDM0_START     mxtdm_adr_base
/** End of TDM0 address */
#define _CYHAL_DMA_ADDR_TDM0_END       (mxtdm_adr_base + _CYHAL_DMA_TDM_STRUCT_SIZE)
/** Start of TDM1 address */
#define _CYHAL_DMA_ADDR_TDM1_START     (mxtdm_adr_base + _CYHAL_DMA_TDM_STRUCT_SIZE)
/** End of TDM1 address */
#define _CYHAL_DMA_ADDR_TDM1_END       (_CYHAL_DMA_ADDR_TDM0_END + _CYHAL_DMA_TDM_STRUCT_SIZE)

/** \} group_hal_impl_dma */

#if defined(__cplusplus)
}
#endif
