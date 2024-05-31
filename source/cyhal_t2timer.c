/***************************************************************************//**
* \file cyhal_t2timer.c
*
* \brief
* Provides a high level interface for interacting with the Infineon T2Timer.
* This interface abstracts out the chip specific details. If any chip specific
* functionality is necessary, or performance is critical the low level functions
* can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2018-2022 Cypress Semiconductor Corporation (an Infineon company) or
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
#include "cyhal_t2timer.h"
#include "cyhal_hwmgr.h"
#include "cyhal_gpio.h"
#include "cyhal_interconnect.h"
#include "cyhal_syspm.h"
#include "cyhal_clock.h"


#define _CY_T2TIMER_SUCCESS                 (1U)
#define _CY_T2TIMER_FAILURE                 (0U)

// States the t2timer resources can be in
#define _CYHAL_T2TIMER_FREE                 (0U)
#define _CYHAL_T2TIMER_RESERVED             (1U)

// Convert T2Timer return values into HAL appropriate return values
#define _CY_T2TIMER_TO_HAL_RETURN(retVal)   ((retVal == _CY_T2TIMER_SUCCESS) ? CY_RSLT_SUCCESS : CYHAL_T2TIMER_RSLT_ERR)

static const cyhal_t2timer_t _cyhal_t2timer_timer_default_config =
{
    .which_timer = T2_ARM_TIMER_AUX_1,
    .enabled = T2_ARM_TIMER_EN_ENABLE,
    .mode = T2_ARM_TIMER_MODE_FREERUNNING,
    .int_enable = T2_ARM_TIMER_INT_EN_DISABLE,
    .divisor = T2_ARM_TIMER_DIVISOR_1,
    .size = T2_ARM_TIMER_SIZE_16_BIT,
    .counter_mode = T2_ARM_TIMER_COUNTER_MODE_WRAPPING,
    .timer_duration = 100000,
};

static volatile uint8_t _cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_COUNT] = {_CYHAL_T2TIMER_FREE, _CYHAL_T2TIMER_FREE};

/*******************************************************************************
*       T2Timer HAL Functions
*******************************************************************************/

// T2Timer doesn't check if callback is defined prior to calling.  Use this to avoid possible seg fault if user forgets
// to set a callback
static void _cyhal_t2timer_default_callback(void *arg)
{
    CY_UNUSED_PARAMETER(arg);
    return;
}

void _cyhal_t2timer_register_callback(cyhal_t2timer_t *obj, cy_israddress callback, void *callback_arg)
{
    obj->int_enable = T2_ARM_TIMER_INT_EN_ENABLE;
    obj->callback_func = (void*)callback;
    obj->callback_arg = (int32_t) callback_arg;
}

void _cyhal_t2timer_free(cyhal_t2timer_t *obj)
{
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return;
        // Not reserved, nothing to free
    }
    clock_auxTimerStop(obj->which_timer);
    _cyhal_t2timer_in_use[obj->which_timer] = _CYHAL_T2TIMER_FREE;
    cyhal_hwmgr_free(&(obj->resource));
}

cy_rslt_t _cyhal_t2timer_init(cyhal_t2timer_t *obj, cyhal_t2timer_t* config)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    if(config == NULL)
    {
        // Use default settings.  T2Timer uses these by default, but reassign in case
        // this instance was previously used
        if(_cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_1] == _CYHAL_T2TIMER_FREE)
        {
            _cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_1] = _CYHAL_T2TIMER_RESERVED;
            obj->which_timer    = T2_ARM_TIMER_AUX_1;
            obj->enabled        = _cyhal_t2timer_timer_default_config.enabled;
            obj->mode           = _cyhal_t2timer_timer_default_config.mode;
            obj->int_enable     = _cyhal_t2timer_timer_default_config.int_enable;
            obj->divisor        = _cyhal_t2timer_timer_default_config.divisor;
            obj->size           = _cyhal_t2timer_timer_default_config.size;
            obj->counter_mode   = _cyhal_t2timer_timer_default_config.counter_mode;
            obj->timer_duration = _cyhal_t2timer_timer_default_config.timer_duration;
            obj->callback_func  = (void*)&_cyhal_t2timer_default_callback;
        }
        else if (_cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_2] == _CYHAL_T2TIMER_FREE)
        {
            _cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_2] = _CYHAL_T2TIMER_RESERVED;
            obj->which_timer    = T2_ARM_TIMER_AUX_2;
            obj->enabled        = _cyhal_t2timer_timer_default_config.enabled;
            obj->mode           = _cyhal_t2timer_timer_default_config.mode;
            obj->int_enable     = _cyhal_t2timer_timer_default_config.int_enable;
            obj->divisor        = _cyhal_t2timer_timer_default_config.divisor;
            obj->size           = _cyhal_t2timer_timer_default_config.size;
            obj->counter_mode   = _cyhal_t2timer_timer_default_config.counter_mode;
            obj->timer_duration = _cyhal_t2timer_timer_default_config.timer_duration;
            obj->callback_func  = (void*)&_cyhal_t2timer_default_callback;
        }
        else
        {
            result = CYHAL_T2TIMER_RSLT_ERR_INIT;
        }
    }
    else // config != NULL
    {
        if(_cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_1] == _CYHAL_T2TIMER_FREE)
        {
            _cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_1] = _CYHAL_T2TIMER_RESERVED;
            obj->which_timer    = T2_ARM_TIMER_AUX_1;
            obj->enabled        = config->enabled;
            obj->mode           = config->mode;
            obj->int_enable     = config->int_enable;
            obj->divisor        = config->divisor;
            obj->size           = config->size;
            obj->counter_mode   = config->counter_mode;
            obj->timer_duration = config->timer_duration;
            obj->callback_func  = &(config->callback_func);
            obj->callback_arg   = (int32_t)(config->callback_arg);
        }
        else if (_cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_2] == _CYHAL_T2TIMER_FREE)
        {
            _cyhal_t2timer_in_use[T2_ARM_TIMER_AUX_2] = _CYHAL_T2TIMER_RESERVED;
            obj->which_timer    = T2_ARM_TIMER_AUX_2;
            obj->enabled        = config->enabled;
            obj->mode           = config->mode;
            obj->int_enable     = config->int_enable;
            obj->divisor        = config->divisor;
            obj->size           = config->size;
            obj->counter_mode   = config->counter_mode;
            obj->timer_duration = config->timer_duration;
            obj->callback_func  = &(config->callback_func);
            obj->callback_arg   = (int32_t)(config->callback_arg);
        }
        else
        {
            result = CYHAL_T2TIMER_RSLT_ERR_INIT;
        }
    }
    return result;
}

