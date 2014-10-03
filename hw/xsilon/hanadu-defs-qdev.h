/*
 * Bitmask and offset defintions for all the Hanadu Registers.
 * Processor Interface Version: 1
 * 
 * THIS FILE IS AUTOMATICALLY GENERATED USING THE REGISTER MAP PROJECT.
 * PLEASE DO NOT EDIT.
 *
 * Martin Townsend >martin.townsend@xsilon.com<
 * Copyright Xsilon Limited 2014.
 */

#ifndef _XSILON_REGMAP_AUTOGENERATED_QEMU_HANADU_DEVICE_H
#define _XSILON_REGMAP_AUTOGENERATED_QEMU_HANADU_DEVICE_H

#include <stdint.h>
#include "exec/hwaddr.h"

/* __________________________________________________ Register Access Prototypes
 */

uint64_t
han_trxm_mem_region_read(void *opaque, hwaddr addr, unsigned size) __attribute__((weak));
void
han_trxm_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) __attribute__((weak));


uint64_t
han_mac_mem_region_read(void *opaque, hwaddr addr, unsigned size) __attribute__((weak));
void
han_mac_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) __attribute__((weak));


uint64_t
han_pwr_mem_region_read(void *opaque, hwaddr addr, unsigned size) __attribute__((weak));
void
han_pwr_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) __attribute__((weak));


uint64_t
han_afe_mem_region_read(void *opaque, hwaddr addr, unsigned size) __attribute__((weak));
void
han_afe_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) __attribute__((weak));


uint64_t
han_hwvers_mem_region_read(void *opaque, hwaddr addr, unsigned size) __attribute__((weak));
void
han_hwvers_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) __attribute__((weak));



/* ____________________________________________ HW Block Register Map Structures
 */

/* ___________________ Transceiver
 */
struct han_regmap_trxm
{
    /* 0  */ uint32_t trx_tx_ctrl;
    /* 1  */ uint32_t trx_tx_hwbuf0_rc_psdulen;
    /* 2  */ uint32_t trx_tx_hwbuf0_xtra;
    /* 3  */ uint32_t trx_tx_hwbuf1_rc_psdulen;
    /* 4  */ uint32_t trx_tx_hwbuf1_xtra;
    /* 5  */ uint32_t trx_rx_control;
    /* 6  */ uint32_t trx_rx_not_used1;
    /* 7  */ uint32_t trx_rx_hdr_rep_rate;
    /* 8  */ uint32_t trx_rx_thresholds;
    /* 9  */ uint32_t trx_rx_resets;
    /* 10 */ uint32_t trx_rx_next_membank_to_read;
    /* 11 */ uint32_t trx_rx_hdr_crc_pass_count;
    /* 12 */ uint32_t trx_rx_hdr_crc_fail_count;
    /* 13 */ uint32_t trx_rx_pay_crc_pass_count;
    /* 14 */ uint32_t trx_rx_pay_crc_fail_count;
    /* 15 */ uint32_t trx_rx_buf0_rc_psdulen;
    /* 16 */ uint32_t trx_rx_buf0_xtra;
    /* 17 */ uint32_t trx_rx_rx_buf0_crc;
    /* 18 */ uint32_t trx_rx_buf1_rc_psdulen;
    /* 19 */ uint32_t trx_rx_buf1_xtra;
    /* 20 */ uint32_t trx_rx_rx_buf1_crc;
    /* 21 */ uint32_t trx_rx_buf2_rc_psdulen;
    /* 22 */ uint32_t trx_rx_buf2_xtra;
    /* 23 */ uint32_t trx_rx_rx_buf2_crc;
    /* 24 */ uint32_t trx_rx_buf3_rc_psdulen;
    /* 25 */ uint32_t trx_rx_buf3_xtra;
    /* 26 */ uint32_t trx_rx_rx_buf3_crc;
    /* 27 */ uint32_t dummy27;
    /* 28 */ uint32_t trx_rx_fifo_levels;
    /* 29 */ uint32_t trx_hard_reset;
    /* 30 */ uint32_t trx_pga_gain_cca_flags;
    /* 31 */ uint32_t trx_membank_fifo_flags_rssi_tx_power;
};

