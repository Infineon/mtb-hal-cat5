/***************************************************************************//**
* \file cyhal_hw_resources.h
*
* \brief
* Provides basic resource type definitions used by the HAL.
*
********************************************************************************
* \copyright
* Copyright 2021-2022 Cypress Semiconductor Corporation (an Infineon company) or
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
* \addtogroup group_hal_impl_availability HAL Driver Availability Macros
* \ingroup group_hal_impl
* \{
*/

#pragma once

#include "cy_pdl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Documented in cyhal.h
#define CYHAL_API_VERSION                   (2)

/** \cond INTERNAL */
#define _CYHAL_DRIVER_AVAILABLE_SCB         ((CY_IP_MXSCB_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_ADC_MIC     ((CY_IP_MXS40ADCMIC_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_COMP_LP     ((CY_IP_MXLPCOMP_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_QSPI        (0)/* Disabled temporarily to enable clean compilation */
/* #define _CYHAL_DRIVER_AVAILABLE_QSPI        ((CY_IP_MXSMIF_INSTANCES) > 0) */
#define _CYHAL_DRIVER_AVAILABLE_TCPWM       ((CY_IP_MXTCPWM_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_TDM         ((CY_IP_MXTDM_INSTANCES) > 0)

#define _CYHAL_DRIVER_AVAILABLE_GPIO        ((CY_IP_PL061_ARM_GPIO_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_TIMER       ((CY_IP_SP804_TIMER_INSTANCES) > 0)
#define _CYHAL_DRIVER_AVAILABLE_WATCHDOG    (0) /* Watchdog functionality is not available to the user */
#define _CYHAL_DRIVER_AVAILABLE_DMAC        ((CY_IP_PL081_8CH_DMAC_INSTANCES) > 0)

#define _CYHAL_DRIVER_AVAILABLE_NVM_FLASH   (0)
#define _CYHAL_DRIVER_AVAILABLE_NVM_OTP     (0)
#define _CYHAL_DRIVER_AVAILABLE_NVM_RRAM    (0)

/** \endcond */

// Documented in cyhal.h
#define CYHAL_DRIVER_AVAILABLE_HWMGR        (1)
#define CYHAL_DRIVER_AVAILABLE_GPIO         (_CYHAL_DRIVER_AVAILABLE_GPIO)
#define CYHAL_DRIVER_AVAILABLE_INTERCONNECT (1)
#define CYHAL_DRIVER_AVAILABLE_CLOCK        (1)
#define CYHAL_DRIVER_AVAILABLE_SYSTEM       (1)
#define CYHAL_DRIVER_AVAILABLE_DMA          (_CYHAL_DRIVER_AVAILABLE_DMAC)
#define CYHAL_DRIVER_AVAILABLE_SYSPM        (1)
#define CYHAL_DRIVER_AVAILABLE_QSPI         (_CYHAL_DRIVER_AVAILABLE_QSPI)
#define CYHAL_DRIVER_AVAILABLE_RTC          (1)
#define CYHAL_DRIVER_AVAILABLE_WDT          (_CYHAL_DRIVER_AVAILABLE_WATCHDOG)

#define CYHAL_DRIVER_AVAILABLE_EZI2C        (_CYHAL_DRIVER_AVAILABLE_SCB) //SCB[x]_I2C
#define CYHAL_DRIVER_AVAILABLE_I2C          (_CYHAL_DRIVER_AVAILABLE_SCB) //SCB[x]_I2C
#define CYHAL_DRIVER_AVAILABLE_SPI          (_CYHAL_DRIVER_AVAILABLE_SCB) //SCB[x]_SPI
#define CYHAL_DRIVER_AVAILABLE_UART         (_CYHAL_DRIVER_AVAILABLE_SCB) //SCB[x]_UART

// Temporarily set to 0 until later in dev cycle
#if 0
#define CYHAL_DRIVER_AVAILABLE_ADC          (_CYHAL_DRIVER_AVAILABLE_ADC_MIC)
#define CYHAL_DRIVER_AVAILABLE_COMP         (_CYHAL_DRIVER_AVAILABLE_COMP_LP)
#else
#define CYHAL_DRIVER_AVAILABLE_ADC          (0)
#define CYHAL_DRIVER_AVAILABLE_COMP         (0)
#endif

