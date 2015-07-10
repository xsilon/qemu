/*
 * QEMU model of Xsilon Hanadu 802.15.4 transceiver.
 *
 * Copyright (c) 2014 Martin Townsend <martin.townsend@xsilon.com>
 *
 */
#include "hw/sysbus.h"
#include "qemu/log.h"
#include "hanadu.h"
#include "hanadu-inl.h"
#include "xsilon.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/timerfd.h>

#define NETSIM_DEFAULT_PORT					(HANADU_NODE_PORT)
#define NETSIM_DEFAULT_ADDR					(INADDR_LOOPBACK)
#define NETSIM_PKT_LEN						(256)



struct hanadu han;

void
hanadu_set_default_dip_board(uint8_t dip)
{
	han.dip_board = dip;
}

void
hanadu_set_default_dip_afe(uint8_t dip)
{
	han.dip_afe = dip;
}

void
hanadu_set_netsim_addr(const char * addr)
{
	han.netsim.addr = strdup(addr);
}

void
hanadu_set_netsim_port(int port)
{
	han.netsim.port = port;
}

static void
netsim_init(struct han_trxm_dev *s)
{
	int rv;

	qemu_log("NetSim Initialisation");

	/* Create TCP connection to Network Simulator */
	han.netsim.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (han.netsim.sockfd == -1) {
		perror("Failed to create netsim socket");
		exit(EXIT_FAILURE);
	}
	/* TODO: Non blocking and closeexec flags */

	bzero(&han.netsim.server_addr,sizeof(han.netsim.server_addr));
	han.netsim.server_addr.sin_family = AF_INET;
	if (han.netsim.addr)
		han.netsim.server_addr.sin_addr.s_addr=inet_addr(han.netsim.addr);
	else
		han.netsim.server_addr.sin_addr.s_addr=htonl(NETSIM_DEFAULT_ADDR);
	if (han.netsim.port)
		han.netsim.server_addr.sin_port=htons(han.netsim.port);
	else
		han.netsim.server_addr.sin_port=htons(NETSIM_DEFAULT_PORT);

	han.mac.tx_started = false;
	han.mac.start_tx_latched = false;
	/* Create CSMA Backoff timer */
	han.mac.backoff_timer = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
	if (han.mac.backoff_timer == -1)
		exit(EXIT_FAILURE);

	/* Create Tx timer */
	han.mac.tx_timer = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
	if (han.mac.tx_timer == -1)
		exit(EXIT_FAILURE);

	/* Create IFS timer */
	han.mac.ifs_timer = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
	if (han.mac.ifs_timer == -1)
		exit(EXIT_FAILURE);

	/* Create txstart event */
	if (han_event_init(&han.txstart_event) == -1) {
		perror("Failed to create txstart event");
		exit(EXIT_FAILURE);
	}
	/* Create sysinfo avilable event */
	if (han_event_init(&han.sysinfo_available) == -1) {
		perror("Failed to create sysinfo available event");
		exit(EXIT_FAILURE);
	}

	/* Connect to NetSim server */
	if (connect(han.netsim.sockfd, (struct sockaddr *)&han.netsim.server_addr,
			sizeof(han.netsim.server_addr)) < 0) {
		perror("Failed to connect to Network Simualtor TCP socket");
		exit(EXIT_FAILURE);
	}

	netsim_rxmcast_init();

	rv = pthread_create(&han.sm_threadinfo, NULL,
			    hanadu_modem_model_thread, s);
	if (rv)
		exit(EXIT_FAILURE);

	han.netsim.initialised = true;
}


/* __________________________________________________________ Hanadu Transceiver
 */


static void
han_trxm_tx_enable_changed(uint32_t value, void *hw_block)
{
	if (value) {
		assert(han.tx.enabled == false);
		han.tx.enabled = true;
	} else {
		assert(han.tx.enabled == true);
		han.tx.enabled = false;
	}
}

