/*
 * Register Access Implementatio for QEMU Hanadu Virtual Device.
 * Processor Interface Version: 1
 * 
 * THIS FILE IS AUTOMATICALLY GENERATED USING THE REGISTER MAP PROJECT.
 * PLEASE DO NOT EDIT.
 *
 * Martin Townsend >martin.townsend@xsilon.com<
 * Copyright Xsilon Limited 2014.
 */

#include "hanadu.h"



/* ___________________ Transceiver
 */

void
han_trxm_reg_reset(void *opaque)
{
    struct han_trxm_dev *s = HANADU_TRXM_DEV(opaque);

    memset(&s->regs, 0, sizeof(uint32_t) * 32);
    s->regs.trx_tx_ctrl = 0x04000000;
    s->regs.trx_rx_control = 0x30fdf400;
    s->regs.trx_rx_hdr_rep_rate = 0x00005696;
    s->regs.trx_rx_thresholds = 0x008080ce;
    s->regs.trx_xcorr_thresh = 0x0000008c;
    s->regs.trx_rx_fifo_levels = 0x00003300;
    s->regs.trx_membank_fifo_flags_rssi_tx_power = 0x93400080;
}

uint64_t
han_trxm_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    struct han_trxm_dev *s = HANADU_TRXM_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    uint64_t retvalue = 0;
    reg_read_fn * reg_read_fnp = (reg_read_fn *)&s->regs.reg_read;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_read_fnp[addr])
        if (reg_read_fnp[addr]((uint32_t *)&retvalue, s))
            return retvalue;

    if (s->mem_region_read)
        if (s->mem_region_read(opaque, addr, size, &retvalue))
            return retvalue;

    return regs[addr];
}