/* ___________________ Medium Access Controller
 */
struct han_regmap_mac
{
    /* 0  */ uint32_t mac_traf_mon_ctrl;
    /* 1  */ uint32_t mac_traf_mon_clear;
    /* 2  */ uint32_t dummy2;
    /* 3  */ uint32_t dummy3;
    /* 4  */ uint32_t dummy4;
    /* 5  */ uint32_t dummy5;
    /* 6  */ uint32_t dummy6;
    /* 7  */ uint32_t dummy7;
    /* 8  */ uint32_t dummy8;
    /* 9  */ uint32_t dummy9;
    /* 10 */ uint32_t mac_traf_mon_psdu_avg;
    /* 11 */ uint32_t mac_traf_mon_repcode_avg;
    /* 12 */ uint32_t mac_traf_mon_hdr_crc_pass;
    /* 13 */ uint32_t mac_traf_mon_hdr_crc_fail;
    /* 14 */ uint32_t mac_traf_mon_payload_crc_fail;
    /* 15 */ uint32_t mac_traf_mon_cca;
    /* 16 */ uint32_t mac_traf_mon_pkt_time;
    /* 17 */ uint32_t mac_traf_mon_hp;
    /* 18 */ uint32_t mac_filter_ea_lower;
    /* 19 */ uint32_t mac_filter_ea_upper;
    /* 20 */ uint32_t mac_filter_sa;
    /* 21 */ uint32_t mac_filter_pan_id;
    /* 22 */ uint32_t mac_filter_ctrl;
    /* 23 */ uint32_t dummy23;
    /* 24 */ uint32_t dummy24;
    /* 25 */ uint32_t dummy25;
    /* 26 */ uint32_t dummy26;
    /* 27 */ uint32_t dummy27;
    /* 28 */ uint32_t dummy28;
    /* 29 */ uint32_t dummy29;
    /* 30 */ uint32_t dummy30;
    /* 31 */ uint32_t dummy31;
};

/* ___________________ Power Up Controller
 */
struct han_regmap_pwr
{
    /* 0  */ uint32_t dummy0;
    /* 1  */ uint32_t pup_kick_off_sw_ad9865_spi_write;
    /* 2  */ uint32_t pup_kick_off_sw_ad9865_spi_read;
    /* 3  */ uint32_t pup_tx_rx_select;
    /* 4  */ uint32_t pup_pga_mux_select;
    /* 5  */ uint32_t pup_tx_tongen_mux_select;
    /* 6  */ uint32_t pup_afe_reset;
    /* 7  */ uint32_t pup_dcm_reset;
    /* 8  */ uint32_t dummy8;
    /* 9  */ uint32_t dummy9;
    /* 10 */ uint32_t dummy10;
    /* 11 */ uint32_t dummy11;
    /* 12 */ uint32_t dummy12;
    /* 13 */ uint32_t dummy13;
    /* 14 */ uint32_t pup_leds;
    /* 15 */ uint32_t dummy15;
    /* 16 */ uint32_t dummy16;
    /* 17 */ uint32_t dummy17;
    /* 18 */ uint32_t pup_dips1;
    /* 19 */ uint32_t pup_dips2;
    /* 20 */ uint32_t dummy20;
    /* 21 */ uint32_t dummy21;
    /* 22 */ uint32_t dummy22;
    /* 23 */ uint32_t dummy23;
    /* 24 */ uint32_t dummy24;
    /* 25 */ uint32_t dummy25;
    /* 26 */ uint32_t dummy26;
    /* 27 */ uint32_t dummy27;
    /* 28 */ uint32_t dummy28;
    /* 29 */ uint32_t dummy29;
    /* 30 */ uint32_t dummy30;
    /* 31 */ uint32_t dummy31;
};

/* ___________________ AFE Controller
 */
