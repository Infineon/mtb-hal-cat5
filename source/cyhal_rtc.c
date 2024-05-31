/***************************************************************************//**
* \file cyhal_rtc.c
*
* \brief
* Provides a high level interface for interacting with the Infineon Real-Time Clock.
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
 * \addtogroup group_hal_impl_rtc RTC (Real-Time Clock)
 * \ingroup group_hal_impl
 * \{
 *
 * \section section_rtc_alarms_interrupts Alarm and Interrupt Support
 * Due to hardware limitations, RTC alarms and interrupts are not supported on this device. This
 * limits funtionality and turns RTC into a more traditional timer.  Daylight savings time (DST)
 * is still supported.  Instead of using alarms, DST conditions are checked every time the HAL
 * reads or writes to the hardware clock. Upon entering DST, the hardware clock is incremented by
 * an hour, and upon exiting DST it is decremented by an hour.
 *
 * \} group_hal_impl_rtc
 */

#include "rtc.h"
#include "cy_utils.h"
#include "cyhal_rtc.h"
#include "cyhal_system.h"
#include "cyhal_utils_impl.h"
#include "cyhal_irq_impl.h"

#if (CYHAL_DRIVER_AVAILABLE_RTC)

#include "cyhal_rtc.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define _CYHAL_RTC_STATE_UNINITIALIZED                  (0U)
#define _CYHAL_RTC_STATE_ENABLED                        (1U)
#define _CYHAL_RTC_STATE_TIME_SET                       (2U)
/** Seperate from uninitialized, never_initialized means the clock has never been set.
 * This state should never be manually set */
#define _CYHAL_RTC_STATE_NEVER_INITALIZED               (0xFFFFU)

#define _CYHAL_RTC_DEFAULT_PRIORITY                     (5U)
#define _CYHAL_RTC_TM_YEAR_BASE                         (1900U)
#define _CYHAL_RTC_ROM_YEAR_BASE                        (2010U)

#define _CYHAL_RTC_MAX_SEC_OR_MIN                       (59U)
#define _CYHAL_RTC_MAX_HOURS_24H                        (23U)
#define _CYHAL_RTC_MAX_DAYS_IN_MONTH                    (31U)
#define _CYHAL_RTC_MONTHS_PER_YEAR                      (12U)

#define _CYHAL_RTC_IS_SEC_VALID(sec)                    ((sec) <= _CYHAL_RTC_MAX_SEC_OR_MIN)
#define _CYHAL_RTC_IS_MIN_VALID(min)                    ((min) <= _CYHAL_RTC_MAX_SEC_OR_MIN)
#define _CYHAL_RTC_IS_HOUR_VALID(hour)                  ((hour) <= _CYHAL_RTC_MAX_HOURS_24H)
#define _CYHAL_RTC_IS_MONTH_VALID(month)                ((((month) == 0U) || ((month) > 0U)) && ((month) < _CYHAL_RTC_MONTHS_PER_YEAR))
#define _CYHAL_RTC_IS_YEAR_VALID(year)                  ((year) >= _CYHAL_RTC_ROM_YEAR_BASE)
#define CYHAL_RTC_IS_DST_FORMAT_VALID(format)           (((format) == CY_RTC_DST_RELATIVE) || ((format) == CY_RTC_DST_FIXED))

// Used when converting DSTs into one long value for easy comparison
#define _CYHAL_RTC_MONTHS_PLACE                         (10UL)
#define _CYHAL_RTC_DAYOFMONTHS_PLACE                    (5UL)


static volatile uint16_t _cyhal_rtc_state = _CYHAL_RTC_STATE_NEVER_INITALIZED;
static cy_stc_rtc_dst_t *_cyhal_rtc_dst;
// Alarms aren't supported, so we need a way to know when we have transitioned into or out of DST.
// _cyhal_rts_is_dst records the last dst status value found when reading or writing,
// and when crossing into/out of DST, adds or removes an hour from the hardware RTC
static bool _cyhal_rtc_is_dst;

