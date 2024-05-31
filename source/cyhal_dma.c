/***************************************************************************//**
* \file cyhal_dma.c
*
* \brief
* Implements a high level interface for interacting with the Infineon DMA.
* This implementation abstracts out the chip specific details. If any chip specific
* functionality is necessary, or performance is critical the low level functions
* can be used directly.
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

#include "cy_utils.h"
#include "cyhal_dma.h"
#include "cyhal_system.h"
#include "cyhal_syspm.h"
#include "cyhal_hwmgr.h"

#include "btss_dmac.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/*******************************************************************************
*       Internal - Helper
*******************************************************************************/

#define _CYHAL_DMA_CHANNELS             8
#define _CYHAL_DMA_MAX_LENGTH           (1 << 12u) /* sizeof(BTSS_DMAC_CONTROL_REG_t.bitfieds.length))*/
#define _CYHAL_DMA_REQ_ERR_STATUS_BIT   (1 << 0u)  /* BTSS_DMAC_APP_REQUEST_t.errStatus : 1 */

/* Internal tracking of DMA objects */
static bool _cyhal_dma_arrays_initialized = false;
CY_NOINIT static cyhal_dma_t *_cyhal_dma_obj[_CYHAL_DMA_CHANNELS];


/*******************************************************************************
*       Internal - LPM
*******************************************************************************/

static bool _cyhal_dmac_enabled = false;

static bool _cyhal_dma_pm_has_enabled(void)
{
    for (int idx = 0; idx < _CYHAL_DMA_CHANNELS; idx++)
    {
        if (_cyhal_dma_obj[idx] != NULL)
        {
            return true;
        }
    }
    return false;
}

static bool _cyhal_dma_pm_transition_pending_value = false;

static bool _cyhal_dma_pm_callback(cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode, void* callback_arg)
{
    CY_UNUSED_PARAMETER(state);
    CY_UNUSED_PARAMETER(callback_arg);

    switch(mode)
    {
        case CYHAL_SYSPM_CHECK_FAIL:
        case CYHAL_SYSPM_AFTER_TRANSITION:
            _cyhal_dma_pm_transition_pending_value = false;
            break;
        case CYHAL_SYSPM_CHECK_READY:
            // Iterate through all active DMAs
            for (int idx = 0; idx < _CYHAL_DMA_CHANNELS; idx++)
            {
                if (_cyhal_dma_obj[idx] != NULL)
                {
                    if (cyhal_dma_is_busy(_cyhal_dma_obj[idx]))
                        return false;
                }
            }
            _cyhal_dma_pm_transition_pending_value = true;
            break;
        default:
            break;
    }

    return true;
}

static cyhal_syspm_callback_data_t _cyhal_dma_syspm_callback_data =
{
    .callback = &_cyhal_dma_pm_callback,
    .states = (cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_DEEPSLEEP | CYHAL_SYSPM_CB_SYSTEM_HIBERNATE),
    .next = NULL,
    .args = NULL,
    .ignore_modes = CYHAL_SYSPM_BEFORE_TRANSITION,
};


/*******************************************************************************
*       Internal - Event callback
*******************************************************************************/

void _cyhal_dma_event_handler(void *obj)
{
    cyhal_dma_event_callback_t callback;
    cyhal_dma_event_t event;
    if (obj != NULL)
    {
        callback = (cyhal_dma_event_callback_t)(((cyhal_dma_t *)obj)->callback_data.callback);
        //Check if the DMA error status bit is set.
        if( ((cyhal_dma_t *)obj)->dma_req.errStatus )
        {
            //clear error status bit
            ((cyhal_dma_t *)obj)->dma_req.errStatus &= ~(_CYHAL_DMA_REQ_ERR_STATUS_BIT);
            event = CYHAL_DMA_GENERIC_ERROR;
        }
        else
        {
            event = CYHAL_DMA_TRANSFER_COMPLETE;
        }
        if(callback != NULL)
        {
            (callback)(((cyhal_dma_t *)obj)->callback_data.callback_arg, event);
        }
    }
}


