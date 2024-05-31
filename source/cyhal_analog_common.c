/*******************************************************************************
* File Name: cyhal_analog_common.c
*
* Description:
* Provides common functionality for the analog drivers.
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
#include "cyhal_analog_common.h"

#if (CYHAL_DRIVER_AVAILABLE_ADC || CYHAL_DRIVER_AVAILABLE_COMP)

#if defined(__cplusplus)
extern "C"
{
#endif

extern cyhal_adc_t* _cyhal_adcmic_config_structs[CY_IP_MXS40ADCMIC_INSTANCES];

CyADCCOMP_Type *const _cyhal_adccomp_base[CY_IP_MXS40ADCMIC_INSTANCES] = {ADCCOMP0};

cy_rslt_t _cyhal_adccomp_register_cb(void)
{
    static bool registered = false;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    // The callback needs to be registered only once
    if (!registered)
        result = Cy_ADCCOMP_RegisterIntrCallback(_cyhal_adccomp_cb);
    
    registered = (result == CY_RSLT_SUCCESS);
    return result;
}

// Shared handler between ADC and Comparators
void _cyhal_adccomp_cb(void)
{
    uint32_t intr_status = Cy_ADCCOMP_GetInterruptStatusMasked(_cyhal_adccomp_base[0]);

    /* Start DC calibration. This condition will be set when the ADC is enabled.
    All DC measurements first require that the DC calibration is performed. */
    if ((intr_status & CY_ADCCOMP_INTR_ADC_READY) == CY_ADCCOMP_INTR_ADC_READY)
    {
        #if (CYHAL_DRIVER_AVAILABLE_ADC)
        _cyhal_adcmic_calibrate();
        #endif
    }
    
    if ((intr_status & CY_ADCCOMP_INTR_CIC) == CY_ADCCOMP_INTR_CIC)
    {
        #if (CYHAL_DRIVER_AVAILABLE_ADC)
        _cyhal_adcmic_get_result();
        #endif
    }

    if ((intr_status & CY_ADCCOMP_INTR_LPCOMP1) == CY_ADCCOMP_INTR_LPCOMP1)
    {
        #if (CYHAL_DRIVER_AVAILABLE_COMP)
        _cyhal_comp_process_event(CY_ADCCOMP_INTR_LPCOMP1);
        #endif
    }

    if ((intr_status & CY_ADCCOMP_INTR_LPCOMP2) == CY_ADCCOMP_INTR_LPCOMP2)
    {
        #if (CYHAL_DRIVER_AVAILABLE_COMP)
        _cyhal_comp_process_event(CY_ADCCOMP_INTR_LPCOMP2);
        #endif
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* #if (CYHAL_DRIVER_AVAILABLE_ADC || CYHAL_DRIVER_AVAILABLE_COMP) */
