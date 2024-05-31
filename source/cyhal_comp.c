/***************************************************************************/ /**
* \file cyhal_comp.c
*
* \brief
* Provides a high level interface for interacting with the Infineon analog
* comparator. This interface abstracts out the chip specific details. If any chip
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

#include "cyhal_comp.h"
#include "cyhal_hwmgr.h"
#include "cyhal_system.h"
#include "cyhal_analog_common.h"
#include <string.h>

/**
* \addtogroup group_hal_impl_comp COMP (Analog Comparator)
* \ingroup group_hal_impl
* \{
* On CAT5, the comparator driver is used to control the x2 LPComp (Low Power Comparator) HW blocks.
*
* \section group_hal_impl_comp_power Power Level Mapping
* The CAT5 Comparator supports the following features:
* * Hysteresis limit of (V(+) - V(-) > 0.5*60mV)
* * Rising edge event detection
* 
* The following functions are not supported:
* * Set power level
* * Falling edge event
*
* \} group_hal_impl_comp
*/

#if (CYHAL_DRIVER_AVAILABLE_COMP)
#include "cy_adccomp.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define _CYHAL_COMP_GPIO_0      LHL_GPIO_2
#define _CYHAL_COMP_GPIO_1      LHL_GPIO_3
#define _CYHAL_COMP_GPIO_2      LHL_GPIO_4
#define _CYHAL_COMP_GPIO_3      LHL_GPIO_5
#define _CYHAL_COMP_GPIO_4      LHL_GPIO_6
#define _CYHAL_COMP_GPIO_5      LHL_GPIO_7
#define _CYHAL_COMP_GPIO_6      LHL_GPIO_8
#define _CYHAL_COMP_GPIO_7      LHL_GPIO_9
#define _CYHAL_COMP_GPIO_MIC    MIC_P

#define _CYHAL_COMP_IP_BLOCKS   (CY_IP_MXS40ADCMIC_INSTANCES)
#define _CYHAL_COMP_PER_LP      (CY_IP_MXLPCOMP_INSTANCES)
#define _CYHAL_COMP_GET_CHANNEL(channel_num) ((1u == (channel_num)) ? CY_ADCCOMP_LPCOMP_2 : CY_ADCCOMP_LPCOMP_1)
#define _CYHAL_COMP_GET_INTR(channel_num) ((CY_ADCCOMP_LPCOMP_2 == (channel_num)) ? CY_ADCCOMP_INTR_LPCOMP2 : CY_ADCCOMP_INTR_LPCOMP1)
#define _CYHAL_COMP_GET_STATUS_MASK(channel_num) ((CY_ADCCOMP_LPCOMP_2 == (channel_num)) ? CY_ADCCOMP_STATUS_LPCOMP2_LATCHED_HIGH : CY_ADCCOMP_STATUS_LPCOMP1_LATCHED_HIGH)

static cyhal_comp_t* _cyhal_comp_lp_config_structs[_CYHAL_COMP_IP_BLOCKS * _CYHAL_COMP_PER_LP];


/*******************************************************************************
*       Comp callbacks used in _cyhal_adccomp_cb()
*******************************************************************************/

void _cyhal_comp_process_event(uint32_t intr)
{
    uint8_t index = (intr == CY_ADCCOMP_INTR_LPCOMP1) ? 0u : 1u;
    cyhal_comp_t* obj = _cyhal_comp_lp_config_structs[index];
    Cy_ADCCOMP_ClearInterrupt(obj->base, intr);

    if(0 != (CYHAL_COMP_RISING_EDGE & ((cyhal_comp_event_t)obj->user_enabled_events)))
    {
        cyhal_comp_event_callback_t callback = (cyhal_comp_event_callback_t)obj->callback_data.callback;
        if(NULL != callback)
        {
            callback(obj->callback_data.callback_arg, CYHAL_COMP_RISING_EDGE);
        }
    }
}


/*******************************************************************************
*       Comp HAL Functions
*******************************************************************************/