/*******************************************************************************
*       HAL implementation
*******************************************************************************/

cy_rslt_t cyhal_dma_init_adv(cyhal_dma_t *obj, cyhal_dma_src_t *src, cyhal_dma_dest_t *dest,
                            cyhal_source_t *dest_source, uint8_t priority, cyhal_dma_direction_t direction)
{
    CY_UNUSED_PARAMETER(priority);
    CY_ASSERT(NULL != obj);

    if (!_cyhal_dma_arrays_initialized)
    {
        for (uint8_t i = 0; i < _CYHAL_DMA_CHANNELS; i++)
        {
            _cyhal_dma_obj[i] = NULL;
        }
        _cyhal_dma_arrays_initialized = true;
    }

    // Ignore flow control for now. May be required depending on channel.
    // Used when transfer length is not specified
    BTSS_DMAC_CONFIG_REG_FLOWCONTROL_t transfer_type =
        (direction == CYHAL_DMA_DIRECTION_MEM2MEM) ? BTSS_DMAC_CONFIG_REG_FLOWCNTRL_MEMORY_TO_MEMORY :
        (direction == CYHAL_DMA_DIRECTION_MEM2PERIPH) ? BTSS_DMAC_CONFIG_REG_FLOWCNTRL_MEMORY_TO_PERIPH_NO_FLOW :
        (direction == CYHAL_DMA_DIRECTION_PERIPH2MEM) ? BTSS_DMAC_CONFIG_REG_FLOWCNTRL_PERIPH_TO_MEMORY_NO_FLOW :
        (direction == CYHAL_DMA_DIRECTION_PERIPH2PERIPH) ? BTSS_DMAC_CONFIG_REG_FLOWCNTRL_PERIPH_TO_PERIPH_NO_FLOW :
        0xFF;

    uint8_t channel = _CYHAL_DMA_CH_NOT_ALLOCATED;
    BTSS_DMAC_CONTROL_LINE_t src_trigger = _CYHAL_DMAC_CONTROL_LINE_NONE;
    BTSS_DMAC_CONTROL_LINE_t dest_trigger = _CYHAL_DMAC_CONTROL_LINE_NONE;
    cy_rslt_t rslt = CY_RSLT_SUCCESS;

    // Can be either source or destination but not both. dest_source connection is not supported.
    if (((src != NULL) && (dest != NULL)) || (dest_source != NULL))
    {
        rslt = CYHAL_DMA_RSLT_ERR_INVALID_PARAMETER;
    }

    if (rslt == CY_RSLT_SUCCESS)
    {
        if (src != NULL)
        {
            switch (src->source)
            {
                case CYHAL_TRIGGER_DMAC_PTU_TO_MEM: /* HCI(PTU) and SCB are shared */
                case CYHAL_TRIGGER_DMAC_SCB0_RX_TO_MEM:
                case CYHAL_TRIGGER_DMAC_SCB1_RX_TO_MEM:
                case CYHAL_TRIGGER_DMAC_SCB2_RX_TO_MEM:
                    channel = _CYHAL_DMA_CH_PERIPH_TO_MEM;
                    break;
                case CYHAL_TRIGGER_DMAC_MXTDM0_TO_MEM:
                    channel = _CYHAL_DMA_CH_TDM0_TO_MEM;
                    break;
                case CYHAL_TRIGGER_DMAC_MXTDM1_TO_MEM:
                    channel = _CYHAL_DMA_CH_TDM1_TO_MEM;
                    break;
                default:
                    // Skip channel allocation. Do it during configuration instead
                    break;
            }

            if (channel != _CYHAL_DMA_CH_NOT_ALLOCATED)
                src_trigger = (BTSS_DMAC_CONTROL_LINE_t)src->source;
        }

        if (dest != NULL)
        {
            switch (dest->dest)
            {
                case CYHAL_TRIGGER_DMAC_MEM_TO_PTU: /* HCI(PTU) and SCB are shared */
                case CYHAL_TRIGGER_DMAC_MEM_TO_SCB0_TX:
                case CYHAL_TRIGGER_DMAC_MEM_TO_SCB1_TX:
                case CYHAL_TRIGGER_DMAC_MEM_TO_SCB2_TX:
                    channel = _CYHAL_DMA_CH_MEM_TO_PERIPH;
                    break;
                case CYHAL_TRIGGER_DMAC_MEM_TO_MXTDM0:
                    channel = _CYHAL_DMA_CH_MEM_TO_TDM0;
                    break;
                case CYHAL_TRIGGER_DMAC_MEM_TO_MXTDM1:
                    channel = _CYHAL_DMA_CH_MEM_TO_TDM1;
                    break;
                default:
                    // Skip channel allocation. Do it during configuration instead
                    break;
            }
            if (channel != _CYHAL_DMA_CH_NOT_ALLOCATED)
                dest_trigger = (BTSS_DMAC_CONTROL_LINE_t)dest->dest;
        }
    }

    // Channels can service multiple transfers. Investigate if chaining can be implemented.
    cyhal_resource_inst_t rscObj;

    if (rslt == CY_RSLT_SUCCESS)
    {
        rslt = cyhal_hwmgr_allocate(CYHAL_RSC_DMA, &rscObj);
    }

    if (rslt == (CY_RSLT_SUCCESS))
    {
        obj->resource.type = rscObj.type;
        obj->resource.block_num = rscObj.block_num;
        obj->resource.channel_num = rscObj.channel_num;
        obj->is_enabled = false;
        obj->transfer_type = transfer_type;
        obj->src_ctrl = src_trigger;
        obj->dest_ctrl = dest_trigger;
        obj->dma_req.next = NULL;
        obj->dma_req.channel = channel;
        obj->dma_req.callback = NULL;
        obj->dma_req.callback_arg = NULL;
        obj->dma_req.DMACCx_first_segment_regs.DMACCxSrcAddr = NULL;
        obj->dma_req.DMACCx_first_segment_regs.DMACCxDestAddr = NULL;
        obj->dma_req.DMACCx_first_segment_regs.DMACCxControl = 0;
        obj->dma_req.DMACCx_first_segment_regs.DMACCxLLI = NULL;
        obj->dma_req.DMACCxConfiguration = 0;
        obj->dma_req.msg.next = NULL;
        obj->dma_req.msg.code = 0;
        obj->callback_data.callback = NULL;
        obj->callback_data.callback_arg = NULL;

        if (!_cyhal_dmac_enabled)
        {
            // This is a global enable so do it only once.
            btss_dmac_init();
            _cyhal_dmac_enabled = true;
        }

        if (!_cyhal_dma_pm_has_enabled())
        {
            _cyhal_syspm_register_peripheral_callback(&_cyhal_dma_syspm_callback_data);
        }

        _cyhal_dma_obj[channel] = obj;
    }

    return rslt;
}

