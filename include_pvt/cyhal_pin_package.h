/***************************************************************************//**
* \file cyhal_pin_package.h
*
* Description:
* Provides definitions for the pinout for each supported device.
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
* \addtogroup group_hal_impl_pin_package Pins
* \ingroup group_hal_impl
* \{
* Definitions for the pinout for each supported device
*/

#pragma once

#include "cyhal_hw_resources.h"
#include "btss_pinmux.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/** Definitions for all of the pins that are bonded out on in the package */
typedef enum
{
    NC = 0xFF, //!< No Connect/Invalid Pin
    HSIOM_SEL_GPIO =  NC, //!< BWC with PSoC for LPM transitions

    BT_GPIO_BASE = 0u, //!< Start of reconfigurable pins
    BT_GPIO_0  = 0u, //!< BT General Purpose I/O 0
    BT_GPIO_2  = 1u, //!< BT General Purpose I/O 2
    BT_GPIO_3  = 2u, //!< BT General Purpose I/O 3
    BT_GPIO_4  = 3u, //!< BT General Purpose I/O 4
    BT_GPIO_5  = 4u, //!< BT General Purpose I/O 5
    BT_GPIO_6  = 5u, //!< BT General Purpose I/O 6
    BT_GPIO_7  = 6u, //!< BT General Purpose I/O 7
    BT_GPIO_8  = 7u, //!< BT General Purpose I/O 8
    BT_GPIO_9  = 8u, //!< BT General Purpose I/O 9
    BT_GPIO_10  = 9u, //!< BT General Purpose I/O 10
    BT_GPIO_11  = 10u, //!< BT General Purpose I/O 11
    BT_GPIO_12  = 11u, //!< BT General Purpose I/O 12
    BT_GPIO_13  = 12u, //!< BT General Purpose I/O 13
    BT_GPIO_14  = 13u, //!< BT General Purpose I/O 14
    BT_GPIO_15  = 14u, //!< BT General Purpose I/O 15
    BT_GPIO_16  = 15u, //!< BT General Purpose I/O 16
    BT_GPIO_17  = 16u, //!< BT General Purpose I/O 17
    BT_HOST_WAKE  = 17u, //!< BT Host Wake
    BT_UART_CTS_N  = 18u, //!< UART clear-to-send (active-low)
    BT_UART_RTS_N  = 19u, //!< UART request-to-send (active-low)
    BT_UART_RXD  = 20u, //!< UART serial data input
    BT_UART_TXD  = 21u, //!< UART serial data output
    DMIC_CK  = 22u, //!< Digital Mic Clock
    DMIC_DQ  = 23u, //!< Digital Mic Data
    LHL_GPIO_2  = 24u, //!< LHL General Purpose I/O 2
    LHL_GPIO_3  = 25u, //!< LHL General Purpose I/O 3
    LHL_GPIO_4  = 26u, //!< LHL General Purpose I/O 4
    LHL_GPIO_5  = 27u, //!< LHL General Purpose I/O 5
    LHL_GPIO_6  = 28u, //!< LHL General Purpose I/O 6
    LHL_GPIO_7  = 29u, //!< LHL General Purpose I/O 7
    LHL_GPIO_8  = 30u, //!< LHL General Purpose I/O 8
    LHL_GPIO_9  = 31u, //!< LHL General Purpose I/O 9
    TDM1_DI  = 32u, //!< TDM1 Data Input
    TDM1_DO  = 33u, //!< TDM1 Data Output
    TDM1_MCK  = 34u, //!< TDM1 Master Clock
    TDM1_SCK  = 35u, //!< TDM1 Slave Clock
    TDM1_WS  = 36u, //!< TDM1 WS
    TDM2_SCK  = 37u, //!< TDM2 Slave Clock
    TDM2_DI  = 38u, //!< TDM2 Data Input
    TDM2_MCK  = 39u, //!< TDM1 Master Clock
    TDM2_DO  = 40u, //!< TDM1 Data Output
    TDM2_WS  = 41u, //!< TDM2 WS
    BT_GPIO_LAST, //!< End of reconfigurable pins

    DIRECT_BASE = 50, //!< Start of direct connection pins
    MIC_P = 50,
    SDIO_DATA_0 = 51,
    SDIO_DATA_1 = 52,
    SDIO_DATA_2 = 53,
    SDIO_DATA_3 = 54,
    SDIO_CLK = 55,
    SDIO_CMD = 56
} cyhal_gpio_t;