//This is to handle the DST stop condition, for last one hour before the DST stop time, where device could be with/within the DST.
//After the DST stop time, time is decremented by an hour and this flag would be set. Once set,this flag would be cleared after an hour.
//For eg. If the DST end time is 2 AM, when the time is first between 1 AM and 2AM on the DST stop day, time is within the DST.( flag would be false)
//DST ends at 2 AM and the time is decremented to 1AM. The time again would be between 1AM and 2 AM and time is then out of the DST. ( flag would be true)
static bool _cyhal_rtc_dst_stop_hour;

/* Check for transition into or out of DST, and update time if required
*
* \param time - Current Time
*
* \param is_set
* true  - When the DST check is performed as part of time set operation. Time passed includes
*         the DST offset. No time adjustment required. Just set/clear the flags.
* false - When the DST check is performed as part of time get operation. Adjust the RTC
*         time if the conditions are met
*
* \return
* Returns true if the current time is within the DST and false otherwise
*
*/
static bool _cyhal_check_for_dst_transition(RTC_TIME_t * time, bool is_set)
{
    if(_cyhal_rtc_dst == NULL)
    {
        // DST hasn't been set: do nothing
        return false;
    }

    uint32_t dstStartTime;
    uint32_t currentTime;
    uint32_t dstStopTime;
    uint32_t dstStartDayOfMonth;
    uint32_t dstStopDayOfMonth;
    int32_t  adjustVal = 0;

    // Standardize DST format for easy comparison
    if (CY_RTC_DST_FIXED == _cyhal_rtc_dst->startDst.format)
    {
        dstStartDayOfMonth = _cyhal_rtc_dst->startDst.dayOfMonth;
    }
    else
    {
        dstStartDayOfMonth = _cyhal_rtc_relative_to_fixed(&_cyhal_rtc_dst->startDst);
    }
    if (CY_RTC_DST_FIXED == _cyhal_rtc_dst->stopDst.format)
    {
        dstStopDayOfMonth = _cyhal_rtc_dst->stopDst.dayOfMonth;
    }
    else
    {
        dstStopDayOfMonth = _cyhal_rtc_relative_to_fixed(&_cyhal_rtc_dst->stopDst);
    }

    /* The function forms the date and time values for the DST start time,
     *  the DST Stop Time and for the Current Time. The function that compares
     *  the three formed values returns "true" under condition that:
     *  dstStartTime < currentTime < dstStopTime.
     *  The date and time value are formed this way:
     *  [13-10] - Month
     *  [9-5]   - Day of Month
     *  [0-4]   - Hour
     */
    dstStartTime =    ((uint32_t) (_cyhal_rtc_dst->startDst.month << _CYHAL_RTC_MONTHS_PLACE)  | (dstStartDayOfMonth << _CYHAL_RTC_DAYOFMONTHS_PLACE) \
                                   | (_cyhal_rtc_dst->startDst.hour));
    dstStopTime  =    ((uint32_t) (_cyhal_rtc_dst->stopDst.month << _CYHAL_RTC_MONTHS_PLACE)   | (dstStopDayOfMonth << _CYHAL_RTC_DAYOFMONTHS_PLACE) \
                                   | (_cyhal_rtc_dst->stopDst.hour));
    currentTime  =     ((uint32_t) (time->month << _CYHAL_RTC_MONTHS_PLACE)                    | (time->day << _CYHAL_RTC_DAYOFMONTHS_PLACE) \
                                   | (time->hour));

    //The time set request. Local Time set includes the DST offset. No need to adjust the RTC time.
    if(is_set == true)
    {
        //The time set is within the DST
        if((currentTime >= dstStartTime) && (currentTime < dstStopTime))
        {
           //DST flag is not set
            if(_cyhal_rtc_is_dst == false)
            {
                //_cyhal_rtc_dst_stop_hour flag is being checked to cover an edge case in which time is set when
                //in the last one hour before the DST stop time.
                if( _cyhal_rtc_dst_stop_hour == true )
                {
                    /* Check for the 'an hour before/after DST event' period */
                    if( (dstStopTime - currentTime) != 1)
                    {
                        //No need to adjust the time since the local time includes the DST offset. Just set the dst flag.
                        _cyhal_rtc_is_dst = true;
                    }
                }
                else
                {
                    _cyhal_rtc_is_dst = true;
                }
            }
            //No action required if the DST flag is set already
        }
        //Time set is out of the DST
        else
        {
            //Clear the dst flag
            _cyhal_rtc_is_dst = false;
            _cyhal_rtc_dst_stop_hour = false;
        }
    }
    //Time get reuest
    else
    {
        //Check if the time is within the DST.
        if((currentTime >= dstStartTime) && (currentTime < dstStopTime))
        {
            //Check if the time is within the DST for the first time. And not in the DST last hour
            if((_cyhal_rtc_is_dst == false) && (_cyhal_rtc_dst_stop_hour == false))
            {
                // We have entered DST.Increment the time by an hour on entering the DST for the first time
                adjustVal = 1;
                _cyhal_rtc_is_dst = true;
            }
        }
        //Time is past the DST stop time
        else if(currentTime >= dstStopTime )
        {
            // Not in the DST last hour
            if(_cyhal_rtc_dst_stop_hour == false)
            {
                //Check if the time is past the DST stop for the first time.
                if(_cyhal_rtc_is_dst == true)
                {
                    // We have exited DST.Decrement the time by an hour on exiting the DST for the first time
                    adjustVal = -1;
                    _cyhal_rtc_is_dst = false;
                    _cyhal_rtc_dst_stop_hour = true;
                }
            }
            //Clearing the flag once the time is past the DST stop time
            else
            {
                _cyhal_rtc_dst_stop_hour = false;
            }
        }
        else//No action really required when current time is less than the DST start time. But clearing the flags
        {
            //Clear the dst flag
            _cyhal_rtc_is_dst = false;
            _cyhal_rtc_dst_stop_hour = false;
        }
    }

    // If appropriate, update RTC clock
    if(adjustVal != 0)
    {
        uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
        time->hour += adjustVal;
        rtc_setRTCTime(time);
        cyhal_system_critical_section_exit(savedIntrStatus);
    }
    return _cyhal_rtc_is_dst;
}