cy_rslt_t cyhal_dma_init_cfg(cyhal_dma_t *obj, const cyhal_dma_configurator_t *cfg)
{
    /* No configurators supported on this architecture */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(cfg);
    return CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE;
}

void cyhal_dma_free(cyhal_dma_t *obj)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(!cyhal_dma_is_busy(obj));
    CY_ASSERT(_cyhal_dma_arrays_initialized); /* Should not be freeing if we never initialized anything */

    _cyhal_dma_obj[obj->resource.channel_num] = NULL;

    if (!_cyhal_dma_pm_has_enabled())
    {
        _cyhal_syspm_unregister_peripheral_callback(&_cyhal_dma_syspm_callback_data);
    }

    cyhal_hwmgr_free(&obj->resource);
}

cy_rslt_t cyhal_dma_configure(cyhal_dma_t *obj, const cyhal_dma_cfg_t *cfg)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);

    cy_rslt_t status = CY_RSLT_SUCCESS;
    uint32_t data_width;
    uint32_t burst_size;

    if ((cfg->action == CYHAL_DMA_TRANSFER_BURST)
        || (cfg->action == CYHAL_DMA_TRANSFER_BURST_DISABLE)
        || (cfg->action == CYHAL_DMA_TRANSFER_FULL_DISABLE))
    {
        status = CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE; // Unsupported actions
    }

    if (cfg->length >= _CYHAL_DMA_MAX_LENGTH)
    {
        status = CYHAL_DMA_RSLT_ERR_INVALID_TRANSFER_SIZE;
    }

    // Allocate channel info based on source and destination addresses
    if (obj->dma_req.channel == _CYHAL_DMA_CH_NOT_ALLOCATED)
    {
        uint8_t channel = _CYHAL_DMA_CH_NOT_ALLOCATED;
        BTSS_DMAC_CONTROL_LINE_t src_trigger = _CYHAL_DMAC_CONTROL_LINE_NONE;
        BTSS_DMAC_CONTROL_LINE_t dest_trigger = _CYHAL_DMAC_CONTROL_LINE_NONE;

        // Memory to peripheral path
        if ((cfg->src_addr >= _CYHAL_DMA_ADDR_SRAM_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_SRAM_END))
        {
            channel = _CYHAL_DMA_CH_MEM_TO_MEM;
            src_trigger = _CYHAL_DMAC_CONTROL_LINE_M2M;

            if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_SCB0_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_SCB0_END))
            {
                channel = _CYHAL_DMA_CH_MEM_TO_PERIPH;
                dest_trigger = BTSS_DMAC_CONTROL_LINE_DST_SCB0;
            }
            else if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_SCB1_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_SCB1_END))
            {
                channel = _CYHAL_DMA_CH_MEM_TO_PERIPH;
                dest_trigger = BTSS_DMAC_CONTROL_LINE_DST_SCB1;
            }
            else if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_SCB2_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_SCB2_END))
            {
                channel = _CYHAL_DMA_CH_MEM_TO_PERIPH;
                dest_trigger = BTSS_DMAC_CONTROL_LINE_DST_SCB2;
            }
            else if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_TDM0_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_TDM0_END))
            {
                channel = _CYHAL_DMA_CH_MEM_TO_TDM0;
                dest_trigger = BTSS_DMAC_CONTROL_LINE_DST_MXTDM0;
            }
            else if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_TDM1_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_TDM1_END))
            {
                channel = _CYHAL_DMA_CH_MEM_TO_TDM1;
                dest_trigger = BTSS_DMAC_CONTROL_LINE_DST_MXTDM1;
            }
            else
            {
                /* Either unsupported peripheral or an M2M transfer */
            }
        }

        // Peripheral to memory path
        if ((cfg->dst_addr >= _CYHAL_DMA_ADDR_SRAM_START) && (cfg->dst_addr < _CYHAL_DMA_ADDR_SRAM_END))
        {
            channel = _CYHAL_DMA_CH_MEM_TO_MEM;
            dest_trigger = _CYHAL_DMAC_CONTROL_LINE_M2M;

            if ((cfg->src_addr >= _CYHAL_DMA_ADDR_SCB0_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_SCB0_END))
            {
                channel = _CYHAL_DMA_CH_PERIPH_TO_MEM;
                src_trigger = BTSS_DMAC_CONTROL_LINE_SRC_SCB0;
            }
            else if ((cfg->src_addr >= _CYHAL_DMA_ADDR_SCB1_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_SCB1_END))
            {
                channel = _CYHAL_DMA_CH_PERIPH_TO_MEM;
                src_trigger = BTSS_DMAC_CONTROL_LINE_SRC_SCB1;
            }
            else if ((cfg->src_addr >= _CYHAL_DMA_ADDR_SCB2_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_SCB2_END))
            {
                channel = _CYHAL_DMA_CH_PERIPH_TO_MEM;
                src_trigger = BTSS_DMAC_CONTROL_LINE_SRC_SCB2;
            }
            else if ((cfg->src_addr >= _CYHAL_DMA_ADDR_TDM0_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_TDM0_END))
            {
                channel = _CYHAL_DMA_CH_TDM0_TO_MEM;
                src_trigger = BTSS_DMAC_CONTROL_LINE_SRC_MXTDM0;
            }
            else if ((cfg->src_addr >= _CYHAL_DMA_ADDR_TDM1_START) && (cfg->src_addr < _CYHAL_DMA_ADDR_TDM1_END))
            {
                channel = _CYHAL_DMA_CH_TDM1_TO_MEM;
                src_trigger = BTSS_DMAC_CONTROL_LINE_SRC_MXTDM1;
            }
            else
            {
                /* Either unsupported peripheral or an M2M transfer */
            }
        }

        if (!((src_trigger == _CYHAL_DMAC_CONTROL_LINE_NONE) && (dest_trigger == _CYHAL_DMAC_CONTROL_LINE_NONE)))
        {
            obj->dma_req.channel = channel;
            obj->src_ctrl = src_trigger;
            obj->dest_ctrl = dest_trigger;
        }
    }

    if (obj->dma_req.channel == _CYHAL_DMA_CH_NOT_ALLOCATED)
    {
        status = CYHAL_DMA_RSLT_ERR_INVALID_PARAMETER; // Couldn't find a valid channel
    }

    if (status == CY_RSLT_SUCCESS)
    {
        data_width = (cfg->transfer_width == 8) ? BTSS_DMAC_CONTROL_REG_DATAWIDTH_1BYTE :
                    (cfg->transfer_width == 16) ? BTSS_DMAC_CONTROL_REG_DATAWIDTH_HALF_WORD :
                    (cfg->transfer_width == 32) ? BTSS_DMAC_CONTROL_REG_DATAWIDTH_WORD :
                    CYHAL_DMA_RSLT_ERR_INVALID_TRANSFER_WIDTH;

        status = (data_width != CYHAL_DMA_RSLT_ERR_INVALID_TRANSFER_WIDTH) ?
                    CY_RSLT_SUCCESS : CYHAL_DMA_RSLT_ERR_INVALID_TRANSFER_WIDTH;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        burst_size = (cfg->burst_size == 0) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_1 : /* intentionally set to 1 */
                    (cfg->burst_size == 1) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_1 :
                    (cfg->burst_size == 4) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_4 :
                    (cfg->burst_size == 8) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_8 :
                    (cfg->burst_size == 16) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_16 :
                    (cfg->burst_size == 32) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_32 :
                    (cfg->burst_size == 64) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_64 :
                    (cfg->burst_size == 128) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_128 :
                    (cfg->burst_size == 256) ? BTSS_DMAC_CONTROL_REG_BURST_SIZE_256 :
                    CYHAL_DMA_RSLT_ERR_INVALID_BURST_SIZE;

        status = (burst_size != CYHAL_DMA_RSLT_ERR_INVALID_BURST_SIZE) ?
                    CY_RSLT_SUCCESS : CYHAL_DMA_RSLT_ERR_INVALID_BURST_SIZE;
    }

    if (status == CY_RSLT_SUCCESS)
    {
        BTSS_DMAC_CONFIG_REG_t cfg_reg;
        cfg_reg.u32 = (uint32_t)(obj->dma_req.DMACCxConfiguration);
        cfg_reg.bitfields.srcDmaLine = obj->src_ctrl;
        cfg_reg.bitfields.dstDmaLine = obj->dest_ctrl;
        cfg_reg.bitfields.flowcontrol = obj->transfer_type;
        // Potential ROM bug: errIntMask must be 1 for interrupts & transfers to work
        // Set this back to 0 once resolved
        cfg_reg.bitfields.errIntMask = 1;

        BTSS_DMAC_CONTROL_REG_t ctrl_reg;
        ctrl_reg.u32 = (uint32_t)(obj->dma_req.DMACCx_first_segment_regs.DMACCxControl);
        ctrl_reg.bitfields.length = cfg->length;
        ctrl_reg.bitfields.srcBurstSize = burst_size;
        ctrl_reg.bitfields.dstBurstSize = burst_size;
        ctrl_reg.bitfields.srcDataWidth = data_width;
        ctrl_reg.bitfields.dstDataWidth = data_width;
        ctrl_reg.bitfields.srcIncrement = cfg->src_increment;
        ctrl_reg.bitfields.dstIncrement = cfg->dst_increment;

        btss_dmac_dmaReqSetChannel(&(obj->dma_req), obj->dma_req.channel);
        // Writes to DMACCx_first_segment_regs (channel is already set so skipping configuring that)
        btss_dmac_dmaReqSetSrc(&(obj->dma_req), cfg->src_addr);
        btss_dmac_dmaReqSetDst(&(obj->dma_req), cfg->dst_addr);
        btss_dmac_dmaReqSetLLI(&(obj->dma_req), NULL); // Linked list item currently not used

        // Writes to DMACCx_first_segment_regs.DMACCxControl
        btss_dmac_dmaReqSetControl(&(obj->dma_req), ctrl_reg);

        // Writes to DMACCxConfiguration
        btss_dmac_dmaReqSetConfig(&(obj->dma_req), cfg_reg);

        if(obj->transfer_type == BTSS_DMAC_CONFIG_REG_FLOWCNTRL_MEMORY_TO_PERIPH_NO_FLOW)
        {
            btss_dmac_setPeripheralDMACSync(obj->dest_ctrl, true);
        }
        else if (obj->transfer_type == BTSS_DMAC_CONFIG_REG_FLOWCNTRL_PERIPH_TO_MEMORY_NO_FLOW)
        {
            btss_dmac_setPeripheralDMACSync(obj->src_ctrl, true);
        }
        else
        {
            btss_dmac_setPeripheralDMACSync(obj->src_ctrl, false);
            btss_dmac_setPeripheralDMACSync(obj->dest_ctrl, false);
        }
   }

    return status;
}

