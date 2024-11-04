/*******************************************************************************
* File Name: cyhal_sdio.c
*
* Description:
* Provides a high level interface for interacting with the Infineon SDIO. This
* is a wrapper around the lower level PDL API.
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

#include "cyhal_hwmgr.h"
#include "cyhal_sdio.h"
#include "cyhal_system.h"
#include "cyhal_syspm.h"
#include "cyhal_utils.h"

/**
* \addtogroup group_hal_impl_sdio SDIO (Secure Digital Input Output)
* \ingroup group_hal_impl
* \{
*
* The SDIO device HAL implemenation for CAT5 is available only for CYW955900 family of devices.
* SDIO host functionality is not available on these devices.
*
* \note \ref cyhal_sdio_register_callback parameter <b>callback_arg</b> for this function
* is slightly different from other HAL register_callback functions. If the parameter
* <b>callback_arg</b> is NULL the returned argument of the callback is the pointer to
* the internal SDIO Event Data structure sdiod_event_data_t.
*
* \} group_hal_impl_sdio
*/


#if (CYHAL_DRIVER_AVAILABLE_SDIO) && (CYHAL_DRIVER_AVAILABLE_SDIO_DEV)

#include "sdiod_api.h"

#if defined(__cplusplus)
extern "C"
{
#endif

static cyhal_sdio_t *_cyhal_sdio_config_struct[1];
sdiod_dma_descs_buf_t* _cyhal_sdio_dma_desc = NULL;

/*******************************************************************************
*       Callback Interrupt Service Routine
*******************************************************************************/

void _cyhal_sdio_cb(sdiod_event_code_t event_code, void *event_data)
{
    cyhal_sdio_event_t event;

    switch (event_code)
    {
        case SDIOD_EVENT_CODE_HOST_INFO:
            event = _cyhal_sdio_config_struct[0]->events & CYHAL_SDIO_DEV_HOST_INFO;
            break;
        case SDIOD_EVENT_CODE_RX_DONE:
            event = _cyhal_sdio_config_struct[0]->events & CYHAL_SDIO_DEV_READ_COMPLETE;
            break;
        case SDIOD_EVENT_CODE_TX_DONE:
            event = _cyhal_sdio_config_struct[0]->events & CYHAL_SDIO_DEV_WRITE_COMPLETE;
            break;
        case SDIOD_EVENT_CODE_RX_ERROR:
            event = _cyhal_sdio_config_struct[0]->events & CYHAL_SDIO_DEV_READ_ERROR;
            break;
        case SDIOD_EVENT_CODE_TX_ERROR:
            event = _cyhal_sdio_config_struct[0]->events & CYHAL_SDIO_DEV_WRITE_ERROR;
            break;
        default:
            event = 0;
            break;
    }

    cyhal_sdio_event_callback_t callback = (cyhal_sdio_event_callback_t) _cyhal_sdio_config_struct[0]->callback_data.callback;

    if (callback != NULL)
    {
        if (_cyhal_sdio_config_struct[0]->callback_data.callback_arg == NULL)
        {
            (callback)(event_data, event);
        }
        else
        {
            (callback)(_cyhal_sdio_config_struct[0]->callback_data.callback_arg, event);
        }
    }
}

/*******************************************************************************
*       Deep Sleep Callback Service Routine
*******************************************************************************/

static bool _cyhal_sdio_syspm_callback(cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode, void *callback_arg)
{
    bool allow = true;
    cyhal_sdio_t *sdio = (cyhal_sdio_t *)callback_arg;

    if (state == CYHAL_SYSPM_CB_CPU_DEEPSLEEP)
    {
        switch (mode)
        {
            case CYHAL_SYSPM_CHECK_READY:
            {
                if(!(cyhal_sdio_is_busy(sdio)))
                {
                    /* if device is not busy the sleepCSR is checked to decide if sleep is allowed */
                    sdio->pm_transition_pending = (SDIOD_STATUS_SUCCESS == sdiod_IsSleepAllowed()) ? true : false;
                }
                else
                {
                    sdio->pm_transition_pending = false;
                }
                allow = sdio->pm_transition_pending;
                break;
            }

            case CYHAL_SYSPM_BEFORE_TRANSITION:
            {
                (void)sdiod_preSleep();
                break;
            }

            case CYHAL_SYSPM_AFTER_DS_WFI_TRANSITION:
            {
                break;
            }

            case CYHAL_SYSPM_AFTER_TRANSITION:
            {
                (void)sdiod_postSleep();
                break;
            }
            case CYHAL_SYSPM_CHECK_FAIL:
            {
                sdio->pm_transition_pending = false;
                break;
            }

            default:
                CY_ASSERT(false);
                break;
        }
    }

    return allow;
}


/*******************************************************************************
*       Public API implementation
*******************************************************************************/

cy_rslt_t cyhal_sdio_init(cyhal_sdio_t *obj, cyhal_gpio_t cmd, cyhal_gpio_t clk, cyhal_gpio_t data0, cyhal_gpio_t data1,
        cyhal_gpio_t data2, cyhal_gpio_t data3)
{
    CY_ASSERT(NULL != obj);
    obj->resource.type = CYHAL_RSC_INVALID;
    obj->pin_clk = NC;
    obj->pin_cmd = NC;
    obj->pin_data_0 = NC;
    obj->pin_data_1 = NC;
    obj->pin_data_2 = NC;
    obj->pin_data_3 = NC;
    obj->hw_inited = false;
    obj->is_ready = false;
    obj->callback_data.callback = NULL;
    obj->callback_data.callback_arg = NULL;

    cy_rslt_t result = CY_RSLT_SUCCESS;

    const cyhal_resource_pin_mapping_t* clk_map = _CYHAL_UTILS_GET_RESOURCE(clk, cyhal_pin_map_sdio_clk);
    const cyhal_resource_pin_mapping_t* cmd_map = _CYHAL_UTILS_GET_RESOURCE(cmd, cyhal_pin_map_sdio_cmd);
    const cyhal_resource_pin_mapping_t* data_0_map = _CYHAL_UTILS_GET_RESOURCE(data0, cyhal_pin_map_sdio_data_0);
    const cyhal_resource_pin_mapping_t* data_1_map = _CYHAL_UTILS_GET_RESOURCE(data1, cyhal_pin_map_sdio_data_1);
    const cyhal_resource_pin_mapping_t* data_2_map = _CYHAL_UTILS_GET_RESOURCE(data2, cyhal_pin_map_sdio_data_2);
    const cyhal_resource_pin_mapping_t* data_3_map = _CYHAL_UTILS_GET_RESOURCE(data3, cyhal_pin_map_sdio_data_3);

    if ((clk_map == NULL) || (cmd_map == NULL) || (data_0_map == NULL)
        || (data_1_map == NULL) || (data_2_map == NULL) || (data_3_map == NULL))
    {
        result = CYHAL_SDIO_RSLT_ERR_BAD_PARAM;
    }
    else
    {
        cyhal_resource_inst_t rsc = { CYHAL_RSC_SDIO, clk_map->block_num, clk_map->channel_num };
        result = cyhal_hwmgr_reserve(&rsc);
        if (result == CY_RSLT_SUCCESS)
        {
            _CYHAL_UTILS_ASSIGN_RESOURCE(obj->resource, rsc.type, clk_map);
        }
    }

    // reserve the clk pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(clk_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_CLK);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_clk = clk;
        }
    }

    // reserve the cmd pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(cmd_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_CMD);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_cmd = cmd;
        }
    }

    // reserve the data0 pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(data_0_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_0);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_data_0 = data0;
        }
    }

    // reserve the data1 pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(data_1_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_1);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_data_1 = data1;
        }
    }

    // reserve the data2 pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(data_2_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_2);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_data_2 = data2;
        }
    }

    // reserve the data3 pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(data_3_map, CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_3);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_data_3 = data3;
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        obj->pm_transition_pending = false;
        obj->pm_callback_data.callback = &_cyhal_sdio_syspm_callback;
        obj->pm_callback_data.states = (cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_DEEPSLEEP);
        obj->pm_callback_data.next = NULL;
        obj->pm_callback_data.args = obj;
        /* Note: Some modes may be ignored for out-of-band use-case */
        obj->pm_callback_data.ignore_modes = (cyhal_syspm_callback_mode_t)0;
        _cyhal_syspm_register_peripheral_callback(&(obj->pm_callback_data));

        _cyhal_sdio_config_struct[0] = obj;
    }
    else
    {
        cyhal_sdio_free(obj);
    }

    return result;
}