static void
han_trxm_rx_enable_changed(uint32_t value, void *hw_block)
{
	if (value) {
		assert(han.rx.enabled == false);
		han.rx.enabled = true;
	} else {
		assert(han.rx.enabled == true);
		han.rx.enabled = false;
	}
}

static void
han_trxm_tx_start_changed(uint32_t value, void *hw_block)
{
	if (value) {
		/* Start transmission */
		han_event_signal(&han.txstart_event);

	} else {
		/* The Hanadu Tx IRQ will call xsi_hal_txm_transmit_stop(...)
		 * which will end up here so any Hanadu HW emulation code for
		 * this event should go here. */
	}
}

static int
han_trxm_rx_next_membank_to_proc_read(uint32_t *value_out, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	uint8_t next_membank, fifo_used;

	assert(!fifo8_is_empty(&han.rx.nextbuf));
	next_membank = fifo8_pop(&han.rx.nextbuf);
	han_trxm_rx_mem_bank_next_to_process_set(s, next_membank);
	fifo8_push(&han.rx.proc, next_membank);

	/* The buffer would be moved from nextbuf FIFO to proc FIFO */
	fifo_used = (uint8_t)han.rx.nextbuf.num;
	han_trxm_rx_nextbuf_fifo_wr_level_set(s, fifo_used);
	han_trxm_rx_nextbuf_fifo_rd_level_set(s, fifo_used);
	fifo_used = (uint8_t)han.rx.proc.num;
	han_trxm_rx_proc_fifo_wr_level_set(s, fifo_used);
	han_trxm_rx_proc_fifo_rd_level_set(s, fifo_used);

	/* We aren't going to terminate the read so return 0 so it's handled by
	 * the caller which will go ahead and return the next bank bank to process */
	return 0;
}

static void
han_trxm_rx_clear_membank_full_changed(uint32_t value, void *hw_block, uint8_t membank)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);
	/* Should be called twice, as the bit has to be pulsed by the driver */

	if (value) {
		uint8_t membank_processed, fifo_used;

		/* when high adjust fifo levels. */
		assert(!fifo8_is_empty(&han.rx.proc));
		membank_processed = fifo8_pop(&han.rx.proc);
		assert(membank == membank_processed);
		fifo_used = (uint8_t)han.rx.proc.num;
		han_trxm_rx_proc_fifo_wr_level_set(s, fifo_used);
		han_trxm_rx_proc_fifo_rd_level_set(s, fifo_used);

	} else {
		assert((han.rx.bufs_avail_bitmap & (1 << membank)) == 0);
		han.rx.bufs_avail_bitmap |= 1 << membank;
	}
}

static void
han_trxm_rx_clear_membank_full0_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 0);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full0_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full1_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 1);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full1_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full2_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 2);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full2_flag_set(s, 0);
}
static void
han_trxm_rx_clear_membank_full3_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	han_trxm_rx_clear_membank_full_changed(value, hw_block, 3);
	/* Clear the actual flag in the membank status register */
	han_trxm_rx_mem_bank_full3_flag_set(s, 0);
}

static void
han_trxm_rx_clear_membank_oflow_changed(uint32_t value, void *hw_block)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(hw_block);

	if (!value) {
		/* Clear the actual flag in the membank status register */
		han_trxm_rx_mem_bank_overflow_set(s, 0);
	}
}

static Property han_trxm_properties[] = {
	DEFINE_PROP_UINT32("rxmem", struct han_trxm_dev, c_rxmem, 0x10000),
	DEFINE_PROP_UINT32("txmem", struct han_trxm_dev, c_txmem, 0x10000),
//	DEFINE_NIC_PROPERTIES(XilinxAXIEnet, conf),
	DEFINE_PROP_END_OF_LIST(),
};

static const MemoryRegionOps han_trxm_mem_region_ops = {
	.read = han_trxm_mem_region_read,
	.write = han_trxm_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};


static void han_trxm_realize(DeviceState *dev, Error **errp)
{
//	struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_trxm_reset(DeviceState *dev)
{
//	struct han_trxm_dev *s = HANADU_TRXM_DEV(dev);
}


static void han_trxm_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_trxm_realize;
	dc->props = han_trxm_properties;
	dc->reset = han_trxm_reset;
}