cy_rslt_t _cyhal_t2timer_configure(cyhal_t2timer_t *obj, cyhal_t2timer_cfg_t* config)
{
    obj->counter_mode = config->counter_mode;
    obj->mode = config->mode;
    obj->timer_duration = config->duration;

    // Use smaller size unless specifically need larger
    if(config->duration >= 0xFFFF)
    {
        obj->size = T2_ARM_TIMER_SIZE_32_BIT;
    }
    else
    {
        obj->size = T2_ARM_TIMER_SIZE_16_BIT;
    }

    return CY_RSLT_SUCCESS;
}


cy_rslt_t _cyhal_t2timer_set_frequency(cyhal_t2timer_t *obj, uint32_t hz)
{
    // First check that we've "initialized" this timer
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }
    
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t base_freq = 1000000; // 1 MHz
    uint32_t base_freq_16_divided = 62500; // 62.5 kHz
    uint32_t base_freq_256_divided = 3906; // 3.91 kHz
 
    // Check if within +/- 2% of each valid frequency.
    if(hz >= (base_freq * 0.98) && hz <= (base_freq * 1.02))
    {
        obj->divisor = T2_ARM_TIMER_DIVISOR_1;
    }
    else if ((hz >= (base_freq_16_divided * 0.98)) && (hz <= (base_freq_16_divided * 1.02)))
    {
        obj->divisor = T2_ARM_TIMER_DIVISOR_16;
    }
    else if ((hz >= (base_freq_256_divided * 0.98)) && (hz <= (base_freq_256_divided * 1.02)))
    {
        obj->divisor = T2_ARM_TIMER_DIVISOR_256;
    }
    else
    {
        result = CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }
    return result;
}

cy_rslt_t _cyhal_t2timer_start(cyhal_t2timer_t *obj)
{
    // First check that we've "initialized" this timer
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }

    uint32_t options = obj->enabled | obj->mode | obj->int_enable | obj->divisor | \
                       obj->size | obj->counter_mode;
    return _CY_T2TIMER_TO_HAL_RETURN(clock_auxTimerStart(obj->which_timer, obj->timer_duration, options, (void (*)(INT32))obj->callback_func, obj->callback_arg));
}

cy_rslt_t _cyhal_t2timer_stop(cyhal_t2timer_t *obj)
{
    // First check that we've "initialized" this timer
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }

    return _CY_T2TIMER_TO_HAL_RETURN(clock_auxTimerStop(obj->which_timer));
}

cy_rslt_t _cyhal_t2timer_reset(cyhal_t2timer_t *obj)
{
    // First check that we've "initialized" this timer
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }

    uint32_t options = obj->enabled | obj->mode | obj->int_enable | obj->divisor | \
                       obj->size | obj->counter_mode;
    return _CY_T2TIMER_TO_HAL_RETURN(clock_auxTimerStart(obj->which_timer, obj->timer_duration, options, (void (*)(INT32))obj->callback_func, obj->callback_arg));
}

uint32_t _cyhal_t2timer_read(const cyhal_t2timer_t *obj)
{
    // First check that we've "initialized" this timer
    if(_cyhal_t2timer_in_use[obj->which_timer] != _CYHAL_T2TIMER_RESERVED)
    {
        return CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT;
    }
    uint32_t time_left = clock_auxTimerUsToExpiry(obj->which_timer);
    switch(obj->divisor)
    {
        case(T2_ARM_TIMER_DIVISOR_1):
            break;
        case(T2_ARM_TIMER_DIVISOR_16):
            time_left *= 16;
            break;
        case(T2_ARM_TIMER_DIVISOR_256):
            time_left *= 256;
            break;
        default:
            // Should never get here
            CY_ASSERT(0);
    }
    return time_left;
}
