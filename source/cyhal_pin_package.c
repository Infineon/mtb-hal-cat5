/***************************************************************************//**
* \file cyhal_pin_package.c
*
* \brief
* Device GPIO HAL header for package
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

#include "cyhal_pin_package.h"


/////////////////////////////////////////////// ADC ////////////////////////////////////////////////

/* Connections for: dc conversion */
const cyhal_resource_pin_mapping_t cyhal_pin_map_adcmic_gpio_adc_in[8] = {
    {0, 0, LHL_GPIO_2, FUNC_NONE},
    {0, 1, LHL_GPIO_3, FUNC_NONE},
    {0, 2, LHL_GPIO_4, FUNC_NONE},
    {0, 3, LHL_GPIO_5, FUNC_NONE},
    {0, 4, LHL_GPIO_6, FUNC_NONE},
    {0, 6, LHL_GPIO_8, FUNC_NONE},
    {0, 7, LHL_GPIO_9, FUNC_NONE},
};

/////////////////////////////////////////////// Comp ////////////////////////////////////////////////

/* Connections for: Vp dc mode */
const cyhal_resource_pin_mapping_t cyhal_pin_map_lpcomp_inp_comp[4] = {
    {0, 1, LHL_GPIO_4, FUNC_NONE},
    {0, 1, LHL_GPIO_5, FUNC_NONE},
    {0, 0, LHL_GPIO_8, FUNC_NONE},
    {0, 0, LHL_GPIO_9, FUNC_NONE}
};

/* Connections for: Vm dc mode */
const cyhal_resource_pin_mapping_t cyhal_pin_map_lpcomp_inn_comp[4] = {
    {0, 1, LHL_GPIO_2, FUNC_NONE},
    {0, 1, LHL_GPIO_3, FUNC_NONE},
    {0, 0, LHL_GPIO_6, FUNC_NONE}
};

///////////////////////////////////////// DMIC/AMIC -> PDM-PCM ///////////////////////////////////////
/* Connections for: (ADC) mic_p */
const cyhal_resource_pin_mapping_t cyhal_pin_map_mic_p[1] = {
    {0, 0, MIC_P, FUNC_NONE}
};

/* Connections for: dmic_ck */
const cyhal_resource_pin_mapping_t cyhal_pin_map_dmic_ck[2] = {
    {0, 0, DMIC_CK, FUNC_DMIC_CK},
    {0, 0, LHL_GPIO_4, FUNC_DMIC_CK}
};

/* Connections for: dmic_dq */
const cyhal_resource_pin_mapping_t cyhal_pin_map_dmic_dq[2] = {
    {0, 0, DMIC_DQ, FUNC_DMIC_DQ},
    {0, 0, LHL_GPIO_5, FUNC_DMIC_DQ}
};