void cyhal_sdio_free(cyhal_sdio_t *obj)
{
    CY_ASSERT(NULL != obj);

    if (obj->hw_inited)
    {
        sdiod_Deinit();
        obj->hw_inited = false;
    }

    if (obj->resource.type != CYHAL_RSC_INVALID)
    {
        cyhal_hwmgr_free(&(obj->resource));
        obj->resource.type = CYHAL_RSC_INVALID;
    }

    _cyhal_syspm_unregister_peripheral_callback(&(obj->pm_callback_data));

    _cyhal_utils_release_if_used(&(obj->pin_clk));
    _cyhal_utils_release_if_used(&(obj->pin_cmd));
    _cyhal_utils_release_if_used(&(obj->pin_data_0));
    _cyhal_utils_release_if_used(&(obj->pin_data_1));
    _cyhal_utils_release_if_used(&(obj->pin_data_2));
    _cyhal_utils_release_if_used(&(obj->pin_data_3));
}

cy_rslt_t cyhal_sdio_configure(cyhal_sdio_t *obj, const cyhal_sdio_cfg_t *config)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (config == NULL)
    {
        result = CYHAL_SDIO_RSLT_ERR_BAD_PARAM;
    }
    else
    {
        // Supports SDIO device with max 50 MHz and max block size of 512
        if ((config->frequencyhal_hz > 50000000) || (config->block_size > 512) || !(config->is_sdio_dev))
        {
            result = CYHAL_SDIO_RSLT_ERR_UNSUPPORTED;
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        if (_cyhal_sdio_dma_desc == NULL)
        {
            _cyhal_sdio_dma_desc = (sdiod_dma_descs_buf_t*)thread_ap_memory_AllocatePermanent(SDIO_F2_DMA_BUFFER_SIZE);
        }
        if (_cyhal_sdio_dma_desc != NULL)
        {
            result = (SDIOD_STATUS_SUCCESS == sdiod_Init((_cyhal_sdio_dma_desc))) ? CY_RSLT_SUCCESS : CYHAL_SDIO_RSLT_ERR_CONFIG;
            if (result == CY_RSLT_SUCCESS)
            {
                thread_ap_sec_hw_openDeviceAccess(SEC_HW_DEVICE_SDIO, (uint8_t*)_cyhal_sdio_dma_desc, SDIO_F2_DMA_BUFFER_SIZE, 0);

                obj->buffer.rx_header = (uint8_t*)_cyhal_sdio_dma_desc + sizeof(sdiod_dma_descs_buf_t);
                obj->buffer.rx_payload = obj->buffer.rx_header + sizeof(sdiod_f2_rx_frame_hdr_t);
                obj->buffer.tx_payload = obj->buffer.rx_payload + SDIO_F2_FRAME_MAX_PAYLOAD;
            }
            obj->hw_inited = (result == CY_RSLT_SUCCESS);
        }
        else
        {
            result = CYHAL_SDIO_RSLT_ERR_CONFIG;
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        result = (SDIOD_STATUS_SUCCESS == sdiod_RegisterCallback(_cyhal_sdio_cb)) ? CY_RSLT_SUCCESS : CYHAL_SDIO_RSLT_ERR_CONFIG;
    }

    return result;
}

bool cyhal_sdio_is_busy(const cyhal_sdio_t *obj)
{
    CY_ASSERT(NULL != obj);

    if (obj->is_ready)
    {
        sdiod_status_t dev_status_rx = sdiod_get_RxStatus();
        sdiod_status_t dev_status_tx = sdiod_get_TxStatus();

        if ((dev_status_rx == SDIOD_STATUS_ASYNC_IN_PROGRESS)
            || (dev_status_rx == SDIOD_STATUS_ERR_BUSY)
            || (dev_status_tx == SDIOD_STATUS_ASYNC_IN_PROGRESS)
            || (dev_status_tx == SDIOD_STATUS_ERR_BUSY))
        {
            return true;
        }
    }

    return false;
}

cy_rslt_t cyhal_sdio_abort_async(cyhal_sdio_t *obj)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (obj->is_ready)
    {
        sdiod_status_t dev_status;

        dev_status = sdiod_get_RxStatus();
        if ((dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS)
            || (dev_status == SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Receive_Abort();
            if (dev_status != SDIOD_STATUS_SUCCESS)
            {
                result = CYHAL_SDIO_RSLT_CANCELED;
            }
        }

        dev_status = sdiod_get_TxStatus();
        if ((dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS)
            || (dev_status == SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Transmit_Abort();
            if (dev_status != SDIOD_STATUS_SUCCESS)
            {
                result = CYHAL_SDIO_RSLT_CANCELED;
            }
        }
    }

    return result;
}

void cyhal_sdio_register_callback(cyhal_sdio_t *obj, cyhal_sdio_event_callback_t callback, void *callback_arg)
{
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    obj->callback_data.callback = (cy_israddress) callback;
    obj->callback_data.callback_arg = callback_arg;
    cyhal_system_critical_section_exit(savedIntrStatus);
}

void cyhal_sdio_enable_event(cyhal_sdio_t *obj, cyhal_sdio_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);

    obj->events = (enable) ? (obj->events | event) : (obj->events & ~event);
    (obj->events != 0) ? sdiod_EnableInterrupt() : sdiod_DisableInterrupt();
}

cy_rslt_t cyhal_sdio_init_cfg(cyhal_sdio_t *obj, const cyhal_sdio_configurator_t *cfg)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(cfg);
    return CYHAL_SDIO_RSLT_ERR_UNSUPPORTED;
}