static void han_trxm_instance_init(Object *obj)
{
	struct han_trxm_dev *s = HANADU_TRXM_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_trxm_mem_region_ops, s,
			      "hantrxm", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);

	sysbus_init_irq(sbd, &s->rx_irq);
	sysbus_init_irq(sbd, &s->rx_fail_irq);
	sysbus_init_irq(sbd, &s->tx_irq);

	/* Override Register/Bitfield changed functions */
	s->regs.reg_read.trx_rx_next_membank_to_proc_read_cb = han_trxm_rx_next_membank_to_proc_read;
	s->regs.field_changed.tx_enable_changed = han_trxm_tx_enable_changed;
	s->regs.field_changed.rx_enable_changed = han_trxm_rx_enable_changed;
	s->regs.field_changed.tx_start_changed = han_trxm_tx_start_changed;
	s->regs.field_changed.rx_clear_membank_full0_changed = han_trxm_rx_clear_membank_full0_changed;
	s->regs.field_changed.rx_clear_membank_full1_changed = han_trxm_rx_clear_membank_full1_changed;
	s->regs.field_changed.rx_clear_membank_full2_changed = han_trxm_rx_clear_membank_full2_changed;
	s->regs.field_changed.rx_clear_membank_full3_changed = han_trxm_rx_clear_membank_full3_changed;
	s->regs.field_changed.rx_clear_membank_oflow_changed = han_trxm_rx_clear_membank_oflow_changed;

	han_trxm_reg_reset(s);
	/* Setup callbacks to capture filter reg changes */
	han.trx_dev = s;

	netsim_init(s);
}

/* __________________________________________________________________ Hanadu AFE
 */

static const MemoryRegionOps han_afe_mem_region_ops = {
	.read = han_afe_mem_region_read,
	.write = han_afe_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_afe_realize(DeviceState *dev, Error **errp)
{
//	struct han_afe_dev *s = HANADU_AFE_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_afe_reset(DeviceState *dev)
{
//	struct han_afe_dev *s = HANADU_AFE_DEV(dev);
}


static void han_afe_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_afe_realize;
	dc->props = NULL;
	dc->reset = han_afe_reset;
}

static void han_afe_instance_init(Object *obj)
{
	struct han_afe_dev *s = HANADU_AFE_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_afe_mem_region_ops, s,
			      "hanafe", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;
	han.afe_dev = s;
	han_afe_reg_reset(s);
	han.ad9865.regs[0] = s->regs.afe_ad9865_write_reg_0_3;
	han.ad9865.regs[1] = s->regs.afe_ad9865_write_reg_4_7;
	han.ad9865.regs[2] = s->regs.afe_ad9865_write_reg_8_11;
	han.ad9865.regs[3] = s->regs.afe_ad9865_write_reg_12_15;
	han.ad9865.regs[4] = s->regs.afe_ad9865_write_reg_16_19;
}

/* __________________________________________________________________ Hanadu PWR
 */

static const MemoryRegionOps han_pwr_mem_region_ops = {
	.read = han_pwr_mem_region_read,
	.write = han_pwr_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void
han_pup_kick_off_spi_write_changed(uint32_t value, void *hw_block)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(hw_block);

	if (value) {
		assert(han_pwr_pup_kick_off_spi_write_get(s) == false);
		han_afe_afe_ad9865_spi_write_done_set(han.afe_dev, false);
	} else {
		assert(han_pwr_pup_kick_off_spi_write_get(s) == true);
		/* When it's de-asserted copy the contents of the AD9865 registers in
		 * the AFE into our internal AD9865 registers */
		han.ad9865.regs[0] = han.afe_dev->regs.afe_ad9865_write_reg_0_3;
		han.ad9865.regs[1] = han.afe_dev->regs.afe_ad9865_write_reg_4_7;
		han.ad9865.regs[2] = han.afe_dev->regs.afe_ad9865_write_reg_8_11;
		han.ad9865.regs[3] = han.afe_dev->regs.afe_ad9865_write_reg_12_15;
		han.ad9865.regs[4] = han.afe_dev->regs.afe_ad9865_write_reg_16_19;
		han_afe_afe_ad9865_spi_write_done_set(han.afe_dev, true);
	}
}

static void
han_pup_kick_off_spi_read_changed(uint32_t value, void *hw_block)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(hw_block);

	if (value) {
		assert(han_pwr_pup_kick_off_spi_read_get(s) == false);
		han_afe_afe_ad9865_spi_read_done_set(han.afe_dev, false);
	} else {
		assert(han_pwr_pup_kick_off_spi_read_get(s) == true);
		han.afe_dev->regs.afe_ad9865_read_reg_0_3 = han.ad9865.regs[0];
		han.afe_dev->regs.afe_ad9865_read_reg_4_7 = han.ad9865.regs[1];
		han.afe_dev->regs.afe_ad9865_read_reg_8_11 = han.ad9865.regs[2];
		han.afe_dev->regs.afe_ad9865_read_reg_12_15 = han.ad9865.regs[3];
		han.afe_dev->regs.afe_ad9865_read_reg_16_19 = han.ad9865.regs[4];
		han_afe_afe_ad9865_spi_read_done_set(han.afe_dev, true);
	}
}

