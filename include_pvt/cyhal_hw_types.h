/***************************************************************************//**
* \file cyhal_hw_types.h
*
* \brief
* Provides a struct definitions for configuration resources in the HAL.
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
* \addtogroup group_hal_impl CAT5 (AIROC™) Implementation Specific
* \{
* This section provides details about the CAT5 (AIROC™) implementation of the Cypress HAL.
* All information within this section is platform specific and is provided for reference.
* Portable application code should depend only on the APIs and types which are documented
* in the @ref group_hal section.
*
* \section group_hal_impl_mapping HAL Resource Hardware Mapping
* The following table shows a mapping of each HAL driver to the lower level firmware driver
* and the corresponding hardware resource. This is intended to help understand how the HAL
* is implemented for CAT5 and what features the underlying hardware supports.
*
* | HAL Resource       | CAT5 Hardware                    |
* | ------------------ | -------------------------------- |
* | ADC                | ADCMic                           |
* | Clock              | All clocks (system & peripheral) |
* | Comparator         | LPComp                           |
* | DMA                | DMA Controller                   |
* | EZ-I2C             | SCB                              |
* | GPIO               | GPIO                             |
* | Hardware Manager   | NA                               |
* | I2C                | SCB                              |
* | I2S                | I2S                              |
* | PDM/PCM            | PDM-PCM                          |
* | PWM                | TCPWM                            |
* | Quadrature Decoder | TCPWM                            |
* | RTC                | RTC                              |
* | SPI                | SCB                              |
* | SysPM              | System Power Resources           |
* | System             | System Resources                 |
* | TDM                | I2S                              |
* | Timer              | TCPWM                            |
* | UART               | SCB                              |
* | SDIO               | SDIOD                            |
*
*/

/**
* \addtogroup group_hal_impl_hw_types CAT5 Specific Hardware Types
* \{
* Aliases for types which are part of the public HAL interface but whose representations
* need to vary per HAL implementation
*/

#pragma once

#include "cy_pdl.h"
#include "cyhal_general_types.h"
#include "cyhal_hw_resources.h"
#include "cyhal_pin_package.h"
#include "cyhal_triggers.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CYHAL_ISR_PRIORITY_DEFAULT
/** Priority that is applied by default to all drivers when initialized. Priorities can be
 * overridden on each driver as part of enabling events.
 */
#define CYHAL_ISR_PRIORITY_DEFAULT  (7)
#endif

/**
* \cond INTERNAL
*/

#define CYHAL_CLOCK_IMPL_HEADER         "cyhal_clock_impl.h"    //!< Implementation specific header for Clocks
#define CYHAL_DMA_IMPL_HEADER           "cyhal_dma_impl.h"      //!< Implementation specific header for DMA
#define CYHAL_GPIO_IMPL_HEADER          "cyhal_gpio_impl.h"     //!< Implementation specific header for GPIO
#define CYHAL_I2S_IMPL_HEADER           "cyhal_i2s_impl.h"      //!< Implementation specific header for I2S
#define CYHAL_PWM_IMPL_HEADER           "cyhal_pwm_impl.h"      //!< Implementation specific header for PWM
#define CYHAL_QUADDEC_IMPL_HEADER       "cyhal_quaddec_impl.h"  //!< Implementation specific header for Quaddec
#define CYHAL_RTC_IMPL_HEADER           "cyhal_rtc_impl.h"      //!< Implementation specific header for RTC
#define CYHAL_SYSTEM_IMPL_HEADER        "cyhal_system_impl.h"   //!< Implementation specific header for System
#define CYHAL_SYSPM_IMPL_HEADER         "cyhal_syspm_impl.h"    //!< Implementation specific header for System Power Management
#define CYHAL_TDM_IMPL_HEADER           "cyhal_tdm_impl.h"      //!< Implementation specific header for TDM
#define CYHAL_TIMER_IMPL_HEADER         "cyhal_timer_impl.h"    //!< Implementation specific header for Timer

/** \endcond */

/************************************************************/
/*       The following type is not available in ROM         */
/************************************************************/