struct han_regmap_afe
{
    /* 0  */ uint32_t afe_agc_fixed_gain;
    /* 1  */ uint32_t afe_status;
    /* 2  */ uint32_t afe_ad9865_write_reg_0_3;
    /* 3  */ uint32_t afe_ad9865_write_reg_4_7;
    /* 4  */ uint32_t afe_ad9865_write_reg_8_11;
    /* 5  */ uint32_t afe_ad9865_write_reg_12_15;
    /* 6  */ uint32_t afe_ad9865_write_reg_16_19;
    /* 7  */ uint32_t afe_ad9865_read_reg_0_3;
    /* 8  */ uint32_t afe_ad9865_read_reg_4_7;
    /* 9  */ uint32_t afe_ad9865_read_reg_8_11;
    /* 10 */ uint32_t afe_ad9865_read_reg_12_15;
    /* 11 */ uint32_t afe_ad9865_read_reg_16_19;
    /* 12 */ uint32_t dummy12;
    /* 13 */ uint32_t dummy13;
    /* 14 */ uint32_t dummy14;
    /* 15 */ uint32_t dummy15;
    /* 16 */ uint32_t dummy16;
    /* 17 */ uint32_t dummy17;
    /* 18 */ uint32_t dummy18;
    /* 19 */ uint32_t dummy19;
    /* 20 */ uint32_t dummy20;
    /* 21 */ uint32_t dummy21;
    /* 22 */ uint32_t dummy22;
    /* 23 */ uint32_t dummy23;
    /* 24 */ uint32_t dummy24;
    /* 25 */ uint32_t dummy25;
    /* 26 */ uint32_t dummy26;
    /* 27 */ uint32_t dummy27;
    /* 28 */ uint32_t dummy28;
    /* 29 */ uint32_t dummy29;
    /* 30 */ uint32_t dummy30;
    /* 31 */ uint32_t dummy31;
};

/* ___________________ Hardware Version Control
 */
struct han_regmap_hwvers
{
    /* 0  */ uint32_t major_ver;
    /* 1  */ uint32_t minor_ver;
    /* 2  */ uint32_t provisional_ver;
    /* 3  */ uint32_t proc_interface_ver;
    /* 4  */ uint32_t dummy4;
    /* 5  */ uint32_t dummy5;
    /* 6  */ uint32_t dummy6;
    /* 7  */ uint32_t dummy7;
    /* 8  */ uint32_t dummy8;
    /* 9  */ uint32_t dummy9;
    /* 10 */ uint32_t dummy10;
    /* 11 */ uint32_t dummy11;
    /* 12 */ uint32_t dummy12;
    /* 13 */ uint32_t dummy13;
    /* 14 */ uint32_t dummy14;
    /* 15 */ uint32_t dummy15;
    /* 16 */ uint32_t dummy16;
    /* 17 */ uint32_t dummy17;
    /* 18 */ uint32_t dummy18;
    /* 19 */ uint32_t dummy19;
    /* 20 */ uint32_t dummy20;
    /* 21 */ uint32_t dummy21;
    /* 22 */ uint32_t dummy22;
    /* 23 */ uint32_t dummy23;
    /* 24 */ uint32_t dummy24;
    /* 25 */ uint32_t dummy25;
    /* 26 */ uint32_t dummy26;
    /* 27 */ uint32_t dummy27;
    /* 28 */ uint32_t dummy28;
    /* 29 */ uint32_t dummy29;
    /* 30 */ uint32_t dummy30;
    /* 31 */ uint32_t dummy31;
};


/* ____________________________________________________________________ Bitmasks
 */

/* ___________________ Transceiver
 */
