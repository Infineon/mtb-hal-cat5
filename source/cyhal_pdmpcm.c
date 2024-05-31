/*******************************************************************************
* File Name: cyhal_pdmpcm.c
*
* Description:
* Provides a high level interface for interacting with the Infineon PDMPCM. This is
* a wrapper around the lower level PDL API.
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

#include "cyhal_adc.h"
#include "cyhal_comp.h"
#include "cyhal_hwmgr.h"
#include "cyhal_pdmpcm.h"
#include "cyhal_system.h"
#include "cyhal_syspm.h"
#include "cyhal_utils.h"

/**
* \addtogroup group_hal_impl_pdmpcm PDM/PCM (Pulse Density Modulation to Pulse Code Modulation Converter)
* \ingroup group_hal_impl
* \{
*
* The CAT5 PDM/PCM Supports the following conversion parameters:<ul>
* <li>Mode: Mono Left, Mono Right
* <li>Word Length: 16 bits</li>
* <li>Sampling Rate: PDM/PCM rates:<ul>
*   <li>2.048M/16k (decimation rate=128)</li>
*   <li>2.048M/8k  (decimation rate=256)</li>
*   <li>1.024M/16k  (decimation rate=64)</li>
*   <li>1.024M/8k  (decimation rate=128)</li>
*   <li>(DMIC only) 512k/16k  (decimation rate=32)</li>
*   <li>(DMIC only) 512/8k  (decimation rate=64)</li>
*   </ul>
* </li>
* </ul>
*
* The following features are not supported:<ul>
* <li>Stereo</li>
* <li>DMA Transfers</li>
* <li>Left/Right Gain Amplifier Configuration. The gain is always unity</li>
* <li>CYHAL_PDM_PCM_RX_NOT_EMPTY and CYHAL_PDM_PCM_RX_UNDERFLOW events</li>
* </ul>
*
* Limitations:
* The PDM-PCM read using cyhal_pdm_pcm_read() can only be performed with length of 128.
*
* \} group_hal_impl_pdmpcm
*/


#if (CYHAL_DRIVER_AVAILABLE_PDMPCM)

#include "cy_pdm_pcm.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define _CYHAL_PDM_PCM_SAMPLE_RATE_2MHZ         (2048000UL)
#define _CYHAL_PDM_PCM_SAMPLE_RATE_1MHZ         (1024000UL)
#define _CYHAL_PDM_PCM_SAMPLE_RATE_512KHZ       (512000UL)

#define _CYHAL_PDM_PCM_DECIMATION_RATE_32       (32UL)
#define _CYHAL_PDM_PCM_DECIMATION_RATE_64       (64UL)
#define _CYHAL_PDM_PCM_DECIMATION_RATE_128      (128UL)
#define _CYHAL_PDM_PCM_DECIMATION_RATE_256      (256UL)

#define _CYHAL_PDM_PCM_SAMPLE_RATE_UNDEFINED    ((cy_en_pdm_pcm_sample_rate_t)0xFF)
#define _CYHAL_PDM_PCM_STABILIZE_TIME_MS        (25u)
#define _CYHAL_PDM_PCM_MILLISECONDS             (1000u)

#define _CYHAL_PDM_PCM_MAX_FIFO_LEVEL           (0xFFUL)
#define _CYHAL_PDM_PCM_HALF_FIFO_LEVEL          ((uint32_t)((_CYHAL_PDM_PCM_MAX_FIFO_LEVEL) >> 1u))
#define _CYHAL_PDM_PCM_NOT_EMPTY_FIFO_LEVEL     (0UL)

static cyhal_pdm_pcm_t *_cyhal_pdm_pcm_config_struct;
static bool _cyhal_amic_ready = false;

/*******************************************************************************
*       Callback Interrupt Service Routine
*******************************************************************************/

void _cyhal_pdm_pcm_adc_cb (void *handler_arg, cyhal_adc_event_t event)
{
    CY_UNUSED_PARAMETER(handler_arg);
    CY_UNUSED_PARAMETER(event);
    // ADCMic is ready so allow enabling the PDM-PCM.
	// The event may occur before PDMPCM init, so keep it as a global,
    // and then assign it to the pdm_pcm object later when checking the status.
	_cyhal_amic_ready = true;
}