void
han_trxm_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    struct han_trxm_dev *s = HANADU_TRXM_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    reg_changed_fn * reg_changes_fnp = (reg_changed_fn *)&s->regs.reg_changed;

    uint32_t old_reg_value;

    if (s->mem_region_write)
        if (s->mem_region_write(opaque, addr, value, size))
            return;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_changes_fnp[addr])
        if (reg_changes_fnp[addr]((uint32_t)value) )
            return;


    old_reg_value = regs[addr];
    regs[addr] = value;


    switch(addr) {
    
    case TX_CONTROL:
        if ((value & USER_TX_RAM_MASK) != (old_reg_value & USER_TX_RAM_MASK)) {
            if (s->regs.field_changed.tx_use_ram_changed) {
                s->regs.field_changed.tx_use_ram_changed(((uint32_t)value & USER_TX_RAM_MASK) >> USER_TX_RAM_SHIFT, s);
            }
        }
        if ((value & TX_MEMBANK_SEL_MASK) != (old_reg_value & TX_MEMBANK_SEL_MASK)) {
            if (s->regs.field_changed.tx_mem_bank_select_changed) {
                s->regs.field_changed.tx_mem_bank_select_changed(((uint32_t)value & TX_MEMBANK_SEL_MASK) >> TX_MEMBANK_SEL_SHIFT, s);
            }
        }
        if ((value & TX_START_MASK) != (old_reg_value & TX_START_MASK)) {
            if (s->regs.field_changed.tx_start_changed) {
                s->regs.field_changed.tx_start_changed(((uint32_t)value & TX_START_MASK) >> TX_START_SHIFT, s);
            }
        }
        if ((value & TX_ENABLE_MASK) != (old_reg_value & TX_ENABLE_MASK)) {
            if (s->regs.field_changed.tx_enable_changed) {
                s->regs.field_changed.tx_enable_changed(((uint32_t)value & TX_ENABLE_MASK) >> TX_ENABLE_SHIFT, s);
            }
        }

        break;

    case TX_PARAMS_BANK0:
        if ((value & TX_ATT_20DB_MASK) != (old_reg_value & TX_ATT_20DB_MASK)) {
            if (s->regs.field_changed.tx_att_20db0_changed) {
                s->regs.field_changed.tx_att_20db0_changed(((uint32_t)value & TX_ATT_20DB_MASK) >> TX_ATT_20DB_SHIFT, s);
            }
        }
        if ((value & TX_ATT_10DB_MASK) != (old_reg_value & TX_ATT_10DB_MASK)) {
            if (s->regs.field_changed.tx_att_10db0_changed) {
                s->regs.field_changed.tx_att_10db0_changed(((uint32_t)value & TX_ATT_10DB_MASK) >> TX_ATT_10DB_SHIFT, s);
            }
        }
        if ((value & TX_PGA_GAIN_MASK) != (old_reg_value & TX_PGA_GAIN_MASK)) {
            if (s->regs.field_changed.tx_pga_gain0_changed) {
                s->regs.field_changed.tx_pga_gain0_changed(((uint32_t)value & TX_PGA_GAIN_MASK) >> TX_PGA_GAIN_SHIFT, s);
            }
        }
        if ((value & TX_PSDU_LENGTH_MASK) != (old_reg_value & TX_PSDU_LENGTH_MASK)) {
            if (s->regs.field_changed.tx_psdu_len0_changed) {
                s->regs.field_changed.tx_psdu_len0_changed(((uint32_t)value & TX_PSDU_LENGTH_MASK) >> TX_PSDU_LENGTH_SHIFT, s);
            }
        }
        if ((value & TX_REP_CODE_MASK) != (old_reg_value & TX_REP_CODE_MASK)) {
            if (s->regs.field_changed.tx_rep_code0_changed) {
                s->regs.field_changed.tx_rep_code0_changed(((uint32_t)value & TX_REP_CODE_MASK) >> TX_REP_CODE_SHIFT, s);
            }
        }

        break;

    case TX_EXTRA_HDR_BANK0:
        if ((value & TX_PREAMBLE_MASK) != (old_reg_value & TX_PREAMBLE_MASK)) {
            if (s->regs.field_changed.tx_hdr_preamble0_changed) {
                s->regs.field_changed.tx_hdr_preamble0_changed(((uint32_t)value & TX_PREAMBLE_MASK) >> TX_PREAMBLE_SHIFT, s);
            }
        }
        if ((value & TX_HDR_XTRA_MASK) != (old_reg_value & TX_HDR_XTRA_MASK)) {
            if (s->regs.field_changed.tx_hdr_extra0_changed) {
                s->regs.field_changed.tx_hdr_extra0_changed(((uint32_t)value & TX_HDR_XTRA_MASK) >> TX_HDR_XTRA_SHIFT, s);
            }
        }

        break;

    case TX_PARAMS_BANK1:
        if ((value & TX_ATT_20DB_MASK) != (old_reg_value & TX_ATT_20DB_MASK)) {
            if (s->regs.field_changed.tx_att_20db1_changed) {
                s->regs.field_changed.tx_att_20db1_changed(((uint32_t)value & TX_ATT_20DB_MASK) >> TX_ATT_20DB_SHIFT, s);
            }
        }
        if ((value & TX_ATT_10DB_MASK) != (old_reg_value & TX_ATT_10DB_MASK)) {
            if (s->regs.field_changed.tx_att_10db1_changed) {
                s->regs.field_changed.tx_att_10db1_changed(((uint32_t)value & TX_ATT_10DB_MASK) >> TX_ATT_10DB_SHIFT, s);
            }
        }
        if ((value & TX_PGA_GAIN_MASK) != (old_reg_value & TX_PGA_GAIN_MASK)) {
            if (s->regs.field_changed.tx_pga_gain1_changed) {
                s->regs.field_changed.tx_pga_gain1_changed(((uint32_t)value & TX_PGA_GAIN_MASK) >> TX_PGA_GAIN_SHIFT, s);
            }
        }
        if ((value & TX_PSDU_LENGTH_MASK) != (old_reg_value & TX_PSDU_LENGTH_MASK)) {
            if (s->regs.field_changed.tx_psdu_len1_changed) {
                s->regs.field_changed.tx_psdu_len1_changed(((uint32_t)value & TX_PSDU_LENGTH_MASK) >> TX_PSDU_LENGTH_SHIFT, s);
            }
        }
        if ((value & TX_REP_CODE_MASK) != (old_reg_value & TX_REP_CODE_MASK)) {
            if (s->regs.field_changed.tx_rep_code1_changed) {
                s->regs.field_changed.tx_rep_code1_changed(((uint32_t)value & TX_REP_CODE_MASK) >> TX_REP_CODE_SHIFT, s);
            }
        }

        break;

    case TX_EXTRA_HDR_BANK1:
        if ((value & TX_PREAMBLE_MASK) != (old_reg_value & TX_PREAMBLE_MASK)) {
            if (s->regs.field_changed.tx_preamble1_changed) {
                s->regs.field_changed.tx_preamble1_changed(((uint32_t)value & TX_PREAMBLE_MASK) >> TX_PREAMBLE_SHIFT, s);
            }
        }
        if ((value & TX_HDR_XTRA_MASK) != (old_reg_value & TX_HDR_XTRA_MASK)) {
            if (s->regs.field_changed.tx_hdr_extra1_changed) {
                s->regs.field_changed.tx_hdr_extra1_changed(((uint32_t)value & TX_HDR_XTRA_MASK) >> TX_HDR_XTRA_SHIFT, s);
            }
        }

        break;

    case RX_CONTROL:
        if ((value & AGC_MAXDB_MASK) != (old_reg_value & AGC_MAXDB_MASK)) {
            if (s->regs.field_changed.rx_acg_max_db_changed) {
                s->regs.field_changed.rx_acg_max_db_changed(((uint32_t)value & AGC_MAXDB_MASK) >> AGC_MAXDB_SHIFT, s);
            }
        }
        if ((value & AGC_LOWERTHRESH_MASK) != (old_reg_value & AGC_LOWERTHRESH_MASK)) {
            if (s->regs.field_changed.rx_acg_lower_thresh_changed) {
                s->regs.field_changed.rx_acg_lower_thresh_changed(((uint32_t)value & AGC_LOWERTHRESH_MASK) >> AGC_LOWERTHRESH_SHIFT, s);
            }
        }
        if ((value & AGC_SETPOINT_MASK) != (old_reg_value & AGC_SETPOINT_MASK)) {
            if (s->regs.field_changed.rx_acg_set_point_changed) {
                s->regs.field_changed.rx_acg_set_point_changed(((uint32_t)value & AGC_SETPOINT_MASK) >> AGC_SETPOINT_SHIFT, s);
            }
        }
        if ((value & AGC_MANUAL_MEMSEL_MASK) != (old_reg_value & AGC_MANUAL_MEMSEL_MASK)) {
            if (s->regs.field_changed.rx_manual_membank_sel_changed) {
                s->regs.field_changed.rx_manual_membank_sel_changed(((uint32_t)value & AGC_MANUAL_MEMSEL_MASK) >> AGC_MANUAL_MEMSEL_SHIFT, s);
            }
        }
        if ((value & PAYLOAD_CRCFAIL_INTR_MASK) != (old_reg_value & PAYLOAD_CRCFAIL_INTR_MASK)) {
            if (s->regs.field_changed.rx_payload_fail_crc_intr_en_changed) {
                s->regs.field_changed.rx_payload_fail_crc_intr_en_changed(((uint32_t)value & PAYLOAD_CRCFAIL_INTR_MASK) >> PAYLOAD_CRCFAIL_INTR_SHIFT, s);
            }
        }
        if ((value & CLEAR_MEMBANK_OFLOW_MASK) != (old_reg_value & CLEAR_MEMBANK_OFLOW_MASK)) {
            if (s->regs.field_changed.rx_clear_membank_oflow_changed) {
                s->regs.field_changed.rx_clear_membank_oflow_changed(((uint32_t)value & CLEAR_MEMBANK_OFLOW_MASK) >> CLEAR_MEMBANK_OFLOW_SHIFT, s);
            }
        }
        if ((value & CLEAR_MEM_FULL_FLAG_BANK3_MASK) != (old_reg_value & CLEAR_MEM_FULL_FLAG_BANK3_MASK)) {
            if (s->regs.field_changed.rx_clear_membank_full3_changed) {
                s->regs.field_changed.rx_clear_membank_full3_changed(((uint32_t)value & CLEAR_MEM_FULL_FLAG_BANK3_MASK) >> CLEAR_MEM_FULL_FLAG_BANK3_SHIFT, s);
            }
        }
        if ((value & CLEAR_MEM_FULL_FLAG_BANK2_MASK) != (old_reg_value & CLEAR_MEM_FULL_FLAG_BANK2_MASK)) {
            if (s->regs.field_changed.rx_clear_membank_full2_changed) {
                s->regs.field_changed.rx_clear_membank_full2_changed(((uint32_t)value & CLEAR_MEM_FULL_FLAG_BANK2_MASK) >> CLEAR_MEM_FULL_FLAG_BANK2_SHIFT, s);
            }
        }
        if ((value & CLEAR_MEM_FULL_FLAG_BANK1_MASK) != (old_reg_value & CLEAR_MEM_FULL_FLAG_BANK1_MASK)) {
            if (s->regs.field_changed.rx_clear_membank_full1_changed) {
                s->regs.field_changed.rx_clear_membank_full1_changed(((uint32_t)value & CLEAR_MEM_FULL_FLAG_BANK1_MASK) >> CLEAR_MEM_FULL_FLAG_BANK1_SHIFT, s);
            }
        }
        if ((value & CLEAR_MEM_FULL_FLAG_BANK0_MASK) != (old_reg_value & CLEAR_MEM_FULL_FLAG_BANK0_MASK)) {
            if (s->regs.field_changed.rx_clear_membank_full0_changed) {
                s->regs.field_changed.rx_clear_membank_full0_changed(((uint32_t)value & CLEAR_MEM_FULL_FLAG_BANK0_MASK) >> CLEAR_MEM_FULL_FLAG_BANK0_SHIFT, s);
            }
        }
        if ((value & RXM_ENABLE_MASK) != (old_reg_value & RXM_ENABLE_MASK)) {
            if (s->regs.field_changed.rx_enable_changed) {
                s->regs.field_changed.rx_enable_changed(((uint32_t)value & RXM_ENABLE_MASK) >> RXM_ENABLE_SHIFT, s);
            }
        }

        break;

    case RX_HDR_REP_RATE:
        if ((value & NUM_CARRIERS_MASK) != (old_reg_value & NUM_CARRIERS_MASK)) {
            if (s->regs.field_changed.trx_num_carriers_changed) {
                s->regs.field_changed.trx_num_carriers_changed(((uint32_t)value & NUM_CARRIERS_MASK) >> NUM_CARRIERS_SHIFT, s);
            }
        }
        if ((value & HDR_REPRATE_MASK) != (old_reg_value & HDR_REPRATE_MASK)) {
            if (s->regs.field_changed.rx_hdr_reprate_changed) {
                s->regs.field_changed.rx_hdr_reprate_changed(((uint32_t)value & HDR_REPRATE_MASK) >> HDR_REPRATE_SHIFT, s);
            }
        }

        break;

    case RX_THRESHOLDS:
        if ((value & HP_THRESHOLD_MASK) != (old_reg_value & HP_THRESHOLD_MASK)) {
            if (s->regs.field_changed.rx_hp_auto_threshold_changed) {
                s->regs.field_changed.rx_hp_auto_threshold_changed(((uint32_t)value & HP_THRESHOLD_MASK) >> HP_THRESHOLD_SHIFT, s);
            }
        }
        if ((value & CCA_THRESHOLD_MASK) != (old_reg_value & CCA_THRESHOLD_MASK)) {
            if (s->regs.field_changed.rx_cca_auto_threshold_changed) {
                s->regs.field_changed.rx_cca_auto_threshold_changed(((uint32_t)value & CCA_THRESHOLD_MASK) >> CCA_THRESHOLD_SHIFT, s);
            }
        }
        if ((value & ED_THRESHOLD_MASK) != (old_reg_value & ED_THRESHOLD_MASK)) {
            if (s->regs.field_changed.rx_ed_threshold_changed) {
                s->regs.field_changed.rx_ed_threshold_changed(((uint32_t)value & ED_THRESHOLD_MASK) >> ED_THRESHOLD_SHIFT, s);
            }
        }

        break;

    }
}