#define TX_BUSY_MASK                                                (0x00000010)
#define TX_BUSY_SHIFT                                               (4)
#define USER_TX_RAM_MASK                                            (0x00000008)
#define USER_TX_RAM_SHIFT                                           (3)
#define TX_MEMBANK_SEL_MASK                                         (0x00000004)
#define TX_MEMBANK_SEL_SHIFT                                        (2)
#define TX_START_MASK                                               (0x00000002)
#define TX_START_SHIFT                                              (1)
#define TX_ENABLE_MASK                                              (0x00000001)
#define TX_ENABLE_SHIFT                                             (0)
#define TX_PGA_GAIN_MASK                                            (0x3F000000)
#define TX_PGA_GAIN_SHIFT                                           (24)
#define TX_PSDU_LENGTH_MASK                                         (0x0003FF00)
#define TX_PSDU_LENGTH_SHIFT                                        (8)
#define TX_REP_CODE_MASK                                            (0x000000FF)
#define TX_REP_CODE_SHIFT                                           (0)
#define TX_PGA_GAIN_MASK                                            (0x3F000000)
#define TX_PGA_GAIN_SHIFT                                           (24)
#define AGC_MAXDB_MASK                                              (0xFF000000)
#define AGC_MAXDB_SHIFT                                             (24)
#define AGC_LOWERTHRESH_MASK                                        (0x00FF0000)
#define AGC_LOWERTHRESH_SHIFT                                       (16)
#define AGC_SETPOINT_MASK                                           (0x0000FF00)
#define AGC_SETPOINT_SHIFT                                          (8)
#define AGC_MANUAL_MEMSEL_MASK                                      (0x00000080)
#define AGC_MANUAL_MEMSEL_SHIFT                                     (7)
#define PAYLOAD_CRCFAIL_INTR_MASK                                   (0x00000040)
#define PAYLOAD_CRCFAIL_INTR_SHIFT                                  (6)
#define CLEAR_MEMBANK_OFLOW_MASK                                    (0x00000020)
#define CLEAR_MEMBANK_OFLOW_SHIFT                                   (5)
#define CLEAR_MEM_FULL_FLAG_BANK0_MASK                              (0x00000002)
#define CLEAR_MEM_FULL_FLAG_BANK0_SHIFT                             (1)
#define RXM_ENABLE_MASK                                             (0x00000001)
#define RXM_ENABLE_SHIFT                                            (0)
#define HDR_REPRATE_MASK                                            (0x000000FF)
#define HDR_REPRATE_SHIFT                                           (0)
#define ED_THRESHOLD_MASK                                           (0x00FF0000)
#define ED_THRESHOLD_SHIFT                                          (16)
#define CCA_THRESHOLD_MASK                                          (0x0000FF00)
#define CCA_THRESHOLD_SHIFT                                         (8)
#define HP_THRESHOLD_MASK                                           (0x000000FF)
#define HP_THRESHOLD_SHIFT                                          (0)
#define RESET_EXTRA_HDR3_MASK                                       (0x00800000)
#define RESET_EXTRA_HDR3_SHIFT                                      (23)
#define RESET_EXTRA_HDR2_MASK                                       (0x00400000)
#define RESET_EXTRA_HDR2_SHIFT                                      (22)
#define RESET_EXTRA_HDR1_MASK                                       (0x00200000)
#define RESET_EXTRA_HDR1_SHIFT                                      (21)
#define RESET_EXTRA_HDR0_MASK                                       (0x00100000)
#define RESET_EXTRA_HDR0_SHIFT                                      (20)
#define RESET_PAYLOAD_CRC3_MASK                                     (0x00080000)
#define RESET_PAYLOAD_CRC3_SHIFT                                    (19)
#define RESET_HEADER_CRC3_MASK                                      (0x00040000)
#define RESET_HEADER_CRC3_SHIFT                                     (18)
#define RESET_PSDU_LEN3_MASK                                        (0x00020000)
#define RESET_PSDU_LEN3_SHIFT                                       (17)
#define RESET_REPCODE3_MASK                                         (0x00010000)
#define RESET_REPCODE3_SHIFT                                        (16)
#define RESET_PAYLOAD_CRC2_MASK                                     (0x00008000)
#define RESET_PAYLOAD_CRC2_SHIFT                                    (15)
#define RESET_HEADER_CRC2_MASK                                      (0x00004000)
#define RESET_HEADER_CRC2_SHIFT                                     (14)
#define RESET_PSDU_LEN2_MASK                                        (0x00002000)
#define RESET_PSDU_LEN2_SHIFT                                       (13)
#define RESET_REPCODE2_MASK                                         (0x00001000)
#define RESET_REPCODE2_SHIFT                                        (12)
#define RESET_PAYLOAD_CRC1_MASK                                     (0x00000800)
#define RESET_PAYLOAD_CRC1_SHIFT                                    (11)
#define RESET_HEADER_CRC1_MASK                                      (0x00000400)
#define RESET_HEADER_CRC1_SHIFT                                     (10)
#define RESET_PSDU_LEN1_MASK                                        (0x00000200)
#define RESET_PSDU_LEN1_SHIFT                                       (9)
#define RESET_REPCODE1_MASK                                         (0x00000100)
#define RESET_REPCODE1_SHIFT                                        (8)
#define RESET_PAYLOAD_CRC0_MASK                                     (0x00000080)
#define RESET_PAYLOAD_CRC0_SHIFT                                    (7)
#define RESET_HEADER_CRC0_MASK                                      (0x00000040)
#define RESET_HEADER_CRC0_SHIFT                                     (6)
#define RESET_PSDU_LEN0_MASK                                        (0x00000020)
#define RESET_PSDU_LEN0_SHIFT                                       (5)
#define RESET_REPCODE0_MASK                                         (0x00000010)
#define RESET_REPCODE0_SHIFT                                        (4)
#define RESET_PAYLOAD_CRC_FAIL_MASK                                 (0x00000008)
#define RESET_PAYLOAD_CRC_FAIL_SHIFT                                (3)
#define RESET_PAYLOAD_CRC_PASS_MASK                                 (0x00000004)
#define RESET_PAYLOAD_CRC_PASS_SHIFT                                (2)
#define RESET_HEADER_CRC_FAIL_MASK                                  (0x00000002)
#define RESET_HEADER_CRC_FAIL_SHIFT                                 (1)
#define RESET_HEADER_CRC_PASS_MASK                                  (0x00000001)
#define RESET_HEADER_CRC_PASS_SHIFT                                 (0)
#define NEXT_MEMBANK_TO_READ_MASK                                   (0x00000003)
#define NEXT_MEMBANK_TO_READ_SHIFT                                  (0)
#define RX_PSDU_LENGTH_MASK                                         (0x03FF0000)
#define RX_PSDU_LENGTH_SHIFT                                        (16)
#define RX_REP_CODE_MASK                                            (0x000000FF)
#define RX_REP_CODE_SHIFT                                           (0)
#define PROC_FIFO_WR_LEVEL_MASK                                     (0x0000F000)
#define PROC_FIFO_WR_LEVEL_SHIFT                                    (12)
#define PROC_FIFO_RD_LEVEL_MASK                                     (0x00000F00)
#define PROC_FIFO_RD_LEVEL_SHIFT                                    (8)
#define NEXTBUF_FIFO_WR_LEVEL_MASK                                  (0x000000F0)
#define NEXTBUF_FIFO_WR_LEVEL_SHIFT                                 (4)
#define NEXTBUF_FIFO_RD_LEVEL_MASK                                  (0x0000000F)
#define NEXTBUF_FIFO_RD_LEVEL_SHIFT                                 (0)
#define CCA_AUTO_CORR_MASK                                          (0x80000000)
#define CCA_AUTO_CORR_SHIFT                                         (31)
#define ED_CHAN_BUSY_MASK                                           (0x40000000)
#define ED_CHAN_BUSY_SHIFT                                          (30)
#define HP_AUTO_CORR_MASK                                           (0x20000000)
#define HP_AUTO_CORR_SHIFT                                          (29)
#define RX_RSSI_LATCHED_MASK                                        (0x00FF0000)
#define RX_RSSI_LATCHED_SHIFT                                       (16)
#define RX_PGA_FREE_MASK                                            (0x00003F00)
#define RX_PGA_FREE_SHIFT                                           (8)
#define RX_PGA_LATCHED_MASK                                         (0x0000003F)
#define RX_PGA_LATCHED_SHIFT                                        (0)
#define RX_FIFO_EMPTY_MASK                                          (0x00400000)
#define RX_FIFO_EMPTY_SHIFT                                         (22)
#define RX_FIFO_FULL_MASK                                           (0x00200000)
#define RX_FIFO_FULL_SHIFT                                          (21)
#define RX_MEMBANK_OFLOW_MASK                                       (0x00100000)
#define RX_MEMBANK_OFLOW_SHIFT                                      (20)
#define MEMBANK3_FULL_MASK                                          (0x00080000)
#define MEMBANK3_FULL_SHIFT                                         (19)
#define MEMBANK2_FULL_MASK                                          (0x00040000)
#define MEMBANK2_FULL_SHIFT                                         (18)
#define MEMBANK1_FULL_MASK                                          (0x00020000)
#define MEMBANK1_FULL_SHIFT                                         (17)
#define MEMBANK0_FULL_MASK                                          (0x00010000)
#define MEMBANK0_FULL_SHIFT                                         (16)
#define RX_RSSI_FREE_MASK                                           (0x000000FF)
#define RX_RSSI_FREE_SHIFT                                          (0)