static void han_pwr_realize(DeviceState *dev, Error **errp)
{
//	struct han_pwr_dev *s = HANADU_PWR_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_pwr_reset(DeviceState *dev)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(dev);

	s->regs.pup_dips1 = han.dip_board;
	s->regs.pup_dips2 = han.dip_afe;
}


static void han_pwr_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_pwr_realize;
	dc->props = NULL;
	dc->reset = han_pwr_reset;
}

static void han_pwr_instance_init(Object *obj)
{
	struct han_pwr_dev *s = HANADU_PWR_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_pwr_mem_region_ops, s,
			      "hanpwr", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;

	han.pwr_dev = s;
	s->regs.field_changed.pup_kick_off_spi_write_changed = han_pup_kick_off_spi_write_changed;
	s->regs.field_changed.pup_kick_off_spi_read_changed = han_pup_kick_off_spi_read_changed;
	han_pwr_reg_reset(s);
}

/* __________________________________________________________________ Hanadu MAC
 */

static int
han_mac_filter_ea_upper_changed(uint32_t value)
{
	han.mac_addr[0] = (uint8_t)((value >> 24) & 0xff);
	han.mac_addr[1] = (uint8_t)((value >> 16) & 0xff);
	han.mac_addr[2] = (uint8_t)((value >> 8) & 0xff);
	han.mac_addr[3] = (uint8_t)(value & 0xff);

	return 0;
}

static int
han_mac_filter_ea_lower_changed(uint32_t value)
{
	han.mac_addr[4] = (uint8_t)((value >> 24) & 0xff);
	han.mac_addr[5] = (uint8_t)((value >> 16) & 0xff);
	han.mac_addr[6] = (uint8_t)((value >> 8) & 0xff);
	han.mac_addr[7] = (uint8_t)(value & 0xff);

	return 0;
}

static const MemoryRegionOps han_mac_mem_region_ops = {
	.read = han_mac_mem_region_read,
	.write = han_mac_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_mac_realize(DeviceState *dev, Error **errp)
{
//	struct han_mac_dev *s = HANADU_MAC_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_mac_reset(DeviceState *dev)
{
//	struct han_mac_dev *s = HANADU_MAC_DEV(dev);
}


static void han_mac_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_mac_realize;
	dc->props = NULL;
	dc->reset = han_mac_reset;
}

static void han_mac_instance_init(Object *obj)
{
	struct han_mac_dev *s = HANADU_MAC_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_mac_mem_region_ops, s,
			      "hanmac", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;

	han.mac_dev = s;
	s->regs.reg_changed.mac_filter_ea_upper_changed_cb = han_mac_filter_ea_upper_changed;
	s->regs.reg_changed.mac_filter_ea_lower_changed_cb = han_mac_filter_ea_lower_changed;
	han_mac_reg_reset(s);
}

/* __________________________________________________________________ Hanadu TXB
 */

static uint64_t
han_txb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if (addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	return (uint64_t)*buf;
}

static void
han_txb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if (addr < 0x1000) {
		addr >>= 2;
		buf = han.tx.buf[0].data + addr;
	} else {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.tx.buf[1].data + addr;
	}
	assert(addr <= 128);

	*buf = (uint8_t)value;
}

