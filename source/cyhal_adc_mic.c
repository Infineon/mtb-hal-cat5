/***************************************************************************/ /**
* \file cyhal_adc_mic.c
*
* \brief
* Provides a high level interface for interacting with the Infineon Analog/Digital
* converter. This interface abstracts out the chip specific details. If any chip
* specific functionality is necessary, or performance is critical the low level
* functions can be used directly.
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

/**
 * \addtogroup group_hal_impl_adcmic ADC (Analog Digital Converter)
 * \ingroup group_hal_impl
 * \{
 * \section cyhal_adcmic_impl_features Features
 * The CAT5 ADC supports the following features:
 * * Resolution: 15 bit
 * * Input range from 0V to 1V
 * * Sample rate: Fixed 4096 ksps
 * * Minimum acquisition time: Up to 244 ns
 * * SW-based async transfer only (DMA is not supported)
 * * VREF: @ref CYHAL_ADC_REF_INTERNAL (0.5V) only
 * * Single ended vneg: @ref CYHAL_ADC_VNEG_VSSA
 * * Programmable gains of 8/8, 8/7, 8/4, 8/1
 * * DC measurement through 8 pins
 * * ADC Mic audio through MIC_P pin to the PDM-PCM
 *
 * The following functions are not supported:
 * * Differential channels
 * * Continuous scanning
 * * Averaging. In @ref cyhal_adc_config_t, average count must be 1 and average_mode_flags must be 0.
 *   In @ref cyhal_adc_channel_config_t, enable_averaging must be false.
 * * External vref and bypass pins
 * 
 * There are 8 power gain amplifier (PGA) levels in the hardware. These are truncated to the 3 levels
 * suported by the HAL. The default power is set to maximum gain. To control the gain, use @ref cyhal_adc_set_power.
 * \} group_hal_impl_adcmic
 */

#include "cyhal_adc.h"
#include "cyhal_clock.h"
#include "cyhal_gpio.h"
#include "cyhal_hwmgr.h"
#include "cyhal_system.h"
#include "cyhal_analog_common.h"
#include <string.h>

#if (CYHAL_DRIVER_AVAILABLE_ADC)
#include "cy_adccomp.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define _CYHAL_ADCMIC_DEFAULT_READ              (0xADC0)
#define _CYHAL_ADCMIC_DC_CALIBRATION_GAIN       (0x8000) /* Amount of raw counts per 1 volt of input voltage */
#define _CYHAL_ADCMIC_NUM_CHANNELS(obj)         (sizeof(obj->channel_config) / sizeof(obj->channel_config[0]))

static const uint8_t  _CYHAL_ADCMIC_RESOLUTION          = 15u;
static const uint32_t _CYHAL_ADCMIC_SAMPLE_RATE_HZ      = 4096000u; /* Fixed by the HW */
static const uint32_t _CYHAL_ADCMIC_ACQUISITION_TIME_NS = 244u;   /* 1 / 4096 ksps */

cyhal_adc_t* _cyhal_adcmic_config_structs[CY_IP_MXS40ADCMIC_INSTANCES];

/* Find the next enabled channel, starting from current_idx and adjusting the buffer
* along the way to account for disabled channels */
static void _cyhal_adcmic_find_next_channel(cyhal_adc_t* obj, uint8_t* current_idx)
{
    uint8_t start_idx = *current_idx;
    do
    {
        if(NULL != obj->channel_config[*current_idx])
        {
            if(obj->channel_config[*current_idx]->enabled)
            {
                break;
            }
        }
        *current_idx = (*current_idx + 1) % _CYHAL_ADCMIC_NUM_CHANNELS(obj);
    } while(*current_idx != start_idx); /* While we haven't wrapped completely around */
}

static cy_en_adccomp_adc_dc_channel_t _cyhal_adcmic_convert_channel_sel(uint8_t bit_index)
{
    static const cy_en_adccomp_adc_dc_channel_t gpio_channel[] =
    {
        CY_ADCCOMP_ADC_IN_GPIO0,
        CY_ADCCOMP_ADC_IN_GPIO1,
        CY_ADCCOMP_ADC_IN_GPIO2,
        CY_ADCCOMP_ADC_IN_GPIO3,
        CY_ADCCOMP_ADC_IN_GPIO4,
        CY_ADCCOMP_ADC_IN_GPIO5,
        CY_ADCCOMP_ADC_IN_GPIO6,
        CY_ADCCOMP_ADC_IN_GPIO7,
    };

    if (bit_index < sizeof(gpio_channel) / sizeof(gpio_channel[0]))
    {
        return gpio_channel[bit_index];
    }
    else
    {
        /* We only support GPIO channels and the above defines all of them */
        return CY_ADCCOMP_ADC_IN_OPEN;
    }
}