/* ___________________ Medium Access Controller
 */
#define TRAFMON_ENABLE_MASK                                         (0x00000001)
#define TRAFMON_ENABLE_SHIFT                                        (0)
#define TRAFMON_CLEAR_MASK                                          (0x00000001)
#define TRAFMON_CLEAR_SHIFT                                         (0)
#define TRAFMON_PSDULEN_AVG_MASK                                    (0x0000007F)
#define TRAFMON_PSDULEN_AVG_SHIFT                                   (0)
#define TRAFMON_REPCODE_AVG_MASK                                    (0x000000FF)
#define TRAFMON_REPCODE_AVG_SHIFT                                   (0)
#define TRAFMON_HDR_CRC_PASS_MASK                                   (0x00003FFF)
#define TRAFMON_HDR_CRC_PASS_SHIFT                                  (0)
#define TRAFMON_HDR_CRC_FAIL_MASK                                   (0x00003FFF)
#define TRAFMON_HDR_CRC_FAIL_SHIFT                                  (0)
#define TRAFMON_PAY_CRC_FAIL_MASK                                   (0x00003FFF)
#define TRAFMON_PAY_CRC_FAIL_SHIFT                                  (0)
#define TRAFMON_CCA_MASK                                            (0x00003FFF)
#define TRAFMON_CCA_SHIFT                                           (0)
#define TRAFMON_PKT_TIME_MASK                                       (0x00003FFF)
#define TRAFMON_PKT_TIME_SHIFT                                      (0)
#define TRAFMON_HP_MASK                                             (0x00003FFF)
#define TRAFMON_HP_SHIFT                                            (0)
#define MAC_EA_LOWER_MASK                                           (0xFFFFFFFF)
#define MAC_EA_LOWER_SHIFT                                          (0)
#define MAC_EA_UPPER_MASK                                           (0xFFFFFFFF)
#define MAC_EA_UPPER_SHIFT                                          (0)
#define MAC_SA_MASK                                                 (0x0000FFFF)
#define MAC_SA_SHIFT                                                (0)
#define MAC_PAN_ID_MASK                                             (0x0000FFFF)
#define MAC_PAN_ID_SHIFT                                            (0)
#define MAC_CTRL_IGN_FCS_PAYLOAD_MASK                               (0x00000008)
#define MAC_CTRL_IGN_FCS_PAYLOAD_SHIFT                              (3)
#define MAC_CTRL_IGN_FCS_HEADER_MASK                                (0x00000004)
#define MAC_CTRL_IGN_FCS_HEADER_SHIFT                               (2)
#define MAC_CTRL_PAN_COORD_MASK                                     (0x00000002)
#define MAC_CTRL_PAN_COORD_SHIFT                                    (1)
#define MAC_CTRL_FILTER_EN_MASK                                     (0x00000001)
#define MAC_CTRL_FILTER_EN_SHIFT                                    (0)