static const MemoryRegionOps han_txb_mem_region_ops = {
	.read = han_txb_mem_region_read,
	.write = han_txb_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_txb_realize(DeviceState *dev, Error **errp)
{
//	struct han_txb_dev *s = HANADU_TXB_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_txb_reset(DeviceState *dev)
{
//	struct han_txb_dev *s = HANADU_TXB_DEV(dev);
}


static void han_txb_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_txb_realize;
	dc->props = NULL;
	dc->reset = han_txb_reset;
}

static void han_txb_instance_init(Object *obj)
{
	struct han_txb_dev *s = HANADU_TXB_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_txb_mem_region_ops, s,
			      "hantxb", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu RXB
 */

static uint64_t
han_rxb_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if (addr < 0x1000) {
		addr >>= 2;
		buf = han.rx.buf[0].data + addr;
	} else if (addr < 0x2000) {
		addr -= 0x1000;
		addr >>= 2;
		buf = han.rx.buf[1].data + addr;
	} else if (addr < 0x3000) {
		addr -= 0x2000;
		addr >>= 2;
		buf = han.rx.buf[2].data + addr;
	} else if (addr < 0x4000) {
		addr -= 0x3000;
		addr >>= 2;
		buf = han.rx.buf[3].data + addr;
	} else {
		assert(addr >= 0x4000);
	}
	assert(addr <= 128);

	return (uint64_t)*buf;
}

static void
han_rxb_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	/* Shouldn't be writing to receive memories from the driver */
	assert(1==0);
}

static const MemoryRegionOps han_rxb_mem_region_ops = {
	.read = han_rxb_mem_region_read,
	.write = han_rxb_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_rxb_realize(DeviceState *dev, Error **errp)
{
//	struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_rxb_reset(DeviceState *dev)
{
//	struct han_rxb_dev *s = HANADU_RXB_DEV(dev);
}


static void han_rxb_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_rxb_realize;
	dc->props = NULL;
	dc->reset = han_rxb_reset;
}

static void han_rxb_instance_init(Object *obj)
{
	struct han_rxb_dev *s = HANADU_RXB_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_rxb_mem_region_ops, s,
			      "hanrxb", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
}

/* __________________________________________________________________ Hanadu HWV
 */

/* Override the auto generated functions */
uint64_t
han_hwvers_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	addr >>= 2;

	switch (addr) {
	case 0:
	case 1:
	case 2:
		return 9999;
	case 3:
		return 1;
	}

	return 0;
}

void
han_hwvers_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	/* This is a read only register block */
}

static const MemoryRegionOps han_hwvers_mem_region_ops = {
	.read = han_hwvers_mem_region_read,
	.write = han_hwvers_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_hwvers_realize(DeviceState *dev, Error **errp)
{
//	struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
//	Error *local_err = NULL;

	return;

//realize_fail:
//	if (!*errp) {
//		*errp = local_err;
//	}
}

static void han_hwvers_reset(DeviceState *dev)
{
//	struct han_hwv_dev *s = HANADU_HWV_DEV(dev);
}


static void han_hwvers_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_hwvers_realize;
	dc->props = NULL;
	dc->reset = han_hwvers_reset;
}

static void han_hwvers_instance_init(Object *obj)
{
	struct han_hwvers_dev *s = HANADU_HWVERS_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_hwvers_mem_region_ops, s,
			      "hanhwv", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
	s->mem_region_write = NULL;
}

/* __________________________________________________________ Hanadu System Info
 */