void _cyhal_pdm_pcm_cb(uint8_t *pFifoCntx)
{
    /* The interrupt triggers when either of the ping-pong buffers are filled.
    * A read empties the current FIFO and sets up the interrupt for the next iteration.
    * If two interrupts occur in succession and no read is performed then an overflow event has occurred.
    * All FIFO reads must happen in this callback context as the FIFO needs to be frozen in the ISR.
    */

    cyhal_pdm_pcm_event_callback_t callback = (cyhal_pdm_pcm_event_callback_t) _cyhal_pdm_pcm_config_struct->callback_data.callback;
    uint8_t length = _CYHAL_PDM_PCM_HALF_FIFO_LEVEL + 1;
    bool ov_status = false;

    if ((_cyhal_pdm_pcm_config_struct->fifo_context != NULL)
        && (_cyhal_pdm_pcm_config_struct->events & CYHAL_PDM_PCM_RX_OVERFLOW) && (callback != NULL))
    {
        ov_status = true;
        (callback)(_cyhal_pdm_pcm_config_struct->callback_data.callback_arg, CYHAL_PDM_PCM_RX_OVERFLOW);
    }

    // Update the context for the ping-pong buffer read.
    // This is done regardless of the overflow status as the latest context is the only valid one.
    _cyhal_pdm_pcm_config_struct->fifo_context = pFifoCntx;

    if (_cyhal_pdm_pcm_config_struct->stabilization_cycles > 0UL)
    {
        // The first 25ms of data is not reliable and must be discarded.
        // A read is performed instead of FIFO clearing, as that allows triggering of interrupts.
        _cyhal_pdm_pcm_config_struct->stabilization_cycles--;
        uint32_t stabilization_data[_CYHAL_PDM_PCM_HALF_FIFO_LEVEL + 1];
        (void) Cy_PDM_PCM_ReadFifoAll(&stabilization_data[0], &length, _cyhal_pdm_pcm_config_struct->fifo_context);
        _cyhal_pdm_pcm_config_struct->fifo_context = NULL;
    }
    else
    {
        if (_cyhal_pdm_pcm_config_struct->async_data != NULL)
        {
            (void) Cy_PDM_PCM_ReadFifoAll((uint32_t *)_cyhal_pdm_pcm_config_struct->async_data, &length, _cyhal_pdm_pcm_config_struct->fifo_context);
            _cyhal_pdm_pcm_config_struct->fifo_context = NULL;
            _cyhal_pdm_pcm_config_struct->async_data = NULL;
            if ((_cyhal_pdm_pcm_config_struct->events & CYHAL_PDM_PCM_ASYNC_COMPLETE) && (callback != NULL))
            {
                (callback)(_cyhal_pdm_pcm_config_struct->callback_data.callback_arg, CYHAL_PDM_PCM_ASYNC_COMPLETE);
            }
        }

        // There are two "FIFOs" in the PDM-PCM HW, which act like ping-pong buffers.
        // We treat the interrupt for one buffer full as a "half-full" event.
        if (!ov_status && (_cyhal_pdm_pcm_config_struct->events & CYHAL_PDM_PCM_RX_HALF_FULL) && (callback != NULL))
        {
            (callback)(_cyhal_pdm_pcm_config_struct->callback_data.callback_arg, CYHAL_PDM_PCM_RX_HALF_FULL);
        }
    }
}


/*******************************************************************************
*       Deep Sleep Callback Service Routine
*******************************************************************************/