/* ___________________ Medium Access Controller
 */

void
han_mac_reg_reset(void *opaque)
{
    struct han_mac_dev *s = HANADU_MAC_DEV(opaque);

    memset(&s->regs, 0, sizeof(uint32_t) * 32);
    s->regs.mac_traf_mon_clear = 0x00000001;
    s->regs.mac_ifs = 0x280C1200;
    s->regs.mac_lower_ctrl = 0x00045491;
    s->regs.mac_ack_control = 0x000f2000;
    s->regs.mac_ack_payload = 0x00000002;
    s->regs.mac_ack_processing_ctrl = 0x00040400;
}

uint64_t
han_mac_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    struct han_mac_dev *s = HANADU_MAC_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    uint64_t retvalue = 0;
    reg_read_fn * reg_read_fnp = (reg_read_fn *)&s->regs.reg_read;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_read_fnp[addr])
        if (reg_read_fnp[addr]((uint32_t *)&retvalue, s))
            return retvalue;

    if (s->mem_region_read)
        if (s->mem_region_read(opaque, addr, size, &retvalue))
            return retvalue;

    return regs[addr];
}

void
han_mac_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    struct han_mac_dev *s = HANADU_MAC_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    reg_changed_fn * reg_changes_fnp = (reg_changed_fn *)&s->regs.reg_changed;

    uint32_t old_reg_value;

    if (s->mem_region_write)
        if (s->mem_region_write(opaque, addr, value, size))
            return;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_changes_fnp[addr])
        if (reg_changes_fnp[addr]((uint32_t)value) )
            return;


    old_reg_value = regs[addr];
    regs[addr] = value;


    switch(addr) {
    
    case MAC_IFS:
        if ((value & MAC_LIFS_PERIOD_MASK) != (old_reg_value & MAC_LIFS_PERIOD_MASK)) {
            if (s->regs.field_changed.lifs_period_changed) {
                s->regs.field_changed.lifs_period_changed(((uint32_t)value & MAC_LIFS_PERIOD_MASK) >> MAC_LIFS_PERIOD_SHIFT, s);
            }
        }
        if ((value & MAC_SIFS_PERIOD_MASK) != (old_reg_value & MAC_SIFS_PERIOD_MASK)) {
            if (s->regs.field_changed.sifs_period_changed) {
                s->regs.field_changed.sifs_period_changed(((uint32_t)value & MAC_SIFS_PERIOD_MASK) >> MAC_SIFS_PERIOD_SHIFT, s);
            }
        }
        if ((value & MAC_MAX_SIFS_FRAME_SIZE_MASK) != (old_reg_value & MAC_MAX_SIFS_FRAME_SIZE_MASK)) {
            if (s->regs.field_changed.max_sifs_frame_size_changed) {
                s->regs.field_changed.max_sifs_frame_size_changed(((uint32_t)value & MAC_MAX_SIFS_FRAME_SIZE_MASK) >> MAC_MAX_SIFS_FRAME_SIZE_SHIFT, s);
            }
        }
        if ((value & MAC_IFS_ENABLE_MASK) != (old_reg_value & MAC_IFS_ENABLE_MASK)) {
            if (s->regs.field_changed.ifs_enable_changed) {
                s->regs.field_changed.ifs_enable_changed(((uint32_t)value & MAC_IFS_ENABLE_MASK) >> MAC_IFS_ENABLE_SHIFT, s);
            }
        }

        break;

    case MAC_FILTER_EA_LOWER:
        if ((value & MAC_EA_LOWER_MASK) != (old_reg_value & MAC_EA_LOWER_MASK)) {
            if (s->regs.field_changed.ctrl_ea_lower_changed) {
                s->regs.field_changed.ctrl_ea_lower_changed(((uint32_t)value & MAC_EA_LOWER_MASK) >> MAC_EA_LOWER_SHIFT, s);
            }
        }

        break;

    case MAC_FILTER_EA_UPPER:
        if ((value & MAC_EA_UPPER_MASK) != (old_reg_value & MAC_EA_UPPER_MASK)) {
            if (s->regs.field_changed.ctrl_ea_upper_changed) {
                s->regs.field_changed.ctrl_ea_upper_changed(((uint32_t)value & MAC_EA_UPPER_MASK) >> MAC_EA_UPPER_SHIFT, s);
            }
        }

        break;

    case MAC_FILTER_SA:
        if ((value & MAC_SA_MASK) != (old_reg_value & MAC_SA_MASK)) {
            if (s->regs.field_changed.ctrl_short_addr_changed) {
                s->regs.field_changed.ctrl_short_addr_changed(((uint32_t)value & MAC_SA_MASK) >> MAC_SA_SHIFT, s);
            }
        }

        break;

    case MAC_FILTER_PAN_ID:
        if ((value & MAC_PAN_ID_MASK) != (old_reg_value & MAC_PAN_ID_MASK)) {
            if (s->regs.field_changed.ctrl_pan_id_changed) {
                s->regs.field_changed.ctrl_pan_id_changed(((uint32_t)value & MAC_PAN_ID_MASK) >> MAC_PAN_ID_SHIFT, s);
            }
        }

        break;

    case MAC_FILTER_CTRL:
        if ((value & MAC_CTRL_PAN_COORD_MASK) != (old_reg_value & MAC_CTRL_PAN_COORD_MASK)) {
            if (s->regs.field_changed.ctrl_pan_coord_changed) {
                s->regs.field_changed.ctrl_pan_coord_changed(((uint32_t)value & MAC_CTRL_PAN_COORD_MASK) >> MAC_CTRL_PAN_COORD_SHIFT, s);
            }
        }
        if ((value & MAC_CTRL_FILTER_EN_MASK) != (old_reg_value & MAC_CTRL_FILTER_EN_MASK)) {
            if (s->regs.field_changed.ctrl_filter_enable_changed) {
                s->regs.field_changed.ctrl_filter_enable_changed(((uint32_t)value & MAC_CTRL_FILTER_EN_MASK) >> MAC_CTRL_FILTER_EN_SHIFT, s);
            }
        }

        break;

    case MAC_LOWER_CTRL:
        if ((value & MAC_TIMEOUT_STRATEGY_MASK) != (old_reg_value & MAC_TIMEOUT_STRATEGY_MASK)) {
            if (s->regs.field_changed.timeout_strategy_changed) {
                s->regs.field_changed.timeout_strategy_changed(((uint32_t)value & MAC_TIMEOUT_STRATEGY_MASK) >> MAC_TIMEOUT_STRATEGY_SHIFT, s);
            }
        }
        if ((value & MAC_MAX_CSMA_BACKOFFS_MASK) != (old_reg_value & MAC_MAX_CSMA_BACKOFFS_MASK)) {
            if (s->regs.field_changed.max_csma_backoffs_changed) {
                s->regs.field_changed.max_csma_backoffs_changed(((uint32_t)value & MAC_MAX_CSMA_BACKOFFS_MASK) >> MAC_MAX_CSMA_BACKOFFS_SHIFT, s);
            }
        }
        if ((value & MAC_MIN_BE_MASK) != (old_reg_value & MAC_MIN_BE_MASK)) {
            if (s->regs.field_changed.min_be_changed) {
                s->regs.field_changed.min_be_changed(((uint32_t)value & MAC_MIN_BE_MASK) >> MAC_MIN_BE_SHIFT, s);
            }
        }
        if ((value & MAC_MAX_BE_MASK) != (old_reg_value & MAC_MAX_BE_MASK)) {
            if (s->regs.field_changed.max_be_changed) {
                s->regs.field_changed.max_be_changed(((uint32_t)value & MAC_MAX_BE_MASK) >> MAC_MAX_BE_SHIFT, s);
            }
        }
        if ((value & MAC_CSMA_IGN_RX_BUSY_FOR_TX_MASK) != (old_reg_value & MAC_CSMA_IGN_RX_BUSY_FOR_TX_MASK)) {
            if (s->regs.field_changed.csma_ign_rx_busy_changed) {
                s->regs.field_changed.csma_ign_rx_busy_changed(((uint32_t)value & MAC_CSMA_IGN_RX_BUSY_FOR_TX_MASK) >> MAC_CSMA_IGN_RX_BUSY_FOR_TX_SHIFT, s);
            }
        }
        if ((value & LOWER_MAC_ACK_OVERRIDE_MASK) != (old_reg_value & LOWER_MAC_ACK_OVERRIDE_MASK)) {
            if (s->regs.field_changed.lower_ack_override_changed) {
                s->regs.field_changed.lower_ack_override_changed(((uint32_t)value & LOWER_MAC_ACK_OVERRIDE_MASK) >> LOWER_MAC_ACK_OVERRIDE_SHIFT, s);
            }
        }
        if ((value & LOWER_MAC_RESET_MASK) != (old_reg_value & LOWER_MAC_RESET_MASK)) {
            if (s->regs.field_changed.lower_reset_changed) {
                s->regs.field_changed.lower_reset_changed(((uint32_t)value & LOWER_MAC_RESET_MASK) >> LOWER_MAC_RESET_SHIFT, s);
            }
        }
        if ((value & MAC_BYPASS_MASK) != (old_reg_value & MAC_BYPASS_MASK)) {
            if (s->regs.field_changed.lower_mac_bypass_changed) {
                s->regs.field_changed.lower_mac_bypass_changed(((uint32_t)value & MAC_BYPASS_MASK) >> MAC_BYPASS_SHIFT, s);
            }
        }

        break;

    case MAC_ACK_CTRL:
        if ((value & MAC_ACK_PGA_GAIN_MASK) != (old_reg_value & MAC_ACK_PGA_GAIN_MASK)) {
            if (s->regs.field_changed.ack_pga_gain_changed) {
                s->regs.field_changed.ack_pga_gain_changed(((uint32_t)value & MAC_ACK_PGA_GAIN_MASK) >> MAC_ACK_PGA_GAIN_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_REP_CODE_MASK) != (old_reg_value & MAC_ACK_REP_CODE_MASK)) {
            if (s->regs.field_changed.ack_rep_code_changed) {
                s->regs.field_changed.ack_rep_code_changed(((uint32_t)value & MAC_ACK_REP_CODE_MASK) >> MAC_ACK_REP_CODE_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_SEQ_CHECK_ENABLE_MASK) != (old_reg_value & MAC_ACK_SEQ_CHECK_ENABLE_MASK)) {
            if (s->regs.field_changed.ack_seq_check_changed) {
                s->regs.field_changed.ack_seq_check_changed(((uint32_t)value & MAC_ACK_SEQ_CHECK_ENABLE_MASK) >> MAC_ACK_SEQ_CHECK_ENABLE_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_DESTINATION_MASK) != (old_reg_value & MAC_ACK_DESTINATION_MASK)) {
            if (s->regs.field_changed.ack_destination_changed) {
                s->regs.field_changed.ack_destination_changed(((uint32_t)value & MAC_ACK_DESTINATION_MASK) >> MAC_ACK_DESTINATION_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_ENABLE_MASK) != (old_reg_value & MAC_ACK_ENABLE_MASK)) {
            if (s->regs.field_changed.ack_enable_changed) {
                s->regs.field_changed.ack_enable_changed(((uint32_t)value & MAC_ACK_ENABLE_MASK) >> MAC_ACK_ENABLE_SHIFT, s);
            }
        }

        break;

    case MAC_ACK_PAYLOAD:
        if ((value & MAC_ACK_EXTRA_HDR_MASK) != (old_reg_value & MAC_ACK_EXTRA_HDR_MASK)) {
            if (s->regs.field_changed.ack_extra_hdr_changed) {
                s->regs.field_changed.ack_extra_hdr_changed(((uint32_t)value & MAC_ACK_EXTRA_HDR_MASK) >> MAC_ACK_EXTRA_HDR_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_FC1_MASK) != (old_reg_value & MAC_ACK_FC1_MASK)) {
            if (s->regs.field_changed.ack_fc1_changed) {
                s->regs.field_changed.ack_fc1_changed(((uint32_t)value & MAC_ACK_FC1_MASK) >> MAC_ACK_FC1_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_FC0_MASK) != (old_reg_value & MAC_ACK_FC0_MASK)) {
            if (s->regs.field_changed.ack_fc0_changed) {
                s->regs.field_changed.ack_fc0_changed(((uint32_t)value & MAC_ACK_FC0_MASK) >> MAC_ACK_FC0_SHIFT, s);
            }
        }

        break;

    case MAC_ACK_PROC_CTRL:
        if ((value & MAC_MAX_RETRIES_MASK) != (old_reg_value & MAC_MAX_RETRIES_MASK)) {
            if (s->regs.field_changed.max_retries_changed) {
                s->regs.field_changed.max_retries_changed(((uint32_t)value & MAC_MAX_RETRIES_MASK) >> MAC_MAX_RETRIES_SHIFT, s);
            }
        }
        if ((value & MAC_ACK_WAIT_DURATION_MASK) != (old_reg_value & MAC_ACK_WAIT_DURATION_MASK)) {
            if (s->regs.field_changed.ack_wait_dur_changed) {
                s->regs.field_changed.ack_wait_dur_changed(((uint32_t)value & MAC_ACK_WAIT_DURATION_MASK) >> MAC_ACK_WAIT_DURATION_SHIFT, s);
            }
        }

        break;

    }
}


/* ___________________ Power Up Controller
 */

void
han_pwr_reg_reset(void *opaque)
{
    struct han_pwr_dev *s = HANADU_PWR_DEV(opaque);

    memset(&s->regs, 0, sizeof(uint32_t) * 32);
    s->regs.pup_afe_reset = 0x00000001;
}

uint64_t
han_pwr_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    struct han_pwr_dev *s = HANADU_PWR_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    uint64_t retvalue = 0;
    reg_read_fn * reg_read_fnp = (reg_read_fn *)&s->regs.reg_read;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_read_fnp[addr])
        if (reg_read_fnp[addr]((uint32_t *)&retvalue, s))
            return retvalue;

    if (s->mem_region_read)
        if (s->mem_region_read(opaque, addr, size, &retvalue))
            return retvalue;

    return regs[addr];
}

void
han_pwr_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    struct han_pwr_dev *s = HANADU_PWR_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    reg_changed_fn * reg_changes_fnp = (reg_changed_fn *)&s->regs.reg_changed;

    uint32_t old_reg_value;

    if (s->mem_region_write)
        if (s->mem_region_write(opaque, addr, value, size))
            return;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_changes_fnp[addr])
        if (reg_changes_fnp[addr]((uint32_t)value) )
            return;


    old_reg_value = regs[addr];
    regs[addr] = value;


    switch(addr) {
    
    case PWR_SPI_WRITE:
        if ((value & PUP_KICK_OFF_SPI_WRITE_MASK) != (old_reg_value & PUP_KICK_OFF_SPI_WRITE_MASK)) {
            if (s->regs.field_changed.pup_kick_off_spi_write_changed) {
                s->regs.field_changed.pup_kick_off_spi_write_changed(((uint32_t)value & PUP_KICK_OFF_SPI_WRITE_MASK) >> PUP_KICK_OFF_SPI_WRITE_SHIFT, s);
            }
        }

        break;

    case PWR_SPI_READ:
        if ((value & PUP_KICK_OFF_SPI_READ_MASK) != (old_reg_value & PUP_KICK_OFF_SPI_READ_MASK)) {
            if (s->regs.field_changed.pup_kick_off_spi_read_changed) {
                s->regs.field_changed.pup_kick_off_spi_read_changed(((uint32_t)value & PUP_KICK_OFF_SPI_READ_MASK) >> PUP_KICK_OFF_SPI_READ_SHIFT, s);
            }
        }

        break;

    case PWR_RELAY:
        if ((value & RELAY_MASK) != (old_reg_value & RELAY_MASK)) {
            if (s->regs.field_changed.relay_changed) {
                s->regs.field_changed.relay_changed(((uint32_t)value & RELAY_MASK) >> RELAY_SHIFT, s);
            }
        }

        break;

    case PWR_FPGA_CONFIG:
        if ((value & FPGA_CONFIG_MASK) != (old_reg_value & FPGA_CONFIG_MASK)) {
            if (s->regs.field_changed.fpga_config_changed) {
                s->regs.field_changed.fpga_config_changed(((uint32_t)value & FPGA_CONFIG_MASK) >> FPGA_CONFIG_SHIFT, s);
            }
        }

        break;

    }
}


/* ___________________ AFE Controller
 */

void
han_afe_reg_reset(void *opaque)
{
    struct han_afe_dev *s = HANADU_AFE_DEV(opaque);

    memset(&s->regs, 0, sizeof(uint32_t) * 32);
    s->regs.afe_agc_fixed_gain = 0x000000600;
    s->regs.afe_ad9865_write_reg_0_3 = 0x00750080;
    s->regs.afe_ad9865_write_reg_4_7 = 0x01c40036;
    s->regs.afe_ad9865_write_reg_8_11 = 0x207f0066;
    s->regs.afe_ad9865_write_reg_12_15 = 0x00010343;
    s->regs.afe_ad9865_write_reg_16_19 = 0x00015492;
}

uint64_t
han_afe_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    struct han_afe_dev *s = HANADU_AFE_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    uint64_t retvalue = 0;
    reg_read_fn * reg_read_fnp = (reg_read_fn *)&s->regs.reg_read;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_read_fnp[addr])
        if (reg_read_fnp[addr]((uint32_t *)&retvalue, s))
            return retvalue;

    if (s->mem_region_read)
        if (s->mem_region_read(opaque, addr, size, &retvalue))
            return retvalue;

    return regs[addr];
}

