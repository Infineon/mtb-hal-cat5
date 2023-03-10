/***************************************************************************//**
* \file cyhal_triggers.h
*
* Description:
* Provides definitions for the triggers for each supported device family.
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

/**
* \addtogroup group_hal_impl_triggers Trigger Connections
* \ingroup group_hal_impl_interconnect
* \{
* Trigger connections for supported device families:
*
* CYW55500
*/

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "cy_tcpwm.h"
#include "btss_dmac.h"
#include "cy_result.h"

/**
* \cond INTERNAL
*/

typedef enum
{
    // TODO: Required for compilation only. Review when working on TCPWM triggers.
    // Adding in ZERO_LEVEL and ZERO_EDGE for testing purposes
    CYHAL_TRIGGER_CPUSS_ZERO = 0, //!< cpuss.zero
    CYHAL_TRIGGER_CPUSS_ZERO_LEVEL = 1, //!< cpuss.zero_level
    CYHAL_TRIGGER_CPUSS_ZERO_EDGE = 2, //!< cpuss.zero_edge
    _CYHAL_TRIGGER_SCB0_TR_RX_REQ = 0, //!< scb[0].tr_rx_req
    _CYHAL_TRIGGER_SCB0_TR_TX_REQ = 0, //!< scb[0].tr_tx_req
    _CYHAL_TRIGGER_TCPWM0_TR_OUT00 = 0, //!< tcpwm[0].tr_out0[0]
    _CYHAL_TRIGGER_TCPWM0_TR_OUT10 = 0, //!< tcpwm[0].tr_out0[0]

    // Keep these for DMA HAL
    CYHAL_TRIGGER_DMAC_PTU_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_HCI_UART, //!< DMAC trigger - PTU as DMA source
    CYHAL_TRIGGER_DMAC_MXTDM0_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_MXTDM0, //!< DMAC trigger - MXTDM0 as DMA source
    CYHAL_TRIGGER_DMAC_MXTDM1_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_MXTDM1, //!< DMAC trigger - MXTDM1 as DMA source
    CYHAL_TRIGGER_DMAC_SCB1_RX_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_SCB1, //!< DMAC trigger - SCB1 as DMA source
    CYHAL_TRIGGER_DMAC_SCB0_RX_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_SCB0, //!< DMAC trigger - SCB0 as DMA source
    CYHAL_TRIGGER_DMAC_SCB2_RX_TO_MEM = BTSS_DMAC_CONTROL_LINE_SRC_SCB2, //!< DMAC trigger - SCB2 as DMA source
} cyhal_trigger_source_t;


/** Typedef for internal device family specific trigger source to generic trigger source */
typedef cyhal_trigger_source_t cyhal_internal_source_t;

/** @brief Get a public source signal type (cyhal_trigger_source_psoc6_02_t) given an internal source signal and signal type */
#define _CYHAL_TRIGGER_CREATE_SOURCE(src, type)    ((src) << 1 | (type))
/** @brief Get an internal source signal (_cyhal_trigger_source_psoc6_02_t) given a public source signal. */
#define _CYHAL_TRIGGER_GET_SOURCE_SIGNAL(src)      ((cyhal_internal_source_t)((src) >> 1))
/** @brief Get the signal type (cyhal_signal_type_t) given a public source signal. */
#define _CYHAL_TRIGGER_GET_SOURCE_TYPE(src)        ((cyhal_signal_type_t)((src) & 1))


/** Typedef from device family specific trigger source to generic trigger source */
typedef cyhal_trigger_source_t cyhal_source_t;

typedef enum
{
    // TODO: Required for compilation only. Review when working on TCPWM triggers.
    CYHAL_TRIGGER_TCPWM0_TR_ALL_CNT_IN0 = 0, //!< TCPWM0 trigger multiplexer - tcpwm[0].tr_all_cnt_in[0]
    CYHAL_TRIGGER_TCPWM0_TR_ALL_CNT_IN1 = 1, //!< TCPWM0 trigger multiplexer - tcpwm[0].tr_all_cnt_in[1]
    CYHAL_TRIGGER_TCPWM0_TR_ALL_CNT_IN2 = 2, //!< TCPWM0 trigger multiplexer - tcpwm[0].tr_all_cnt_in[2]
    CYHAL_TRIGGER_TCPWM0_TR_ALL_CNT_IN3 = 3, //!< TCPWM0 trigger multiplexer - tcpwm[0].tr_all_cnt_in[3]

    CYHAL_TRIGGER_DMAC_MEM_TO_PTU = BTSS_DMAC_CONTROL_LINE_DST_HCI_UART, //!< DMAC trigger - PTU as DMA destination
    CYHAL_TRIGGER_DMAC_MEM_TO_MXTDM0 = BTSS_DMAC_CONTROL_LINE_DST_MXTDM0, //!< DMAC trigger - MXTDM0 as DMA destination
    CYHAL_TRIGGER_DMAC_MEM_TO_MXTDM1 = BTSS_DMAC_CONTROL_LINE_DST_MXTDM1, //!< DMAC trigger - MXTDM1 as DMA destination
    CYHAL_TRIGGER_DMAC_MEM_TO_SCB0_TX = BTSS_DMAC_CONTROL_LINE_DST_SCB0, //!< DMAC trigger - SCB0 as DMA destination
    CYHAL_TRIGGER_DMAC_MEM_TO_SCB1_TX = BTSS_DMAC_CONTROL_LINE_DST_SCB1, //!< DMAC trigger - SCB1 as DMA destination    
    CYHAL_TRIGGER_DMAC_MEM_TO_SCB2_TX = BTSS_DMAC_CONTROL_LINE_DST_SCB2, //!< DMAC trigger - SCB2 as DMA destination
} cyhal_trigger_dest_t;

/** Typedef from device family specific trigger dest to generic trigger dest */
typedef cyhal_trigger_dest_t cyhal_dest_t;

static inline bool _cyhal_can_connect_signal(cyhal_source_t source, cyhal_dest_t dest)
{
    // Note: Referenced only for Quaddec connection. All TCPWM counters connect to same pin source.
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(dest);
    return true;
}

static inline cy_rslt_t _cyhal_connect_signal(cyhal_source_t source, cyhal_dest_t dest)
{
    // Note: Referenced only in Quaddec connection. Functionally it is a NOP on this device.
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(dest);
    return CY_RSLT_SUCCESS;
}

static inline cy_rslt_t _cyhal_disconnect_signal(cyhal_source_t source, cyhal_dest_t dest)
{
    // Note: Referenced only in Quaddec connection. Functionally it is a NOP on this device.
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(dest);
    return CY_RSLT_SUCCESS;
}

/** \endcond */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/** \} group_hal_impl_triggers */