/** Pin mux connections */
typedef BTSS_PINMUX_FUNC_LIST_t cyhal_pinmux_t;

/** Map for compatibility with PSoC drivers */
typedef cyhal_pinmux_t en_hsiom_sel_t;

/* Connection type definition */
/** Represents an association between a pin and a resource */
typedef struct
{
    uint8_t                     block_num;      //!< The associated resource block number
    uint8_t                     channel_num;    //!< The associated resource block's channel number
    cyhal_gpio_t                pin;            //!< The GPIO pin
    cyhal_pinmux_t              functionality;  //!< Purpose of the pin
} cyhal_resource_pin_mapping_t;

/** \cond INTERNAL */

// Output enable is directly driven by the connected hardware so we don't set it here
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_I2C_SCL        (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_I2C_SDA        (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_PULL_UP_ENABLE_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )

#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_M_CLK      (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_M_MISO     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_M_MOSI     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_CLK      (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_MISO     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_MOSI     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_SELECT0  (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_SELECT1  (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_SELECT2  (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_SPI_S_SELECT3  (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )

#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_UART_CTS       (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_UART_RTS       (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_UART_RX        (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_SCB_UART_TX        (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )

#define CYHAL_PIN_MAP_DRIVE_MODE_TCPWM_TR_IN        (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_TCPWM_LINE         (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_TCPWM_LINE_COMPL   (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK )

#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_MCK     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_SCK     (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_FSYNC   (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_SD      (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK )
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_RX_MCK     (CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_MCK)
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_RX_SCK     (CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_SCK)
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_RX_FSYNC   (CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_TX_FSYNC)
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_TDM_RX_SD      (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)
#define CYHAL_PIN_MAP_DRIVE_MODE_TDM_SLAVE          (BTSS_PAD_CONFIG_DRIVE_SEL_16MA_MASK | BTSS_PAD_CONFIG_HYSTERESIS_ENABLE_MASK)

#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_CLK           (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_CMD           (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_0        (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_1        (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_2        (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_SDIO_DATA_3        (0)

#define CYHAL_PIN_MAP_DRIVE_MODE_PDM_PCM_CLK        (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_PDM_PCM_DATA       (0)

#define CYHAL_PIN_MAP_DRIVE_MODE_ADCMIC_GPIO_ADC_IN (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_LPCOMP_INP_COMP    (0)
#define CYHAL_PIN_MAP_DRIVE_MODE_LPCOMP_INN_COMP    (0)

extern const cyhal_resource_pin_mapping_t cyhal_pin_map_adcmic_gpio_adc_in[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_lpcomp_inp_comp[4];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_lpcomp_inn_comp[4];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_mic_p[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_dmic_ck[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_dmic_dq[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sw_gpio[49];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_pcm_in[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_i2c_scl[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_i2c_sda[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_clk[6];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_miso[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_mosi[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select0[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select1[5];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select2[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select3[5];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_clk[6];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_miso[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_mosi[8];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_cts[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_rts[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_rx[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_tx[7];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_clk[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_cmd[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_0[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_1[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_2[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_3[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_1[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_2[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_3[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_4[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_5[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_6[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_7[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_8[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_cnt_in[16];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_11[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_12[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_21[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_22[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_23[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_24[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_25[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_26[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_27[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_11[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_12[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_21[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_22[1];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_line[21];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_line_compl[6];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_fsync[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_mck[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_sck[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_sd[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_fsync[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_mck[2];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_sck[3];
extern const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_sd[4];

/** \endcond */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/** \} group_hal_impl */