/* Set RTC peripheral time, minus DST */
static void _cyhal_rtc_set_rtc_time(RTC_TIME_t * time)
{
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    rtc_setRTCTime(time);
    cyhal_system_critical_section_exit(savedIntrStatus);

    // Check for transition into/out of DST.
    _cyhal_check_for_dst_transition(time,true);
}

/* Get RTC peripheral time, plus DST */
static void _cyhal_rtc_get_rtc_time(RTC_TIME_t * time)
{
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    rtc_getRTCTime(time);
    cyhal_system_critical_section_exit(savedIntrStatus);

    // Check for transition into/out of DST.  This will adjust time value if needed
    _cyhal_check_for_dst_transition(time,false);
}

static cy_rslt_t _cyhal_rtc_init_common(const RTC_TIME_t* default_time)
{
    /* Init Driver */
    rtc_init();

    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    if (_cyhal_rtc_state == _CYHAL_RTC_STATE_NEVER_INITALIZED)
    {
        // Need to set the time on first use of RTC

        // Making a clone of default_time soley to avoid a warning due to setRTCTime not having a const.
        // Can't just remove it from our prototype, else we'd have to remove it from all implementations'
        RTC_TIME_t timeBuf = {
                .second = default_time->second,
                .minute = default_time->minute,
                .hour   = default_time->hour,
                .day    = default_time->day,
                .month  = default_time->month,
                .year   = default_time->year,
        };
        _cyhal_rtc_set_rtc_time(&timeBuf);

        _cyhal_rtc_state = _CYHAL_RTC_STATE_ENABLED;
    }
    else if (_cyhal_rtc_state == _CYHAL_RTC_STATE_UNINITIALIZED)
    {
        _cyhal_rtc_state = _CYHAL_RTC_STATE_ENABLED;
    }

    _cyhal_rtc_dst = NULL;
    _cyhal_rtc_is_dst = false;
    _cyhal_rtc_dst_stop_hour = false;

    return rslt;
}

cy_rslt_t cyhal_rtc_init(cyhal_rtc_t *obj)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);

    static const RTC_TIME_t default_time =
    {
        .second = 0,
        .minute = 0,
        .hour   = 0,
        .day    = 1,
        .month  = 0,
        .year   = 2011,
    };
    return _cyhal_rtc_init_common(&default_time);
}