bool cyhal_sdio_dev_is_ready(cyhal_sdio_t *obj)
{
    CY_ASSERT(NULL != obj);

    if (obj->hw_inited)
    {
        sdiod_status_t dev_status = sdiod_IsHostIOEnabled();
        if ((dev_status != SDIOD_STATUS_ERR_IO_NOT_READY) && !(obj->is_ready))
        {
            /* Immediately enable F2 functionality */
            dev_status = sdiod_SetIOReady();
            obj->is_ready = (dev_status == SDIOD_STATUS_SUCCESS);
        }
    }

    return obj->is_ready;
}

cy_rslt_t cyhal_sdio_dev_mailbox_write(cyhal_sdio_t *obj, uint32_t bits, uint32_t *data)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_MAILBOX_WRITE_ERROR;

    if (obj->is_ready)
    {
        sdiod_status_t dev_status = SDIOD_STATUS_SUCCESS;

        if (data != NULL)
        {
            dev_status = sdiod_set_ToHostMailboxData(*data);
        }

        if (dev_status == SDIOD_STATUS_SUCCESS)
        {
            dev_status = sdiod_send_ToHostMailboxSignal(bits);
        }

        if (dev_status == SDIOD_STATUS_SUCCESS)
        {
            result = CY_RSLT_SUCCESS;
        }
    }

    return result;
}