void
han_afe_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    struct han_afe_dev *s = HANADU_AFE_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    reg_changed_fn * reg_changes_fnp = (reg_changed_fn *)&s->regs.reg_changed;

    uint32_t old_reg_value;

    if (s->mem_region_write)
        if (s->mem_region_write(opaque, addr, value, size))
            return;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_changes_fnp[addr])
        if (reg_changes_fnp[addr]((uint32_t)value) )
            return;


    old_reg_value = regs[addr];
    regs[addr] = value;


    switch(addr) {
    
    case AFE_STATUS:
        if ((value & SPI_READ_DONE_MASK) != (old_reg_value & SPI_READ_DONE_MASK)) {
            if (s->regs.field_changed.afe_ad9865_spi_read_done_changed) {
                s->regs.field_changed.afe_ad9865_spi_read_done_changed(((uint32_t)value & SPI_READ_DONE_MASK) >> SPI_READ_DONE_SHIFT, s);
            }
        }
        if ((value & SPI_WRITE_DONE_MASK) != (old_reg_value & SPI_WRITE_DONE_MASK)) {
            if (s->regs.field_changed.afe_ad9865_spi_write_done_changed) {
                s->regs.field_changed.afe_ad9865_spi_write_done_changed(((uint32_t)value & SPI_WRITE_DONE_MASK) >> SPI_WRITE_DONE_SHIFT, s);
            }
        }

        break;

    }
}