/*******************************************************************************
*       ADC callbacks used in _cyhal_adccomp_cb()
*******************************************************************************/

void _cyhal_adcmic_calibrate(void)
{
    cyhal_adc_t* obj = _cyhal_adcmic_config_structs[0];

    if( obj != NULL )
    {
        Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);

        if (obj->using_audio)
        {
            // Effectively should only be called by the PDM-PCM
            if(0 != (CYHAL_ADC_EOS & ((cyhal_adc_event_t)obj->user_enabled_events)))
            {
                cyhal_adc_event_callback_t callback = (cyhal_adc_event_callback_t)obj->callback_data.callback;
                if(NULL != callback)
                {
                    callback(obj->callback_data.callback_arg, CYHAL_ADC_EOS);
                }
            }
            Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);
        }
        else
        {
            // Only perform DC calibration if using DC measurement
            if (!obj->dc_calibrated)
            {
                if (!obj->dc_calibration_started)
                {
                    Cy_ADCCOMP_DisableTimer(obj->base);
                    Cy_ADCCOMP_ADC_InitiateDcCalibration(obj->base); // enable the timer
                    obj->dc_calibration_started = true;
                }
                else if (obj->dc_calibration_started)
                {
                    Cy_ADCCOMP_ADC_Start(obj->base); // start the conversion to get the calibration result
                    Cy_ADCCOMP_DisableTimer(obj->base);
                    // Disable the timer interrupt, as it is no longer needed
                    Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);
                    Cy_ADCCOMP_EnableInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
                }
            }
        }
    }
}

void _cyhal_adcmic_get_result(void)
{
    cyhal_adc_t* obj = _cyhal_adcmic_config_structs[0];

    if( obj != NULL )
    {
        Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);

        if ((!obj->dc_calibrated) && (obj->dc_calibration_started))
        {
            obj->calibOffset = Cy_ADCCOMP_GetCalibrationResult(obj->base);
            Cy_ADCCOMP_SetDcOffset(obj->base, obj->calibOffset, &obj->pdl_context);
            Cy_ADCCOMP_SetDcGain(_CYHAL_ADCMIC_DC_CALIBRATION_GAIN, &obj->pdl_context);
            Cy_ADCCOMP_ADC_EndDcCalibration(obj->base);
            Cy_ADCCOMP_ADC_Stop(obj->base);
            Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
            obj->dc_calibrated = true;
            obj->dc_calibration_started = false;
        }
        else
        {
            if (!obj->using_audio)
            {
                obj->conversion_complete = true; // Used only for non-async reads

                Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);

                if(0 != (CYHAL_ADC_EOS & ((cyhal_adc_event_t)obj->user_enabled_events)))
                {
                    cyhal_adc_event_callback_t callback = (cyhal_adc_event_callback_t)obj->callback_data.callback;
                    if(NULL != callback)
                    {
                        callback(obj->callback_data.callback_arg, CYHAL_ADC_EOS);
                    }
                }

                if(obj->async_scans_remaining > 0)
                {
                    int16_t dc_data = Cy_ADCCOMP_GetDcResult(obj->base);

                    if (obj->async_transfer_in_uv)
                    {
                        *(obj->async_buff_next) = Cy_ADCCOMP_CountsTo_uVolts(dc_data, &obj->pdl_context);
                    }
                    else
                    {
                        *(obj->async_buff_next) = (int32_t)dc_data;
                    }

                    Cy_ADCCOMP_ADC_Stop(obj->base);

                    obj->async_buff_next++;
                    uint8_t old_channel = obj->current_channel_index;
                    /* Look for the next available channel. Wrap around if needed. */
                    obj->current_channel_index = (obj->current_channel_index + 1) % _CYHAL_ADCMIC_NUM_CHANNELS(obj);
                    _cyhal_adcmic_find_next_channel(obj, &(obj->current_channel_index));
                    if (old_channel >= obj->current_channel_index)
                    {
                        --(obj->async_scans_remaining);
                    }

                    if(obj->async_scans_remaining != 0)
                    {
                        Cy_ADCCOMP_ADC_SelectDcChannel(obj->base, obj->channel_config[obj->current_channel_index]->channel_sel);
                        Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
                        Cy_ADCCOMP_ADC_Start(obj->base); // Start the next scan
                        Cy_ADCCOMP_EnableInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
                    }
                    else
                    {
                        /* We're done, notify the user if they asked us to */
                        obj->async_buff_next = NULL;
                        if(0 != (CYHAL_ADC_ASYNC_READ_COMPLETE & ((cyhal_adc_event_t)obj->user_enabled_events)))
                        {
                            cyhal_adc_event_callback_t callback = (cyhal_adc_event_callback_t)obj->callback_data.callback;
                            if(NULL != callback)
                            {
                                callback(obj->callback_data.callback_arg, CYHAL_ADC_ASYNC_READ_COMPLETE);
                            }
                        }
                    }
                }
            }
        }
    }
}