cy_rslt_t cyhal_sdio_dev_mailbox_read(cyhal_sdio_t *obj, uint32_t *data)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_MAILBOX_READ_ERROR;

    if (data == NULL)
    {
        result = CYHAL_SDIO_RSLT_ERR_BAD_PARAM;
    }
    else
    {
        if (obj->is_ready)
        {
            sdiod_status_t dev_status = SDIOD_STATUS_SUCCESS;
            dev_status = sdiod_read_FromHostMailboxData(data);
            result = (dev_status == SDIOD_STATUS_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_SDIO_DEV_RSLT_MAILBOX_READ_ERROR;
        }
    }

    return result;
}

cy_rslt_t cyhal_sdio_dev_read(cyhal_sdio_t *obj, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_READ_ERROR;

    if (obj->pm_transition_pending)
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }

    if (obj->is_ready)
    {
        sdiod_status_t dev_status = sdiod_get_RxStatus();
        // Initiate async read if it was not initiated before
        if ((dev_status != SDIOD_STATUS_ASYNC_IN_PROGRESS)
            && (dev_status != SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Receive_Async(data, length);
        }

        if ((dev_status == SDIOD_STATUS_SUCCESS) || (dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS))
        {
            dev_status = sdiod_get_RxStatus();
            while ((SDIOD_STATUS_SUCCESS != dev_status) && (timeout_ms > 0))
            {
                cyhal_system_delay_ms(1);
                dev_status = sdiod_get_RxStatus();
                timeout_ms--;
            }
        }

        if ((dev_status == SDIOD_STATUS_SUCCESS) && (timeout_ms != 0))
        {
            result = CY_RSLT_SUCCESS;
        }
    }

    return result;
}