/* ___________________ Hardware Version Control
 */

void
han_hwvers_reg_reset(void *opaque)
{
    struct han_hwvers_dev *s = HANADU_HWVERS_DEV(opaque);

    memset(&s->regs, 0, sizeof(uint32_t) * 32);
}

uint64_t
han_hwvers_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
    struct han_hwvers_dev *s = HANADU_HWVERS_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    uint64_t retvalue = 0;
    reg_read_fn * reg_read_fnp = (reg_read_fn *)&s->regs.reg_read;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_read_fnp[addr])
        if (reg_read_fnp[addr]((uint32_t *)&retvalue, s))
            return retvalue;

    if (s->mem_region_read)
        if (s->mem_region_read(opaque, addr, size, &retvalue))
            return retvalue;

    return regs[addr];
}

void
han_hwvers_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    struct han_hwvers_dev *s = HANADU_HWVERS_DEV(opaque);
    uint32_t * regs = (uint32_t *)&s->regs;
    reg_changed_fn * reg_changes_fnp = (reg_changed_fn *)&s->regs.reg_changed;


    if (s->mem_region_write)
        if (s->mem_region_write(opaque, addr, value, size))
            return;

    addr >>= 2;
    assert(addr < 32);
    assert(size <= 4);

    if (reg_changes_fnp[addr])
        if (reg_changes_fnp[addr]((uint32_t)value) )
            return;


    regs[addr] = value;


}