/**
* \cond INTERNAL
*/

typedef uint32_t GPIO_PRT_Type;

/** \endcond */

/************************************************************/


/** Callbacks for Sleep and Deepsleep APIs */
#define cyhal_system_callback_t cy_stc_syspm_callback_t

typedef void (* cy_israddress)(void);   /**< Type of ISR callbacks */
#if defined (__ICCARM__)
    typedef union { cy_israddress __fun; void * __ptr; } cy_intvec_elem;
#endif  /* defined (__ICCARM__) */

/** @brief Event callback data object */
typedef struct {
    cy_israddress                       callback;
    void*                               callback_arg;
} cyhal_event_callback_data_t;

/**
  * @brief Store information about buffer
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    union
    {
        void     *v;
        uint8_t  *u8;
        uint16_t *u16;
        uint32_t *u32;
    } addr;
    uint32_t size;
    uint32_t index;
} _cyhal_buffer_info_t;

#if (CYHAL_DRIVER_AVAILABLE_SDIO_DEV)
/**
 * @brief SDIO Buffer info for device mode
 *
 * Application code relies on the specific content of this struct for data.
 * Header information and payload are stored in this buffer.
 */
typedef struct
{
    uint8_t *rx_header;            //!< Represents the header of a received data packet
    uint8_t *rx_payload;           //!< Represents the payload of a received data packet
    uint8_t *tx_payload;           //!< Represents the payload of a transmitted data packet
} cyhal_sdio_buffer_t;
#endif

/**
 * @brief Shared data between timer/counter and PWM
 *
 * Application code should not rely on the specific content of this struct.
 * They are considered an implementation detail which is subject to change
 * between platforms and/or HAL releases.
 */
typedef struct {
    bool                                owned_by_configurator;
    bool                                presleep_state;
    TCPWM_Type*                         base;
    cyhal_resource_inst_t               resource;
    cyhal_clock_t                       clock;
    bool                                dedicated_clock;
    uint32_t                            clock_hz;
    cyhal_event_callback_data_t         callback_data;
    cyhal_source_t                      inputs[5];
} cyhal_tcpwm_t;

/* This is presented out of order because many other structs depend on it */
/**
  * @brief DMA object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    cyhal_resource_inst_t               resource;
    bool                                is_enabled;
    BTSS_DMAC_CONFIG_REG_FLOWCONTROL_t  transfer_type;
    BTSS_DMAC_CONTROL_LINE_t            src_ctrl;
    BTSS_DMAC_CONTROL_LINE_t            dest_ctrl;
    BTSS_DMAC_APP_REQUEST_t             dma_req;
    cyhal_event_callback_data_t         callback_data;
} cyhal_dma_t;


/**
  * @brief DMA configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    void *empty;
} cyhal_dma_configurator_t;

struct _cyhal_audioss_s;

/**
  * @brief Interface to abstract away the driver-specific differences between TDM and I2S
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct
{
    /** Convert a PDL-level interrupt cause to a HAL-level event */
    uint32_t (*convert_interrupt_cause)(uint32_t pdl_event, bool is_tx);
    /** Convert a HAL-level event to a PDL-level interrupt cause */
    uint32_t (*convert_to_pdl)(uint32_t hal_event, bool is_tx);
    /** Invoke the user callback with the specified HAL event.
     * Only called after the user callback has been verified to not be null. */
    void     (*invoke_user_callback)(struct _cyhal_audioss_s* obj, uint32_t hal_event);
    /** HAL event mask that represents the empty state */
    uint32_t event_mask_empty;
    /** HAL event mask that represents the half empty state */
    uint32_t event_mask_half_empty;
    /** HAL event mask that represents the full state */
    uint32_t event_mask_full;
    /** HAL event mask that represents the half full state */
    uint32_t event_mask_half_full;
    /** HAL event mask that represents async rx complete */
    uint32_t event_rx_complete;
    /** HAL event mask that represents async tx complete */
    uint32_t event_tx_complete;
    /** Error code for invalid pin */
    cy_rslt_t err_invalid_pin;
    /** Error code for invalid argument */
    cy_rslt_t err_invalid_arg;
    /** Error code for invalid clock frequency */
    cy_rslt_t err_clock;
    /** Error code for configuration not supported */
    cy_rslt_t err_not_supported;
} _cyhal_audioss_interface_t;