static bool _cyhal_pdm_pcm_syspm_callback(cyhal_syspm_callback_state_t state, cyhal_syspm_callback_mode_t mode, void *callback_arg)
{
    CY_UNUSED_PARAMETER(callback_arg);
    bool allow = true;

    if (state == CYHAL_SYSPM_CB_CPU_DEEPSLEEP)
    {
        switch (mode)
        {
            case CYHAL_SYSPM_CHECK_READY:
            {
                if (!cyhal_pdm_pcm_is_pending(_cyhal_pdm_pcm_config_struct))
                {
                    _cyhal_pdm_pcm_config_struct->pm_transition_pending = true;
                    allow = true;
                }
                else
                {
                    _cyhal_pdm_pcm_config_struct->pm_transition_pending = false;
                    allow = false;
                }
                break;
            }

            case CYHAL_SYSPM_BEFORE_TRANSITION:
            case CYHAL_SYSPM_AFTER_DS_WFI_TRANSITION:
            {
                break;
            }

            case CYHAL_SYSPM_AFTER_TRANSITION:
            case CYHAL_SYSPM_CHECK_FAIL:
            {
                _cyhal_pdm_pcm_config_struct->pm_transition_pending = false;
                allow = false;
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

cy_rslt_t cyhal_pdm_pcm_init(cyhal_pdm_pcm_t *obj, cyhal_gpio_t pin_data, cyhal_gpio_t pin_clk,
                const cyhal_clock_t *clk_source, const cyhal_pdm_pcm_cfg_t *cfg)
{
    CY_ASSERT(NULL != obj);
    CY_UNUSED_PARAMETER(clk_source);
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_en_pdm_pcm_sample_rate_t sample_rate;
    const cyhal_resource_pin_mapping_t* clk_map;
    const cyhal_resource_pin_mapping_t* data_map;

    memset(obj, 0, sizeof(cyhal_pdm_pcm_t));
    obj->resource.type = CYHAL_RSC_INVALID;
    obj->pin_clk = NC;
    obj->pin_data = NC;
    obj->source = CY_PDM_PCM_DMIC;
    obj->is_mic_ready = true;
    obj->is_ntd_ready = true;

    if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_2MHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_256))
    {
        sample_rate = CY_PDM_PCM_2M_8K;
    }
    else if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_2MHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_128))
    {
        sample_rate = CY_PDM_PCM_2M_16K;
    }
    else if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_1MHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_128))
    {
        sample_rate = CY_PDM_PCM_1M_8K;
    }
    else if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_1MHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_64))
    {
        sample_rate = CY_PDM_PCM_1M_16K;
    }
    else if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_512KHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_64))
    {
        sample_rate = CY_PDM_PCM_512K_8K;
    }
    else if ((cfg->sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_512KHZ) && (cfg->decimation_rate == _CYHAL_PDM_PCM_DECIMATION_RATE_32))
    {
        sample_rate = CY_PDM_PCM_512K_16K;
    }
    else
    {
        sample_rate = _CYHAL_PDM_PCM_SAMPLE_RATE_UNDEFINED;
    }
    
    if ((sample_rate == _CYHAL_PDM_PCM_SAMPLE_RATE_UNDEFINED) || (cfg->mode == CYHAL_PDM_PCM_MODE_STEREO) 
        || (cfg->word_length != 16) || (cfg->left_gain != 0) || (cfg->right_gain != 0))
    {
        result = CYHAL_PDM_PCM_RSLT_ERR_INVALID_CONFIG_PARAM;
    }
    else
    {
        obj->stabilization_cycles = ((cfg->sample_rate * _CYHAL_PDM_PCM_STABILIZE_TIME_MS)
                / (_CYHAL_PDM_PCM_HALF_FIFO_LEVEL * cfg->decimation_rate * _CYHAL_PDM_PCM_MILLISECONDS)) + 1;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        clk_map = _CYHAL_UTILS_GET_RESOURCE(pin_clk, cyhal_pin_map_dmic_ck);
        data_map = _CYHAL_UTILS_GET_RESOURCE(pin_data, cyhal_pin_map_dmic_dq);

        if ((clk_map == NULL) || (data_map == NULL))
        {
            // DMIC allocation failed. Try AMIC.
            data_map = _CYHAL_UTILS_GET_RESOURCE(pin_data, cyhal_pin_map_mic_p);
            if (data_map != NULL)
            {
                obj->source = CY_PDM_PCM_AMIC;
            }
            else
            {
                result = CYHAL_PDM_PCM_RSLT_ERR_INVALID_PIN;
            }
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        // Regardless of whether an AMIC or DMIC is used, there's only one PDM-PCM.
        cyhal_resource_inst_t rsc = { CYHAL_RSC_PDMPCM, data_map->block_num, data_map->channel_num };
        result = cyhal_hwmgr_reserve(&rsc);
        if (result == CY_RSLT_SUCCESS)
        {
            _CYHAL_UTILS_ASSIGN_RESOURCE(obj->resource, rsc.type, data_map);
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        if (obj->source == CY_PDM_PCM_AMIC)
        {
            // Set up Comp and ADC for AMIC operation
            obj->is_ntd_ready = false; // Set to true when Comp is ready
            obj->is_mic_ready = false; // Set to true when ADC is ready

            cyhal_comp_config_t comp_cfg = {.power = CYHAL_POWER_LEVEL_DEFAULT, .hysteresis = true};
            result = cyhal_comp_init(&obj->comp_obj, pin_data, NC, NC, &comp_cfg); // Set lpcomp0 to NTD mode

            if (result == CY_RSLT_SUCCESS)
            {
                _cyhal_utils_release_if_used(&(obj->comp_obj.pin_vin_p)); // Free up the DATA pin so that it can be reserved by adc
                cyhal_adc_channel_t mic_ch;
                cyhal_adc_channel_config_t mic_ch_cfg = {.enabled = true, .enable_averaging = false, .min_acquisition_ns = 200};
                result = cyhal_adc_init(&obj->adc_obj, pin_data, NULL); // Set ADC to Mic mode

                if (result == CY_RSLT_SUCCESS)
                {
                    result = cyhal_adc_channel_init_diff(&mic_ch, &obj->adc_obj, pin_data, CYHAL_ADC_VNEG, &mic_ch_cfg);
                    if (result == CY_RSLT_SUCCESS)
                    {
                        _cyhal_utils_release_if_used(&(mic_ch.vplus)); // Free up the DATA pin so that it can be reserved by pdm-pcm
                        cyhal_adc_register_callback(&obj->adc_obj, _cyhal_pdm_pcm_adc_cb, NULL);
                        cyhal_adc_enable_event(&obj->adc_obj, CYHAL_ADC_EOS, 0, true);
                    }
                }
            }
        }
        else
        {
            // reserve the clk pin (DMIC only)
            result = _cyhal_utils_reserve_and_connect(clk_map, CYHAL_PIN_MAP_DRIVE_MODE_PDM_PCM_CLK);
            if (result == CY_RSLT_SUCCESS)
            {
                obj->pin_clk = pin_clk;
            }
        }
    }

    // reserve the data pin
    if (result == CY_RSLT_SUCCESS)
    {
        result = _cyhal_utils_reserve_and_connect(data_map, CYHAL_PIN_MAP_DRIVE_MODE_PDM_PCM_DATA);
        if (result == CY_RSLT_SUCCESS)
        {
            obj->pin_data = pin_data;
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        obj->pm_transition_pending = false;
        obj->pm_callback_data.callback = &_cyhal_pdm_pcm_syspm_callback;
        obj->pm_callback_data.states = (cyhal_syspm_callback_state_t)(CYHAL_SYSPM_CB_CPU_DEEPSLEEP);
        obj->pm_callback_data.next = NULL;
        obj->pm_callback_data.args = obj;
        obj->pm_callback_data.ignore_modes = (cyhal_syspm_callback_mode_t)0;
        _cyhal_syspm_register_peripheral_callback(&(obj->pm_callback_data));

        obj->base = PDMPCM0;
        cy_stc_pdm_pcm_inf_ctrl_sel_t inf_ctrl_sel = {1, 2, 0};
        cy_stc_pdm_pcm_config_t config = {
            .mic_source = obj->source,
            .pdm_pcm_sample_rate = sample_rate,
            .pdm_pcm_inf = &inf_ctrl_sel, // This should be revised (or set to NULL when PDL is updated)
            .bq_fir_coeffs = NULL
        };

        cy_en_pdm_pcm_status_t status = Cy_PDM_PCM_Init(obj->base, &config);
        if (status == CY_PDM_PCM_SUCCESS)
        {
            status = Cy_PDM_PCM_RegisterCallback(_cyhal_pdm_pcm_cb);
        }
        if (status == CY_PDM_PCM_SUCCESS)
        {
            status = Cy_PDM_PCM_SetFifoLevel(_CYHAL_PDM_PCM_HALF_FIFO_LEVEL);
        }
        if (status == CY_PDM_PCM_SUCCESS)
        {
            _cyhal_pdm_pcm_config_struct = obj;
        }

        result = (status == CY_PDM_PCM_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_PDM_PCM_RSLT_ERR_INVALID_CONFIG_PARAM;
    }
    
    if (result != CY_RSLT_SUCCESS)
    {
        cyhal_pdm_pcm_free(obj);
    }

    return result;
}

cy_rslt_t cyhal_pdm_pcm_init_cfg(cyhal_pdm_pcm_t *obj, const cyhal_pdm_pcm_configurator_t* cfg)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(cfg);
    return CYHAL_PDM_PCM_RSLT_ERR_UNSUPPORTED;
}

void cyhal_pdm_pcm_free(cyhal_pdm_pcm_t *obj)
{
    CY_ASSERT(NULL != obj);

    Cy_PDM_PCM_Disable(obj->base);
    Cy_PDM_PCM_ClearFifo();
    Cy_PDM_PCM_DeInit(obj->base);

    if (obj->resource.type != CYHAL_RSC_INVALID)
    {
        cyhal_hwmgr_free(&(obj->resource));
        obj->resource.type = CYHAL_RSC_INVALID;
    }

    if (obj->source == CY_PDM_PCM_AMIC)
    {
        _cyhal_amic_ready = false;
        if (obj->comp_obj.resource.type != CYHAL_RSC_INVALID)
            cyhal_comp_free(&obj->comp_obj);

        if (obj->adc_obj.resource.type != CYHAL_RSC_INVALID)
        {
            cyhal_adc_free(&obj->adc_obj);
            cyhal_adc_channel_free(obj->adc_obj.channel_config[0]);
        }
    }

    if (obj->pm_callback_data.callback != NULL)
        _cyhal_syspm_unregister_peripheral_callback(&(obj->pm_callback_data));

    _cyhal_utils_release_if_used(&(obj->pin_clk));
    _cyhal_utils_release_if_used(&(obj->pin_data));
}

void _cyhal_pdm_pcm_check_ready(cyhal_pdm_pcm_t *obj)
{
    // NTD settling check needs to be performed only once at initialization
    if (obj->source == CY_PDM_PCM_AMIC)
    {
        if (!obj->is_ntd_ready)
        {
            bool comp_read = cyhal_comp_read(&obj->comp_obj);
            cyhal_system_delay_ms(1);
            comp_read = cyhal_comp_read(&obj->comp_obj);

            obj->is_ntd_ready = (!comp_read);
        }
        obj->is_mic_ready = _cyhal_amic_ready;
    }
}

cy_rslt_t cyhal_pdm_pcm_start(cyhal_pdm_pcm_t *obj)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result;

    if (obj->pm_transition_pending)
    {
        result = CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }
    else
    {
        _cyhal_pdm_pcm_check_ready(obj);
        cy_en_pdm_pcm_status_t status = (obj->is_mic_ready && obj->is_ntd_ready) ? Cy_PDM_PCM_Enable(obj->base) : CY_PDM_PCM_ERROR;
        obj->is_enabled = (status == CY_PDM_PCM_SUCCESS);

        result = (status == CY_PDM_PCM_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_PDM_PCM_RSLT_ERR_BUSY_OPERATION;
    }

    return result;
}

cy_rslt_t cyhal_pdm_pcm_stop(cyhal_pdm_pcm_t *obj)
{
    CY_ASSERT(NULL != obj);
    cy_en_pdm_pcm_status_t status = Cy_PDM_PCM_Disable(obj->base);
    obj->is_enabled = !(status == CY_PDM_PCM_SUCCESS);
    return (status == CY_PDM_PCM_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_PDM_PCM_RSLT_ERR_BUSY_OPERATION;
}

bool cyhal_pdm_pcm_is_enabled(cyhal_pdm_pcm_t *obj)
{
    CY_ASSERT(NULL != obj);
    return (obj->stabilization_cycles == 0UL) && obj->is_enabled;
}

cy_rslt_t cyhal_pdm_pcm_set_gain(cyhal_pdm_pcm_t *obj, int16_t gain_left, int16_t gain_right)
{
    CY_UNUSED_PARAMETER(obj);
    return ((gain_left == 0) && (gain_right == 0)) ? CY_RSLT_SUCCESS : CYHAL_PDM_PCM_RSLT_ERR_UNSUPPORTED;
}

cy_rslt_t cyhal_pdm_pcm_clear(cyhal_pdm_pcm_t *obj)
{
    CY_ASSERT(NULL != obj);
    Cy_PDM_PCM_ClearFifo();
    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_pdm_pcm_read(cyhal_pdm_pcm_t *obj, void *data, size_t *length)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = cyhal_pdm_pcm_read_async(obj, data, *length);
    
    if (result == CY_RSLT_SUCCESS)
    {
        int retry = 20; /* Conversion should take less than 20ms */
        while (cyhal_pdm_pcm_is_pending(obj) && retry > 0)
        {
            cyhal_system_delay_ms(1u);
            --retry;
        }

        if (retry == 0)
        {
            cyhal_pdm_pcm_abort_async(obj);
            *length = 0;
        }
    }
    else
    {
        *length = 0;
    }

    return result;
}

cy_rslt_t cyhal_pdm_pcm_read_async(cyhal_pdm_pcm_t *obj, void *data, size_t length)
{
    CY_ASSERT(NULL != obj);
    cy_rslt_t result = CYHAL_PDM_PCM_RSLT_ERR_BAD_PARAM;

    if (obj->pm_transition_pending)
    {
        result = CYHAL_SYSPM_RSLT_ERR_PM_PENDING;
    }
    else
    {
        if ((length == _CYHAL_PDM_PCM_HALF_FIFO_LEVEL + 1) && (data != NULL))
        {
            if ((obj->async_data != NULL) || !(obj->is_enabled))
            {
                result = CYHAL_PDM_PCM_RSLT_ERR_ASYNC_IN_PROGRESS;
            }
            else
            {
                obj->async_data = data;
                result = CY_RSLT_SUCCESS;
            }
        }
    }

    return result;
}

bool cyhal_pdm_pcm_is_pending(cyhal_pdm_pcm_t *obj)
{
    return (obj->async_data != NULL);
}

cy_rslt_t cyhal_pdm_pcm_abort_async(cyhal_pdm_pcm_t *obj)
{
    obj->async_data = NULL;
    return CY_RSLT_SUCCESS;
}

void cyhal_pdm_pcm_register_callback(cyhal_pdm_pcm_t *obj, cyhal_pdm_pcm_event_callback_t callback, void *callback_arg)
{
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    obj->callback_data.callback = (cy_israddress) callback;
    obj->callback_data.callback_arg = callback_arg;
    cyhal_system_critical_section_exit(savedIntrStatus);
}

void cyhal_pdm_pcm_enable_event(cyhal_pdm_pcm_t *obj, cyhal_pdm_pcm_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);
    obj->events = (enable) ? (obj->events | event) : (obj->events & ~event);
}

cy_rslt_t cyhal_pdm_pcm_set_async_mode(cyhal_pdm_pcm_t *obj, cyhal_async_mode_t mode, uint8_t dma_priority)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(dma_priority);
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (mode != CYHAL_ASYNC_SW)
    {
        result = CYHAL_PDM_PCM_RSLT_ERR_UNSUPPORTED;
    }

    return result;
}

#if defined(__cplusplus)
}
#endif

#endif /* (CYHAL_DRIVER_AVAILABLE_PDMPCM) */