cy_rslt_t cyhal_rtc_init_cfg(cyhal_rtc_t *obj, const cyhal_rtc_configurator_t *cfg)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);

    cy_rslt_t rslt = _cyhal_rtc_init_common(cfg->config);
    if (NULL != cfg->dst_config)
    {
        _cyhal_rtc_state = _CYHAL_RTC_STATE_TIME_SET;

        obj->dst = *(cfg->dst_config);
        _cyhal_rtc_dst = &(obj->dst);
    }
    return rslt;
}

void cyhal_rtc_free(cyhal_rtc_t *obj)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);

    _cyhal_rtc_dst = NULL;
    _cyhal_rtc_is_dst = false;
    _cyhal_rtc_dst_stop_hour = false;
}

bool cyhal_rtc_is_enabled(cyhal_rtc_t *obj)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);
    return (_cyhal_rtc_state == _CYHAL_RTC_STATE_TIME_SET);
}

cy_rslt_t cyhal_rtc_read(cyhal_rtc_t *obj, struct tm *time)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);

    RTC_TIME_t dateTime;
    _cyhal_rtc_get_rtc_time(&dateTime);

    // The number of days that precede each month of the year, not including Feb 29
    static const uint16_t cumulative_days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    time->tm_sec  = (int)dateTime.second;
    time->tm_min  = (int)dateTime.minute;
    time->tm_hour = (int)dateTime.hour;
    time->tm_mday = (int)dateTime.day;
    // tm and RTC_TIME_t both use 0-based months
    time->tm_mon  = (int)dateTime.month;
    time->tm_year = (int)dateTime.year - _CYHAL_RTC_TM_YEAR_BASE;
    time->tm_wday = _cyhal_rtc_calculate_day_of_week(dateTime.day, dateTime.month, dateTime.year);
    // Days so far + 1 if a leap year and passed the leap day
    time->tm_yday = (int)cumulative_days[time->tm_mon] + (int)dateTime.day + \
        (((int)(dateTime.month) >= 2 && ((((dateTime.year % 400) == 0) || (((dateTime.year % 4) == 0) && ((dateTime.year % 100) != 0))) ? 1u : 0u)));
    time->tm_isdst = -1;

    return CY_RSLT_SUCCESS;
}

cy_rslt_t cyhal_rtc_write(cyhal_rtc_t *obj, const struct tm *time)
{
    CY_ASSERT(NULL != obj);
    return cyhal_rtc_write_direct(obj, time->tm_sec, time->tm_min, time->tm_hour, time->tm_mday,
                                    time->tm_mon + 1, _CYHAL_RTC_TM_YEAR_BASE + time->tm_year);
}

cy_rslt_t cyhal_rtc_write_direct(cyhal_rtc_t *obj, uint32_t sec, uint32_t min, uint32_t hour,
                                 uint32_t day, uint32_t month, uint32_t year)
{
    CY_UNUSED_PARAMETER(obj);
    // cyhal_rtc.h wants month 1-based (1=January), but tm and ROM both assume 0-based
    month -= 1;
    cy_rslt_t rslt;
    if (!_CYHAL_RTC_IS_SEC_VALID(sec) || !_CYHAL_RTC_IS_MIN_VALID(min) || !_CYHAL_RTC_IS_HOUR_VALID(hour) || !_CYHAL_RTC_IS_MONTH_VALID(month) || !_CYHAL_RTC_IS_YEAR_VALID(year))
    {
        return CY_RSLT_RTC_BAD_ARGUMENT;
    }

    rslt = _cyhal_rtc_set_rtc_direct((uint16_t) sec, (uint16_t) min, (uint16_t) hour, (uint16_t) day, (uint16_t) month, (uint16_t) year);

    if (rslt == CY_RSLT_SUCCESS)
    {
        _cyhal_rtc_state = _CYHAL_RTC_STATE_TIME_SET;
    }
    return rslt;
}