/*******************************************************************************
*       ADC HAL Functions
*******************************************************************************/

cy_rslt_t _cyhal_adc_config_hw(cyhal_adc_t *obj, const cyhal_adc_configurator_t* cfg, cyhal_gpio_t pin, bool owned_by_configurator)
{
    CY_ASSERT(NULL != obj);

    cy_en_adccomp_status_t pdl_result;
    cy_rslt_t result = CY_RSLT_SUCCESS;
    memset(obj, 0, sizeof(cyhal_adc_t));
    obj->resource.type = CYHAL_RSC_INVALID;

    obj->owned_by_configurator = owned_by_configurator;
    cy_en_adccomp_adc_mode_t adcMode = CY_ADCCOMP_ADC_DC;

    if(NULL == cfg->resource && NC != pin)
    {
        if (pin == cyhal_pin_map_mic_p[0].pin)
        {
            // ADC Mic audio
            cyhal_resource_inst_t inst = { CYHAL_RSC_ADCMIC, cyhal_pin_map_mic_p[0].block_num, 0 };
            if (CY_RSLT_SUCCESS == cyhal_hwmgr_reserve(&inst))
            {
                obj->resource = inst;
                adcMode = CY_ADCCOMP_ADC_MIC;
                obj->using_audio = true;
            }
        }
        else
        {
            for (uint32_t i = 0; i < sizeof(cyhal_pin_map_adcmic_gpio_adc_in)/sizeof(cyhal_resource_pin_mapping_t); i++)
            {
                if (pin == cyhal_pin_map_adcmic_gpio_adc_in[i].pin)
                {
                    // ADC DC measurement
                    cyhal_resource_inst_t inst = { CYHAL_RSC_ADCMIC, cyhal_pin_map_adcmic_gpio_adc_in[i].block_num, 0 };
                    if (CY_RSLT_SUCCESS == cyhal_hwmgr_reserve(&inst))
                    {
                        obj->resource = inst;
                        break;
                    }
                }
            }
        }

        if (obj->resource.type == CYHAL_RSC_INVALID)
        {
            result = CYHAL_ADC_RSLT_BAD_ARGUMENT;
        }
    }
    else if(NULL != cfg->resource)
    {
        obj->resource = *cfg->resource;
    }
    else
    {
        result = CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        obj->base = _cyhal_adccomp_base[obj->resource.block_num];
        pdl_result = Cy_ADCCOMP_ADC_Init(obj->base, adcMode, (cy_stc_adccomp_adc_config_t *)(cfg->config));
        result = (pdl_result == CY_ADCCOMP_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }

    if (result == CY_RSLT_SUCCESS)
    {
        /* Shared callback between ADC and comparators */
        result = _cyhal_adccomp_register_cb();
    }

    if (result == CY_RSLT_SUCCESS)
    {
        /* Store the ADC object before enabling the interrupts */
        _cyhal_adcmic_config_structs[obj->resource.block_num] = obj;
        if (!obj->using_audio)
        {
            /* No need to explicitly start conversion as that happens automatically when we enable.
            * This will also trigger the interrupt for DC calibration. After that, DC measurement will be available.
            * The Mic mode on the other hand needs to enable when the callback is set up as it needs to signal
            * the PDM-PCM block on the ADC ready status.
            */
            Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY | CY_ADCCOMP_INTR_CIC);
            Cy_ADCCOMP_EnableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);
            pdl_result = Cy_ADCCOMP_ADC_Enable(obj->base);
            result = (pdl_result == CY_ADCCOMP_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_ADC_RSLT_BAD_ARGUMENT;
        }
    }

    if (result != CY_RSLT_SUCCESS)
    {
        cyhal_adc_free(obj);
    }
    return result;
}