/**
  * @brief Shared data between i2s and tdm
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct _cyhal_audioss_s { /* Explicit name to enable forward declaration */
    bool                            owned_by_configurator;
#if (CYHAL_DRIVER_AVAILABLE_TDM)
    /* None of the PDL APIs actually want a bare TDM_Type */
    TDM_STRUCT_Type                 *base;
#endif
    cyhal_resource_inst_t           resource;
    cyhal_gpio_t                    pin_tx_sck;
    cyhal_gpio_t                    pin_tx_ws;
    cyhal_gpio_t                    pin_tx_sdo;
    cyhal_gpio_t                    pin_rx_sck;
    cyhal_gpio_t                    pin_rx_mclk;
    cyhal_gpio_t                    pin_rx_ws;
    cyhal_gpio_t                    pin_rx_sdi;
    cyhal_gpio_t                    pin_tx_mclk;
    uint8_t                         user_fifo_level_rx;
    uint32_t                        mclk_hz_rx;
    uint8_t                         channel_length_rx;
    uint8_t                         word_length_rx;
    uint32_t                        mclk_hz_tx;
    uint8_t                         channel_length_tx;
    uint8_t                         word_length_tx;
    cyhal_clock_t                   clock;
    bool                            is_clock_owned;
    uint16_t                        user_enabled_events;
    cyhal_event_callback_data_t     callback_data;
    cyhal_async_mode_t              async_mode;
    uint8_t                         async_dma_priority;
    cyhal_dma_t                     tx_dma;
    cyhal_dma_t                     rx_dma;
    // Note: When the async DMA mode is in use, these variables will always reflect the state
    // that the transfer will be in after the in-progress DMA transfer, if any, is complete
    volatile const void             *async_tx_buff;
    volatile size_t                 async_tx_length;
    volatile void                   *async_rx_buff;
    volatile size_t                 async_rx_length;
    volatile bool                   pm_transition_ready;
    cyhal_syspm_callback_data_t     pm_callback;
    const _cyhal_audioss_interface_t *interface;
} _cyhal_audioss_t;

/**
  * @brief Shared I2S/TDM configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*            resource;
#if (CYHAL_DRIVER_AVAILABLE_TDM)
    const cy_stc_tdm_config_t*              config;
#endif
    const cyhal_clock_t *                   clock;
    uint32_t                                mclk_hz_rx; /* Must be 0 is mclk is not in use for this direction */
    uint32_t                                mclk_hz_tx; /* Must be 0 is mclk is not in use for this direction */
} _cyhal_audioss_configurator_t;

struct _cyhal_adc_channel_s;

/**
  * @brief ADC object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    bool                                owned_by_configurator;
#if (CYHAL_DRIVER_AVAILABLE_ADC)
    CyADCCOMP_Type*                     base;
    cy_stc_adccomp_adc_context_t        pdl_context;
#endif
    /* When doing a full scan, which channel are we on */
    uint8_t                             current_channel_index;
    /* We implement multi-channel sequencing in firmware; there's no fixed channel count specified
    * in hardware. So size the array based on the number of input pins that are connected */
    struct _cyhal_adc_channel_s*        channel_config[sizeof(cyhal_pin_map_adcmic_gpio_adc_in) / sizeof(cyhal_pin_map_adcmic_gpio_adc_in[0])];
    cyhal_resource_inst_t               resource;
    cyhal_clock_t                       clock;
    bool                                using_audio;
    bool                                dc_calibrated;
    bool                                dc_calibration_started;
    int16_t                             calibOffset;
    /* Has at least one conversion completed since the last configuration change */
    volatile bool                       conversion_complete;
    bool                                stop_after_scan;
    uint8_t                             user_enabled_events;
    cyhal_event_callback_data_t         callback_data;
    /* Always updated to contain the location where the next result should be stored */
    int32_t                             *async_buff_next;
    bool                                async_transfer_in_uv; /* Default is counts */
    /* Only decremented after all elements from a scan have been copied into async_buff */
    size_t                              async_scans_remaining;
} cyhal_adc_t;

