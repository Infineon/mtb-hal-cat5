/***************************************************************************//**
* \file cyhal_rtc_impl.h
*
* Description:
* Provides a high level interface for interacting with the Infineon RTC.
*
********************************************************************************
* \copyright
* Copyright 2019-2022 Cypress Semiconductor Corporation (an Infineon company) or
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

/** \addtogroup group_hal_impl_rtc RTC (Real-Time Clock)
 * \ingroup group_hal_impl
 * \{
 * \} group_hal_impl_pwm */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif


/** @brief TODO: very simple guesstimplementation, not connected to anything yet.  Remove once ROM stuff is accessible */
typedef struct {
    uint16_t rtc16[4];                                  /**< Real-time clock register mapped into subsections */
} reg16map_t;

/** @brief TODO: very simple guesstimplementation, not connected to anything yet.  Remove once ROM stuff is accessible */
typedef union {
    reg16map_t reg16map;                                /**< Real-time clock register mapped into subsections */
    uint64_t rtc64;                                     /**< Real-time clock register as one full value*/
} tRTC_REAL_TIME_CLOCK;

/** @brief TODO: very simple guesstimplementation, not connected to anything yet.  Remove once ROM stuff is accessible */

typedef struct {
    uint64_t rtc64;                                     /**< Stores the value read from hardware RTC */
} rtcHWTimeStamp;

/** @brief TODO: very simple guesstimplementation, not connected to anything yet.  Remove once ROM stuff is accessible */
typedef struct
{
    uint32_t userSetRtcClockInSeconds;                  /**< A function to set the RTC time given seconds since 2010/1/1 00:00:00 */
    rtcHWTimeStamp userSetRtcHWTimeStamp;               /**< A function to set a reference time, e.g. used to compare for elapsed time */
} RtcState;

/* These should be defined in the ROM stuff.  Guessing values here just to compile for now */
#define BASE_LINE_REF_YEAR                      0x0UL //2010UL
#define BASE_LINE_REF_MONTH                     0x1UL


/** @brief Wrties given time to RTC peripheral. Note that month is 0-based here.
  * @param[in] sec   Second to set (0-59)
  * @param[in] min   Minute to set (0-59)
  * @param[in] hour  Hour to set (0-23)
  * @param[in] date   Day of month to set (1-31)
  * @param[in] month Month to set (0-11)
  * @param[in] year  4-digit year to set
  * @return The status of the write request
  */
cy_rslt_t _cyhal_rtc_set_rtc_direct(uint16_t sec, uint16_t min, uint16_t hour,
                                            uint16_t date, uint16_t month, uint16_t year);

/* Return true if currently DST, else false.  DST exists only in HAL, not in hardware RTC */
bool _cyhal_rtc_get_dst_status(cy_stc_rtc_dst_t const *dstTime, RTC_TIME_t const *timeDate);

/* Returns the number of days in a given month of a given year.  Assumes month[0-11], 4 digit year */
uint32_t _cyhal_rtc_days_in_month(uint32_t month, uint32_t year);

/* 
 * Calculate the day of week using Zeller's congruence algorithm.  Assumes HAL style values, so dayOfMonth[1-31],
 * month[0-11], and four digit year, and returns dayOfWeek[0-6]
 */
uint32_t _cyhal_rtc_calculate_day_of_week(uint16_t dayOfMonth, uint16_t month, uint16_t year);

/* Convert a RELATIVE format DST to a FIXED format DST dayOfMonth. Does not convert full stucture */
uint32_t _cyhal_rtc_relative_to_fixed(cy_rtc_dst_t const *convertDst);

#if defined(__cplusplus)
}
#endif

/* [] END OF FILE */