cy_rslt_t _cyhal_comp_init_hw(cyhal_comp_t *obj, const cy_stc_adccomp_lpcomp_config_t* cfg)
{
    obj->base = _cyhal_adccomp_base[obj->resource.block_num];
    cy_en_adccomp_lpcomp_id_t comp_ch = _CYHAL_COMP_GET_CHANNEL(obj->resource.channel_num);
    cy_en_adccomp_status_t pdl_result = Cy_ADCCOMP_LPCOMP_Init(obj->base, comp_ch, obj->mode, (cy_stc_adccomp_lpcomp_config_t *)cfg);
    cy_rslt_t result = (pdl_result == CY_ADCCOMP_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_COMP_RSLT_ERR_BAD_ARGUMENT;

    if(CY_RSLT_SUCCESS == result)
        result = _cyhal_adccomp_register_cb();
    
    if (CY_RSLT_SUCCESS == result)
    {
        pdl_result = Cy_ADCCOMP_LPCOMP_Enable(obj->base, comp_ch);
        result = (pdl_result == CY_ADCCOMP_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_COMP_RSLT_ERR_BAD_ARGUMENT;
    }

    if (CY_RSLT_SUCCESS == result)
    {
        Cy_ADCCOMP_LPCOMP_ClearLatch(obj->base, comp_ch); // Start off with a clean slate
        _cyhal_comp_lp_config_structs[(obj->resource.block_num * _CYHAL_COMP_PER_LP) + obj->resource.channel_num] = obj;
    }

    return result;
}

cy_rslt_t cyhal_comp_init(cyhal_comp_t *obj, cyhal_gpio_t vin_p, cyhal_gpio_t vin_m, cyhal_gpio_t output, cyhal_comp_config_t *cfg)
{
    CY_ASSERT(NULL != obj);

    /* Initial values */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_en_adccomp_lpcomp_positive_channel_t inP = CY_ADCCOMP_LPCOMP_IN_P_OPEN;
    cy_en_adccomp_lpcomp_negative_channel_t inN = CY_ADCCOMP_LPCOMP_IN_N_OPEN;
    cy_en_adccomp_lpcomp_mode_t mode = CY_ADCCOMP_LPCOMP_DC;

    memset(obj, 0, sizeof(cyhal_comp_t));
    obj->user_enabled_events = 0;

    /* Mark pins in obj NC until they are successfully reserved */
    obj->pin_vin_p = NC;
    obj->pin_vin_m = NC;
    obj->resource.type = CYHAL_RSC_INVALID;

    /* Output pin is not supported */
    if (NC != output)
    {
        result = CYHAL_COMP_RSLT_ERR_INVALID_PIN;
    }

    /* Get mapping for pins */
    const cyhal_resource_pin_mapping_t *vin_p_map   = (NC != vin_p) ? _CYHAL_UTILS_GET_RESOURCE(vin_p, cyhal_pin_map_lpcomp_inp_comp) : NULL;
    const cyhal_resource_pin_mapping_t *vin_m_map   = (NC != vin_m) ? _CYHAL_UTILS_GET_RESOURCE(vin_m, cyhal_pin_map_lpcomp_inn_comp) : NULL;

    /* Verify if dc mode mapping was successful */
    if ((NULL == vin_p_map) || (NULL == vin_m_map))
    {
        result = CYHAL_COMP_RSLT_ERR_INVALID_PIN;
    }
    else
    {
        if (false == _cyhal_utils_map_resources_equal(vin_p_map, vin_m_map))
        {
            result = CYHAL_COMP_RSLT_ERR_INVALID_PIN;
        }
    }

    /* DC mode failed. Check for NTD (AMIC) mode. Negative input should not be used */
    if (result == CYHAL_COMP_RSLT_ERR_INVALID_PIN)
    {
        vin_p_map = (NC != vin_p) ? _CYHAL_UTILS_GET_RESOURCE(vin_p, cyhal_pin_map_mic_p) : NULL;
        if ((NULL != vin_p_map) && (NULL == vin_m_map))
        {
            result = CY_RSLT_SUCCESS;
            mode = CY_ADCCOMP_LPCOMP_NTD;
        }
    }

    if (CY_RSLT_SUCCESS == result)
    {
        cyhal_resource_inst_t rsc = { CYHAL_RSC_LPCOMP, vin_p_map->block_num, vin_p_map->channel_num };
        if (CY_RSLT_SUCCESS == result)
            result = cyhal_hwmgr_reserve(&rsc);
        
        if (CY_RSLT_SUCCESS == result)
            obj->resource = rsc;
    }

    if (CY_RSLT_SUCCESS == result)
    {
        result = _cyhal_utils_reserve_and_connect(vin_p_map, CYHAL_PIN_MAP_DRIVE_MODE_LPCOMP_INP_COMP);
        if (CY_RSLT_SUCCESS == result)
        {
            switch (vin_p)
            {
                case _CYHAL_COMP_GPIO_2:
                case _CYHAL_COMP_GPIO_6:
                {
                    inP = CY_ADCCOMP_LPCOMP_IN_P_GPIO26;
                    break;
                }
                case _CYHAL_COMP_GPIO_3:
                case _CYHAL_COMP_GPIO_7:
                {
                    inP = CY_ADCCOMP_LPCOMP_IN_P_GPIO37;
                    break;
                }
                case _CYHAL_COMP_GPIO_MIC:
                {
                    inP = CY_ADCCOMP_LPCOMP_IN_P_MIC;
                    break;
                }
                default:
                {
                    inP = CY_ADCCOMP_LPCOMP_IN_P_OPEN;
                    break;
                }
            }
            obj->pin_vin_p = vin_p;
            obj->inP = inP;
        }
    }

    if (CY_RSLT_SUCCESS == result)
    {
        if (NULL != vin_m_map)
        {
            result = _cyhal_utils_reserve_and_connect(vin_m_map, CYHAL_PIN_MAP_DRIVE_MODE_LPCOMP_INN_COMP);
            if (result == CY_RSLT_SUCCESS)
            {
                switch (vin_m)
                {
                    case _CYHAL_COMP_GPIO_0:
                    case _CYHAL_COMP_GPIO_4:
                    {
                        inN = CY_ADCCOMP_LPCOMP_IN_N_GPIO04;
                        break;
                    }
                    case _CYHAL_COMP_GPIO_1:
                    case _CYHAL_COMP_GPIO_5:
                    {
                        inN = CY_ADCCOMP_LPCOMP_IN_N_GPIO15;
                        break;
                    }
                    default:
                    {
                        // Expected to be the case for NTD mode
                        inN = CY_ADCCOMP_LPCOMP_IN_N_OPEN;
                        break;
                    }
                }
                obj->pin_vin_m = vin_m;
                obj->inN = inN;
            }
        }
    }

    if (result == CY_RSLT_SUCCESS)
    {
        cy_stc_adccomp_lpcomp_dc_config_t  dcConfig = {
            .lpcompHyst = cfg->hysteresis ? CY_ADCCOMP_LPCOMP_HYST_LIMIT_60MV_101_5DB : CY_ADCCOMP_LPCOMP_HYST_LIMIT_0MV_NONE,
            .lpcompHyst2x = false,
            .inN = obj->inN,
            .inP = obj->inP
        };
        cy_stc_adccomp_lpcomp_ntd_config_t ntdConfig = {
            .lpcompHyst = cfg->hysteresis ? CY_ADCCOMP_LPCOMP_HYST_LIMIT_60MV_101_5DB : CY_ADCCOMP_LPCOMP_HYST_LIMIT_0MV_NONE,
            .lpcompHyst2x = false,
        };
        const cy_stc_adccomp_lpcomp_config_t comp_lp_config = {
            .dcConfig = &dcConfig,
            .ntdConfig = &ntdConfig
        };
        
        obj->mode = mode;
        result = _cyhal_comp_init_hw(obj, &comp_lp_config);
    }

    /* Free the resource in case of failure */
    if (result != CY_RSLT_SUCCESS)
    {
        cyhal_comp_free(obj);
    }
    
    return result;
}

cy_rslt_t cyhal_comp_init_cfg(cyhal_comp_t *obj, const cyhal_comp_configurator_t *cfg)
{
    cy_rslt_t result = CYHAL_COMP_RSLT_ERR_BAD_ARGUMENT;
    memset(obj, 0, sizeof(cyhal_comp_t));
    obj->owned_by_configurator = true;
    obj->resource = *cfg->resource;
    obj->pin_vin_p = NC;
    obj->pin_vin_m = NC;
    
    if (CYHAL_RSC_LPCOMP == cfg->resource->type)
    {
        result = _cyhal_comp_init_hw(obj, cfg->lpcomp);
        if(CY_RSLT_SUCCESS != result)
        {
            cyhal_comp_free(obj);
        }
    }

    return result;
}

void cyhal_comp_free(cyhal_comp_t *obj)
{
    CY_ASSERT(NULL != obj);

    if(CYHAL_RSC_INVALID != obj->resource.type)
    {
        _cyhal_comp_lp_config_structs[(obj->resource.block_num * _CYHAL_COMP_PER_LP) + obj->resource.channel_num] = NULL;

        if(NULL != obj->base)
        {
            cy_en_adccomp_lpcomp_id_t comp_ch = _CYHAL_COMP_GET_CHANNEL(obj->resource.channel_num);
            uint32_t comp_intr = _CYHAL_COMP_GET_INTR(comp_ch);
            Cy_ADCCOMP_LPCOMP_Disable(obj->base, comp_ch);
            Cy_ADCCOMP_LPCOMP_ClearLatch(obj->base, comp_ch);
            Cy_ADCCOMP_LPCOMP_DeInit(obj->base, comp_ch);
            Cy_ADCCOMP_DisableInterrupt(obj->base, comp_intr);
        }

        if(false == obj->owned_by_configurator)
        {
            cyhal_hwmgr_free(&(obj->resource));
        }
        
        obj->base = NULL;
    }

    _cyhal_utils_release_if_used(&(obj->pin_vin_p));
    _cyhal_utils_release_if_used(&(obj->pin_vin_m));
}

cy_rslt_t cyhal_comp_set_power(cyhal_comp_t *obj, cyhal_power_level_t power)
{
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(power);
    return CYHAL_COMP_RSLT_ERR_NOT_SUPPORTED;
}

cy_rslt_t cyhal_comp_configure(cyhal_comp_t *obj, cyhal_comp_config_t *cfg)
{
    CY_ASSERT(NULL != obj);

    cy_stc_adccomp_lpcomp_dc_config_t dcConfig = {
        .lpcompHyst = cfg->hysteresis ? CY_ADCCOMP_LPCOMP_HYST_LIMIT_60MV_101_5DB : CY_ADCCOMP_LPCOMP_HYST_LIMIT_0MV_NONE,
        .lpcompHyst2x = false,
        .inN = obj->inN,
        .inP = obj->inP
    };
    cy_stc_adccomp_lpcomp_ntd_config_t ntdConfig = {
        .lpcompHyst = cfg->hysteresis ? CY_ADCCOMP_LPCOMP_HYST_LIMIT_60MV_101_5DB : CY_ADCCOMP_LPCOMP_HYST_LIMIT_0MV_NONE,
        .lpcompHyst2x = false,
    };
    cy_stc_adccomp_lpcomp_config_t comp_lp_config = {
        .dcConfig = &dcConfig,
        .ntdConfig = &ntdConfig
    };

    return _cyhal_comp_init_hw(obj, &comp_lp_config);
}

bool cyhal_comp_read(cyhal_comp_t *obj)
{
    CY_ASSERT(obj->resource.type == CYHAL_RSC_LPCOMP);
    cy_en_adccomp_lpcomp_id_t comp_ch = _CYHAL_COMP_GET_CHANNEL(obj->resource.channel_num);
    uint32_t status_mask = _CYHAL_COMP_GET_STATUS_MASK(comp_ch);

    /* Latched high when V(+) - V(-) > 0.5* hysteresis limit.
    * Do a couple of reads to determine the current state rather
    * than relying on a state change from the previous latching.
    */
    bool read_val = ((status_mask & Cy_ADCCOMP_GetStatusRegisterVal(obj->base)) != 0);
    Cy_ADCCOMP_LPCOMP_ClearLatch(obj->base, comp_ch);
    cyhal_system_delay_us(100); // Give time to latch again
    read_val = ((status_mask & Cy_ADCCOMP_GetStatusRegisterVal(obj->base)) != 0);
    Cy_ADCCOMP_LPCOMP_ClearLatch(obj->base, comp_ch);

    return read_val;
}

void cyhal_comp_register_callback(cyhal_comp_t *obj, cyhal_comp_event_callback_t callback, void *callback_arg)
{
    CY_ASSERT(NULL != obj);

    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    obj->callback_data.callback = (cy_israddress) callback;
    obj->callback_data.callback_arg = callback_arg;
    cyhal_system_critical_section_exit(savedIntrStatus);
}

void cyhal_comp_enable_event(cyhal_comp_t *obj, cyhal_comp_event_t event, uint8_t intr_priority, bool enable)
{
    CY_ASSERT(obj->resource.type == CYHAL_RSC_LPCOMP);
    CY_UNUSED_PARAMETER(intr_priority);
    cy_en_adccomp_lpcomp_id_t comp_ch = _CYHAL_COMP_GET_CHANNEL(obj->resource.channel_num);
    uint32_t comp_intr = _CYHAL_COMP_GET_INTR(comp_ch);

    if(enable)
    {
        obj->user_enabled_events |= event;
        Cy_ADCCOMP_ClearInterrupt(obj->base, comp_intr);
        Cy_ADCCOMP_EnableInterrupt(obj->base, comp_intr);
    }
    else
    {
        obj->user_enabled_events &= (~event);
        Cy_ADCCOMP_DisableInterrupt(obj->base, comp_intr);
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* CYHAL_DRIVER_AVAILABLE_COMP */