/**
  * @brief ADC configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*        resource;
#if (CYHAL_DRIVER_AVAILABLE_ADC)
    cy_stc_adccomp_adc_config_t const*  config;
#endif
    const cyhal_clock_t *               clock;
    uint8_t                             num_channels;
    const uint32_t*                     achieved_acquisition_time;
    /* Pins are deliberately omitted from this struct. The configurator supports routing
    * from arbitrary sources that aren't necessarily pins. The HAL only needs to know what
    * the pins are for the purposes of reservation, freeing, and routing - all of which the
    * configurators take care of in this flow */
} cyhal_adc_configurator_t;

/**
  * @brief ADC channel object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct _cyhal_adc_channel_s { /* Struct given an explicit name to make the forward declaration above work */
    cyhal_adc_t*                        adc;
    cyhal_gpio_t                        vplus;
    uint8_t                             channel_idx;
#if (CYHAL_DRIVER_AVAILABLE_ADC)
    cy_en_adccomp_adc_dc_channel_t      channel_sel;
#endif
    bool                                enabled;
} cyhal_adc_channel_t;

/** @brief Comparator object */
typedef struct {
    bool                                owned_by_configurator;
    cyhal_resource_inst_t               resource;
#if (CYHAL_DRIVER_AVAILABLE_COMP)
    CyADCCOMP_Type*                     base;
#endif
    cyhal_gpio_t                        pin_vin_p;
    cyhal_gpio_t                        pin_vin_m;
    int16_t                             inP;
    int16_t                             inN;
    int16_t                             mode;
    cyhal_event_callback_data_t         callback_data;
    uint32_t                            user_enabled_events;
    cyhal_syspm_callback_data_t         pm_callback;
} cyhal_comp_t;

/**
  * @brief Comp configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*        resource;
#if (CYHAL_DRIVER_AVAILABLE_COMP)
    const cy_stc_adccomp_lpcomp_config_t *lpcomp;
#endif
    /* No GPIOs specified. The configurator could have routed from a non-preferred
    * GPIO, or from another non-GPIO on-chip source. */
} cyhal_comp_configurator_t;

/**
  * @brief I2C object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    CySCB_Type*                               base;
    cyhal_resource_inst_t                     resource;
    cyhal_gpio_t                              pin_sda;
    cyhal_gpio_t                              pin_scl;
    cyhal_clock_t                             clock;
    bool                                      is_clock_owned;
    cy_stc_scb_i2c_context_t                  context;
    cy_stc_scb_i2c_master_xfer_config_t       rx_config;
    cy_stc_scb_i2c_master_xfer_config_t       tx_config;
    uint32_t                                  irq_cause;
    uint8_t                                   addr_irq_cause;
    uint16_t                                  pending;
    bool                                      op_in_callback;
    _cyhal_buffer_info_t                      rx_slave_buff;
    _cyhal_buffer_info_t                      tx_slave_buff;
    cyhal_event_callback_data_t               callback_data;
    cyhal_event_callback_data_t               addr_callback_data;
    bool                                      dc_configured;
} cyhal_i2c_t;

/**
  * @brief I2C configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    const cyhal_resource_inst_t*            resource;
    const cy_stc_scb_i2c_config_t*          config;
    const cyhal_clock_t*                    clock;
} cyhal_i2c_configurator_t;

/**
  * @brief EZI2C object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    CySCB_Type*                         base;
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_sda;
    cyhal_gpio_t                        pin_scl;
    cyhal_clock_t                       clock;
    bool                                is_clock_owned;
    cy_stc_scb_ezi2c_context_t          context;
    uint32_t                            irq_cause;
    cyhal_event_callback_data_t         callback_data;
    bool                                two_addresses;
    bool                                dc_configured;
} cyhal_ezi2c_t;

/**
  * @brief EZI2C configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    const cyhal_resource_inst_t*            resource;
    const cy_stc_scb_ezi2c_config_t*        config;
    const cyhal_clock_t*                    clock;
} cyhal_ezi2c_configurator_t;

/**
  * @brief I2S object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef _cyhal_audioss_t cyhal_i2s_t;

/**
  * @brief I2S configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef _cyhal_audioss_configurator_t cyhal_i2s_configurator_t;

/**
  * @brief LPTimer object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    void *empty;
} cyhal_lptimer_t;

/**
  * @brief PDM-PCM object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
#if (CYHAL_DRIVER_AVAILABLE_PDMPCM)
    CyPdmPcm_Type*                      base;
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_data;
    cyhal_gpio_t                        pin_clk;
    cy_en_pdm_pcm_mic_source_t          source;
    cyhal_comp_t                        comp_obj;
    cyhal_adc_t                         adc_obj;
    uint32_t                            stabilization_cycles;
    bool                                is_enabled;
    bool                                is_mic_ready;
    bool                                is_ntd_ready;
    int32_t                             events;
    void*                               async_data;  
    uint8_t*                            fifo_context;
    cyhal_event_callback_data_t         callback_data;
    bool                                pm_transition_pending;
    cyhal_syspm_callback_data_t         pm_callback_data;
#else
    void                                *empty;
#endif /* (CYHAL_DRIVER_AVAILABLE_PDMPCM) */
} cyhal_pdm_pcm_t;