cy_rslt_t cyhal_adc_init(cyhal_adc_t *obj, cyhal_gpio_t pin, const cyhal_clock_t *clk)
{
    cyhal_adc_configurator_t config;
    config.resource = NULL;
    cy_stc_adccomp_adc_dc_config_t dcConfig = {.channel = CY_ADCCOMP_ADC_IN_OPEN, .context = &obj->pdl_context};
    cy_stc_adccomp_adc_mic_config_t micConfig = {.adcMicPgaGain = CY_ADCCOMP_ADC_PGA_GAIN_CTRL_0};
    cy_stc_adccomp_adc_config_t pdl_config =
    {
        .adcClkInPdmOut = CY_ADCCOMP_ADC_CLK_IN_4MHZ_PDM_OUT_2MHZ, /* Default and recommended by PDL/HW */
        .dcConfig    = &dcConfig,
        .micConfig   = &micConfig
    };

    config.config = &pdl_config;
    config.clock = clk;
    config.num_channels = 0u;

    cy_rslt_t result = _cyhal_adc_config_hw(obj, &config, pin, false);
    return result;
}

cy_rslt_t cyhal_adc_init_cfg(cyhal_adc_t *adc, cyhal_adc_channel_t** channels, uint8_t* num_channels,
                                const cyhal_adc_configurator_t *cfg)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    if(*num_channels < cfg->num_channels)
    {
        result = CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }

    if(CY_RSLT_SUCCESS == result)
    {
        *num_channels = cfg->num_channels;
        result = _cyhal_adc_config_hw(adc, cfg, NC, true);
    }

    if(CY_RSLT_SUCCESS == result)
    {
        /* config_hw will have initialized the channels in the ADC HW and the configurator will
        * have set up the routing, but we need to initialize the channel structs */
        for(int i = 0; i < *num_channels; ++i)
        {
            cyhal_adc_channel_t* channel = channels[i];
            memset(channel, 0, sizeof(cyhal_adc_channel_t));
            channel->adc = adc;
            channel->channel_idx = i;
            /* Nothing in this flow needs to know what the pins are - and the inputs aren't even
            * necesssarily pins. The configurator takes care of resource reservation and routing for us */
            channel->vplus = NC;
            channel->enabled = true;
        }
    }
    return result;
}

void cyhal_adc_free(cyhal_adc_t *obj)
{
    CY_ASSERT(NULL != obj);
    if (CYHAL_RSC_INVALID != obj->resource.type)
    {
        if (NULL != obj->base)
        {
            Cy_ADCCOMP_DisableTimer(obj->base);
            Cy_ADCCOMP_ADC_Stop(obj->base);
            Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY | CY_ADCCOMP_INTR_CIC);
            Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY | CY_ADCCOMP_INTR_CIC);
            Cy_ADCCOMP_ADC_Disable(obj->base);
            obj->base = NULL;
        }

        _cyhal_adcmic_config_structs[obj->resource.block_num] = NULL;

        if(false == obj->owned_by_configurator)
        {
            cyhal_hwmgr_free(&obj->resource);
        }
    }
}