static uint64_t
han_sysinfo_mem_region_read(void *opaque, hwaddr addr, unsigned size)
{
	assert(1==0);
}

static void
han_sysinfo_mem_region_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
	uint8_t * buf;

	assert(size == 1);
	if (addr < 0x1000) {
//		addr >>= 2;
		buf = (uint8_t *)&han.sysinfo;
		buf += addr;
	}
	assert(addr <= 128);

	*buf = (uint8_t)value;

	if (addr == sizeof(han.sysinfo) - 1) {
		han_event_signal(&han.sysinfo_available);
	}
}

static const MemoryRegionOps han_sysinfo_mem_region_ops = {
	.read = han_sysinfo_mem_region_read,
	.write = han_sysinfo_mem_region_write,
	.endianness = DEVICE_LITTLE_ENDIAN,
};

static void han_sysinfo_realize(DeviceState *dev, Error **errp)
{
	return;
}

static void han_sysinfo_reset(DeviceState *dev)
{
}


static void han_sysinfo_class_init(ObjectClass *klass, void *data)
{
	DeviceClass *dc = DEVICE_CLASS(klass);

	dc->realize = han_sysinfo_realize;
	dc->props = NULL;
	dc->reset = han_sysinfo_reset;
}

static void han_sysinfo_instance_init(Object *obj)
{
	struct han_sysinfo_dev *s = HANADU_SYSINFO_DEV(obj);
	SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

	memory_region_init_io(&s->iomem, &han_sysinfo_mem_region_ops, s,
			      "hansysinfo", 0x10000);
	sysbus_init_mmio(sbd, &s->iomem);
}

/* _________________________________________________________ Hanadu Registration
 */

static const TypeInfo han_trxm_info = {
	.name  = TYPE_HANADU_TRXM,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_trxm_dev),
	.class_init = han_trxm_class_init,
	.instance_init = han_trxm_instance_init,
};

static const TypeInfo han_afe_info = {
	.name  = TYPE_HANADU_AFE,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_afe_dev),
	.class_init = han_afe_class_init,
	.instance_init = han_afe_instance_init,
};

static const TypeInfo han_pwr_info = {
	.name  = TYPE_HANADU_PWR,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_pwr_dev),
	.class_init = han_pwr_class_init,
	.instance_init = han_pwr_instance_init,
};

static const TypeInfo han_mac_info = {
	.name  = TYPE_HANADU_MAC,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_mac_dev),
	.class_init = han_mac_class_init,
	.instance_init = han_mac_instance_init,
};

static const TypeInfo han_txb_info = {
	.name  = TYPE_HANADU_TXB,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_txb_dev),
	.class_init = han_txb_class_init,
	.instance_init = han_txb_instance_init,
};

static const TypeInfo han_rxb_info = {
	.name  = TYPE_HANADU_RXB,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_rxb_dev),
	.class_init = han_rxb_class_init,
	.instance_init = han_rxb_instance_init,
};

static const TypeInfo han_hwv_info = {
	.name  = TYPE_HANADU_HWVERS,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_hwvers_dev),
	.class_init = han_hwvers_class_init,
	.instance_init = han_hwvers_instance_init,
};

static const TypeInfo han_sysinfo_info = {
	.name  = TYPE_HANADU_SYSINFO,
	.parent = TYPE_SYS_BUS_DEVICE,
	.instance_size  = sizeof(struct han_hwvers_dev),
	.class_init = han_sysinfo_class_init,
	.instance_init = han_sysinfo_instance_init,
};

static void han_register_types(void)
{
	fifo8_create(&han.rx.nextbuf, 4);
	fifo8_create(&han.rx.proc, 4);
	han.rx.bufs_avail_bitmap = 0x0f;

	type_register_static(&han_trxm_info);
	type_register_static(&han_afe_info);
	type_register_static(&han_pwr_info);
	type_register_static(&han_mac_info);
	type_register_static(&han_txb_info);
	type_register_static(&han_rxb_info);
	type_register_static(&han_hwv_info);
	type_register_static(&han_sysinfo_info);
}

type_init(han_register_types)