/**
  * @brief PDM-PCM configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    void *empty;
} cyhal_pdm_pcm_configurator_t;

/**
  * @brief PWM object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    cyhal_tcpwm_t                       tcpwm;
    cyhal_gpio_t                        pin;
    cyhal_gpio_t                        pin_compl;
    bool                                dead_time_set;
} cyhal_pwm_t;

/**
  * @brief PWM configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*        resource;
    cy_stc_tcpwm_pwm_config_t const*    config;
    const cyhal_clock_t *               clock;
} cyhal_pwm_configurator_t;


/**
  * @brief QSPI object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
#ifdef CY_IP_MXSMIF
  #define SMIF_CHIP_TOP_SPI_SEL_NR    1

    SMIF_Type*                          base;
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_sclk;
    en_hsiom_sel_t                      saved_sclk_hsiom;
    cyhal_gpio_t                        pin_io[8];
    en_hsiom_sel_t                      saved_io_hsiom[8];
    cyhal_gpio_t                        pin_ssel[SMIF_CHIP_TOP_SPI_SEL_NR];
    en_hsiom_sel_t                      saved_ssel_hsiom[SMIF_CHIP_TOP_SPI_SEL_NR];
    /* Active slave select */
    cy_en_smif_slave_select_t           slave_select;
    cyhal_clock_t                       clock;
    bool                                is_clock_owned;
    uint8_t                             mode;
    cy_stc_smif_context_t               context;
    uint32_t                            irq_cause;
    cyhal_event_callback_data_t         callback_data;
    cyhal_syspm_callback_data_t         pm_callback;
    bool                                pm_transition_pending;
    bool                                dc_configured;
#else
    void *empty;
#endif /* ifdef CY_IP_MXSMIF */
} cyhal_qspi_t;

/**
  * @brief QSPI configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
#ifdef CY_IP_MXSMIF
    const cyhal_resource_inst_t*            resource;
    const cy_stc_smif_config_t*             config;
    cyhal_clock_t*                          clock;
    struct
    {
        cyhal_gpio_t                        sclk;
        cyhal_gpio_t                        ssel[4];
        cyhal_gpio_t                        io[8];
    } gpios;
    /* Bit representation of currently not supported interrupts:
        Bit 5 : Memory Mode Alignment Error
        Bit 4 : RX Data FIFO Underflow
        Bit 3 : TX Command FIFO Overflow
        Bit 2 : TX Data FIFO Overflow
        Bit 1 : RX FIFO Level Trigger
        Bit 0 : TX FIFO Level Trigger
    */
    uint8_t                                 irqs;
    /* Bit representation of DMA triggers activation indicators:
        Bit 1 : RX Trigger Output activated in configurator
        Bit 0 : TX Trigger Output activated in configurator
    */
    uint8_t                                 dmas;