cy_rslt_t cyhal_rtc_set_dst(cyhal_rtc_t *obj, const cyhal_rtc_dst_t *start, const cyhal_rtc_dst_t *stop)
{
    CY_ASSERT(NULL != obj);
    CY_ASSERT(NULL != start);
    CY_ASSERT(NULL != stop);

    static RTC_TIME_t rtc_time;

    obj->dst.startDst.format = (cy_rtc_dst_format_t)(start->format);
    obj->dst.startDst.hour = start->hour;
    // Adjust for 0=Jan vs 1=Jan
    obj->dst.startDst.month = start->month - 1;
    if(start->format == CYHAL_RTC_DST_RELATIVE)
    {
        obj->dst.startDst.weekOfMonth = start->weekOfMonth;
        obj->dst.startDst.dayOfWeek = start->dayOfWeek; 
    }
    else // start->format == CYHAL_RTC_DST_FIXED
    {
        obj->dst.startDst.dayOfMonth = start->dayOfMonth;
    }

    obj->dst.stopDst.format = (cy_rtc_dst_format_t)(stop->format);
    obj->dst.stopDst.hour = stop->hour;
    // Adjust for 0=Jan vs 1=Jan
    obj->dst.stopDst.month = stop->month - 1;
    if(stop->format == CYHAL_RTC_DST_RELATIVE)
    {
        obj->dst.stopDst.weekOfMonth = stop->weekOfMonth;
        obj->dst.stopDst.dayOfWeek = stop->dayOfWeek; 
    }
    else // stop->format == CYHAL_RTC_DST_FIXED
    {
        obj->dst.stopDst.dayOfMonth = stop->dayOfMonth;
    }

    _cyhal_rtc_dst = &(obj->dst);

    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    rtc_getRTCTime(&rtc_time);
    cyhal_system_critical_section_exit(savedIntrStatus);

    // Check for transition into/out of DST.
    _cyhal_check_for_dst_transition(&rtc_time, true);
    return CY_RSLT_SUCCESS;
}

bool cyhal_rtc_is_dst(cyhal_rtc_t *obj)
{
    CY_UNUSED_PARAMETER(obj);
    CY_ASSERT(NULL != obj);

    RTC_TIME_t dateTime;
    _cyhal_rtc_get_rtc_time(&dateTime);
    return _cyhal_check_for_dst_transition(&dateTime,false);
}

cy_rslt_t cyhal_rtc_set_alarm(cyhal_rtc_t *obj, const struct tm *time, cyhal_alarm_active_t active)
{
    /* Feature not supported on this device */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(time);
    CY_UNUSED_PARAMETER(active);
    return CYHAL_RTC_RSLT_ERR_NOT_SUPPORTED;
}

cy_rslt_t cyhal_rtc_set_alarm_by_seconds(cyhal_rtc_t *obj, const uint32_t seconds)
{
    /* Feature not supported on this device */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(seconds);
    return CYHAL_RTC_RSLT_ERR_NOT_SUPPORTED;
}

void cyhal_rtc_register_callback(cyhal_rtc_t *obj, cyhal_rtc_event_callback_t callback, void *callback_arg)
{
    /* Feature not supported on this device */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(callback);
    CY_UNUSED_PARAMETER(callback_arg);
}

void cyhal_rtc_enable_event(cyhal_rtc_t *obj, cyhal_rtc_event_t event, uint8_t intr_priority, bool enable)
{
    /* Feature not supported on this device */
    CY_UNUSED_PARAMETER(obj);
    CY_UNUSED_PARAMETER(event);
    CY_UNUSED_PARAMETER(intr_priority);
    CY_UNUSED_PARAMETER(enable);
}

cy_rslt_t _cyhal_rtc_set_rtc_direct(uint16_t sec, uint16_t min, uint16_t hour,
                                               uint16_t date, uint16_t month, uint16_t year)
{
    uint16_t status = 0;
    RTC_TIME_t timeBuf =
    {
        .second = sec,
        .minute = min,
        .hour = hour,
        .day = date,
        .month = month,
        .year = year,
    };
    _cyhal_rtc_set_rtc_time(&timeBuf);

    return status;
}

uint32_t _cyhal_rtc_days_in_month(uint32_t month, uint32_t year)
{
    UINT8 months[12] ={31,28,31,30,31,30,31,31,30,31,30,31};
    // leap year = year divisible by 400, but not by 100 when divisible by 4.
    if ( ( (year % 400) == 0 ) || ( ( (year % 4) == 0 ) && ( (year % 100) != 0 ) )  )
    {
        months[1] = 29;
    }
    return months[month];
}

