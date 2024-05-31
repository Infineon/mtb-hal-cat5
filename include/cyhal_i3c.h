/***************************************************************************//**
* \file cyhal_i3c.h
*
* \brief
* Provides a high level interface for interacting with the Infineon I3C.
* This interface abstracts out the chip specific details. If any chip specific
* functionality is necessary, or performance is critical the low level functions
* can be used directly.
*
********************************************************************************
* \copyright
* Copyright 2023 Cypress Semiconductor Corporation (an Infineon company) or
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
* \addtogroup group_hal_i3c I3C (Improved Inter-Integrated Circuit)
* \ingroup group_hal
* \{
* High level interface for interacting with the Infineon I3C.
*
* The I3C is a MIPI standardized protocol designed to overcome I2C limitations
* (limited speed, external signals needed for interrupts, no automatic detection of the devices connected to the bus, ...)
* while remaining power-efficient.
*
* \section section_i3c_features Features
*
* * An industry-standard I3C bus interface
* * Supports target, main controller, and secondary controller operation
* * SCL frequency up to 12.5MHz (SDR data rate 12.5Mbps, HDR-DDR data rate at 25Mbps)
* * In controller mode, number of addressable target devices is 11
* * Works in active power mode only
*
* \section section_i3c_quickstart Quick Start
* Initialize an I3C instance using the \ref cyhal_i3c_init and provide <b>sda</b> (I3C data) and <b>scl</b> (I3C clock) pins.<br>
* By default, this initializes the resource as an I3C controller.<br>
* Configure the behavior (controller/secondary controller/target) and the interface (bus frequency, target address) using the  \ref cyhal_i3c_configure function. <br>
* Attach I2C or I3C targets to the bus using the \ref cyhal_i3c_controller_attach_targets function. <br>
*
* \section section_i3c_snippets Code Snippets
*
* \subsection subsection_i3c_snippet_1 Snippet 1: Controller: Read or write data from or to I2C(static).
* This snippet initializes an I3C resource as controller and read or write data from or to I2C target using static address (7-bit).
*
* Initializing as I3C controller
* \snippet hal_i3c.c snippet_cyhal_controller_read_write
*
* \subsection subsection_i3c_snippet_2 Snippet 2: Controller: Read or write data from or to I3C(dynamic).
* This snippet initializes an I3C resource as controller and read or write data from or to I3C target using dynamic address.
*
* Initializing as I3C controller
* \snippet hal_i3c.c snippet_cyhal_controller_read_write_dynamic
*
* \subsection subsection_i3c_snippet_3 Snippet 3: Use I3C callbacks.
* This snippet initializes an I3C resource and describe how to use callbacks.
*
* Initializing as I3C controller
* \snippet hal_i3c.c snippet_cyhal_controller_callbacks
*
*/


#pragma once