#else
    void *empty;
#endif /* defined(CY_IP_MXSMIF) */
} cyhal_qspi_configurator_t;

/**
  * @brief Quadrature Decoder object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    cyhal_tcpwm_t                       tcpwm;
    cyhal_gpio_t                        phi_a;
    cyhal_gpio_t                        phi_b;
    cyhal_gpio_t                        index;
    uint32_t                            last_counter_value;
} cyhal_quaddec_t;

/**
  * @brief Quadrature Decoder configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*            resource;
    const cy_stc_tcpwm_quaddec_config_t*    config;
    const cyhal_clock_t *                   clock;
} cyhal_quaddec_configurator_t;

/**
  * @brief RTC object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
#if CYHAL_DRIVER_AVAILABLE_RTC
    cy_stc_rtc_dst_t                    dst;
#else
    void                                *empty;
#endif
} cyhal_rtc_t;

/**
  * @brief RTC configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
#if CYHAL_DRIVER_AVAILABLE_RTC
    const cyhal_resource_inst_t*        resource;
    const RTC_TIME_t *                  config;
    const cy_stc_rtc_dst_t *            dst_config;
#else
    void                                *empty;
#endif /* CYHAL_DRIVER_AVAILABLE_RTC */
} cyhal_rtc_configurator_t;

/**
  * @brief SDIO object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
#if (CYHAL_DRIVER_AVAILABLE_SDIO_DEV)
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_clk;
    cyhal_gpio_t                        pin_cmd;
    cyhal_gpio_t                        pin_data_0;
    cyhal_gpio_t                        pin_data_1;
    cyhal_gpio_t                        pin_data_2;
    cyhal_gpio_t                        pin_data_3;
    bool                                hw_inited;
    bool                                is_ready;
    int32_t                             events;
    cyhal_event_callback_data_t         callback_data;
    bool                                pm_transition_pending;
    cyhal_syspm_callback_data_t         pm_callback_data;
    cyhal_sdio_buffer_t                 buffer;
#else
    void                                *empty;
#endif /* (CYHAL_DRIVER_AVAILABLE_SDIO_DEV) */
} cyhal_sdio_t;

/**
  * @brief SDIO configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    void *empty;
} cyhal_sdio_configurator_t;

/**
  * @brief SPI object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    CySCB_Type*                         base;
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_miso;
    cyhal_gpio_t                        pin_mosi;
    cyhal_gpio_t                        pin_sclk;
    cyhal_gpio_t                        pin_ssel[4];
    cy_en_scb_spi_polarity_t            ssel_pol[4];
    cyhal_pinmux_t                      ssel_func[4];
    uint8_t                             active_ssel;
    cyhal_clock_t                       clock;
    cy_en_scb_spi_sclk_mode_t           clk_mode;
    uint8_t                             mode;
    uint8_t                             data_bits;
    bool                                is_slave;
    bool                                alloc_clock;
    uint8_t                             oversample_value;
    bool                                msb_first;
    cy_stc_scb_spi_context_t            context;
    uint32_t                            irq_cause;
    uint16_t volatile                   pending;
    bool                                op_in_callback;
    uint8_t                             write_fill;
    void                                *rx_buffer;
    uint32_t                            rx_buffer_size;
    const void                          *tx_buffer;
    uint32_t                            tx_buffer_size;
    bool                                is_async;
    cyhal_event_callback_data_t         callback_data;
    bool                                dc_configured;
} cyhal_spi_t;

/**
  * @brief SPI configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    const cyhal_resource_inst_t*            resource;
    const cy_stc_scb_spi_config_t*          config;
    const cyhal_clock_t*                    clock;
    struct
    {
        cyhal_gpio_t                        sclk;
        cyhal_gpio_t                        ssel[4];
        cyhal_gpio_t                        mosi;
        cyhal_gpio_t                        miso;
    } gpios;
} cyhal_spi_configurator_t;

/**
  * @brief TDM object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef _cyhal_audioss_t cyhal_tdm_t;

/**
  * @brief TDM configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef _cyhal_audioss_configurator_t cyhal_tdm_configurator_t;

/**
 * @brief T2Timer configurator struct
 *
 * This struct is used to track the T2Timer settings.  Every time the timer is
 * started or reset, these settings are required.  Maintaining them so the user
 * does not have to supply them every time.
 *
 */