/* ___________________ Power Up Controller
 */

/* ___________________ AFE Controller
 */
#define AFE__MASK                                                   (0xFF000000)
#define AFE__SHIFT                                                  (24)
#define AFE_SINGLE_AD9865_ADDR_MASK                                 (0x001F0000)
#define AFE_SINGLE_AD9865_ADDR_SHIFT                                (16)
#define AFE_AGC_FIXED_GAIN_MASK                                     (0x00003F00)
#define AFE_AGC_FIXED_GAIN_SHIFT                                    (8)
#define AFE_SINGLE_AD9865_ACCESS_MASK                               (0x00000001)
#define AFE_SINGLE_AD9865_ACCESS_SHIFT                              (0)
#define SPI_READ_DONE_MASK                                          (0x00000004)
#define SPI_READ_DONE_SHIFT                                         (2)
#define SPI_WRITE_DONE_MASK                                         (0x00000002)
#define SPI_WRITE_DONE_SHIFT                                        (1)
#define SW_RESET_DONE_MASK                                          (0x00000001)
#define SW_RESET_DONE_SHIFT                                         (0)

/* ___________________ Hardware Version Control
 */


/* _____________________________________________________________________ Offsets
 */

/* ___________________ Transceiver
 */
#define TX_CONTROL                                                 (0)
#define TX_PARAMS_BANK0                                            (1)
#define TX_EXTRA_HDR_BANK0                                         (2)
#define TX_PARAMS_BANK1                                            (3)
#define TX_EXTRA_HDR_BANK1                                         (4)
#define RX_CONTROL                                                 (5)
#define RX_HDR_REP_RATE                                            (7)
#define RX_THRESHOLDS                                              (8)
#define RX_RESETS                                                  (9)
#define RX_NEXT_MEM_BANK_TO_READ                                   (10)
#define RX_HDR_CRC_PASS                                            (11)
#define RX_HDR_CRC_FAIL                                            (12)
#define RX_PAY_CRC_PASS                                            (13)
#define RX_PAY_CRC_FAIL                                            (14)
#define RX_RC_PSDU_BANK0                                           (15)
#define RX_HDR_XTRA_BANK0                                          (16)
#define RX_CRCS_BANK0                                              (17)
#define RX_RC_PSDU_BANK1                                           (18)
#define RX_HDR_XTRA_BANK1                                          (19)
#define RX_CRCS_BANK1                                              (20)
#define RX_RC_PSDU_BANK2                                           (21)
#define RX_HDR_XTRA_BANK2                                          (22)
#define RX_CRCS_BANK2                                              (23)
#define RX_RC_PSDU_BANK3                                           (24)
#define RX_HDR_XTRA_BANK3                                          (25)
#define RX_CRCS_BANK3                                              (26)
#define RX_FIFO_LEVELS                                             (28)
#define TRX_HARD_RESET                                             (29)
#define RX_PGA_CCA_FLAGS                                           (30)
#define RX_RSSI_MEM_FLAGS_TXPWR                                    (31)