#include "cyhal_hw_types.h"
#include "cyhal_gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "cy_result.h"
#include "cy_pdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** \addtogroup group_hal_results
 *  \{ *//**
 *  \{ @name I3C Results
 */

/** The requested resource type is invalid */
#define CYHAL_I3C_RSLT_ERR_INVALID_PIN                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 0))
/** Can not reach desired data rate */
#define CYHAL_I3C_RSLT_ERR_CAN_NOT_REACH_DR             \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 1))
/** Address size is not correct, should be one or two bytes */
#define CYHAL_I3C_RSLT_ERR_INVALID_ADDRESS_SIZE         \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 2))
/** User buffer is empty (TX and RX). Should be at least TX or RX or both buffers */
#define CYHAL_I3C_RSLT_ERR_TX_RX_BUFFERS_ARE_EMPTY      \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 3))
/** Previous Async operation is pending */
#define CYHAL_I3C_RSLT_ERR_PREVIOUS_ASYNC_PENDING       \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 4))
/** Failed to register I3C pm callback */
#define CYHAL_I3C_RSLT_ERR_PM_CALLBACK                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 5))
/** I3C operation failed with timeout */
#define CYHAL_I3C_RSLT_ERR_OPERATION_TIMEOUT            \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 6))
/** Bad argument provided */
#define CYHAL_I3C_RSLT_ERR_BAD_ARGUMENT                 \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 7))
/** Unsupported by this device */
#define CYHAL_I3C_RSLT_ERR_UNSUPPORTED                  \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 8))
/** No ACK received */
#define CYHAL_I3C_RSLT_ERR_NO_ACK                       \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 9))
/** Command error */
#define CYHAL_I3C_RSLT_ERR_CMD_ERROR                    \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 10))
/** The pointer to RX or TX Buffer is not valid */
#define CYHAL_I3C_RSLT_ERR_BUFFERS_NULL_PTR             \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_ERROR, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 11))

/** Other operation in progress */
#define CYHAL_I3C_RSLT_WARN_DEVICE_BUSY                 \
    (CY_RSLT_CREATE_EX(CY_RSLT_TYPE_WARNING, CY_RSLT_MODULE_ABSTRACTION_HAL, CYHAL_RSLT_MODULE_I3C, 20))

/**
 * \} \}
 */

/** Named define for maximum number of the devices attached to the I3C bus. */
#define CYHAL_I3C_MAX_ATTACHED_DEVICES     (CY_I3C_MAX_DEVS)

/** I3C HAL Operation Modes */
typedef enum
{
    /** Configures I3C for Target operation */
    CYHAL_I3C_MODE_TARGET                  = 0U,
    /** Configures I3C for Secondary Controller-Target operation */
    CYHAL_I3C_MODE_SECONDARY_CONTROLLER    = 1U,
    /** Configures I3C for Main Controller operation */
    CYHAL_I3C_MODE_CONTROLLER              = 2U,
} cyhal_i3c_mode_t;

/** I3C HAL Bus Modes */
typedef enum
{
    /** Indicates only I3C devices are connected to the bus */
    CYHAL_I3C_MODE_PURE                   = 0U,
    /** Indicates I2C and I3C devices are present on the bus */
    CYHAL_I3C_MODE_COMBINED               = 1U,
} cyhal_i3c_bus_mode_t;

/** I3C HAL target device types */
typedef enum
{
    /** Indicates target device is I3C */
    CYHAL_I3C_TARGET_TYPE_I3C             = 0U,
    /** Indicates target device is I2C */
    CYHAL_I3C_TARGET_TYPE_I2C             = 1U,
} cyhal_i3c_target_type_t;

/** Enum of possible output signals from an I3C */
typedef enum
{
    CYHAL_I3C_OUTPUT_TRIGGER_RX_FIFO_LEVEL_REACHED, //!< Output the RX FIFO signal which is triggered when the receive FIFO has more entries than the configured level.
    CYHAL_I3C_OUTPUT_TRIGGER_TX_FIFO_LEVEL_REACHED, //!< Output the TX FIFO signal which is triggered when the transmit FIFO has more empty locations than the configured level.
} cyhal_i3c_output_t;

/** I3C FIFO type */
typedef enum
{
    CYHAL_I3C_FIFO_RX, //!< Set RX FIFO level
    CYHAL_I3C_FIFO_TX, //!< Set TX FIFO level
} cyhal_i3c_fifo_type_t;

/** I3C bus configuration structure */
typedef struct
{
    /** Specifies the mode of I3C controller operation:
    * Controller, Target or Secondary Controller
    */
    cyhal_i3c_mode_t i3c_mode;

    /** Specifies the mode of I3C bus:
    * Pure or Combined
    */
    cyhal_i3c_bus_mode_t i3c_bus_mode;

    /** Desired static address for I3C target device. */
    uint8_t  target_address;

    /** The desired I2C data Rate in Hz. */
    uint32_t i2c_data_rate;

    /** The desired I3C data Rate in Hz. */
    uint32_t i3c_data_rate;

} cyhal_i3c_cfg_t;

/** I3C/I2C device configuration structure */
typedef struct
{
    /** The device 7-bit static address. */
    uint8_t static_address;

    /** The desired device 7-bit dynamic address. */
    uint8_t dynamic_address;

    /** Device type: I3C or I2C */
    cyhal_i3c_target_type_t  device_type;

} cyhal_i3c_device_info_t;

/** Enum to enable/disable/report interrupt cause flags. */
typedef enum
{
    CYHAL_I3C_EVENT_NONE                   = 0,       /**< No event */
    CYHAL_I3C_TARGET_READ_EVENT            = 1 << 1,  /**< Indicates that the target was addressed and the controller wants to read data. */
    CYHAL_I3C_TARGET_WRITE_EVENT           = 1 << 2,  /**< Indicates that the target was addressed and the controller wants to write data. */
    CYHAL_I3C_TARGET_RD_IN_FIFO_EVENT      = 1 << 3,  /**< All target data from the configured Read buffer has been loaded into the TX FIFO. */
    CYHAL_I3C_TARGET_RD_BUF_EMPTY_EVENT    = 1 << 4,  /**< The controller has read all data out of the configured Read buffer. */
    CYHAL_I3C_TARGET_RD_CMPLT_EVENT        = 1 << 5,  /**< Indicates the controller completed reading from the target (set by the controller NAK or Stop) */
    CYHAL_I3C_TARGET_WR_CMPLT_EVENT        = 1 << 6,  /**< Indicates the controller completed writing to the target (set by the controller Stop or Restart)*/
    CYHAL_I3C_TARGET_ERR_EVENT             = 1 << 7,  /**< Indicates the I3C hardware detected an error. */
    CYHAL_I3C_CONTROLLER_WR_IN_FIFO_EVENT  = 1 << 17, /**< All data has been loaded into the TX FIFO. */
    CYHAL_I3C_CONTROLLER_WR_CMPLT_EVENT    = 1 << 18, /**< The controller write is complete.*/
    CYHAL_I3C_CONTROLLER_RD_CMPLT_EVENT    = 1 << 19, /**< The controller read is complete.*/
    CYHAL_I3C_CONTROLLER_ERR_EVENT         = 1 << 20, /**< Indicates the I3C hardware has detected an error. */
} cyhal_i3c_event_t;

/** Enum to enable/disable/report interrupt cause flags. */
typedef enum
{
    CYHAL_I3C_IBI_NONE                   = 0,       /**< No event */
    CYHAL_I3C_IBI_HOTJOIN                = 1 << 1,  /**< IBI Hot join Request  */
    CYHAL_I3C_IBI_SIR                    = 1 << 2,  /**< IBI Target Interrupt Request */
    CYHAL_I3C_IBI_CONTROLLER_REQ         = 1 << 3,  /**< IBI Controller ownership Request*/
} cyhal_i3c_ibi_event_t;

/** Handler for I3C Device interrupts */
typedef void (*cyhal_i3c_event_callback_t)(void *callback_arg, cyhal_i3c_event_t event);
/** Handler for I3C IBI events */
typedef void (*cyhal_i3c_ibi_callback_t)(void *callback_arg, cyhal_i3c_ibi_event_t event, uint16_t address);

/** Initialize the I3C peripheral, and configures its specifieds pins.
 * NOTE: Controller/Target specific functions only work when the block is configured
 * to be in that mode.
 * Use \ref cyhal_i3c_configure() function to change settings after initialized.
 *
 * @param[out] obj The I3C object
 * @param[in]  sda The sda pin
 * @param[in]  scl The scl pin
 * @param[in]  clk The clock to use can be shared, if not provided a new clock will be allocated
 * @return The status of the init request
 */
cy_rslt_t cyhal_i3c_init(cyhal_i3c_t *obj, cyhal_gpio_t sda, cyhal_gpio_t scl, const cyhal_clock_t *clk);

/** Deinitialize the i3c object
 *
 * @param[in,out] obj The i3c object
 */
void cyhal_i3c_free(cyhal_i3c_t *obj);

/** Configure the I3C block. Apply user defined configuration. Refer to \ref cyhal_i3c_cfg_t
 * for details. Call only after succesfull \ref cyhal_i3c_init()
 *
 * @param[in] obj        The I3C object
 * @param[in] cfg        Configuration settings to apply
 * @return The status of the configure request
 */
cy_rslt_t cyhal_i3c_configure(cyhal_i3c_t *obj, const cyhal_i3c_cfg_t *cfg);

/** Attach I2C and I3C targets to the bus and assign dynamic addresses for the I3C targets
 *
 * @param[in]  obj              The I3C object
 * @param[in]  dev_list         The array with I2C and I3C targets configurations
 * @param[in]  dev_list_size    The size of \p dev_list array
 * @return      The status of the request
 */
cy_rslt_t cyhal_i3c_controller_attach_targets(cyhal_i3c_t *obj, cyhal_i3c_device_info_t* dev_list, uint32_t dev_list_size);

/** I3C controller asynchronous transfer
 * This function is non blocking. Use \ref cyhal_i3c_is_busy() API to check status.
 * Execution flow is:
 * 1. If tx_size and rx_size are equal zero or NULL - return error \ref CYHAL_I3C_RSLT_ERR_TX_RX_BUFFERS_ARE_EMPTY
 * 2. If tx_size or rx_size is equal zero or NULL - do separate write or read operation.
 * 3. If tx_size and rx_size are non zero - do write operation, wait for busy clear event
 *    and do read operation.
 * When a write transaction is completed (requested number of bytes are written
 * or error occurred)the busy status is cleared and
 * the \ref CYHAL_I3C_CONTROLLER_WR_CMPLT_EVENT event is generated.
 *
 * When a read transaction is completed (requested number of bytes are read or
 * error occurred) the  busy status is cleared and
 * the \ref CYHAL_I3C_CONTROLLER_RD_CMPLT_EVENT event is generated.
 *
 * @param[in]  obj                 The I3C object
 * @param[in]  address             device address (7-bit)
 * @param[in]  tx                  Pointer to the byte-array of the transmit buffer
 * @param[in]  tx_size             The number of bytes to transmit
 * @param[out] rx                  Pointer to the byte-array of the receive buffer
 * @param[in]  rx_size             The number of bytes to receive
 * @return The status of the controller_transfer_async request
 */
cy_rslt_t cyhal_i3c_controller_transfer_async(cyhal_i3c_t *obj, uint16_t address, const void *tx, size_t tx_size, void *rx, size_t rx_size);

/** Checks if the specified I3C peripheral is use
 * In use - I3C pheripheral 'actively sending/receiving data'
 *
 * @param[in] obj  The I3C peripheral to check
 * @return Indication of whether the I3C is still transmitting
 */
bool cyhal_i3c_is_busy(cyhal_i3c_t *obj);

/** Abort asynchronous transfer
 *
 * This function does not perform any check - that should happen in upper layers.
 * @param[in] obj The I3C object
 * @return The status of the abort_async request
 */
cy_rslt_t cyhal_i3c_controller_abort_async(cyhal_i3c_t *obj);

/**
 * I3C controller blocking write
 *
 * This will write `size` bytes of data from the buffer pointed to by `data`. It will not return
 * until either all of the data has been written, or the timeout has elapsed.
 *
 * @param[in]  obj                 The I3C object
 * @param[in]  address             device address (7-bit)
 * @param[in]  data                Pointer to the byte-array of data send to the target device
 * @param[in]  size                i3c send data size
 * @param[in]  timeout             timeout in milisecond, set this value to 0 if you want to wait forever
 *
 * @return The status of the controller_write request
 */
cy_rslt_t cyhal_i3c_controller_write(cyhal_i3c_t *obj, uint16_t address, const uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * I3C controller blocking read
 *
 * This will read `size` bytes of data into the buffer pointed to by `data`. It will not return
 * until either all of the data has been read, or the timeout has elapsed.
 *
 * @param[in]   obj                 The I3C object
 * @param[in]   address             device address (7-bit)
 * @param[out]  data                Pointer to the byte-array of data receive from the target device
 * @param[in]   size                i3c receive data size
 * @param[in]   timeout             timeout in milisecond, set this value to 0 if you want to wait forever
 *
 * @return The status of the controller_read request
 */
cy_rslt_t cyhal_i3c_controller_read(cyhal_i3c_t *obj, uint16_t address, uint8_t *data, uint16_t size, uint32_t timeout);

/** Perform an i3c write using a block of data stored at the specified memory location
 * This function is blocking.
 *
 * @param[in]  obj                  The I3C object
 * @param[in]  address              device address (7-bit)
 * @param[in]  mem_addr             mem address to store the written data
 * @param[in]  mem_addr_size        number of bytes in the mem address
 * @param[in]  data                 Pointer to the byte-array of data send to the target device
 * @param[in]  size                 i3c controller send data size
 * @param[in]  timeout              timeout in milisecond, set this value to 0 if you want to wait forever
 * @return The status of the write request
 */
cy_rslt_t cyhal_i3c_controller_mem_write(cyhal_i3c_t *obj, uint16_t address, uint16_t mem_addr, uint16_t mem_addr_size, const uint8_t *data, uint16_t size, uint32_t timeout);

/** Perform an i3c read using a block of data stored at the specified memory location
 * This function is blocking.
 *
 * @param[in]  obj                  The I3C object
 * @param[in]  address              device address (7-bit)
 * @param[in]  mem_addr             mem address to store the written data
 * @param[in]  mem_addr_size        number of bytes in the mem address
 * @param[out] data                 Pointer to the byte-array of data receive from the target device
 * @param[in]  size                 i3c controller send data size
 * @param[in]  timeout              timeout in milisecond, set this value to 0 if you want to wait forever
 * @return The status of the read request
 */
cy_rslt_t cyhal_i3c_controller_mem_read(cyhal_i3c_t *obj, uint16_t address, uint16_t mem_addr, uint16_t mem_addr_size, uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * I3C target config write buffer
 * The user needs to setup a new buffer every time (i.e. call \ref cyhal_i3c_target_config_write_buffer and
 * \ref cyhal_i3c_target_config_read_buffer every time the buffer has been used up)
 * Note that after transaction completion the buffer must be configured again. Otherwise, the same buffer is
 * used starting from the point where the controller stopped a previous transaction. For example: The read buffer
 * is configured to be 10 bytes and the controller reads 8 bytes. If the read buffer is not configured again,
 * the next controller read will start from the 9th byte.
 *
 * @param[in]  obj      The I3C object
 * @param[in]  data     Pointer to the byte-array of data send to the controller device
 * @param[in]  size     i3c target send data size
 *
 * @return The status of the target_config_write_buff request
 */
cy_rslt_t cyhal_i3c_target_config_write_buffer(cyhal_i3c_t *obj, const uint8_t *data, uint16_t size);

/**
 * I3C target config read buffer
 * The user needs to setup a new buffer every time (i.e. call \ref cyhal_i3c_target_config_write_buffer and
 * \ref cyhal_i3c_target_config_read_buffer every time the buffer has been used up)
 * Note that after transaction completion the buffer must be configured again. Otherwise, the same buffer is
 * used starting from the point where the controller stopped a previous transaction. For example: The read buffer
 * is configured to be 10 bytes and the controller reads 8 bytes. If the read buffer is not configured again,
 * the next controller read will start from the 9th byte.
 *
 * @param[in]   obj      The I3C object
 * @param[out]  data     Pointer to the byte-array of data receive from controller device
 * @param[in]   size     i3c target receive data size
 *
 * @return The status of the target_config_read_buff request
 */
cy_rslt_t cyhal_i3c_target_config_read_buffer(cyhal_i3c_t *obj, uint8_t *data, uint16_t size);

/** The I3C event callback handler registration
 *
 * @param[in] obj          The I3C object
 * @param[in] callback     The callback handler which will be invoked when an event triggers
 * @param[in] callback_arg Generic argument that will be provided to the callback when called
 */
void cyhal_i3c_register_callback(cyhal_i3c_t *obj, cyhal_i3c_event_callback_t callback, void *callback_arg);

/** The I3C IBI callback handler registration
 *
 * @param[in] obj          The I3C object
 * @param[in] callback     The callback handler which will be invoked when IBI event triggers
 * @param[in] callback_arg Generic argument that will be provided to the callback when called
 */
void cyhal_i3c_register_ibi_callback(cyhal_i3c_t *obj, cyhal_i3c_ibi_callback_t callback, void *callback_arg);

/** Configure and Enable or Disable I3C Interrupt.
 *
 * @param[in] obj            The I3C object
 * @param[in] event          The I3C event type
 * @param[in] intr_priority  The priority for NVIC interrupt events
 * @param[in] enable         True to turn on interrupts, False to turn off
 */
void cyhal_i3c_enable_event(cyhal_i3c_t *obj, cyhal_i3c_event_t event, uint8_t intr_priority, bool enable);

/** Configure and Enable or Disable I3C Interrupt.
 *
 * @param[in] obj            The I3C object
 * @param[in] event          The I3C IBI type
 * @param[in] intr_priority  The priority for NVIC interrupt events
 * @param[in] enable         True to turn on interrupts, False to turn off
 */
void cyhal_i3c_enable_ibi_event(cyhal_i3c_t *obj, cyhal_i3c_ibi_event_t event, uint8_t intr_priority, bool enable);

/** Sets a threshold level for a FIFO that will generate an interrupt and a
 * trigger output. The RX FIFO interrupt and trigger will be activated when
 * the receive FIFO has more entries than the threshold. The TX FIFO interrupt
 * and trigger will be activated when the transmit FIFO has less entries than
 * the threshold.
 *
 * @param[in]  obj        The I3C object
 * @param[in]  type       FIFO type to set level for
 * @param[in]  level      Level threshold to set
 * @return The status of the level set
 * */
cy_rslt_t cyhal_i3c_set_fifo_level(cyhal_i3c_t *obj, cyhal_i3c_fifo_type_t type, uint16_t level);

/** Enables the specified output signal from an I3C.
 *
 * @param[in]  obj        The I3C object
 * @param[in]  output     Which output signal to enable
 * @param[out] source     Pointer to user-allocated source signal object which
 * will be initialized by enable_output. \p source should be passed to
 * (dis)connect_digital functions to (dis)connect the associated endpoints.
 * @return The status of the output enable
 * */
cy_rslt_t cyhal_i3c_enable_output(cyhal_i3c_t *obj, cyhal_i3c_output_t output, cyhal_source_t *source);

/** Disables the specified output signal from an I3C
 *
 * @param[in]  obj        The I3C object
 * @param[in]  output     Which output signal to disable
 * @return The status of the output disable
 * */
cy_rslt_t cyhal_i3c_disable_output(cyhal_i3c_t *obj, cyhal_i3c_output_t output);

/** Initialize the I3C peripheral using a configurator generated configuration struct.
 *
 * @param[in]  obj              The I3C peripheral to configure
 * @param[in]  cfg              Configuration structure generated by a configurator.
 * @return The status of the operation
 */
cy_rslt_t cyhal_i3c_init_cfg(cyhal_i3c_t *obj, const cyhal_i3c_configurator_t *cfg);

/** Returns the number of bytes written by the I3C controller.
 * Calling the \ref cyhal_i3c_target_config_write_buffer API will clear the counter of bytes sent by controller
 *
 * @param[in]  obj          The I3C object
 * @return  The number of bytes written by the I3C controller.
 * */
uint32_t cyhal_i3c_target_readable(cyhal_i3c_t *obj);

/** Returns the number of bytes can be read by the I3C controller.
 * Calling the \ref cyhal_i3c_target_config_read_buffer API will clear the counter of bytes read by controller
 *
 * @param[in]  obj          The I3C object
 * @return  The number of bytes can be read by the I3C controller.
 * */
uint32_t cyhal_i3c_target_writable(cyhal_i3c_t *obj);


#if defined(__cplusplus)
}
#endif

#ifdef CYHAL_I3C_IMPL_HEADER
#include CYHAL_I3C_IMPL_HEADER
#endif /* CYHAL_I3C_IMPL_HEADER */

/** \} group_hal_i3c */