cy_rslt_t cyhal_dma_start_transfer(cyhal_dma_t *obj)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);
    bool status = false;

    if (_cyhal_dma_pm_transition_pending_value)
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }

    if (obj->is_enabled)
    {
        if (cyhal_dma_is_busy(obj))
            return CYHAL_DMA_RSLT_WARN_TRANSFER_ALREADY_STARTED;

        status = btss_dmac_requestTransfer(&(obj->dma_req));
        if(!status)
            return CYHAL_DMA_RSLT_ERR_INVALID_PARAMETER;
    }

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_dma_enable(cyhal_dma_t *obj)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);

    // Enable DMA
    obj->is_enabled = true;
    BTSS_DMAC_CONFIG_REG_t cfg_reg;
    cfg_reg.u32 = (uint32_t)(obj->dma_req.DMACCxConfiguration);
    cfg_reg.bitfields.channelEn = 1;
    btss_dmac_dmaReqSetConfig(&(obj->dma_req), cfg_reg);
    btss_dmac_dmaReqGetStatus(&(obj->dma_req)); // Read back status to ensure propagation

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_dma_disable(cyhal_dma_t *obj)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);

    btss_dmac_killTransfer(&(obj->dma_req));

    // Disable DMA
    obj->is_enabled = false;
    BTSS_DMAC_CONFIG_REG_t cfg_reg;
    cfg_reg.u32 = (uint32_t)(obj->dma_req.DMACCxConfiguration);
    cfg_reg.bitfields.channelEn = 0;
    btss_dmac_dmaReqSetConfig(&(obj->dma_req), cfg_reg);
    btss_dmac_dmaReqGetStatus(&(obj->dma_req)); // Read back status to ensure propagation

    return CY_RSLT_SUCCESS;
}