/////////////////////////////////////////////// GPIO ////////////////////////////////////////////////
/* Connections for: sw_gpio clustered */
#if defined (CYW55900)
const cyhal_resource_pin_mapping_t cyhal_pin_map_sw_gpio[3][65] = {
#else
const cyhal_resource_pin_mapping_t cyhal_pin_map_sw_gpio[1][49] = {
#endif // defined (CYW55900)
   {{0, BT_GPIO_0, BT_GPIO_0, FUNC_GPIO_0},
    {0, BT_GPIO_2, BT_GPIO_2, FUNC_GPIO_2},
    {0, BT_GPIO_3, BT_GPIO_3, FUNC_GPIO_3},
    {0, BT_GPIO_4, BT_GPIO_4, FUNC_GPIO_4},
    {0, BT_GPIO_5, BT_GPIO_5, FUNC_GPIO_5},
    {0, BT_GPIO_6, BT_GPIO_6, FUNC_GPIO_6},
    {0, BT_GPIO_7, BT_GPIO_7, FUNC_GPIO_7},
    {0, BT_GPIO_8, BT_GPIO_8, FUNC_A_GPIO_0},
    {0, BT_GPIO_9, BT_GPIO_9, FUNC_A_GPIO_1},
    {0, BT_GPIO_10, BT_GPIO_10, FUNC_A_GPIO_2}, // Shared
    {0, BT_GPIO_10, BT_GPIO_10, FUNC_A_GPIO_5}, // Shared
    {0, BT_GPIO_11, BT_GPIO_11, FUNC_A_GPIO_3}, // shared
    {0, BT_GPIO_11, BT_GPIO_11, FUNC_A_GPIO_6}, // shared
    {0, BT_GPIO_12, BT_GPIO_12, FUNC_A_GPIO_4}, // shared
    {0, BT_GPIO_12, BT_GPIO_12, FUNC_GPIO_7}, // shared
    {0, BT_GPIO_13, BT_GPIO_13, FUNC_A_GPIO_5}, // shared
    {0, BT_GPIO_13, BT_GPIO_13, FUNC_GPIO_6}, // shared
    {0, BT_GPIO_14, BT_GPIO_14, FUNC_A_GPIO_6},
    {0, BT_GPIO_15, BT_GPIO_15, FUNC_A_GPIO_7},
    {0, BT_GPIO_16, BT_GPIO_16, FUNC_A_GPIO_0},
    {0, BT_GPIO_16, BT_GPIO_16, FUNC_GPIO_7},
    {0, BT_GPIO_17, BT_GPIO_17, FUNC_A_GPIO_1},
    {0, BT_GPIO_17, BT_GPIO_17, FUNC_GPIO_7},
    {0, BT_HOST_WAKE, BT_HOST_WAKE, FUNC_GPIO_1},
    {0, BT_UART_CTS_N, BT_UART_CTS_N, FUNC_A_GPIO_1},
    {0, BT_UART_RTS_N, BT_UART_RTS_N, FUNC_A_GPIO_0},
    {0, BT_UART_RXD, BT_UART_RXD, FUNC_GPIO_5},
    {0, BT_UART_TXD, BT_UART_TXD, FUNC_GPIO_4},
    {0, DMIC_CK, DMIC_CK, FUNC_GPIO_0},
    {0, DMIC_DQ, DMIC_DQ, FUNC_GPIO_1},
    {0, LHL_GPIO_2, LHL_GPIO_2, FUNC_B_GPIO_0},
    {0, LHL_GPIO_3, LHL_GPIO_3, FUNC_B_GPIO_1},
    {0, LHL_GPIO_4, LHL_GPIO_4, FUNC_B_GPIO_2},
    {0, LHL_GPIO_5, LHL_GPIO_5, FUNC_B_GPIO_3},
    {0, LHL_GPIO_6, LHL_GPIO_6, FUNC_B_GPIO_4},
    {0, LHL_GPIO_8, LHL_GPIO_8, FUNC_B_GPIO_6},
    {0, LHL_GPIO_9, LHL_GPIO_9, FUNC_B_GPIO_7},
    {0, TDM1_DI, TDM1_DI, FUNC_A_GPIO_5},
    {0, TDM1_DO, TDM1_DO, FUNC_A_GPIO_6},
    {0, TDM1_MCK, TDM1_MCK, FUNC_A_GPIO_4},
    {0, TDM1_SCK, TDM1_SCK, FUNC_A_GPIO_3},
    {0, TDM1_WS, TDM1_WS, FUNC_A_GPIO_2},
    {0, TDM2_SCK, TDM2_SCK, FUNC_A_GPIO_0},
    {0, TDM2_DI, TDM2_DI, FUNC_GPIO_6},
    {0, TDM2_MCK, TDM2_MCK, FUNC_A_GPIO_1}, // shared
    {0, TDM2_MCK, TDM2_MCK, FUNC_A_GPIO_7}, // shared
    {0, TDM2_DO, TDM2_DO, FUNC_GPIO_7},
    {0, TDM2_WS, TDM2_WS, FUNC_A_GPIO_7}}
#if defined (CYW55900)
   ,
   {{0, LHL_IO_0, LHL_IO_0, FUNC_LHL_IO_0},
    {0, LHL_IO_1, LHL_IO_1, FUNC_LHL_IO_1},
    {0, LHL_IO_2, LHL_IO_2, FUNC_LHL_IO_2_ADC_MUX_SEL},
    {0, LHL_IO_3, LHL_IO_3, FUNC_LHL_IO_3_ADC_MUX_SEL},
    {0, LHL_IO_4, LHL_IO_4, FUNC_LHL_IO_4_ADC_MUX_SEL},
    {0, LHL_IO_5, LHL_IO_5, FUNC_LHL_IO_5_ADC_MUX_SEL},
    {0, LHL_IO_6, LHL_IO_6, FUNC_LHL_IO_6_ADC_MUX_SEL},
    {0, LHL_IO_8, LHL_IO_8, FUNC_LHL_IO_8_ADC_MUX_SEL},
    {0, LHL_IO_9, LHL_IO_9, FUNC_LHL_IO_9_ADC_MUX_SEL},
    {0, LHL_IO_10, LHL_IO_10, FUNC_LHL_IO_10}}
   ,
    {{0, WL_GPIO_0, WL_GPIO_0, FUNC_WL_GPIO_0_GPIO},
    {0, WL_GPIO_2, WL_GPIO_2, FUNC_WL_GPIO_2_GPIO},
    {0, WL_GPIO_3, WL_GPIO_3, FUNC_WL_GPIO_3_GPIO},
    {0, WL_GPIO_4, WL_GPIO_4, FUNC_WL_GPIO_4_GPIO},
    {0, WL_GPIO_5, WL_GPIO_5, FUNC_WL_GPIO_5_GPIO},
    {0, WL_GPIO_6, WL_GPIO_6, FUNC_WL_GPIO_6_GPIO},
    {0, SDIO_CMD, SDIO_CMD, FUNC_SDIO_CMD_GPIO},
    {0, SDIO_DATA_0, SDIO_DATA_0, FUNC_SDIO_DATA_0_GPIO},
    {0, SDIO_DATA_1, SDIO_DATA_1, FUNC_SDIO_DATA_1_GPIO},
    {0, SDIO_DATA_2, SDIO_DATA_2, FUNC_SDIO_DATA_2_GPIO},
    {0, SDIO_DATA_3, SDIO_DATA_3, FUNC_SDIO_DATA_3_GPIO},
    {0, RFSW_CTRL_6, RFSW_CTRL_6, FUNC_RFSW_CTRL_6_GPIO},
    {0, RFSW_CTRL_7, RFSW_CTRL_7, FUNC_RFSW_CTRL_7_GPIO}}
#endif // defined (CYW55900)
};

/////////////////////////////////////////////// PCM ////////////////////////////////////////////////
/* Connections for: pcm_in */
const cyhal_resource_pin_mapping_t cyhal_pin_map_pcm_in[1] = {
    {0, 0, BT_GPIO_17, FUNC_PCM_IN},
};

///////////////////////////////////////////// SCB I2C ////////////////////////////////////////////////
/* Connections for: i2c_scl */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_i2c_scl[8] = {
    {0, 0, BT_GPIO_12, FUNC_SCB0_SCL},
    {0, 0, BT_GPIO_5, FUNC_SCB0_SCL},
    {0, 0, LHL_GPIO_6, FUNC_SCB0_SCL},
    {1, 0, TDM2_WS, FUNC_SCB1_SCL},
    {1, 0, BT_GPIO_3, FUNC_SCB1_SCL},
    {1, 0, BT_GPIO_16, FUNC_SCB1_SCL},
    {2, 0, TDM2_MCK, FUNC_SCB2_SCL},
    {2, 0, BT_GPIO_7, FUNC_SCB2_SCL}
};

/* Connections for: i2c_sda */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_i2c_sda[8] = {
    {0, 0, BT_GPIO_13, FUNC_SCB0_SDA},
    {0, 0, BT_GPIO_4, FUNC_SCB0_SDA},
    {1, 0, TDM2_SCK, FUNC_SCB1_SDA},
    {1, 0, BT_GPIO_2, FUNC_SCB1_SDA},
    {1, 0, BT_GPIO_17, FUNC_SCB1_SDA},
    {2, 0, TDM2_SCK, FUNC_SCB2_SDA},
    {2, 0, BT_GPIO_6, FUNC_SCB2_SDA},
};

/////////////////////////////////////////////SCB SPI ////////////////////////////////////////////////
/* Connections for: spi_clk */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_clk[6] = {
    {0, 0, BT_UART_RTS_N, FUNC_SCB0_SPI_CLK},
    {0, 0, BT_GPIO_9, FUNC_SCB0_SPI_CLK},
    {1, 0, BT_GPIO_17, FUNC_SCB1_SPI_CLK},
    {1, 0, LHL_GPIO_6, FUNC_SCB1_SPI_CLK},
    {2, 0, DMIC_CK, FUNC_SCB2_SPI_CLK}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_clk[6] = {
    {0, 0, BT_UART_RTS_N, FUNC_SCB0_SPI_CLK},
    {0, 0, BT_GPIO_9, FUNC_SCB0_SPI_CLK},
    {1, 0, BT_GPIO_17, FUNC_SCB1_SPI_CLK},
    {1, 0, LHL_GPIO_6, FUNC_SCB1_SPI_CLK},
    {2, 0, DMIC_CK, FUNC_SCB2_SPI_CLK}
};

/* Connections for: spi_miso */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_miso[8] = {
    {0, 0, BT_UART_TXD, FUNC_SCB0_SPI_MISO},
    {0, 0, BT_GPIO_11, FUNC_SCB0_SPI_MISO},
    {0, 0, LHL_GPIO_5, FUNC_SCB0_SPI_MISO},
    {1, 0, TDM2_DI, FUNC_SCB1_SPI_MISO},
    {1, 0, BT_GPIO_15, FUNC_SCB1_SPI_MISO},
    {1, 0, BT_GPIO_2, FUNC_SCB1_SPI_MISO},
    {1, 0, LHL_GPIO_9, FUNC_SCB1_SPI_MISO},
    {2, 0, TDM2_DO, FUNC_SCB2_SPI_MISO}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_miso[8] = {
    {0, 0, BT_UART_TXD, FUNC_SCB0_SPI_MISO},
    {0, 0, BT_GPIO_11, FUNC_SCB0_SPI_MISO},
    {0, 0, LHL_GPIO_5, FUNC_SCB0_SPI_MISO},
    {1, 0, TDM2_DI, FUNC_SCB1_SPI_MISO},
    {1, 0, BT_GPIO_15, FUNC_SCB1_SPI_MISO},
    {1, 0, BT_GPIO_2, FUNC_SCB1_SPI_MISO},
    {1, 0, LHL_GPIO_9, FUNC_SCB1_SPI_MISO},
    {2, 0, TDM2_DO, FUNC_SCB2_SPI_MISO}
};

/* Connections for: spi_mosi */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_mosi[8] = {
    {0, 0, BT_UART_RXD, FUNC_SCB0_SPI_MOSI},
    {0, 0, BT_GPIO_10, FUNC_SCB0_SPI_MOSI},
    {0, 0, LHL_GPIO_4, FUNC_SCB0_SPI_MOSI},
    {1, 0, TDM2_DO, FUNC_SCB1_SPI_MOSI},
    {1, 0, BT_GPIO_14, FUNC_SCB1_SPI_MOSI},
    {1, 0, BT_GPIO_3, FUNC_SCB1_SPI_MOSI},
    {1, 0, LHL_GPIO_8, FUNC_SCB1_SPI_MOSI},
    {2, 0, TDM2_DI, FUNC_SCB2_SPI_MOSI}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_m_mosi[8] = {
    {0, 0, BT_UART_RXD, FUNC_SCB0_SPI_MOSI},
    {0, 0, BT_GPIO_10, FUNC_SCB0_SPI_MOSI},
    {0, 0, LHL_GPIO_4, FUNC_SCB0_SPI_MOSI},
    {1, 0, TDM2_DO, FUNC_SCB1_SPI_MOSI},
    {1, 0, BT_GPIO_14, FUNC_SCB1_SPI_MOSI},
    {1, 0, BT_GPIO_3, FUNC_SCB1_SPI_MOSI},
    {1, 0, LHL_GPIO_8, FUNC_SCB1_SPI_MOSI},
    {2, 0, TDM2_DI, FUNC_SCB2_SPI_MOSI}
};

/* Connections for: spi_cs */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select0[7] = {
    {0, 0, BT_UART_CTS_N, FUNC_SCB0_SPI_SELECT0},
    {0, 0, BT_GPIO_12, FUNC_SCB0_SPI_SELECT0},
    {0, 0, LHL_GPIO_6, FUNC_SCB0_SPI_SELECT0},
    {1, 0, TDM2_WS, FUNC_SCB1_SPI_SELECT0},
    {1, 0, BT_GPIO_16, FUNC_SCB1_SPI_SELECT0},
    {2, 0, DMIC_DQ, FUNC_SCB2_SPI_SEL0}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select1[5] = {
    {0, 0, BT_GPIO_5, FUNC_SCB0_SPI_SELECT1},
    {0, 0, BT_GPIO_17, FUNC_SCB0_SPI_SELECT1},
    {0, 0, TDM2_MCK, FUNC_SCB0_SPI_SEL1},
    {1, 0, BT_GPIO_4, FUNC_SCB1_SPI_SELECT1},
    {2, 0, TDM2_SCK, FUNC_SCB2_SPI_SEL1}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select2[7] = {
    {0, 0, TDM1_MCK, FUNC_SCB0_SPI_SELECT2},
    {0, 0, BT_GPIO_4, FUNC_SCB0_SPI_SELECT2},
    {0, 0, BT_GPIO_16, FUNC_SCB0_SPI_SELECT2},
    {1, 0, TDM1_DO, FUNC_SCB1_SPI_SELECT2},
    {1, 0, BT_GPIO_7, FUNC_SCB1_SPI_SELECT2},
    {1, 0, BT_GPIO_3, FUNC_SCB1_SPI_SELECT2},
    {2, 0, TDM2_MCK, FUNC_SCB2_SPI_SEL2}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_spi_s_select3[5] = {
    {0, 0, BT_GPIO_2, FUNC_SCB0_SPI_SELECT3},
    {1, 0, TDM1_DI, FUNC_SCB1_SPI_SELECT3},
    {1, 0, BT_GPIO_6, FUNC_SCB1_SPI_SELECT3},
    {1, 0, BT_GPIO_2, FUNC_SCB1_SPI_SELECT3},
    {2, 0, BT_GPIO_0, FUNC_SCB2_SPI_SEL3}
};

/////////////////////////////////////////////SCB UART ///////////////////////////////////////////////
/* Connections for: uart_cts */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_cts[7] = {
    {0, 0, BT_UART_CTS_N, FUNC_SCB0_UART_CTS},
    {1, 0, TDM2_DI, FUNC_SCB1_UART_CTS},
    {1, 0, BT_GPIO_10, FUNC_SCB1_UART_CTS},
    {1, 0, BT_GPIO_4, FUNC_SCB1_UART_CTS},
    {1, 0, LHL_GPIO_4, FUNC_SCB1_UART_CTS},
    {2, 0, BT_GPIO_14, FUNC_SCB2_UART_CTS},
    {2, 0, LHL_GPIO_6, FUNC_SCB2_UART_CTS}
};

/* Connections for: uart_rts */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_rts[7] = {
    {0, 0, BT_UART_RTS_N, FUNC_SCB0_UART_RTS},
    {1, 0, TDM2_DO, FUNC_SCB1_UART_RTS},
    {1, 0, BT_GPIO_11, FUNC_SCB1_UART_RTS},
    {1, 0, BT_GPIO_5, FUNC_SCB1_UART_RTS},
    {1, 0, LHL_GPIO_5, FUNC_SCB1_UART_RTS},
    {2, 0, BT_GPIO_15, FUNC_SCB2_UART_RTS}
};

/* Connections for: uart_rx */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_rx[7] = {
    {0, 0, BT_UART_RXD, FUNC_SCB0_UART_RXD},
    {1, 0, TDM2_WS, FUNC_SCB1_UART_RXD},
    {1, 0, BT_GPIO_9, FUNC_SCB1_UART_RXD},
    {1, 0, BT_GPIO_2, FUNC_SCB1_UART_RXD},
    {1, 0, LHL_GPIO_3, FUNC_SCB1_UART_RXD},
    {2, 0, BT_GPIO_12, FUNC_SCB2_UART_RXD},
    {2, 0, LHL_GPIO_8, FUNC_SCB2_UART_RXD}
};

/* Connections for: uart_tx */
const cyhal_resource_pin_mapping_t cyhal_pin_map_scb_uart_tx[7] = {
    {0, 0, BT_UART_TXD, FUNC_SCB0_UART_TXD},
    {1, 0, TDM2_SCK, FUNC_SCB1_UART_TXD},
    {1, 0, BT_GPIO_8, FUNC_SCB1_UART_TXD},
    {1, 0, BT_GPIO_3, FUNC_SCB1_UART_TXD},
    {1, 0, LHL_GPIO_2, FUNC_SCB1_UART_TXD},
    {2, 0, BT_GPIO_13, FUNC_SCB2_UART_TXD},
    {2, 0, LHL_GPIO_9, FUNC_SCB2_UART_TXD}
};

///////////////////////////////////////////// SDIO ////////////////////////////////////////////////
/* Connections for: sdio_clk */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_clk[1] = {
    {0, 0, SDIO_CLK, FUNC_NONE}
};

/* Connections for: sdio_cmd */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_cmd[1] = {
    {0, 0, SDIO_CMD, FUNC_NONE}
};

/* Connections for: sdio_data_0 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_0[1] = {
    {0, 0, SDIO_DATA_0, FUNC_NONE}
};

/* Connections for: sdio_data_1 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_1[1] = {
    {0, 0, SDIO_DATA_1, FUNC_NONE}
};

/* Connections for: sdio_data_2 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_2[1] = {
    {0, 0, SDIO_DATA_2, FUNC_NONE}
};

/* Connections for: sdio_data_3 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_sdio_data_3[1] = {
    {0, 0, SDIO_DATA_3, FUNC_NONE}
};

///////////////////////////////////////////// TCPWM ////////////////////////////////////////////////
/* Connections for: tcpwm_tr_all_1 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_1[3] = {
    {0, 0, BT_GPIO_13, FUNC_TCPWM_TR_ALL_1},
    {0, 0, BT_GPIO_8, FUNC_TCPWM_TR_ALL_1},
    {0, 0, LHL_GPIO_2, FUNC_TCPWM_TR_ALL_1}
};

/* Connections for: tcpwm_tr_all_2 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_2[3] = {
    {0, 1, BT_GPIO_12, FUNC_TCPWM_TR_ALL_2},
    {0, 1, BT_GPIO_9, FUNC_TCPWM_TR_ALL_2},
    {0, 1, LHL_GPIO_3, FUNC_TCPWM_TR_ALL_2}
};

/* Connections for: tcpwm_tr_all_3 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_3[3] = {
    {0, 2, BT_GPIO_11, FUNC_TCPWM_TR_ALL_3},
    {0, 2, BT_GPIO_14, FUNC_TCPWM_TR_ALL_3},
    {0, 2, LHL_GPIO_5, FUNC_TCPWM_TR_ALL_3}
};

/* Connections for: tcpwm_tr_all_4 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_4[3] = {
    {0, 3, BT_GPIO_10, FUNC_TCPWM_TR_ALL_4},
    {0, 3, BT_GPIO_15, FUNC_TCPWM_TR_ALL_4},
    {0, 3, BT_GPIO_4, FUNC_TCPWM_TR_ALL_4}
};

/* Connections for: tcpwm_tr_all_5 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_5[1] = {
    {0, 4, BT_GPIO_0, FUNC_TCPWM_TR_ALL_5}
};

/* Connections for: tcpwm_tr_all_6 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_6[1] = {
    {0, 5, BT_HOST_WAKE, FUNC_TCPWM_TR_ALL_6}
};

/* Connections for: tcpwm_tr_all_7 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_7[1] = {
    {0, 6, BT_GPIO_2, FUNC_TCPWM_TR_ALL_7}
};

/* Connections for: tcpwm_tr_all_8 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_8[1] = {
    {0, 7, BT_GPIO_3, FUNC_TCPWM_TR_ALL_8}
};

/* Connections for: tcpwm_tr_all_cnt_in */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_tr_all_cnt_in[16] = {
    {0, 0, BT_GPIO_13, FUNC_TCPWM_TR_ALL_1},
    {0, 0, BT_GPIO_8, FUNC_TCPWM_TR_ALL_1},
    {0, 0, LHL_GPIO_2, FUNC_TCPWM_TR_ALL_1},
    {0, 1, BT_GPIO_12, FUNC_TCPWM_TR_ALL_2},
    {0, 1, BT_GPIO_9, FUNC_TCPWM_TR_ALL_2},
    {0, 1, LHL_GPIO_3, FUNC_TCPWM_TR_ALL_2},
    {0, 2, BT_GPIO_11, FUNC_TCPWM_TR_ALL_3},
    {0, 2, BT_GPIO_14, FUNC_TCPWM_TR_ALL_3},
    {0, 2, LHL_GPIO_5, FUNC_TCPWM_TR_ALL_3},
    {0, 3, BT_GPIO_10, FUNC_TCPWM_TR_ALL_4},
    {0, 3, BT_GPIO_15, FUNC_TCPWM_TR_ALL_4},
    {0, 3, LHL_GPIO_4, FUNC_TCPWM_TR_ALL_4},
    {0, 4, BT_GPIO_0, FUNC_TCPWM_TR_ALL_5},
    {0, 5, BT_HOST_WAKE, FUNC_TCPWM_TR_ALL_6},
    {0, 6, BT_GPIO_2, FUNC_TCPWM_TR_ALL_7},
    {0, 7, BT_GPIO_3, FUNC_TCPWM_TR_ALL_8}
};

/* Connections for: tcpwm_out_11 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_11[3] = {
    {0, 0, TDM2_SCK, FUNC_TCPWM_OUT_11},
    {0, 0, BT_GPIO_4, FUNC_TCPWM_OUT_11},
    {0, 0, BT_GPIO_17, FUNC_TCPWM_OUT_11}
};

/* Connections for: tcpwm_out_12 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_12[3] = {
    {0, 1, TDM2_WS, FUNC_TCPWM_OUT_12},
    {0, 1, BT_GPIO_5, FUNC_TCPWM_OUT_12},
    {0, 1, BT_GPIO_16, FUNC_TCPWM_OUT_12}
};

/* Connections for: tcpwm_out_21 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_21[3] = {
    {1, 0, TDM2_DI, FUNC_TCPWM_OUT_21},
    {1, 0, BT_GPIO_15, FUNC_TCPWM_OUT_21},
    {1, 0, BT_GPIO_6, FUNC_TCPWM_OUT_21}
};

/* Connections for: tcpwm_out_22 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_22[3] = {
    {1, 1, TDM2_DO, FUNC_TCPWM_OUT_22},
    {1, 1, BT_GPIO_14, FUNC_TCPWM_OUT_22},
    {1, 1, BT_GPIO_7, FUNC_TCPWM_OUT_22}
};

/* Connections for: tcpwm_out_23 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_23[2] = {
    {1, 2, TDM1_WS, FUNC_TCPWM_OUT_23},
    {1, 2, LHL_GPIO_6, FUNC_TCPWM_OUT_23}
};

/* Connections for: tcpwm_out_24 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_24[2] = {
    {1, 3, TDM1_SCK, FUNC_TCPWM_OUT_24},
};

/* Connections for: tcpwm_out_25 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_25[2] = {
    {1, 4, TDM1_MCK, FUNC_TCPWM_OUT_25},
    {1, 4, LHL_GPIO_8, FUNC_TCPWM_OUT_25}
};

/* Connections for: tcpwm_out_26 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_26[2] = {
    {1, 5, TDM1_DI, FUNC_TCPWM_OUT_26},
    {1, 5, LHL_GPIO_9, FUNC_TCPWM_OUT_26}
};

/* Connections for: tcpwm_out_27 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_out_27[1] = {
    {1, 6, TDM1_DO, FUNC_TCPWM_OUT_27}
};


/* Connections for: tcpwm_comp_out_11 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_11[1] = {
    {0, 0, LHL_GPIO_6, FUNC_TCPWM_COMP_OUT_11}
};

/* Connections for: tcpwm_comp_out_12 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_12[2] = {
    {0, 1, BT_GPIO_16, FUNC_TCPWM_COMP_OUT_12},
};

/* Connections for: tcpwm_comp_out_21 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_21[2] = {
    {1, 0, BT_GPIO_17, FUNC_TCPWM_COMP_OUT_21},
    {1, 0, LHL_GPIO_8, FUNC_TCPWM_COMP_OUT_21}
};

/* Connections for: tcpwm_comp_out_22 */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_comp_out_22[1] = {
    {1, 1, LHL_GPIO_9, FUNC_TCPWM_COMP_OUT_22}
};

// The indexes appear to be count group number followed by the counter number. 
// E.g. CNT_GRP0 and CNT0 = "11". CNT_GRP1 and CNT0 = "21".
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_line[21] = {
    {0, 0, TDM2_SCK,   FUNC_TCPWM_OUT_11},
    {0, 0, BT_GPIO_4,  FUNC_TCPWM_OUT_11},
    {0, 0, BT_GPIO_17, FUNC_TCPWM_OUT_11},
    {0, 1, TDM2_WS,    FUNC_TCPWM_OUT_12},
    {0, 1, BT_GPIO_5,  FUNC_TCPWM_OUT_12},
    {0, 1, BT_GPIO_16, FUNC_TCPWM_OUT_12},
    {1, 0, TDM2_WS,    FUNC_TCPWM_OUT_21},
    {1, 0, BT_GPIO_15, FUNC_TCPWM_OUT_21},
    {1, 0, BT_GPIO_6,  FUNC_TCPWM_OUT_21},
    {1, 1, TDM2_DO,    FUNC_TCPWM_OUT_22},
    {1, 1, BT_GPIO_14, FUNC_TCPWM_OUT_22},
    {1, 1, BT_GPIO_7,  FUNC_TCPWM_OUT_22},
    {1, 2, TDM1_WS,    FUNC_TCPWM_OUT_23},
    {1, 2, LHL_GPIO_6, FUNC_TCPWM_OUT_23},
    {1, 3, TDM1_SCK,   FUNC_TCPWM_OUT_24},
    {1, 4, TDM1_MCK,   FUNC_TCPWM_OUT_25},
    {1, 4, LHL_GPIO_8, FUNC_TCPWM_OUT_25},
    {1, 5, TDM1_DI,    FUNC_TCPWM_OUT_26},
    {1, 5, LHL_GPIO_9, FUNC_TCPWM_OUT_26},
    {1, 6, TDM1_DO,    FUNC_TCPWM_OUT_27}
};
const cyhal_resource_pin_mapping_t cyhal_pin_map_tcpwm_line_compl[6] = {
    {0, 0, LHL_GPIO_6, FUNC_TCPWM_COMP_OUT_11},
    {0, 1, BT_GPIO_16, FUNC_TCPWM_COMP_OUT_12},
    {1, 0, BT_GPIO_17, FUNC_TCPWM_COMP_OUT_21},
    {1, 0, LHL_GPIO_8, FUNC_TCPWM_COMP_OUT_21},
    {1, 1, LHL_GPIO_9, FUNC_TCPWM_COMP_OUT_22}
};

/////////////////////////////////////////////// TDM ///////////////////////////////////////////////
/* Connections for: tdm_tdm_rx_fsync */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_fsync[3] = {
    {0, 0, TDM1_WS, FUNC_TDM1_WS},
    {0, 1, TDM2_WS, FUNC_TDM2_WS},
    {0, 1, BT_GPIO_12, FUNC_TDM2_WS}
};

/* Connections for: tdm_tdm_rx_mck */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_mck[2] = {
    {0, 0, TDM1_MCK, FUNC_TDM1_MCK},
    {0, 1, TDM2_MCK, FUNC_TDM2_MCK}
};

/* Connections for: tdm_tdm_rx_sck */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_sck[3] = {
    {0, 0, TDM1_SCK, FUNC_TDM1_SCK},
    {0, 1, TDM2_SCK, FUNC_TDM2_SCK},
    {0, 1, BT_GPIO_13, FUNC_TDM2_SCK}
};

/* Connections for: tdm_tdm_rx_sd */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_rx_sd[3] = {
    {0, 0, TDM1_DI, FUNC_TDM1_DI},
    {0, 1, TDM2_DI, FUNC_TDM2_DI},
    {0, 1, BT_GPIO_11, FUNC_TDM2_DI}
};

/* Connections for: tdm_tdm_tx_fsync */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_fsync[3] = {
    {0, 0, TDM1_WS, FUNC_TDM1_WS},
    {0, 1, TDM2_WS, FUNC_TDM2_WS},
    {0, 1, BT_GPIO_12, FUNC_TDM2_WS}
};

/* Connections for: tdm_tdm_tx_mck */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_mck[2] = {
    {0, 0, TDM1_MCK, FUNC_TDM1_MCK},
    {0, 1, TDM2_MCK, FUNC_TDM2_MCK}
};

/* Connections for: tdm_tdm_tx_sck */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_sck[3] = {
    {0, 0, TDM1_SCK, FUNC_TDM1_SCK},
    {0, 1, TDM2_SCK, FUNC_TDM2_SCK},
    {0, 1, BT_GPIO_13, FUNC_TDM2_SCK}
};

/* Connections for: tdm_tdm_tx_sd */
const cyhal_resource_pin_mapping_t cyhal_pin_map_tdm_tdm_tx_sd[4] = {
    {0, 0, TDM1_DO, FUNC_TDM1_DO},
    {0, 1, TDM2_DO, FUNC_TDM2_DO},
    {0, 1, BT_GPIO_10, FUNC_TDM2_DO},
    {0, 1, BT_GPIO_16, FUNC_TDM2_DO}
};