/* Convert a RELATIVE format DST to a FIXED format DST dayOfMonth. Does not convert full stucture */
uint32_t _cyhal_rtc_relative_to_fixed(cy_rtc_dst_t const *convertDst)
{
    uint16_t currentYear;
    uint16_t currentDayOfMonth;
    uint16_t currentWeekOfMonth;
    uint16_t daysInMonth;
    uint16_t tmpDayOfMonth;
    RTC_TIME_t timebuf;

    /* Read the current year */
    uint32_t savedIntrStatus = cyhal_system_critical_section_enter();
    rtc_getRTCTime(&timebuf);
    cyhal_system_critical_section_exit(savedIntrStatus);

    currentYear = timebuf.year;

    /* Start with min values, see cy_rtc_dst_t */
    currentDayOfMonth  = 0x1U;
    currentWeekOfMonth = 0x0U;
    /* currentYear requires to be always > 2010 */
    daysInMonth = _cyhal_rtc_days_in_month(convertDst->month, currentYear);
    tmpDayOfMonth  = currentDayOfMonth;

    while((currentWeekOfMonth <= convertDst->weekOfMonth) && (currentDayOfMonth <= daysInMonth))
    {
        if (convertDst->dayOfWeek == _cyhal_rtc_calculate_day_of_week(currentDayOfMonth, convertDst->month, currentYear))
        {
            tmpDayOfMonth = currentDayOfMonth;
            currentWeekOfMonth++;
        }
        currentDayOfMonth++;
    }
    return(tmpDayOfMonth);
}

/*  Zeller's congruence is an algorithm used to calculate the day of the week
* for any date.Returns day of the week for the year, month, and day of month that are passed
* as parameters.
*
* For the Georgian calendar, Zeller's congruence is:
* h = (q + [(13 * (m + 1))/5] + K + [K/4] + [J/4] - 2J) mod 7
*
* h - The day of the week (0 = Saturday, 1 = Sunday, 2 = Monday, ., 6 = Friday).
* q - The day of the month.
* m - The month (3 = March, 4 = April, 5 = May, ..., 14 = February)
* K - The year of the century (year mod 100).
* J - The zero-based century (actually [year/100]) For example, the zero-based
* centuries for 1995 and 2000 are 19 and 20 respectively (not to be
* confused with the common ordinal century enumeration which indicates
* 20th for both cases).
* The formulas rely on the mathematician's definition of modulo division, which means that
* -2 mod 7 is equal to positive 5. Unfortunately, in the truncating way most computer languages
* implement the remainder function, -2 mod 7 returns a result of -2. So, to implement Zeller's
* congruence on a computer, the formulas should be altered slightly to ensure a positive numerator.
* The simplest way to do this is to replace - 2J by + 5J. So the formula becomes
*
* h = (q + [(13 * (m + 1))/5] + K + [K/4] + [J/4] + 5J) mod 7
*
* \note In this algorithm January and February are counted as months 13 and 14
* of the previous year.
*
* \param dayOfMonth
* The day of the month, Valid range [1..31]
*
* \param month
* The month of the year[0-11]
*
* \param year
* The year value. Valid range non-zero value.
*
* \return
* Returns a day of the week, [0-6]
*
*/
uint32_t _cyhal_rtc_calculate_day_of_week(uint16_t dayOfMonth, uint16_t month, uint16_t year)
{
    if (month < 2 ) {
        month += 12;
        year -= 1;
    }

    // This algorithm is structured 0 = Saturday, need to adjust (+6)%7 to convert to 0 = Sunday for HAL and tm
    // It does expect Jan = 1 with month though, hence the +2 instead of +1 there
    return ((dayOfMonth + ((13 * (month + 2))/5) + (year % 100) + ((year % 100) / 4) + ((year / 100) / 4) + (5 *(year / 100)) + 6) % 7);
}

#if defined(__cplusplus)
}
#endif

#endif /* CYHAL_DRIVER_AVAILABLE_RTC */