#define CYHAL_DRIVER_AVAILABLE_I2S          (_CYHAL_DRIVER_AVAILABLE_TDM) //AUDIOSS[x]_I2S
#define CYHAL_DRIVER_AVAILABLE_I2S_TX       (CYHAL_DRIVER_AVAILABLE_I2S)
#define CYHAL_DRIVER_AVAILABLE_I2S_RX       (CYHAL_DRIVER_AVAILABLE_I2S)
#define CYHAL_DRIVER_AVAILABLE_PDMPCM       (_CYHAL_DRIVER_AVAILABLE_TDM) //AUDIOSS[x]_PDM
#define CYHAL_DRIVER_AVAILABLE_TDM          (_CYHAL_DRIVER_AVAILABLE_TDM) //AUDIOSS[x]_I2S
#define CYHAL_DRIVER_AVAILABLE_TDM_TX       (CYHAL_DRIVER_AVAILABLE_TDM)
#define CYHAL_DRIVER_AVAILABLE_TDM_RX       (CYHAL_DRIVER_AVAILABLE_TDM)

#define CYHAL_DRIVER_AVAILABLE_PWM          (_CYHAL_DRIVER_AVAILABLE_TCPWM)
#define CYHAL_DRIVER_AVAILABLE_QUADDEC      (_CYHAL_DRIVER_AVAILABLE_TCPWM)
#define CYHAL_DRIVER_AVAILABLE_TIMER        ((_CYHAL_DRIVER_AVAILABLE_TCPWM) || (_CYHAL_DRIVER_AVAILABLE_TIMER))

// Not available
#define CYHAL_DRIVER_AVAILABLE_CRC          (0)
#define CYHAL_DRIVER_AVAILABLE_DAC          (0)
#define CYHAL_DRIVER_AVAILABLE_NVM          (0)
#define CYHAL_DRIVER_AVAILABLE_FLASH        (0)   /* Deprecated */
#define CYHAL_DRIVER_AVAILABLE_KEYSCAN      (0)
#define CYHAL_DRIVER_AVAILABLE_LPTIMER      (0)
#define CYHAL_DRIVER_AVAILABLE_OPAMP        (0)
#define CYHAL_DRIVER_AVAILABLE_SDHC         (0)
#define CYHAL_DRIVER_AVAILABLE_SDIO         (0)
#define CYHAL_DRIVER_AVAILABLE_TRNG         (0)
#define CYHAL_DRIVER_AVAILABLE_USB_DEV      (0)
#define CYHAL_DRIVER_AVAILABLE_IPC          (0)

/** \} group_hal_impl_availability */
/**
* \addtogroup group_hal_impl_hw_types
* \ingroup group_hal_impl
* \{
*/


/* NOTE: Any changes made to this enum must also be made to the hardware manager resource tracking */
/** Resource types that the hardware manager supports */
typedef enum
{
    CYHAL_RSC_ADCMIC,    /*!< Analog to digital converter with Analog Mic support */
    CYHAL_RSC_CLOCK,     /*!< Clock */
    CYHAL_RSC_DMA,       /*!< DMA controller */
    CYHAL_RSC_GPIO,      /*!< General purpose I/O pin */
    CYHAL_RSC_PDM,       /*!< PCM/PDM communications block */
    CYHAL_RSC_RTC,       /*!< Real time clock */
    CYHAL_RSC_SCB,       /*!< Serial Communications Block */
    CYHAL_RSC_TCPWM,     /*!< Timer/Counter/PWM block */
    CYHAL_RSC_TDM,       /*!< TDM block */
    CYHAL_RSC_T2TIMER,   /*!< T2Timer block */
    CYHAL_RSC_INVALID,   /*!< Placeholder for invalid type */
} cyhal_resource_t;

/* NOTE: The peripheral clocks should match the CLOCK_TPORT_PERIPHERAL_t enum */
/** Enum for the different types of clocks that exist on the device. */
typedef enum
{
    CYHAL_CLOCK_BLOCK_PERI_SCB0,  /*!< SCB0 clock */
    CYHAL_CLOCK_BLOCK_PERI_SCB1,  /*!< SCB1 clock */
    CYHAL_CLOCK_BLOCK_PERI_TCPWM, /*!< TCPWM clock */
    CYHAL_CLOCK_BLOCK_PERI_SCB2,  /*!< SCB2 clock */
    CYHAL_CLOCK_BLOCK_TDM,        /*!< TDM clock */
    CYHAL_CLOCK_BLOCK_ADCMIC,     /*!< ADCMIC clock */
    CYHAL_CLOCK_BLOCK_CPU,        /*!< CPU clock */
} cyhal_clock_block_t;

/** @brief Clock object
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases. */
typedef struct
{
    cyhal_clock_block_t     block;
    uint8_t                 channel;
    bool                    reserved;
} cyhal_clock_t;

/**
  * @brief Represents a particular instance of a resource on the chip.
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct
{
    cyhal_resource_t type;      //!< The resource block type
    uint8_t          block_num; //!< The resource block index
    /**
      * The channel number, if the resource type defines multiple channels
      * per block instance. Otherwise, 0 */
    uint8_t          channel_num;
} cyhal_resource_inst_t;

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/** \} group_hal_impl_hw_types */
