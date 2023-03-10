/***************************************************************************//**
* \file cyhal_t2timer.h
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

/**
* \addtogroup group_hal_t2timer T2Timer
* \ingroup group_hal
* \{
* \section group_hal_t2timer T2Timer Usage
* This device has T2Timer timers in addition to TCPWM timers (see cyhal_timer.h).
* T2Timer is limited in functionality. It is a basic timer that can be started,
* stopped, reset, read, and configured. The only notable difference operation
* compared to TCPWM timers is that T2Timer counts down.  For consistency, the value
* obtained when reading the timer is adjusted to simulate counting up, but it will
* never exceed the duration set when started.
* 
* There are two T2Timer timer instances avalible for use.
*
* The intention is for users to interact w/ the Timer HAL, not directly interact with T2Timer HAL, but either work
*/
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "cy_result.h"
#include "cyhal_hw_types.h"

#if defined(__cplusplus)
extern "C" {
#endif


/** Bad argument. eg: null pointer */
#define CYHAL_T2TIMER_RSLT_ERR_BAD_ARGUMENT               \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_T2TIMER, 0))
/** Failed to initialize Timer */
#define CYHAL_T2TIMER_RSLT_ERR_INIT                       \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_T2TIMER, 1))
/** T2Timer backend failed. eg: failed to start or stop */
#define CYHAL_T2TIMER_RSLT_ERR                       \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_T2TIMER, 2))

/*******************************************************************************
*       Data Structures
*******************************************************************************/

// TODO: what is mode versus counter_mode?
/** @brief Describes the current configuration of a T2Timer timer. Used to interface between
 *  Timer configuration function and T2Timer.
 */
typedef struct
{
    bool mode;              //!< Whether the timer operates freerunning (true) or periodic (false)
    bool counter_mode;      //!< Whether the timer/counter operates wrapping (true) or one shot (false)
    uint32_t duration;      //!< Timer comparison value
} cyhal_t2timer_cfg_t;

/** Used to specificially request a T2Timer through timer initialization function */
#define CYHAL_CLOCK_T2TIMER (const void*)(0xFFFFFFFF)

/*******************************************************************************
*       Typedefs
*******************************************************************************/

/** Handler for timer events */
typedef void(*timerCallback)(INT32 arg);


/*******************************************************************************
*       Functions
*******************************************************************************/

/** Registers a callback function and callback arguments to a T2Timer.
 * \note All T2Timer configurations only take effect after starting or restarting the timer.
 *
 * @param[in] obj           The timer object
 * @param[in] callback      The callback function
 * @param[in] callback_arg  Arguments passed to the callback function
 */
void _cyhal_t2timer_register_callback(cyhal_t2timer_t *obj, cy_israddress callback, void *callback_arg);

/** Frees a T2Timer instance
 *
 * @param[in] obj           The timer object to free
 */
void _cyhal_t2timer_free(cyhal_t2timer_t *obj);

/** Initializes a T2Timer timer instance.
 *
 * @param[in] obj           The timer object to initialize
 * @param[in] config        Settings to initialize with.  If NULL, uses default
 * @return The status of the initialization
 */
cy_rslt_t _cyhal_t2timer_init(cyhal_t2timer_t *obj, cyhal_t2timer_t* config);

/** Configures the timer.
 * \note All T2Timer configurations only take effect after starting or restarting the timer.
 *
 * @param[in] obj           The timer object to configure
 * @param[in] config        Settings to define desired timer behavior
 * @return The status of the configure request
 */
cy_rslt_t _cyhal_t2timer_configure(cyhal_t2timer_t *obj, cyhal_t2timer_cfg_t* config);

/** Configures the timer frequency.
 * \note All T2Timer configurations only take effect after starting or restarting the timer.
 *
 * @param[in] obj           The timer object to configure
 * @param[in] hz            The frequency rate in Hz
 * @return The status of the set_frequency request
 */
cy_rslt_t _cyhal_t2timer_set_frequency(cyhal_t2timer_t *obj, uint32_t hz);

/** Start the timer.
 *
 * @param[in] obj           The timer object to start
 * @return The status of the start request
 */
cy_rslt_t _cyhal_t2timer_start(cyhal_t2timer_t *obj);

/** Stop the timer.
 *
 * @param[in] obj           The timer object to stop
 * @return The status of the stop request
 */
cy_rslt_t _cyhal_t2timer_stop(cyhal_t2timer_t *obj);

/** Reset the timer.
 *
 * @param[in] obj           The timer object to restart
 * @return The status of the restart request
 */
cy_rslt_t _cyhal_t2timer_reset(cyhal_t2timer_t *obj);

/** Read the timer value.
 *
 * @param[in] obj           The timer object to read
 * @return The remaining time left on the timer in us
 */
uint32_t _cyhal_t2timer_read(const cyhal_t2timer_t *obj);

#if defined(__cplusplus)
}
#endif

/** \} group_hal_t2timer */