/* ___________________ Medium Access Controller
 */
#define MAC_TRAFMON_CTRL                                           (0)
#define MAC_TRAFMON_CLEAR                                          (1)
#define MAC_TRAFMON_PSDULEN_AVG                                    (10)
#define MAC_TRAFMON_REPCODE_AVG                                    (11)
#define MAC_TRAFMON_HDR_CRC_PASS                                   (12)
#define MAC_TRAFMON_HDR_CRC_FAIL                                   (13)
#define MAC_TRAFMON_PAY_CRC_FAIL                                   (14)
#define MAC_TRAFMON_CCA                                            (15)
#define MAC_TRAFMON_PKT_TIME                                       (16)
#define MAC_TRAFMON_HP                                             (17)
#define MAC_FILTER_EA_LOWER                                        (18)
#define MAC_FILTER_EA_UPPER                                        (19)
#define MAC_FILTER_SA                                              (20)
#define MAC_FILTER_PAN_ID                                          (21)
#define MAC_FILTER_CTRL                                            (22)

/* ___________________ Power Up Controller
 */
#define PWR_DIPS_BOARD                                             (18)
#define PWR_DIPS_MODEM                                             (19)

/* ___________________ AFE Controller
 */
#define AFE_AGC_FIXED_GAIN                                         (0)
#define AFE_STATUS                                                 (1)

/* ___________________ Hardware Version Control
 */
#define HWVERS_MAJOR                                               (0)
#define HWVERS_MINOR                                               (1)
#define HWVERS_REVISION                                            (2)
#define HWVERS_PROC_INTERFACE                                      (3)



#endif

