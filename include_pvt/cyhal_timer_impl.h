/***************************************************************************//**
* \file cyhal_timer_impl.h
*
* Description:
* Provides a high level interface for interacting with the Infineon Timer/Counter.
*
********************************************************************************
* \copyright
* Copyright 2019-2021 Cypress Semiconductor Corporation (an Infineon company) or
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

#include "cyhal_timer.h"
#include "cyhal_tcpwm_common.h"
#if defined(CY_IP_MXTCPWM_INSTANCES) || defined(CY_IP_M0S8TCPWM_INSTANCES)

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

__STATIC_INLINE void _cyhal_timer_free(cyhal_timer_t *obj)
{
    if(obj->is_t2timer)
    {
        _cyhal_t2timer_free(&obj->t2timer);
    }
    else
    {
        _cyhal_tcpwm_free(&obj->tcpwm);
    }
}

#define cyhal_timer_free(obj) _cyhal_timer_free(obj)

__STATIC_INLINE uint32_t _cyhal_timer_convert_event(cyhal_timer_event_t event)
{
    uint32_t pdl_event = 0U;
    if (event & CYHAL_TIMER_IRQ_TERMINAL_COUNT)
    {
        pdl_event |= CY_TCPWM_INT_ON_TC;
    }
    if (event & CYHAL_TIMER_IRQ_CAPTURE_COMPARE)
    {
        pdl_event |= CY_TCPWM_INT_ON_CC;
    }
    return pdl_event;
}

__STATIC_INLINE void _cyhal_timer_register_callback_internal(cyhal_timer_t *obj, cyhal_timer_event_callback_t callback, void *callback_arg)
{
    if(obj->is_t2timer)
    {
        _cyhal_t2timer_register_callback(&obj->t2timer, (cy_israddress) callback, callback_arg);
    }
    else
    {
        _cyhal_tcpwm_register_callback(&obj->tcpwm.resource, (cy_israddress) callback, callback_arg);
    }
}

#define cyhal_timer_register_callback(obj, callback, callback_arg) _cyhal_timer_register_callback_internal(obj, callback, callback_arg)

__STATIC_INLINE void _cyhal_timer_enable_event_internal(cyhal_timer_t *obj, cyhal_timer_event_t event, uint8_t intr_priority, bool enable)
{
    if(!obj->is_t2timer)
    {
        uint32_t converted = _cyhal_timer_convert_event(event);
        _cyhal_tcpwm_enable_event(&obj->tcpwm, &obj->tcpwm.resource, converted, intr_priority, enable);
    }
}

#define cyhal_timer_enable_event(obj, event, intr_priority, enable) _cyhal_timer_enable_event_internal(obj, event, intr_priority, enable)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* defined(CY_IP_MXTCPWM_INSTANCES) */