cy_rslt_t cyhal_sdio_dev_write(cyhal_sdio_t *obj, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_WRITE_ERROR;

    if (obj->pm_transition_pending)
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }

    if (obj->is_ready)
    {
        sdiod_status_t dev_status = sdiod_get_TxStatus();

        // Initiate async write if it was not initiated before
        if ((dev_status != SDIOD_STATUS_ASYNC_IN_PROGRESS)
            && (dev_status != SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Transmit_Async((uint8_t *)data, length);
        }

        if ((dev_status == SDIOD_STATUS_SUCCESS) || (dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS))
        {
            dev_status = sdiod_get_TxStatus();
            while ((SDIOD_STATUS_SUCCESS != dev_status) && (timeout_ms > 0))
            {
                cyhal_system_delay_ms(1);
                dev_status = sdiod_get_TxStatus();
                timeout_ms--;
            }
        }

        if ((dev_status == SDIOD_STATUS_SUCCESS) && (timeout_ms != 0))
        {
            result = CY_RSLT_SUCCESS;
        }

    }

    return result;
}

cy_rslt_t cyhal_sdio_dev_read_async(cyhal_sdio_t *obj, uint8_t *data, size_t length)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_READ_ERROR;

    if (obj->pm_transition_pending)
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }

    if (obj->is_ready)
    {
        sdiod_status_t dev_status = sdiod_get_RxStatus();
        if ((dev_status != SDIOD_STATUS_ASYNC_IN_PROGRESS)
            && (dev_status != SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Receive_Async(data, length);
            if ((dev_status == SDIOD_STATUS_SUCCESS) || (dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS))
            {
                result = CY_RSLT_SUCCESS;
            }
        }
    }

    return result;
}

cy_rslt_t cyhal_sdio_dev_write_async(cyhal_sdio_t *obj, const uint8_t *data, size_t length)
{
    CY_ASSERT(NULL != obj);

    if (obj->pm_transition_pending)
    {
        return CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }

    cy_rslt_t result = CYHAL_SDIO_DEV_RSLT_WRITE_ERROR;

    if (obj->is_ready)
    {
        sdiod_status_t dev_status = sdiod_get_TxStatus();
        if ((dev_status != SDIOD_STATUS_ASYNC_IN_PROGRESS)
            && (dev_status != SDIOD_STATUS_ERR_BUSY))
        {
            dev_status = sdiod_Transmit_Async((uint8_t *)data, length);

            if ((dev_status == SDIOD_STATUS_SUCCESS) || (dev_status == SDIOD_STATUS_ASYNC_IN_PROGRESS))
            {
                result = CY_RSLT_SUCCESS;
            }
        }
    }

    return result;
}

#if defined(__cplusplus)
}
#endif

#endif /* (CYHAL_DRIVER_AVAILABLE_SDIO) && (CYHAL_DRIVER_AVAILABLE_SDIO_DEV) */