cy_rslt_t cyhal_adc_configure(cyhal_adc_t *obj, const cyhal_adc_config_t *config)
{
    /* The hardware is very limited, so all we can do is check that the config matches what we support */
    CY_UNUSED_PARAMETER(obj);
    if((false != config->continuous_scanning)
        || (_CYHAL_ADCMIC_RESOLUTION != config->resolution)
        || (1u != config->average_count)
        || (0u != config->average_mode_flags)
        || (0u != config->ext_vref_mv)
        || (CYHAL_ADC_VNEG_VSSA != config->vneg)
        || (CYHAL_ADC_REF_INTERNAL != config->vref)
        || (NC != config->ext_vref)
        || (false != config->is_bypassed)
        || (NC != config->bypass_pin))
    {
        return CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_adc_set_power(cyhal_adc_t *obj, cyhal_power_level_t power)
{
    // The ADC doesn't have selectable power levels.
    if(CYHAL_POWER_LEVEL_OFF == power)
    {
        return (cy_rslt_t)Cy_ADCCOMP_ADC_Disable(obj->base);
    }
    else
    {
        return (cy_rslt_t)Cy_ADCCOMP_ADC_Enable(obj->base);
    }
}

cy_rslt_t cyhal_adc_set_sample_rate(cyhal_adc_t* obj, uint32_t desired_sample_rate_hz, uint32_t* achieved_sample_rate_hz)
{
    /* Only one sample rate supported, so all we can do is validate */
    CY_UNUSED_PARAMETER(obj);
    *achieved_sample_rate_hz = _CYHAL_ADCMIC_SAMPLE_RATE_HZ;
    return (_CYHAL_ADCMIC_SAMPLE_RATE_HZ == desired_sample_rate_hz) ? CY_RSLT_SUCCESS : CYHAL_ADC_RSLT_BAD_ARGUMENT;
}

cy_rslt_t cyhal_adc_channel_init_diff(cyhal_adc_channel_t *obj, cyhal_adc_t* adc, cyhal_gpio_t vplus, cyhal_gpio_t vminus, const cyhal_adc_channel_config_t* cfg)
{
    CY_ASSERT(obj != NULL);
    CY_ASSERT(adc != NULL);

    cy_rslt_t result = CY_RSLT_SUCCESS;
    memset(obj, 0, sizeof(cyhal_adc_channel_t));
    obj->vplus = NC;

    /* Only single-ended supported on this HW */
    if (CYHAL_ADC_VNEG != vminus)
        result = CYHAL_ADC_RSLT_BAD_ARGUMENT;

    if(CY_RSLT_SUCCESS == result)
    {
        // Check for invalid pin or pin belonging to a different ADC
        const cyhal_resource_pin_mapping_t *vplus_map = _cyhal_utils_get_resource(vplus,
            cyhal_pin_map_adcmic_gpio_adc_in,
            sizeof(cyhal_pin_map_adcmic_gpio_adc_in)/sizeof(cyhal_pin_map_adcmic_gpio_adc_in[0]),
            &(adc->resource), true);

        if (NULL == vplus_map)
        {
            // Check MIC_P
            vplus_map = _cyhal_utils_get_resource(vplus, cyhal_pin_map_mic_p,
            sizeof(cyhal_pin_map_mic_p)/sizeof(cyhal_pin_map_mic_p[0]),
            &(adc->resource), true);
        }
        
        if (NULL == vplus_map)
        {
            result = CYHAL_ADC_RSLT_BAD_ARGUMENT;
        }
        else
        {
            uint8_t bit_index = vplus_map->channel_num;
            obj->channel_sel = _cyhal_adcmic_convert_channel_sel(bit_index);
            result = _cyhal_utils_reserve_and_connect(vplus_map, CYHAL_PIN_MAP_DRIVE_MODE_ADCMIC_GPIO_ADC_IN);
        }
    }

    uint8_t chosen_channel = 0;
    if (CY_RSLT_SUCCESS == result)
    {
        obj->vplus = vplus;
        // Find the first available channel
        for(chosen_channel = 0; chosen_channel < _CYHAL_ADCMIC_NUM_CHANNELS(obj->adc); ++chosen_channel)
        {
            if(NULL == adc->channel_config[chosen_channel])
            {
                break;
            }
        }
        if (chosen_channel >= _CYHAL_ADCMIC_NUM_CHANNELS(obj->adc))
            result = CYHAL_ADC_RSLT_NO_CHANNELS;
    }

    if(CY_RSLT_SUCCESS == result)
    {
        // Don't set the ADC until here so that free knows whether we have allocated
        // the channel on the parent ADC instance (and therefore doesn't try to free it if
        // something fails further up)
        obj->adc = adc;
        obj->channel_idx = chosen_channel;
        obj->adc->channel_config[chosen_channel] = obj;
    }

    if(CY_RSLT_SUCCESS == result)
    {
        result = cyhal_adc_channel_configure(obj, cfg);
    }

    if(CY_RSLT_SUCCESS != result)
    {
        cyhal_adc_channel_free(obj);
    }

    return result;
}

cy_rslt_t cyhal_adc_channel_configure(cyhal_adc_channel_t *obj, const cyhal_adc_channel_config_t *config)
{
    CY_ASSERT(NULL != obj);

    cy_rslt_t result = CY_RSLT_SUCCESS;
    if((config->min_acquisition_ns > _CYHAL_ADCMIC_ACQUISITION_TIME_NS) || config->enable_averaging)
    {
        result = CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }
    if(CY_RSLT_SUCCESS == result)
    {
        obj->enabled = config->enabled;
    }

    return result;
}

void cyhal_adc_channel_free(cyhal_adc_channel_t *obj)
{
    CY_ASSERT(NULL != obj);

    // If obj->adc exists, check whether it was configured via configurator (as we are not allowed to free
    //  pins, that were set up via configurator) and if not, release pin. Also, release pin if obj->adc does not exist.
    if(((obj->adc != NULL) && (false == obj->adc->owned_by_configurator)) || (obj->adc == NULL))
    {
        _cyhal_utils_release_if_used(&(obj->vplus));
    }

    if(obj->adc != NULL)
    {
        // Disable the channel. No per-channel configuration was statically set, so nothing to unconfigure
        obj->adc->channel_config[obj->channel_idx] = NULL;
        obj->adc = NULL;
    }
}

static int16_t _cyhal_adc_read_raw(const cyhal_adc_channel_t *obj)
{
    int16_t data = _CYHAL_ADCMIC_DEFAULT_READ; // Default known value for cases when the read times out

    if ((obj->adc->dc_calibrated) && (obj->adc->async_scans_remaining == 0))
    {
        uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
        Cy_ADCCOMP_ClearInterrupt(obj->adc->base, CY_ADCCOMP_INTR_CIC);
        Cy_ADCCOMP_ADC_SelectDcChannel(obj->adc->base, obj->channel_sel);
        Cy_ADCCOMP_ADC_Start(obj->adc->base);
        obj->adc->conversion_complete = false; // All acquisitions are interrupt-driven and must wait for the event to occur.
        Cy_ADCCOMP_EnableInterrupt(obj->adc->base, CY_ADCCOMP_INTR_CIC);
        cyhal_system_critical_section_exit(savedIntrStatus);

        int retry = 1000;

        while(!obj->adc->conversion_complete && retry > 0)
        {
            cyhal_system_delay_us(1u); /* Conversion should take ~2 us */
            --retry;
        }

        // Get result and take offset into account
        data = (retry > 0) ? Cy_ADCCOMP_GetDcResult(obj->adc->base) : data;
        Cy_ADCCOMP_ADC_Stop(obj->adc->base);
    }

    return data;
}

uint16_t cyhal_adc_read_u16(const cyhal_adc_channel_t *obj)
{
    CY_ASSERT(NULL != obj);
    return (uint16_t)_cyhal_adc_read_raw(obj);;
}

int32_t cyhal_adc_read(const cyhal_adc_channel_t *obj)
{
    CY_ASSERT(NULL != obj);
    return (int32_t)(_cyhal_adc_read_raw(obj));;
}

int32_t cyhal_adc_read_uv(const cyhal_adc_channel_t *obj)
{
    CY_ASSERT(NULL != obj);
    return Cy_ADCCOMP_CountsTo_uVolts(_cyhal_adc_read_raw(obj), &obj->adc->pdl_context);;
}

static cy_rslt_t _cyhal_adcmic_start_async_read(cyhal_adc_t* obj, size_t num_scan, int32_t* result_list)
{
    if ((NULL != obj->async_buff_next) || (!obj->dc_calibrated))
    {
        /* Transfer already in progress */
        return CYHAL_ADC_RSLT_ERR_BUSY;
    }

    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    obj->current_channel_index = 0;
    obj->async_scans_remaining = num_scan;
    obj->async_buff_next = result_list;
    _cyhal_adcmic_find_next_channel(obj, &(obj->current_channel_index));

    if(NULL == obj->channel_config[obj->current_channel_index]
        || (false == obj->channel_config[obj->current_channel_index]->enabled))
    {
        /* No enabled channels found, we're done */
        obj->async_buff_next = NULL;
        obj->async_scans_remaining = 0;
    }
    else
    {
        Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
        Cy_ADCCOMP_ADC_SelectDcChannel(obj->base, obj->channel_config[obj->current_channel_index]->channel_sel);
        Cy_ADCCOMP_ADC_Start(obj->base);
        Cy_ADCCOMP_EnableInterrupt(obj->base, CY_ADCCOMP_INTR_CIC);
    }
    cyhal_system_critical_section_exit(savedIntrStatus);

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_adc_read_async(cyhal_adc_t* obj, size_t num_scan, int32_t* result_list)
{
    CY_ASSERT(NULL != obj);
    obj->async_transfer_in_uv = false;
    return _cyhal_adcmic_start_async_read(obj, num_scan, result_list);
}

cy_rslt_t cyhal_adc_read_async_uv(cyhal_adc_t* obj, size_t num_scan, int32_t* result_list)
{
    CY_ASSERT(NULL != obj);
    obj->async_transfer_in_uv = true;
    return _cyhal_adcmic_start_async_read(obj, num_scan, result_list);
}

cy_rslt_t cyhal_adc_set_async_mode(cyhal_adc_t *obj, cyhal_async_mode_t mode, uint8_t dma_priority)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(dma_priority);
    CY_ASSERT(NULL != obj);
    CY_ASSERT(NULL == obj->async_buff_next); /* Can't swap mode while a transfer is running */
    if(mode == CYHAL_ASYNC_DMA)
    {
        /* DMA not supported on this HW. CPU intervention is required after every sample anyway,
        * so triggering the DMA would involve more overhead than just CPU copying the 32 bits */
        return CYHAL_ADC_RSLT_BAD_ARGUMENT;
    }
    else
    {
        return CY_RSLT_SUCCESS;
    }
}

void cyhal_adc_register_callback(cyhal_adc_t *obj, cyhal_adc_event_callback_t callback, void *callback_arg)
{
    CY_ASSERT(NULL != obj);

    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    obj->callback_data.callback = (cy_israddress) callback;
    obj->callback_data.callback_arg = callback_arg;
    cyhal_system_critical_section_exit(savedIntrStatus);
}

void cyhal_adc_enable_event(cyhal_adc_t *obj, cyhal_adc_event_t event, uint8_t intr_priority, bool enable)
{
    CY_UNUSED_PARAMETER(intr_priority);

    /* Continuous scanning isn't supported (the hardware is converting continuously, but it's not automatically
    * scanning across all enabled channels). We listen for EOC internally at times but that doesn't correspond to
    * the EOS event. So there's no interrupts that we need to enable/disable here. */
    if(enable)
    {
        obj->user_enabled_events |= event;
        if (obj->using_audio)
        {
            Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY | CY_ADCCOMP_INTR_CIC);
            Cy_ADCCOMP_EnableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);
            Cy_ADCCOMP_ADC_Enable(obj->base); /* This will signal the PDM-PCM block on the ADC ready status */
        }
    }
    else
    {
        obj->user_enabled_events &= ~event;
        if (obj->using_audio)
        {
            Cy_ADCCOMP_ClearInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY | CY_ADCCOMP_INTR_CIC);
            Cy_ADCCOMP_DisableInterrupt(obj->base, CY_ADCCOMP_INTR_ADC_READY);
            Cy_ADCCOMP_ADC_Disable(obj->base); /* May not be needed but added for possible need for reset */
        }
    }
}