typedef struct
{
    T2_ARM_TIMER_AUX_t                  which_timer;
    T2_ARM_TIMER_EN_t                   enabled;
    T2_ARM_TIMER_MODE_t                 mode;
    T2_ARM_TIMER_INT_EN_t               int_enable;
    T2_ARM_TIMER_DIVISOR_t              divisor;
    T2_ARM_TIMER_SIZE_t                 size;
    T2_ARM_TIMER_COUNTER_MODE_t         counter_mode;
    uint32_t                            timer_duration;
    void*                               callback_func;
    int32_t                             callback_arg;
    cyhal_resource_inst_t               resource;
} cyhal_t2timer_t;

/**
  * @brief Timer object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    union
    {
      cyhal_tcpwm_t                     tcpwm;
      cyhal_t2timer_t                   t2timer;
    };
    uint32_t                            default_value;
    bool                                running;
    bool                                is_t2timer;
} cyhal_timer_t;

/**
  * @brief Timer configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct
{
    const cyhal_resource_inst_t*            resource;
    const cy_stc_tcpwm_counter_config_t*    config;
    const cyhal_clock_t *                   clock;
} cyhal_timer_configurator_t;

/**
  * @brief UART object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    CySCB_Type*                         base;
    cyhal_resource_inst_t               resource;
    cyhal_gpio_t                        pin_rx;
    cyhal_gpio_t                        pin_tx;
    cyhal_gpio_t                        pin_cts;
    cyhal_gpio_t                        pin_rts;
    bool                                cts_enabled;
    bool                                rts_enabled;
    bool                                is_clock_owned;
    cyhal_clock_t                       clock;
    cy_stc_scb_uart_context_t           context;
    cy_stc_scb_uart_config_t            config;
    uint32_t                            irq_cause;
    en_hsiom_sel_t                      saved_tx_hsiom;
    en_hsiom_sel_t                      saved_rts_hsiom;
    cyhal_event_callback_data_t         callback_data;
    bool                                dc_configured;
    uint32_t                            baud_rate;
#if (CYHAL_DRIVER_AVAILABLE_DMA)
    cyhal_async_mode_t                  async_mode;
    cyhal_dma_t                         dma_tx;
    cyhal_dma_t                         dma_rx;
    volatile uint32_t                   async_tx_length;
    volatile uint32_t                   async_rx_length;
    volatile void                       *async_tx_buff;
    volatile void                       *async_rx_buff;
    uint32_t                            user_fifo_level;
#endif
} cyhal_uart_t;

/**
  * @brief UART configurator struct
  *
  * This struct allows a configurator to provide block configuration information
  * to the HAL. Because configurator-generated configurations are platform
  * specific, the contents of this struct is subject to change between platforms
  * and/or HAL releases.
  */
typedef struct {
    const cyhal_resource_inst_t*            resource;
    const cy_stc_scb_uart_config_t*         config;
    const cyhal_clock_t*                    clock;
    struct
    {
        cyhal_gpio_t                        pin_tx;
        cyhal_gpio_t                        pin_rts;
        cyhal_gpio_t                        pin_cts;
    } gpios;
} cyhal_uart_configurator_t;

/**
  * @brief WDT object
  *
  * Application code should not rely on the specific contents of this struct.
  * They are considered an implementation detail which is subject to change
  * between platforms and/or HAL releases.
  */
typedef struct {
    uint8_t placeholder;
} cyhal_wdt_t;

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/** \} group_hal_impl_hw_types */
/** \} group_hal_impl */