bool cyhal_dma_is_busy(cyhal_dma_t *obj)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);

    BTSS_DMAC_REQ_STATUS_t status = btss_dmac_dmaReqGetStatus(&(obj->dma_req));
    return ((status == BTSS_DMA_REQ_STATUS_IN_PROGRESS) || (status == BTSS_DMA_REQ_STATUS_WAITING_IN_QUEUE));
}

void cyhal_dma_register_callback(cyhal_dma_t *obj, cyhal_dma_event_callback_t callback, void *callback_arg)
{
    CY_ASSERT(NULL != obj);

    if (callback != NULL)
    {
        btss_dmac_dmaReqSetCallback(&(obj->dma_req), _cyhal_dma_event_handler, obj);
    }

    uint32_t saved_intr_status = cyhal_system_critical_section_enter();
    obj->callback_data.callback = (cy_israddress)callback;
    obj->callback_data.callback_arg = callback_arg;
    cyhal_system_critical_section_exit(saved_intr_status);
}

void cyhal_dma_enable_event(cyhal_dma_t *obj, cyhal_dma_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);
    CY_ASSERT(NULL != obj);
    CY_ASSERT(obj->resource.type == CYHAL_RSC_DMA);

    const uint32_t err_mask = CYHAL_DMA_SRC_BUS_ERROR | CYHAL_DMA_DST_BUS_ERROR
                            | CYHAL_DMA_SRC_MISAL | CYHAL_DMA_DST_MISAL
                            | CYHAL_DMA_CURR_PTR_NULL | CYHAL_DMA_ACTIVE_CH_DISABLED
                            | CYHAL_DMA_DESCR_BUS_ERROR;

    BTSS_DMAC_CONFIG_REG_t cfg_reg;
    cfg_reg.u32 = (uint32_t)(obj->dma_req.DMACCxConfiguration);
    BTSS_DMAC_CONTROL_REG_t ctrl_reg;
    ctrl_reg.u32 = (uint32_t)(obj->dma_req.DMACCx_first_segment_regs.DMACCxControl);

    if (((event & CYHAL_DMA_TRANSFER_COMPLETE) == CYHAL_DMA_TRANSFER_COMPLETE)
        || ((event & CYHAL_DMA_DESCRIPTOR_COMPLETE) == CYHAL_DMA_DESCRIPTOR_COMPLETE))
    {
        cfg_reg.bitfields.tcIntMask = 1;
    }
    //if ((event & err_mask) != 0UL)
    // Potential ROM bug: errIntMask must always be 1 for transfers and interrupts to work
    // When resolved, set this to only be 1 if the above commented out conditional is true
    CY_UNUSED_PARAMETER(err_mask);
    cfg_reg.bitfields.errIntMask = 1;

    ctrl_reg.bitfields.intEnable = enable ? 1 : 0;

    btss_dmac_dmaReqSetConfig(&(obj->dma_req), cfg_reg);
    btss_dmac_dmaReqSetControl(&(obj->dma_req), ctrl_reg);
}

cy_rslt_t cyhal_dma_connect_digital(cyhal_dma_t *obj, cyhal_source_t source, cyhal_dma_input_t input)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(input);
    return CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE;
}

cy_rslt_t cyhal_dma_enable_output(cyhal_dma_t *obj, cyhal_dma_output_t output, cyhal_source_t *source)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(output);
    CY_UNUSED_PARAMETER(source);
    return CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE;
}

cy_rslt_t cyhal_dma_disconnect_digital(cyhal_dma_t *obj, cyhal_source_t source, cyhal_dma_input_t input)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(input);
    return CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE;
}

cy_rslt_t cyhal_dma_disable_output(cyhal_dma_t *obj, cyhal_dma_output_t output)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(output);
    return CYHAL_DMA_RSLT_FATAL_UNSUPPORTED_HARDWARE;
}

uint32_t cyhal_dma_get_max_elements_per_burst(cyhal_dma_t *obj)
{
    CY_UNUSED_PARAMETER(obj);
    return BTSS_DMAC_CONTROL_REG_BURST_SIZE_256;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