cy_rslt_t cyhal_adc_connect_digital(cyhal_adc_t *obj, cyhal_source_t source, cyhal_adc_input_t input)
{
    /* No trigger inputs supported on this hardware */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(input);
    return CYHAL_ADC_RSLT_ERR_UNSUPPORTED;
}

cy_rslt_t cyhal_adc_enable_output(cyhal_adc_t *obj, cyhal_adc_output_t output, cyhal_source_t *source)
{
    /* No trigger outputs supported on this hardware */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(output);
    CY_UNUSED_PARAMETER(source);
    return CYHAL_ADC_RSLT_ERR_UNSUPPORTED;
}

cy_rslt_t cyhal_adc_disconnect_digital(cyhal_adc_t *obj, cyhal_source_t source,  cyhal_adc_input_t input)
{
    /* No trigger inputs supported on this hardware */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(source);
    CY_UNUSED_PARAMETER(input);
    return CYHAL_ADC_RSLT_ERR_UNSUPPORTED;
}

cy_rslt_t cyhal_adc_disable_output(cyhal_adc_t *obj, cyhal_adc_output_t output)
{
    /* No trigger outputs supported on this hardware */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(output);
    return CYHAL_ADC_RSLT_ERR_UNSUPPORTED;
}

#if defined(__cplusplus)
}
#endif

#endif /* CYHAL_DRIVER_AVAILABLE_ADC */
